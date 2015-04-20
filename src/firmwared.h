/**
 * @file firmwared.h
 * @brief
 *
 * @date Apr 17, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef FIRMWARED_H_
#define FIRMWARED_H_

#include <stdbool.h>

#include <libpomp.h>

struct firmwared {
	struct pomp_ctx *pomp;
	bool loop;
	struct pomp_decoder *decoder;
	uint32_t msg_id;
};

int firmwared_init(struct firmwared *ctx);
void firmwared_run(struct firmwared *ctx);
void firmwared_clean(struct firmwared *ctx);
/* returns UINT32_MAX on erron (ctx == NULL) */
uint32_t firmwared_get_msg_id(struct firmwared *ctx);

#endif /* FIRMWARED_H_ */
