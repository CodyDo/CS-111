#!/bin/bash

let errors=0

# Check for bad argument
echo | ./lab4b --bagels &> /dev/null;
if [[ $? -ne 1 ]]; then
    echo "ERROR: Failed to recognize bad argument";
    let errors+=1
else
    echo "PASSED: Recognized bad argument";
fi

# Run program, echoing arguments before shutting down
{ echo "STOP"; echo "PERIOD=3"; echo "START"; echo "OFF"; } | ./lab4b --log=loggy.txt

# Check if the program exited with proper exit code
if [ $? -ne 0 ]
then
	echo "ERROR: Program did not exit with code 0"
    let errors+=1
else
	echo "PASSED: Program exited with code 0"
fi

# Check if program made loggy.txt file
if [ ! -s loggy.txt ]
then
	echo "ERROR: Logfile not created"
    let errors+=1
else
	echo "PASSED: Logfile was created"
fi

# Check if loggy.txt holds the input commands
for command in STOP PERIOD=3 START OFF
    do
        grep "$command" loggy.txt > /dev/null
        if [ $? -ne 0 ]
        then
			echo "ERROR: $command command was not properly logged"
            let errors+=1
		else
			echo "$command was properly logged"
		fi
    done

# Clean up and state whether we passed smoketest or not
rm -f loggy.txt
if [ $errors -eq 0 ]
then
    echo "Program passed smoketest"
else
    echo "Program failed smoketest"
fi

