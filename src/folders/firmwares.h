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

struct firmware *firmware;

const char *firmware_get_path(struct firmware *firmware);
const char *firmware_get_sha1(const struct firmware *firmware);

#endif /* FIRMWARES_H_ */
