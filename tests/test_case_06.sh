#!/bin/bash

#set -x

turns=${turns:-2}

echo "*** "$turns" test turns ***"

for (( i=1; i <= $turns; i++)); do
	instance[i]=$(fdc prepare /opt2/work/dragonfly/out/dragonfly-mykonos3_sim_pc/final | tail -n 1 | sed 's/.*: //g');
	fdc start ${instance[i]}
done

sleep 10

for (( i=1; i <= $turns; i++)); do
	fdc kill ${instance[i]}
	fdc drop instances ${instance[i]}
done
