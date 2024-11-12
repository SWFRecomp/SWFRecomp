#!/bin/bash

bash -c "cd $1 && cp ../SWFModernRuntime/build/Release/SWFModernRuntime.lib ./ && ../SWFRecomp test.swf > /dev/null && mkdir -p build && cd build && cmake .. > /dev/null 2> /dev/null && cmake --build . --config Release > /dev/null" || exit
result=$(bash -c "cd $1/build && ./Release/TestSWFRecompiled")

echo ""
echo "Expected: \`$2'"
echo "Actual:   \`$result'"
echo ""

if [[ $result =~ "$2" ]]; then
	echo "PASSED"
else
	echo "FAILED"
fi

echo ""
