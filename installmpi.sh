#!/usr/bin/env bash

sudo agt-get update
sudo apt-get install gcc make g++
cd /home/ubuntu/
wget http://www.mpich.org/static/downloads/3.2/mpich-3.2.tar.gz
tar -xvf mpich-3.2.tar.gz
cd mpich-3.2/
./configure --prefix=/home/ubuntu/mpich-install/ --disable-fortran --disable-cxx
make
make install
