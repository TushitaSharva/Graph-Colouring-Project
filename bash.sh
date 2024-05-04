#!/bin/bash

input_file="inp-params.txt"

# Read the first number from the first line of the input file
first_number=$(head -n 1 "$input_file" | awk '{print $1}')

rm proc*
rm complexity*
rm time*

# Compile the C++ code with mpic++
mpic++ algorithm.cpp -g -o a.exe

# Run with x threads and append output to space-VC.log and space-SK.log
mpirun --mca orte_base_help_aggregate 0 --oversubscribe -n $first_number ./a.exe

# Compile the C++ code with mpic++
mpic++ welsh_algorithm.cpp -g -o b.exe

# Run with x threads and append output to space-VC.log and space-SK.log
mpirun --mca orte_base_help_aggregate 0 --oversubscribe -n $first_number ./b.exe