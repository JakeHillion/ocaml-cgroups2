C=clang

all: c ml

clean:
	rm -f c ml

c: cp.c
	${C} -o c cp.c

ml: cp.ml
	opam switch default
	ocamlopt -o ml cp.ml
