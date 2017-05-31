all: uvsh.c
	gcc -o uvsh uvsh.c -I.
debug: uvsh.c
	gcc -o uvsh -g uvsh.c -I.
clean:
	rm uvsh