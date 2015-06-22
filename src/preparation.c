/**
 * @file preparation.c
 * @brief
 *
 * @date June 16, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <errno.h>
#include <string.h>

#include <ut_string.h>

#include "preparation.h"

int preparation_init(struct preparation *preparation,
		const char *identification_string, uint32_t msgid,
		preparation_completion_cb completion)
{
	if (preparation == NULL ||
			ut_string_is_invalid(identification_string) ||
			completion == NULL)
		return -EINVAL;

	/* no memset, the preparation implementation has filled some fields */
	preparation->msgid = msgid;
	preparation->completion = completion;
	preparation->identification_string = strdup(identification_string);
	if (preparation->identification_string == NULL)
		return -errno;
	preparation->has_ended = false;

	return 0;
}

/* preparation_match_str_identification_string */
RS_NODE_MATCH_STR_MEMBER(preparation, identification_string, node)

void preparation_clean(struct preparation *preparation)
{
	if (preparation == NULL)
		return;
	ut_string_free(&preparation->identification_string);
}
