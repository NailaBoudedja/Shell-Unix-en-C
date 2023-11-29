jsh: jsh.o bibv2.o
	gcc -o jsh jsh.o bibv2.o -lreadline

jsh.o: jsh.c
	gcc -c jsh.c -o jsh.o -lreadline

bibv2.o: bibv2.c
	gcc -c bibv2.c -o bibv2.o

clean:
	rm -f jsh exit *.o
