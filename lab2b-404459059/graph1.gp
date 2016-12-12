#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#
# output:
#	lab2_list-1.png ... cost per operation vs threads and iterations
#	lab2_list-2.png ... threads and iterations that run (un-protected) w/o failure
#	lab2_list-3.png ... threads and iterations that run (protected) w/o failure
#	lab2_list-4.png ... cost per operation vs number of threads
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#

# general plot parameters
set terminal png
set datafile separator ","

set title "Throughput: Operations per second"
set xlabel "Number of threads"
set logscale x 10
set ylabel "Operations per second (1/ns)"
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep 'list-none-m,' lab2a.csv" using ($2):((1000000000)/($7)) \
	title 'Mutex List' with linespoints lc rgb 'red', \
	"< grep 'list-none-s,' lab2a.csv" using ($2):((1000000000)/($7)) \
	title 'Spin Lock List' with linespoints lc rgb 'green', \
	"< grep 'add-m,' lab2a.csv" using ($2):((1000000000)/($6)) \
	title 'Mutex Add' with linespoints lc rgb 'blue', \
	"< grep 'add-s,' lab2a.csv" using ($2):((1000000000)/($6)) \
	title 'Spin Lock Add' with linespoints lc rgb 'orange'


