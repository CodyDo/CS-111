# NAME: Megan Pham, Cody Do
# EMAIL: megspham@ucla.edu, D.Codyx@gmail.com
# ID: 505313300, 105140467

PYTHON = python3
.SILENT:

#create lab3b link
default: lab3b.py
	rm -f lab3b 
	ln lab3b.py lab3b
	chmod u+x lab3b
	echo "Success!"

dist: 
	tar -czf lab3b-105140467.tar.gz lab3b.py README Makefile 

clean: 
	rm -f lab3b *.tar.gz 
