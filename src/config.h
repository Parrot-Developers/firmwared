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

enum config_key {
	CONFIG_FIRST,

	CONFIG_MOUNT_HOOK = CONFIG_FIRST,
	CONFIG_SOCKET_PATH,
	CONFIG_RESOURCES_DIR,
	CONFIG_FIRMWARE_REPOSITORY,
	CONFIG_BASE_MOUNT_PATH,
	CONFIG_NET_HOOK,
	CONFIG_PREVENT_REMOVAL,

	CONFIG_NB,
};

int config_init(const char *path);
const char *config_get(enum config_key);
void config_cleanup(void);

#endif /* CONFIG_H_ */
