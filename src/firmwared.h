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

#include <uv.h>

#include <libpomp.h>

struct firmwared {
	uv_poll_t pomp_handle;
	struct pomp_ctx *pomp;
};

int firmwared_init(struct firmwared *ctx);
void firmwared_run(struct firmwared *ctx);
int firmwared_notify(struct firmwared *ctx, uint32_t msgid, const char *fmt,
		...);
void firmwared_clean(struct firmwared *ctx);

#endif /* FIRMWARED_H_ */
