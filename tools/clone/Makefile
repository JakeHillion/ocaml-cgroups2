all: clone_shim

clone_shim: clone_shim.ml clone3_stub.o
	ocamlc -custom -o clone_shim clone_shim.ml clone3_stub.o

clone3_stub.o: clone3_stub.c
	ocamlc -c clone3_stub.c

clean:
	rm clone_shim.cmi clone_shim.cmo clone3_stub.o
