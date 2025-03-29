#!/bin/bash

readarray -t arr < test_vecs.txt

pids=()

num_tests=$((0))
num_passed=$((0))

for i in $(seq 0 $((${#arr[@]} - 1)))
do
	
	dir="$(echo ${arr[i]} | head -n1 | awk '{print $1;}')"
	bash -c "cd $dir && rm -f build/Release/TestSWFRecompiled.exe && cp ../SWFModernRuntime/SWFModernRuntime.lib ./ && ../SWFRecomp test.swf > /dev/null && mkdir -p build && cd build && cmake .. > /dev/null 2> /dev/null && cmake --build . --config Release > /dev/null" &
	pids+=("$!")
	num_tests=$((num_tests + 1))
	
done

for i in $(seq 0 $((${#arr[@]} - 1)))
do
	
	wait ${pids[i]}
	echo ${arr[i]} | head -n1 | awk '{print $1;}'
	bash -c "./test.sh ${arr[i]}"
	
	if [ $? -eq 0 ]; then
		num_passed=$((num_passed + 1))
	fi
	
	echo ""
	
done

echo "Passed $num_passed/$num_tests tests"
echo ""
