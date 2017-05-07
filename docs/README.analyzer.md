# Using Analyzer

Follow the instruction below to analyze task allocation with fixted priority scheduling. `$(TOPDIR)` represents your top working directory.

## 1. How to make

```sh
$ cd $(TOPDIR)/Analyzer
$ make
``` 

## 2. How to use

Analyzer dipicts a graph and a scheduling result. Note that Analyzer refers to node_graph.yaml Please, change it.

Required ROS node information:

 * __nodename__ : the name of ROS node
 * __core__ : if this node uses n cores, core is n.
 * __sub_topic__ : topic for subscribe
 * __pub_topic__ : topic for publish
 * __run_time__ : the execution time
 * __deadline__ : if not the end node, this value is 0
 * __period__ : if not the entry node, this value is 0 

After that, you can see the graph and the result of scheduling.

```sh 
$ cd $(TOPDIR)/Analyzer/bin
$ ./Analyzer
```

