# NAME: Cody Do
# EMAIL: D.Codyx@gmail.com
# ID: 105140467

# This keeps the commands from appearing on the terminal. It will still
# allow echo strings to show though.
.SILENT:

default: lab0.c
	gcc -Wall -Wextra -g lab0.c -o lab0

clean:
	rm -rf lab0 *.tar.gz *.o

dist: lab0.c Makefile backtrace.png breakpoint.png README 
	tar -czf lab0-105140467.tar.gz lab0.c Makefile backtrace.png breakpoint.png README 

check: default basicTest stdIOTest compIOTest badOptTest badInputTest badOutputTest segFaultTest catchTest

# Must use \ at the end to include bash inside Makefile.
# $? refers to the exit status of previous commands
basicTest:
	echo "... basicTest: Testing input and output of files"
	echo "abcdefghijklmnopqrstuvwxyz" > testinput.txt
	./lab0 --input=testinput.txt --output=testoutput.txt
	if [ $$? -eq 0 ]; then \
		echo "    Exit code is 0...PASSED"; \
	else \
		echo "    Exit code is not 0...FAILED"; \
	fi; \
	rm -f testinput.txt testoutput.txt

stdIOTest:
	echo "... stdIOTest: Testing stdin and stdout"
	echo "~Random test string~" | ./lab0; \
	if [ $$? -eq 0 ]; then \
		echo "    Exit code is 0...PASSED"; \
	else \
		echo "    Exit code is not 0...FAILED"; \
	fi; \

compIOTest:
	echo "... compIOTest: Comparing input and output files"
	echo "abcdefghijklmnopqrstuvwxyz" > testinput.txt
	./lab0 --input=testinput.txt --output=testoutput.txt
	cmp testinput.txt testoutput.txt
	if [ $$? -eq 0 ]; then \
		echo "    Files have the same content...PASSED"; \
	else \
		echo "    Files don't have the same content...FAILED"; \
	fi; \
	rm -f testinput.txt testoutput.txt

badOptTest:
	echo "... badOptTest: Testing response to invalid option";
	./lab0 --badOption=uhOh; \
	if [ $$? -eq 1 ]; then \
		echo "    Exit code is 1...PASSED"; \
	else \
		echo "    Exit code is not 1...FAILED"; \
	fi; \

badInputTest:
	echo "... badInputTest: Testing response to nonexisting input"
	./lab0 --input=FigmentOfUrImagination.txt; \
	if [ $$? -eq 2 ]; then \
		echo "    Exit code is 2...PASSED"; \
	else \
		echo "    Exit code is not 2...FAILED"; \
	fi; \

badOutputTest:
	echo "... badOutputTest: Testing response to output file with no write permission"
	echo "lalalala" > dontWriteInMe.txt
	chmod -w dontWriteInMe.txt
	./lab0 --output=dontWriteInMe.txt; \
	if [ $$? -eq 3 ]; then \
		echo "    Exit code is 3...PASSED"; \
	else \
		echo "    Exit code is not 3...FAILED"; \
	fi; \
	rm -f dontWriteInMe.txt

segFaultTest:
	echo "... segFaultTest: Testing forced segmentation fault"
	./lab0 --segfault; \
	if [ $$? -eq 139 ]; then \
		echo "    Exit code is 139...PASSED"; \
	else \
		echo "    Exit code is not 139...FAILED"; \
	fi; \

catchTest:
	echo "... catchTest: Testing catch option on forced segmentation fault"
	./lab0 --segfault --catch; \
	if [ $$? -eq 4 ]; then \
		echo "    Exit code is 4...PASSED"; \
	else \
		echo "    Exit code is not 4...FAILED"; \
	fi; \