#!/bin/sh

cp ../YAMLs/measurer_rosch.yaml /tmp/
./configure
cd ./RESCH
make
sudo make install
cd ../
cd ./RT-ROS/
make install
#roscore
