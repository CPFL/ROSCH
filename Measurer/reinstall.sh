#!/bin/sh

cp ../node_graph.yaml /tmp/
cd ./RESCH
sudo make uninstall
make
sudo make install
cd ../
cd ./RT-ROS/
make install
roscore
