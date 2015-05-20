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

#define CONFIG_CONSTRUCTOR_PRIORITY 101

enum config_key {
	CONFIG_FIRST,

	CONFIG_MOUNT_HOOK = CONFIG_FIRST,
	CONFIG_SOCKET_PATH,
	CONFIG_RESOURCES_DIR,
	CONFIG_FIRMWARE_REPOSITORY,
	CONFIG_BASE_MOUNT_PATH,
	CONFIG_NET_HOOK,

	CONFIG_NB,
};

const char *config_get(enum config_key);

#endif /* CONFIG_H_ */
