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

set title "Wait for lock and avg operation vs number of threads"
set xlabel "Number of threads"
set logscale x 10
set ylabel "Time (ns)"
set logscale y 10
set output 'lab2b_2.png'

# Plot wait for lock time and average operation time vs number of threads
plot \
     "<(grep -v 'list-none-m,12,[0-9]*,1,' lab_2b_list.csv | grep 'list-none-m,[0-9]*,[0-9]*,1,')" using ($2):($7) \
	title 'Average operation time' with linespoints lc rgb 'red', \
	"<(grep -v 'list-none-m,12,[0-9]*,1,' lab_2b_list.csv | grep 'list-none-m,[0-9]*,[0-9]*,1,')" using ($2):($8) \
	title 'Wait for lock time' with linespoints lc rgb 'blue'

set title "Iterations without failure"                                                                                             
set xlabel "Number of threads"                                                                                                                               
set logscale x 10                                                                                                                                            
set ylabel "Iterations"                                                                                                                                      
set logscale y 10                                                                                                                                            
set output 'lab2b_3.png'

plot \
     "< grep 'list-id-none,[0-9]*,[0-9]*,4,' lab_2b_list.csv" using ($2):($3) \
        title 'Without synchronization' with points lc rgb 'red', \
	"<grep 'list-id-m,[0-9]*,[0-9]*,4,' lab_2b_list.csv" using ($2):($3) \
        title 'Mutex' with points lc rgb 'blue',\
	"<grep 'list-id-s,[0-9]*,[0-9]*,4,' lab_2b_list.csv" using ($2):($3) \
        title 'Spin Lock' with points lc rgb 'green' 

set title "Throughput for mutex partitioned lists"                                                                                             
set xlabel "Threads"                                                                                                                               
set logscale x 10                                                                                                                                            
set ylabel "Throughput"                                                                                                                                      
set logscale y 10                                                                                                                                            
set output 'lab2b_4.png'

plot \
     "<grep 'list-none-m,[0-9][0-2]*,[0-9]*,1,' lab_2b_list.csv" using ($2):((1000000000)/($7)) \
       title 'One list' with linespoints lc rgb 'red', \
     "<grep 'list-none-m,[0-9]*,[0-9]*,4,' lab_2b_list.csv" using ($2):((1000000000)/($7)) \
     title 'Two lists' with linespoints lc rgb 'blue', \
     "<grep 'list-none-m,[0-9]*,[0-9]*,8,' lab_2b_list.csv" using ($2):((1000000000)/($7)) \
     title 'Three lists' with linespoints lc rgb 'orange', \
     "<grep 'list-none-m,[0-9]*,[0-9]*,16,' lab_2b_list.csv" using ($2):((1000000000)/($7)) \
     title 'Four lists' with linespoints lc rgb 'green' 
                                                                                                   
      
set title "Throughput for spin lock partitioned lists"                                                                                             
set xlabel "Threads"                                                                                                                               
set logscale x 10                                                                                                                                            
set ylabel "Throughput"                                                                                                                                      
set logscale y 10                                                                                                                                            
set output 'lab2b_5.png'

plot \
     "<grep 'list-none-s,[0-9]*,[0-9]*,1,' lab_2b_list.csv" using ($2):((1000000000)/($7)) \
       title 'One list' with linespoints lc rgb 'red', \
     "<grep 'list-none-s,[0-9]*,[0-9]*,4,' lab_2b_list.csv" using ($2):((1000000000)/($7)) \
     title 'Two lists' with linespoints lc rgb 'blue', \
     "<grep 'list-none-s,[0-9]*,[0-9]*,8,' lab_2b_list.csv" using ($2):((1000000000)/($7)) \
     title 'Three lists' with linespoints lc rgb 'orange', \
     "<grep 'list-none-s,[0-9]*,[0-9]*,16,' lab_2b_list.csv" using ($2):((1000000000)/($7)) \
     title 'Four lists' with linespoints lc rgb 'green' 


