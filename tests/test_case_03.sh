#!/bin/bash

set -x

instance=$(fdc prepare /opt2/work/dragonfly/out/dragonfly-mykonos3_sim_pc/final | tail -n 1 | sed 's/.*: //g');
fdc drop instances $instance
instance=$(fdc prepare /opt2/work/dragonfly/out/dragonfly-mykonos3_sim_pc/final | tail -n 1 | sed 's/.*: //g');
instance=$(fdc prepare /opt2/work/dragonfly/out/dragonfly-mykonos3_sim_pc/final | tail -n 1 | sed 's/.*: //g');
