#!/bin/sh

cp ../YAMLs/scheduler_rosch.yaml /tmp/
./configure
cd ./RESCH
sudo make uninstall
make
sudo make install
cd ../
cd ./RT-ROS/
make install
#roscore
