# Using Analyzer

Follow the instruction below to analyze task allocation with fixted priority scheduling. 
Analyzer dipicts a graph and a scheduling result. 
`$(TOPDIR)` represents your top working directory.

## 1. Preparation

Please, prepare __anazlyzer_rosch.yaml__ in `$(TOPDIR)/YAMLs/`.

Required ROS node information:

 * `nodename` : the name of ROS node
 * `core` : if this node uses n cores (e.g., OpenMP), core is n.
 * `sub_topic` : topics for subscribe
 * `pub_topic` : topics for publish
 * `run_time` : the execution time
 * `deadline` : if an end node, set a deadline. Otherwise set to 0.
 * `period` : if an entry node, set a period. Otherwise set to 0.


## 2. How to make

```sh
$ cd $(TOPDIR)/Analyzer
$ make
``` 

## 3. How to use

```sh 
$ cd $(TOPDIR)/Analyzer/bin
$ ./Analyzer
```

