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

void
skynet(void *a)
{
	Skynetargs *args;
	Skynetargs *nargs;
	Channel *rc, *c;
	uint i, level;
	u64int sum, subnum, x, num, size, div;

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
		nargs[i] = (Skynetargs){rc, subnum, size/div, div, level+1};
		if(level < p2tthresh)
			proccreate(skynet, &nargs[i], mainstacksize);
		else
			threadcreate(skynet, &nargs[i], mainstacksize);
	}
	for(i = 0; i < div; i++){
		recv(rc, &x);
		sum += x;
	}
	send(c, &sum);
	chanfree(rc);
	free(nargs);
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
	u64int result, size, div;
	Skynetargs *a;

	p2tthresh = 0;
	size = 100000;
	div = 10;

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

	a = malloc(sizeof(Skynetargs));
	c = chancreate(sizeof(u64int), 0);
	*a = (Skynetargs){c, 0, size, div, 0};
	start = nsec();
	threadcreate(skynet, a, mainstacksize);
	recv(c, &result);
	end = nsec();
	chanfree(c);
	free(a);
	print("Result: %ud in %d ms.\n", result, (end-start)/1000000);
	threadexitsall(nil);
}
