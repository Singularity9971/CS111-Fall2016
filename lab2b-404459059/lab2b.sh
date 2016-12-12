#./lab2_list --iterations 1000 --threads 1 --sync m >> lab_2b_list.csv                                                                                        
#./lab2_list --iterations 1000 --threads 2 --sync m >> lab_2b_list.csv                                                                                        
#./lab2_list --iterations 1000 --threads 4 --sync m >> lab_2b_list.csv                                                                                        
#./lab2_list --iterations 1000 --threads 8 --sync m >> lab_2b_list.csv                                                                                        
#./lab2_list --iterations 1000 --threads 16 --sync m >> lab_2b_list.csv                                                                                       
#./lab2_list --iterations 1000 --threads 24 --sync m >> lab_2b_list.csv
 
threads=(1 4 8 12 16)
iterationsPass=(10 20 40 80)
iterationsFail=(1 2 4 8 16)
lists=(16 8 4 1)
sync=(m s)

threadsFinal=(1 2 4 8 12)

for thread in "${threads[@]}"; do
    for opt in "${sync[@]}"; do
	for num in "${iterationsPass[@]}"; do
	    ./lab2_list --yield=id --iterations=$num --threads=$thread --sync=$opt --lists=4 >> lab_2b_list.csv
	done
    done
done

for thread in "${threads[@]}"; do
    for num in "${iterationsFail[@]}"; do
        ./lab2_list --yield=id --iterations=$num --threads=$thread --lists=4 >> lab_2b_list.csv
    done
done

for thread in "${threadsFinal[@]}"; do
    for opt in "${sync[@]}"; do
	for num in "${lists[@]}"; do
	    #if [ "$thread" -eq 12 ] && [ "$num" -eq 1 ]; then
		#continue
	    #fi
	    ./lab2_list --iterations=1000 --threads=$thread --sync=$opt --lists=$num >> lab_2b_list.csv
	done
    done
done

./lab2_list --iterations 1000 --threads 16 --sync m >> lab_2b_list.csv                                                                                        
./lab2_list --iterations 1000 --threads 24 --sync m >> lab_2b_list.csv
