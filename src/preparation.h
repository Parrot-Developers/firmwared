/**
 * @file preparation.h
 * @brief
 *
 * @date june 16, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef PREPARATION_H_
#define PREPARATION_H_
#include <stdint.h>

#include <rs_node.h>

#include "firmwared.h"
#include "folders.h"

struct preparation;

typedef int (*preparation_completion_cb)(struct preparation *preparation,
			struct folder_entity *entity);

struct preparation {
	struct rs_node node;
	/* fields initialized by the get_prepare callback */
	/* actions the folder can initiate on the preparation */
	int (*start)(struct preparation *preparation);
	void (*abort)(struct preparation *preparation);
	const char *folder;

	/* fields initialized by the folder_prepare function */
	char *identification_string;
	uint32_t msgid;
	/* functions the preparation will call */
	/* error when entity is NULL with errno set */
	preparation_completion_cb completion;

	bool has_ended;
};

#define to_preparation(p) ut_container_of(p, struct preparation, node)

int preparation_init(struct preparation *preparation,
		const char *identification_string, uint32_t msgid,
		preparation_completion_cb completion);

int preparation_match_str_identification_string(struct rs_node *node,
		const void *identification_string);

void preparation_clean(struct preparation *preparation);

#endif /* FOLDERS_H_ */
