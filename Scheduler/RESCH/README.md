#Linux-rtx : Linux and Real-Time eXtension

##INSTALL:
###Build and install the RESCH core and library:

	./configure --task=(the maximum nubmer of real-time tasks)  
		--cpus=(the number of used CPUs)
	make
	sudo make install
	
By default, --task=64 and --cpus=NR_ONLINE_CPUS are set.  
###You can also use:  

(1) --disable-load-balance option, if you want to disable  
the load balance function supported by the Linux scheduler  
so that the system becomes more predictable.  

(2) and --use-hrtimer option, if you want to use high-resolution  
timers to release or wake up tasks.

By default, these options are unset.  

##Tests:
###Test the scheduling overhead:  

	./test/test_overhead
    
Note: this procedure may take a couple of minutes.  
	
###Install plugins if you like.  
E.g, you can install the state-of-the-art  
hybrid-partitioned-global EDF scheduler as follows:  
	
	cd plugin/hybrid/edf-wm/
	make
	make install 

You can also install other plugins in the same way.   
Note: some plugins must be exclusive to each other.  

###You can run a sample program that executes a real-time task with  
priority 50 and period 3000ms:  
    
    cd sample
    make
    ./rt_task 50 3000

The range of priority available for user applications is [4, 96].  
Please see /usr/include/resch/api.h for definition.  

###You can also run a sample program for gpu resource management.
You can management gpu resource without kernel modification.

		cd sample/gpu
		make -f Makefile.nvidia
		./user_test 3000 3000 3000 10

Note: you should modify Makefile.nvudia.

		# select GPU architecture  
		NV_ARCH = sm_(GPU architecture)

you can check GPU architecture

		cd sample/gpu/check_GPU
		./check_GPU

##Benchmark Programs:
###Several benchmarking programs are available to assess performance.  
 
	cd bench/  
	./configure  
	make  
	
###For instance, if you want to assess schedulability, run SchedBench.  
	
	cd schedbench  
	./schedbench --help  
	./schedbench (OPTIONS)  

##Have fun with Linux-rtx! You can develop your own plugins.  It would be nice if you notify me of your new plugins!  

-------
 Thanks,  
 Shinpei Kato <shinpei@il.is.s.u-tokyo.ac.jp>  
              <shinpei@ece.cmu.edu>  
	 
