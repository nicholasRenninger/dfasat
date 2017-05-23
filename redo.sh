#!/bin/bash

if [ $# -ne 1 ]; then
    echo "usage: $0 dot-file (from flexfringe)"
    exit 1
fi


read -r COMMAND < $1

${COMMAND:3}
