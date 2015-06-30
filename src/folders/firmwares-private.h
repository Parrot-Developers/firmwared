/**
 * @file firmwares-private.h
 * @brief
 *
 * @date June 30, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef FIRMWARES_PRIVATE_H_
#define FIRMWARES_PRIVATE_H_
#include <openssl/sha.h>

#include "../folders.h"

struct firmware {
	struct folder_entity entity;
	char *path;
	char sha1[2 * SHA_DIGEST_LENGTH + 1];

	/* retrieve from /etc/build.prop */
	char *product;
};

#define to_firmware(p) ut_container_of(p, struct firmware, entity)

#endif /* FIRMWARES_PRIVATE_H_ */
