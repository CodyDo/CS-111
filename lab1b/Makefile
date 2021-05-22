# NAME: Cody Do
# EMAIL: D.Codyx@gmail.com
# ID: 105140467

.SILENT:

# -lz is needed for the zlib library (compression)
default: lab1b-server.c lab1b-client.c
	gcc -Wall -Wextra -g -lz -o lab1b-server lab1b-server.c
	gcc -Wall -Wextra -g -lz -o lab1b-client lab1b-client.c

server: lab1b-server.c
	gcc -Wall -Westra -g -lz -o lab1b-server lab1b-server.c

client: lab1b-client.c
	gcc -Wall -Wextra -g -lz -o lab1b-client lab1b-client.c

clean:
	rm -f lab1b-server lab1b-client *.tar.gz *.o

dist:
	tar -czf lab1b-105140467.tar.gz lab1b-client.c lab1b-server.c Makefile README