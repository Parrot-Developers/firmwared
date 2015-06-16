/**
 * @file utils.h
 * @brief
 *
 * @date Apr 21, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef UTILS_H_
#define UTILS_H_
#include <stddef.h>
#include <stdbool.h>

char *buffer_to_string(const unsigned char *src, size_t len, char *dst);
bool is_directory(const char *path);

#endif /* UTILS_H_ */
