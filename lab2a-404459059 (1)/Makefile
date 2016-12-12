default:
	gcc -o lab2_add lab2_add.c -g -lrt -pthread
	gcc -o lab2_list SortedList.c lab2_list.c -g -lrt -pthread
build: default
clean:
	@-rm -rf lab2_add lab2_list *~ *.tar.gz
tests: default
	@-rm *.csv
	@-./add_script.sh
	@-./add_list.sh
graphs: default tests
	@-/u/cs/grad/zhou/iloveos/gnuplot lab2_add.gp
	@-/u/cs/grad/zhou/iloveos/gnuplot lab2_list.gp
tarball: default tests graphs
	tar -czf lab2a-404459059.tar.gz *.c *.h *.sh *.png Makefile README.txt *.csv
