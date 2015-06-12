#!/bin/bash

# nok

#set -x

turns=${turns:-4}

for (( i=1; i < $turns; i++)); do
		(pomp-cli --timeout 10 --dump --wait i unix:/var/run/firmwared.sock i %s PING) &
		pid[i]=$!
		echo "launched command "${pid[i]}
done

for (( i=1; i < $turns; i++)); do
		echo "wait for command "${pid[i]}
				wait ${pid[i]}
done
