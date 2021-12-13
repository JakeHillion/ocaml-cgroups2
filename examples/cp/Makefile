C=clang

all: c c_cgroup ml

clean:
	rm -f c ml

c: cp.c
	${C} -o c cp.c

c_cgroup: cp_cgroup.c
	${C} -o c_cgroup cp_cgroup.c

ml: cp.ml
	opam switch default
	ocamlopt -o ml cp.ml
