C=clang -Wall

all: fib_priv_sep

clean:
	rm -f fib_priv_sep

fib_priv_sep: fib_priv_sep.c ../../lib/clone3.c ../../include/clone3.h
	${C} -I../../include -o fib_priv_sep fib_priv_sep.c ../../lib/clone3.c
	sudo setcap CAP_SYS_ADMIN+eip ./fib_priv_sep
