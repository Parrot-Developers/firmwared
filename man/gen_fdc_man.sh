#!/bin/bash

set -eux

command='for c in $(fdc commands | sed "s/ /\n/g" | sort); do fdc help $c | egrep -v "^Command " | sed "s/Synopsis:/.TP\n.B/g" | sed "s/Overview: /- /g"; done'
sed -e '/@@@ FDC_COMMAND @@@/,$d' fdc.1.template
echo '.\" '${command}
echo "${command}" | sh
sed -n -e '/@@@ FDC_COMMAND @@@/,$p' fdc.1.template