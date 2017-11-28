# Using Tracer

Tracer supports tracing and vizualization for processes including ROS node.

## 1. How to Install

```sh
$ cd $(TOPDIR)/ROSCH/Tracer
$ make
``` 

## 2. How to use

Please list the nodes you trace on the `$(TOPDIR)/trace_rosch.yaml`.
After that, please run following commands.

```sh
$ cd bin
$ ./Tracer (a GUI application is launched)
``` 

Required ROS node information:

 * `nodename`: the name of ROS node (process)
 * `sub_topic`: topic for subscribe
 * `pub_topic`: topic for publish
 * `deadline`: period  (Optional)
