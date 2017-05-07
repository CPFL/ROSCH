# Using Scheduler

Follow the instruction below to schedule for ROS node with Fixed-Priority. `$(TOPDIR)` represents your top working directory.

## 1. How to Install

```sh
$ cd $(TOPDIR)/Scheduler
$ ./install.sh
``` 

## 2. How to use

node_graph.yaml is listed ROS node informations. Please, change node_graph.yaml.
After that, please run nodes that you wrote in node_graph.yaml.

Required ROS node information:
  
 * __nodename__ : the name of ROS node
 * __core__ : if this node uses n cores, core is n.
 * __sub_topic__ : topic for subscribe
 * __pub_topic__ : topic for publish
 * __run_time__ : the execution time
 * __deadline__ : if not the end node, this value is 0
 * __sched_info__ : scheduling parameters (i.g., `core`, `priority`, `start_time`, `run_time`). Note that `core` at sched_info indicates tha place to assign ROS node.


After that, you can see the graph and the result of scheduling.

## Uninstall or Re-install

```sh
$ cd $(TOPDIR)/Scheduling
$ ./uninstall.sh
```

If you change config or source files, please put following commond.

``` 
$ cd $(TOPDIR)/Schedluing
$ ./reinstall.sh
``` 
