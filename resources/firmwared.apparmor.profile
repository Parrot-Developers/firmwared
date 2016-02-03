 flags=(attach_disconnected) {
# firmwared will load it, with "@{root}=%S1%\nprofile %S2% %S3% " prepended,
# with :
#   S1: the base workspace directory for the instance, containing the ro, rw,
#       union and workdir directories / mountpoints
#   S2: the instance unique name
#   S3: the content of this very file

# this rule set is twisting the apparmor philosophy by creating a blacklist, but
# creating a whitelist really is a PITA, will make devs' work harder and we are
# not protecting the host against a malicious attacker in the first place.

# give the instances all the capabilities known so far. This is surely overkill
# and can surely be refined. I gladly accept patches
   capability sys_chroot,
   capability sys_admin,
   capability chown,
   capability dac_override,
   capability dac_read_search,
   capability fowner,
   capability fsetid,
   capability kill,
   capability setgid,
   capability setuid,
   capability setpcap,
   capability linux_immutable,
   capability net_bind_service,
   capability net_broadcast,
   capability net_admin,
   capability net_raw,
   capability ipc_lock,
   capability ipc_owner,
   capability sys_module,
   capability sys_rawio,
   capability sys_chroot,
   capability sys_ptrace,
   capability sys_pacct,
   capability sys_admin,
   capability sys_boot,
   capability sys_nice,
   capability sys_resource,
   capability sys_tty_config,
   capability mknod,
   capability lease,
   capability audit_write,
   capability audit_control,
   capability setfcap,
   capability mac_override,
   capability mac_admin,
   capability syslog,

   signal,
   mount,
   network,
   ptrace,

# first we allow all filesystem accesses ...
   /** lrwixk,
   /* lrwixk,
   / lrwixk,

# ... then we restrict what is suspected (or known) to be harmful
   audit deny @{root}/union/proc/sysrq-trigger lrwx,
   audit deny @{root}/union/dev/sd** lrwx,
   audit deny @{root}/union/dev/mem lrwx,
   audit deny @{root}/union/dev/watchdog** lrwx,
   audit deny @{root}/union/dev/disk/** lrwx,
   audit deny @{root}/union/dev/disk/ lrwx,
   audit deny @{root}/union/proc/sys/vm/** lwx,
   audit @{root}/union/{proc,dev,sys}/** lrwix,
}
