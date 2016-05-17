#include <u.h>
#include <libc.h>
#include <thread.h>

int p2tthresh = 0;

typedef struct {
	Channel *c;
	u64int num;
	u64int size;
	u64int div;
	uint level;
} Skynetargs;

typedef struct {
	u64int x;
	QLock l;
} SafeInt;

SafeInt *argsiter;
Skynetargs *nargs;

u64int
intnroot(u64int n, u64int x)
{
	u64int i;
	
	i = 1;
	for(;;){
		x = x/n;
		if(x == 1)
			return i;
		i++;
	}
	return 0;
}

u64int
intnpower(u64int n, u64int x)
{
	u64int op;

	op = x;
	if(n == 1)
		return x;
	if(n == 0)
		return 1;
	while(n > 1){
		x = x*op;
		n--;
	}
	return x;
}

u64int
totaliter_old(u64int size, u64int div)
{
	u64int res, a;

	res = 0;
	print("getting intnroot\n");
	a = intnroot(div, size);
	print("doing summation\n");
	for(;;){
		if(a < 0)
			return res;
		res = res + intnpower(a, div);
		a--;
	}
	return res;
}

u64int
totaliter(u64int size, u64int div)
{
	u64int res, a, re, rex;

	a = intnroot(div, size);
	re = intnpower(a, div);
	rex = (re-1)/(div-1);
	res = rex+re;
	return res;
}

u64int
safeint_read(SafeInt *i)
{
	u64int res;
	qlock(&i->l);
	res = i->x;
	qunlock(&i->l);
	return res;
}

u64int
safeint_incr(SafeInt *i)  // returns i->x then i->x++
{
	u64int res;
	qlock(&i->l);
	res = i->x;
	i->x++;
	qunlock(&i->l);
	return res;
}

void
skynet(void *a)
{
	Skynetargs *args;
	Channel *rc, *c;
	uint i, level;
	u64int sum, subnum, x, num, size, div, argi;

	args = a;
	c = args->c;
	num = args->num;
	size = args->size;
	div = args->div;
	level = args->level;
	sum = 0;

	if(size == 1){
		send(c, &num);
		return;
	}
	rc = chancreate(sizeof(u64int), 0);
	nargs = malloc(sizeof(Skynetargs)*div);
	for(i = 0; i < div; i++){
		subnum = num + i*(size/div);
		argi = safeint_incr(argsiter);
		print("argi = %ud\n", argi);
		nargs[argi] = (Skynetargs){rc, subnum, size/div, div, level+1};
		if(level < p2tthresh)
			proccreate(skynet, &nargs[argi], mainstacksize);
		else
			threadcreate(skynet, &nargs[argi], mainstacksize);
	}
	for(i = 0; i < div; i++){
		recv(rc, &x);
		sum += x;
	}
	send(c, &sum);
	chanfree(rc);
}

void
usage(void)
{
	print("usage: skynetc [-s size] [-d div] [-t thresh]\n");
	threadexitsall("usage");
}

void
threadmain(int argc, char *argv[])
{
	Channel *c;
	vlong start, end;
	u64int result, size, div, titers;
	Skynetargs *a;

	p2tthresh = 0;
	size = 100000;
	div = 10;
	argsiter = mallocz(sizeof(SafeInt), 1);
	if(argsiter == nil){
		print("error: could not malloc iterator: %r\n");
		threadexitsall("malloc");
	}
	argsiter->x = 0;

	ARGBEGIN{
	case 's':
		size = atoi(EARGF(usage));
		break;
	case 'd':
		div = atoi(EARGF(usage));
		break;
	case 't':
		p2tthresh = atoi(EARGF(usage));
		break;
	case 'h':
	default:
		usage();
		break;
	}ARGEND

	print("getting iters\n");
	titers = totaliter(size, div);
	print("running malloc (titers = %ud) (tsize = %ud bytes)\n",
		titers, sizeof(Skynetargs)*titers+1);
	nargs = mallocz(sizeof(Skynetargs)*(titers+1), 1);
	if(nargs == nil){
		print("error: could not malloc skynet arguments: %r\n");
		threadexitsall("malloc");
	}
	print("msize(nargs) = %ud, should be %ud\n",
		msize(nargs), (sizeof(Skynetargs)*(titers+1)));
	a = mallocz(sizeof(Skynetargs), 1);
	c = chancreate(sizeof(u64int), 0);
	*a = (Skynetargs){c, 0, size, div, 0};
	start = nsec();
	threadcreate(skynet, a, mainstacksize);
	recv(c, &result);
	end = nsec();
	chanfree(c);
	free(a);
	free(argsiter);
	free(nargs);
	print("Result: %ud in %d ms.\n", result, (end-start)/1000000);
	threadexitsall(nil);
}
