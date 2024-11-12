#!/bin/bash

readarray -t arr < test_vecs.txt

for i in $(seq 0 $((${#arr[@]} - 1)))
do
	
	echo ${arr[i]} | head -n1 | awk '{print $1;}'
	bash -c "./test.sh ${arr[i]}" || exit
	echo ""
	
done
