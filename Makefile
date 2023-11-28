jsh: jsh.o jsh_bib.o
	gcc -o jsh jsh.o jsh_bib.o -lreadline

exit: exit.o
	gcc -o exit exit.o

jsh.o: jsh.c
	gcc -c jsh.c -o jsh.o -lreadline

jsh_bib.o: jsh_bib.c
	gcc -c jsh_bib.c -o jsh_bib.o

exit.o: exit.c
	gcc -c exit.c -o exit.o -lreadline

clean:
	rm -f jsh exit *.o
