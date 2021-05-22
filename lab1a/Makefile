# NAME: Cody Do
# EMAIL: D.Codyx@gmail.com
# ID: 105140467

.SILENT:

default: lab1a.c
	gcc -Wall -Wextra -g lab1a.c -o lab1a

clean:
	rm -rf lab1a *.tar.gz *.o

dist: lab1a Makefile README
	tar -czf lab1a-105140467.tar.gz lab1a.c Makefile README