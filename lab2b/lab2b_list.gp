#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2b_list.csv
# output:
#   lab2b_1.png ... throughput vs. number of threads for mutex and spin-lock synchronized list operations.
#   lab2b_2.png ... mean time per mutex wait and mean time per operation for mutex-synchronized list operations.
#   lab2b_3.png ... successful iterations vs. threads for each synchronization method.
#   lab2b_4.png ... throughput vs. number of threads for mutex synchronized partitioned lists.
#   lab2b_5.png ... throughput vs. number of threads for spin-lock-synchronized partitioned lists.


# general plot parameters
set terminal png
set datafile separator ","

# how many threads/iterations we can run without failure (w/o yielding)
set title "List-1: Throughput vs. Threads"
set xlabel "Threads"
set xrange [0:25]
set ylabel "Throughput (operations per second)"
set logscale y 10
set output 'lab2b_1.png'

# grep out only single threaded, un-protected, non-yield results
plot \
     "< grep 'list-none-s,[0-9]*,1000,1' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'Spin-Lock' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'Mutex-Lock' with linespoints lc rgb 'green'

set title "List-2: Wait-for-lock Time Analysis, Mutex"
set xlabel "Threads"
set xrange [0:25]
set ylabel "Time Per Operation (ns)"
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep 'list-none-m,[0-9]*,1000,1' lab2b_list.csv" using ($2):($7) \
	title 'Per Operation' with linespoints lc rgb 'green', \
     "< grep 'list-none-m,[0-9]*,1000,1' lab2b_list.csv" using ($2):($8) \
	title 'Wait-for-Lock' with linespoints lc rgb 'red', \
     
set title "List-3: Protected/Unprotected Iterations Run Without Failure"
unset logscale x
set xrange [0:20]
set xlabel "Threads"
set ylabel "Successful iterations"
set logscale y 10
set output 'lab2b_3.png'
plot \
    "< grep 'list-id-none,[0-9]*,[0-9]*,4' lab2b_list.csv" using ($2):($3) \
	title 'Unprotected' with points lc rgb 'red', \
    "< grep 'list-id-m,[0-9]*,[0-9]*,4' lab2b_list.csv" using ($2):($3) \
	title 'Mutex-Lock' with points lc rgb 'violet', \
    "< grep 'list-id-s,[0-9]*,[0-9]*,4' lab2b_list.csv" using ($2):($3) \
	title 'Spin-Lock' with points lc rgb 'blue', \

# unset the kinky x axis
unset xtics
set xtics

set title "List-4: Throughput vs. Threads (Mutex)"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:15]
set ylabel "Throughput (Operations/s)"
set logscale y 10
set output 'lab2b_4.png'
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,1' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'Lists=1' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-m,[0-9]*,1000,4' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'Lists=4' with linespoints lc rgb 'green', \
     "< grep -e 'list-none-m,[0-9]*,1000,8' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title 'Lists=8' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-m,[0-9]*,1000,16' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title 'Lists=16' with linespoints lc rgb 'violet'

set title "List-5: Throughput vs. Threads (Spin-Lock)"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:15]
set ylabel "Throughput (Operations/s)"
set logscale y 10
set output 'lab2b_5.png'
plot \
     "< grep -e 'list-none-s,[0-9]*,1000,1' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title 'Lists=1' with linespoints lc rgb 'blue', \
     "< grep -e 'list-none-s,[0-9]*,1000,4' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title 'Lists=4' with linespoints lc rgb 'green', \
     "< grep -e 'list-none-s,[0-9]*,1000,8' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title 'Lists=8' with linespoints lc rgb 'red', \
     "< grep -e 'list-none-s,[0-9]*,1000,16' lab2b_list.csv" using ($2):(1000000000/($7)) \
        title 'Lists=16' with linespoints lc rgb 'violet'