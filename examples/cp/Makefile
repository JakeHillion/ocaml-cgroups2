C=clang

all: c cp_priv_sep c_cgroup ml

clean:
	rm -f c cp_priv_sep c_cgroup ml

c: cp.c
	${C} -o c cp.c

cp_priv_sep: cp_priv_sep.c ../../lib/clone3.c ../../include/clone3.h
	${C} -I../../include -o cp_priv_sep cp_priv_sep.c ../../lib/clone3.c
	sudo setcap CAP_SYS_ADMIN+eip ./cp_priv_sep

c_cgroup: cp_cgroup.c
	${C} -o c_cgroup cp_cgroup.c

ml: cp.ml
	opam switch default
	ocamlopt -o ml cp.ml
