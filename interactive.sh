#!/usr/bin/env bash

# requires inotify-tool package
# based on Jonathan Hartley's snippet at https://superuser.com/questions/181517/how-to-execute-a-command-whenever-a-file-changes

function execute() {
    eval "dot -Tpdf $@ > $@.pdf"
    echo "fuck you "
}


inotifywait --quiet --recursive --monitor --event close_write --format "%w%f" ./tests/ \
| while read change; do
    if [[ "$change" == *.dot ]] 
    then
      execute "$change"
    fi
done 
