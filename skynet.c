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

	if(size == 1){
		send(c, &num);
		return;
		free(args);
	}
	rc = chancreate(sizeof(u64int), 0);
	for(i = 0; i < div; i++){
		nargs = malloc(sizeof(Skynetargs));
		subnum = num + i*(size/div);
		nargs->c = rc;
		nargs->num = subnum;
		nargs->size = size/div;
		nargs->div = div;
		nargs->level = level+1;
		if(level < p2tthresh)
			proccreate(skynet, nargs, mainstacksize);
		else
			threadcreate(skynet, nargs, mainstacksize);
	}
	for(i = 0; i < div; i++){
		recv(rc, &x);
		sum += x;
	}
	send(c, &sum);
	chanfree(rc);
	free(args);
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
	print("Result: %ud in %d ms.\n", result, (end-start)/1000000);
	threadexitsall(nil);
}
