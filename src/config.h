/**
 * @file config.h
 * @brief
 *
 * @date Apr 22, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef CONFIG_H_
#define CONFIG_H_
#include <stdbool.h>

enum config_key {
	CONFIG_FIRST,

	CONFIG_MOUNT_HOOK = CONFIG_FIRST,
	CONFIG_SOCKET_PATH,
	CONFIG_RESOURCES_DIR,
	CONFIG_FIRMWARE_REPOSITORY,
	CONFIG_BASE_MOUNT_PATH,
	CONFIG_NET_HOOK,
	CONFIG_PREVENT_REMOVAL,
	CONFIG_CONTAINER_INTERFACE,
	CONFIG_HOST_INTERFACE_PREFIX,
	CONFIG_NET_FIRST_TWO_BYTES,
	CONFIG_DUMP_PROFILE,
	CONFIG_DISABLE_APPARMOR,

	CONFIG_NB,
};

int config_init(const char *path);
const char *config_get(enum config_key);
bool config_get_bool(enum config_key);
void config_cleanup(void);

#endif /* CONFIG_H_ */
