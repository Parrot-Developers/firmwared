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
#include <argz.h>

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

char *get_argz_i(char *argz, size_t argz_len, int i)
{
	char *entry = NULL;

	for (entry = argz_next(argz, argz_len, entry);
			entry != NULL && i != 0;
			entry = argz_next(argz, argz_len, entry), i--);

	return entry;
}

int argz_property_geti(const char *argz, size_t argz_len, unsigned index,
		char **value)
{
	size_t count;

	if (value == NULL)
		return -EINVAL;

	count = argz_count(argz, argz_len);
	if (index > count)
			return -ERANGE;

	if (index == count)
		*value = strdup("nil");
	else
		*value = strdup(get_argz_i(argz, argz_len, index));

	return *value == NULL ? -errno : 0;
}

