#!/bin/bash

taskset -c 1 stress -c 1 &
taskset -c 2 stress -c 1 &
taskset -c 3 stress -c 1 &
taskset -c 4 stress -c 1 &
taskset -c 5 stress -c 1 &
taskset -c 6 stress -c 1 &
taskset -c 7 stress -c 1 &
taskset -c 8 stress -c 1 &
taskset -c 9 stress -c 1 &
taskset -c 10 stress -c 1 &
taskset -c 11 stress -c 1 &