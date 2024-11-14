#!/bin/bash

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
