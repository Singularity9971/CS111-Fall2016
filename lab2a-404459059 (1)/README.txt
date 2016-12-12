Name --> Avirudh Theraja
ID --> 404459059
TA --> Diyu Zhou

---------------------------------- DESCRIPTION -----------------------------------
The tarball extraction contains all png and csv files. Running make tests overwrites the csv files. Running make graphs overwrites the
graphs. The gnuplot being used is the one in Zhou's iloveos directory. Make clean deletes only the executables and the tarball itself if it
is in the directory. The source files compile cleanly without any warnings. 

---------------------------------- ANSWERS ---------------------------------------

2.1.1 It takes many iterations because the more the iterations, the higher the probabilty of a collision/race condition where
      2 threads are reading/writing at the same time. In the add case the shared memory is a single int variable so to produce an indeterministic
      output we need many iterations.
      If the iterations are quite small, then it is unlikely that 2 or more threads will execute the critical section code at the same
      time hence they seldom fail.

2.1.2 Yield is much slower because it interrupts the execution flow because the thread yeilds the CPU. For this to happen, the thread's state is saved
      and then a context switch occurs. This adds a lot of additional time. It is not possible to get valid per-operation timings by using yield because
      a lot of time is wasted in context switching which will overwhelm the relatively small amount of time required for an operation.

2.1.3 For the average cost we also consider the overhead time to create the threads and the number of operations. The number of operations are directly
      proportional to the number of iterations and thus the average cost per operation becomes inversely proportional to the iteration/operations
      as the overhead cost is the ratio of overhead in thread creation divided by number of iterations.
      To get the 'correct' cost and eliminate the above mentioned ratio, we can just make the number of iterations very large so that they overwhelm
      the cost of thread creation.

2.1.4 The different locking mechanisms perform equivalently because the locks barely tie up the threads as the number of threads is so low. Each thread
      doesn't really have to wait, it just acquires the lock, does its job and frees the lock, all before another thread wants to acquire it.
      They all slow down with an increase in the number of threads because they all essentially block threads from accessing the critical section until the
      one that has acquired it is done with it, thus tying up the threads and slowing down the program.
      Spin locks work by essentially making the thread run in an infinite loop until it acquires the lock. If the number of threads is large, a particular
      thread might be stuck in this loop for a long time, consuming CPU and time. Thus the spin locking mechanisms becomes highly inefficient with a lot
      of threads.

2.2.1 In the add part, the mutex locking seems to be the most efficient as the cost per operation actually goes down when the number of threads is
      greater than 4. This implies that mutex locking scales well with number of threads, in contrast, the cost rises pretty linearly for other locking
      mechanisms. In the list part, the cost per opration rises linearly with the number of threads and is very close to the cost per operation for spin
      locks. In both parts, the cost rises initially but in the add part it goes down after 4 threads but in the list part it keeps rising. This could be
      because in case of list the critical section is much larger and more threads effectively leads to a higher cost per operation as more time consuming
      operations are being performed. For the add part, the cost probably goes down because the increase in the number of threads leads to an increase in
      the number of operations thus reducing the overhead cost of thread creation.

2.2.2 As mentioned above, mutexes scale well with larger number of threads which are executing small critical sections hence the cost per operation
      for mutexes in the add part is much better than spin locks as spin locks tie up the threads in an infinite loop. However in the list part, spin
      locking actually outperforms mutex locking by a small margin. This could be because that once a thread is done with a critical section, a spinning
      thread will instantly break out of the while loop whereas in the case of mutex locking, the next thread will have to be woken up so it can acquire
      the lock and execute the critical section. Hence spin locking works better in the case of lists, but worse in the case of add.
