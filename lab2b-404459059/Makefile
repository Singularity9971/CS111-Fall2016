default:
	@gcc -o lab2_list SortedList.c lab2_list.c -g -lrt -pthread
build: default                                                                                         
profile: default
	@-rm raw.perf
	@LD_PRELOAD=./lib/libprofiler.so.0 CPUPROFILE=raw.perf ./lab2_list --threads=12 --iterations=1000 --sync=s
	@-rm report
	@./pprof --text lab2_list raw.perf >> report
	@printf "\n\n" >> report
	@./pprof --list=thread_function lab2_list raw.perf >> report
tests: default
	@-rm lab_2b_list.csv
	@-./lab2b.sh
graphs: tests
	@-rm *.png
	@/u/cs/grad/zhou/iloveos/gnuplot graph1.gp
	@/u/cs/grad/zhou/iloveos/gnuplot graph2.gp
clean:
	@-rm *.png lab2_list raw.perf *.gz *~
tarball:
	@tar -czf lab2b-404459059.tar.gz *.c report *.sh Makefile README.txt pprof lib/ *.h *.csv *.gp
