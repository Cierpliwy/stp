#!/bin/bash
if [ $# != 1 ]; then
    echo "Pass test folder as a argument!"
    exit 1
fi

for file in $1/*
do
    echo "$file"
    if [ -f $file ]; then
        $COLORTERM -e "./stp $file"
    fi
done


