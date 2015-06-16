/**
 * @file utils.c
 * @brief
 *
 * @date Apr 21, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <sys/stat.h>

#include <string.h>

#define ULOG_TAG firmwared_utils
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_utils);

#include "utils.h"

char *buffer_to_string(const unsigned char *src, size_t len, char *dst)
{
	static const char lut[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8',
		'9', 'a', 'b', 'c', 'd', 'e', 'f'};

	/* not necessary, but more explicit */
	dst[2 * len] = '\0';

	while (len--) {
		dst[2 * len] = lut[(src[len] & 0xF0) >> 4];
		dst[2 * len + 1] = lut[src[len] & 0x0F];
	}

	return dst;
}

// TODO move to ut_file
bool is_directory(const char *path)
{
	int ret;
	struct stat buf;

	memset(&buf, 0, sizeof(buf));
	ret = stat(path, &buf);
	if (ret < 0) {
		ULOGE("stat: %m");
		return false;
	}

	return S_ISDIR(buf.st_mode);
}

