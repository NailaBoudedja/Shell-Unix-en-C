jsh: jsh.o bib_jsh.o
	gcc -o jsh jsh.o bib_jsh.o -lreadline

jsh.o: jsh.c
	gcc -c jsh.c -o jsh.o -lreadline

bib_jsh.o: bib_jsh.c
	gcc -c bib_jsh.c -o bib_jsh.o

clean:
	rm -f jsh exit *.o