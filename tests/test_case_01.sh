#!/bin/bash

set -x

instance=$(fdc prepare /opt2/work/dragonfly/out/dragonfly-mykonos3_sim_pc/final | tail -n 1 | sed 's/.*: //g');
fdc set_property instances $instance cmdline[] nil
fdc set_property instances $instance cmdline[0] /sbin/boxinit
fdc set_property instances $instance cmdline[1] ro.boot.console=/pts/10
fdc set_property instances $instance cmdline[2] ro.hardware=mykonos3board

instance1=$instance

instance=$(fdc prepare /opt2/work/dragonfly/out/dragonfly-mykonos3_sim_pc/final | tail -n 1 | sed 's/.*: //g');
fdc set_property instances $instance cmdline[] nil
fdc set_property instances $instance cmdline[0] /sbin/boxinit
fdc set_property instances $instance cmdline[1] ro.boot.console=/pts/10
fdc set_property instances $instance cmdline[2] ro.hardware=mykonos3board

instance2=$instance

instance=$(fdc prepare /opt2/work/dragonfly/out/dragonfly-mykonos3_sim_pc/final | tail -n 1 | sed 's/.*: //g');
fdc set_property instances $instance cmdline[] nil
fdc set_property instances $instance cmdline[0] /sbin/boxinit
fdc set_property instances $instance cmdline[1] ro.boot.console=/pts/10
fdc set_property instances $instance cmdline[2] ro.hardware=mykonos3board

instance3=$instance

instance=$(fdc prepare /opt2/work/dragonfly/out/dragonfly-mykonos3_sim_pc/final | tail -n 1 | sed 's/.*: //g');
fdc set_property instances $instance cmdline[] nil
fdc set_property instances $instance cmdline[0] /sbin/boxinit
fdc set_property instances $instance cmdline[1] ro.boot.console=/pts/10
fdc set_property instances $instance cmdline[2] ro.hardware=mykonos3board

instance4=$instance

instance=$(fdc prepare /opt2/work/dragonfly/out/dragonfly-mykonos3_sim_pc/final | tail -n 1 | sed 's/.*: //g');
fdc set_property instances $instance cmdline[] nil
fdc set_property instances $instance cmdline[0] /sbin/boxinit
fdc set_property instances $instance cmdline[1] ro.boot.console=/pts/10
fdc set_property instances $instance cmdline[2] ro.hardware=mykonos3board

instance5=$instance

fdc drop instances $instance1
fdc drop instances $instance2
fdc drop instances $instance3
fdc drop instances $instance4
fdc drop instances $instance5

