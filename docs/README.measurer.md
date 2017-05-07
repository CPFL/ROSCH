# Using Measurer

Follow the instruction below to measure the execution time for ROS node. `$(TOPDIR)` represents your top working directory.

## 1. How to Install

```sh
$ cd $(TOPDIR)/Measurer
$ ./install.sh
``` 

## 2. How to use

node_graph.yaml is listed ROS node informations. Please, change node_graph.yaml.
After that, please run nodes that you wrote in node_graph.yaml. Results is in ~/.ros/rosch/***

Required ROS node information:

 * `nodename`: the name of ROS node
 * `core`: if this node uses n cores, core is n.
 * `sub_topic`: topic for subscribe
 * `pub_topic`: topic for publish


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
