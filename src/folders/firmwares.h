/**
 * @file firmwares.h
 * @brief
 *
 * @date Apr 21, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef FIRMWARES_H_
#define FIRMWARES_H_
#include "folders.h"

struct firmware;

#define FIRMWARES_FOLDER_NAME "firmwares"

int firmwares_init(void);
struct firmware *firmware_from_entity(struct folder_entity *entity);
const char *firmware_get_path(const struct firmware *firmware);
const char *firmware_get_sha1(const struct firmware *firmware);
const char *firmware_get_name(const struct firmware *firmware);
const char *firmware_get_uuid(struct firmware *firmware);
void firmwares_cleanup(void);

#endif /* FIRMWARES_H_ */
