# NAME: Cody Do
# EMAIL: D.Codyx@gmail.com
# ID: 105140467

.SILENT:

default:
	gcc -Wall -Wextra -lm -lmraa -g -o lab4c_tcp lab4c_tcp.c
	gcc -Wall -Wextra -lm -lmraa -g -o lab4c_tls lab4c_tls.c  -lssl -lcrypto 

clean:
	rm -f *.tar.gz lab4c_tls lab4c_tcp

dist: default
	tar -cvzf lab4c-105140467.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README
