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

#include <ut_string.h>

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

char *get_argz_i(const char *argz, size_t argz_len, int i)
{
	char *entry = NULL;

	for (entry = argz_next(argz, argz_len, entry);
			entry != NULL && i != 0;
			entry = argz_next(argz, argz_len, entry), i--)
		;

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

int argz_property_seti(char **argz, size_t *argz_len, unsigned index,
		const char *value)
{
	size_t count;
	char *entry;
	char *before;

	if (ut_string_is_invalid(value))
		return -EINVAL;

	count = argz_count(*argz, *argz_len);

	if (index > count) {
		ULOGE("index %u above array size %ju", index, (uintmax_t)count);
		return -ERANGE;
	}

	if (index == count) {
		/* truncation required at the same size the cmdline has */
		if (ut_string_match(value, "nil"))
			return 0;

		/* append */
		return -argz_add(argz, argz_len, value);
	}

	/* truncate */
	entry = get_argz_i(*argz, *argz_len, index);
	if (ut_string_match(value, "nil")) {
		/* changing the size suffices */
		*argz_len = entry - *argz;
		return 0;
	}

	/* delete and insert */
	argz_delete(argz, argz_len, entry);
	entry = NULL; /* /!\ entry is now invalid */
	before = get_argz_i(*argz, *argz_len, index);

	return -argz_insert(argz, argz_len, before, value);
}
