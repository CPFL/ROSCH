# Using Scheduler

Follow the instruction below to schedule for ROS node with Fixed-Priority.
`$(TOPDIR)` represents your top working directory.

## 1. How to use

 Please list ROS node informations you schedule in `$(TOPDIR)/YAMLs/scheduler_rosch.yaml`.
 Then, please change __core__, __run_time__, and __sched_info__ as necessary.
After that, launch ROS nodes listed in scheduler_rosch.yaml.

Required ROS node information:
  
 * `nodename` : the name of ROS node
 * `core` : if this node uses n cores, core is n.
 * `sub_topic` : topic for subscribe
 * `pub_topic` : topic for publish
 * `run_time` : the execution time
 * `sched_info` : scheduling parameters (i.g., core, priority, start_time, run_time). Note that __core__ at sched_info indicates tha place to assign ROS node.

## 2. How to Install

```sh
$ cd $(TOPDIR)/Scheduler
$ ./install.sh
``` 

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
