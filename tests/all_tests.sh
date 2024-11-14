#!/bin/bash

readarray -t arr < test_vecs.txt

pids=()

for i in $(seq 0 $((${#arr[@]} - 1)))
do
	
	dir="$(echo ${arr[i]} | head -n1 | awk '{print $1;}')"
	bash -c "cd $dir && cp ../SWFModernRuntime/SWFModernRuntime.lib ./ && ../SWFRecomp test.swf > /dev/null && mkdir -p build && cd build && cmake .. > /dev/null 2> /dev/null && cmake --build . --config Release > /dev/null" &
	pids+=("$!")
	
done

for i in $(seq 0 $((${#arr[@]} - 1)))
do
	
	wait ${pids[i]}
	echo ${arr[i]} | head -n1 | awk '{print $1;}'
	bash -c "./test.sh ${arr[i]}" || exit
	echo ""
	
done
