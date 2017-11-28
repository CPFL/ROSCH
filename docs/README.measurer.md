# Using Measurer

Follow the instruction below to measure the execution time for ROS node. `$(TOPDIR)` represents your top working directory.

## 1. How to use

Please list ROS node informations you measure in `$(TOPDIR)/YAMLs/measurer_rosch.yaml`. 
Then, please change __core__ as necessary.
After that, launch ROS nodes listed in measurer_rosch.yaml. 
Results is in `~/.ros/rosch/***`.

Required ROS node information:

 * `nodename`: the name of ROS node
 * `core`: if this node uses n cores (e.g., OpenMP), core is n.
 * `sub_topic`: topics for subscribe
 * `pub_topic`: topics for publish

## 2. How to Install

```sh
$ cd $(TOPDIR)/Measurer
$ ./install.sh
``` 

## Uninstall or Re-install

```sh
$ cd $(TOPDIR)/Measurer 
$ ./uninstall.sh
```

If you change config or source files, please put following commond.

``` 
$ cd $(TOPDIR)/Measurer 
$ ./reinstall.sh
``` 
