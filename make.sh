#!/usr/bin/env bash
PATH="$HOME/bin:$HOME/.local/bin:$HOME/mpich-install/bin:$PATH"
mpicc main.cpp -std=c++11 -lstdc++ -O -o mpi
sh workerdeploy.sh
mpirun -np 3 -f hosts /home/ubuntu/MPI/mpi