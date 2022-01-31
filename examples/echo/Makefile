C=clang -Wall

all: echo_priv_sep

clean:
	rm -f echo_priv_sep

echo_priv_sep: echo_priv_sep.c ../../lib/clone3.c ../../include/clone3.h
	${C} -I../../include -o echo_priv_sep echo_priv_sep.c ../../lib/clone3.c
	sudo setcap CAP_SYS_ADMIN+eip ./echo_priv_sep
