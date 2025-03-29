#!/bin/bash

result=$(bash -c "cd $1/build && ./Release/TestSWFRecompiled")

echo ""
echo "Expected: \`$2'"
echo "Actual:   \`$result'"
echo ""

if [[ $result =~ "$2" ]]; then
	echo "PASSED"
	echo ""
	exit 0
else
	echo "FAILED"
	echo ""
	exit -1
fi
