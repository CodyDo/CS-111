# NAME: Cody Do
# EMAIL: D.Codyx@gmail.com
# ID: 105140467

.SILENT:

default:
	gcc -lm -lmraa -Wall -Wextra -g -o lab4b lab4b.c

check: default
	chmod u+x smoketest.sh
	./smoketest.sh

clean:	
	rm -f lab4b *.tar.gz

dist:
	tar -czvf lab4b-105140467.tar.gz Makefile lab4b.c README smoketest.sh