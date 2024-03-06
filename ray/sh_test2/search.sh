#!/bin/bash
#First you can use grep (-n) to find the number of lines of string.
#Then you can use awk to separate the answer.
touch $3
touch tmp
echo "grep -n $2 $1 > tmp"
grep -n $2 $1 > tmp
awk -F: '{print $1}' tmp > $3
rm tmp
