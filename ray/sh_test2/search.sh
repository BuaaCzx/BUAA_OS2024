#!/bin/bash
#First you can use grep (-n) to find the number of lines of string.
#Then you can use awk to separate the answer.
touch $3
tmp='$1'
echo $tmp
echo "grep -n $2 $1 > $3"
grep -n $2 $1 > $3
echo "awk -F: '{print $tmp}' > $3"
awk -F: '{print $1}' $1 > $3
