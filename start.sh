#!/bin/bash

if [ $# -ne 2 ]; then
    echo usage: $0 ini-file
    exit 1
fi

param=$(grep -v '^#' $1 |{
    temp=""
    while read line
    do
        temp="$temp --$(echo $line)"    
    done
    echo -n $temp
})
#echo $param

./flexfringe $param $2
