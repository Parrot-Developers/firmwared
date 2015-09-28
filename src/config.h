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

#ifndef CONFIG_KEYS_PREFIX
#define CONFIG_KEYS_PREFIX "FIRMWARED_"
#endif /* CONFIG_KEYS_PREFIX */

enum config_key {
	CONFIG_FIRST,

	CONFIG_APPARMOR_PROFILE = CONFIG_FIRST,
	CONFIG_CONTAINER_INTERFACE,
	CONFIG_CURL_HOOK,
	CONFIG_DISABLE_APPARMOR,
	CONFIG_DUMP_PROFILE,
	CONFIG_HOST_INTERFACE_PREFIX,
	CONFIG_MOUNT_HOOK,
	CONFIG_MOUNT_PATH,
	CONFIG_NET_FIRST_TWO_BYTES,
	CONFIG_NET_HOOK,
	CONFIG_PREVENT_REMOVAL,
	CONFIG_RESOURCES_DIR,
	CONFIG_REPOSITORY_PATH,
	CONFIG_SOCKET_PATH,
	CONFIG_USE_AUFS,

	CONFIG_NB,
};

int config_init(const char *path);
enum config_key config_key_from_string(const char *key);
const char *config_get(enum config_key);
bool config_get_bool(enum config_key);
char *config_list_keys(void);
void config_cleanup(void);

#endif /* CONFIG_H_ */
