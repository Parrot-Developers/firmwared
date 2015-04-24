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

int firmwared_init(struct firmwared *f);
void firmwared_run(struct firmwared *f);
int firmwared_notify(struct firmwared *f, uint32_t msgid, const char *fmt, ...);
#define firmwared_answer(c, m, f, ...) pomp_conn_send(c, pomp_msg_get_id(m), \
		f, __VA_ARGS__)

void firmwared_clean(struct firmwared *f);

#endif /* FIRMWARED_H_ */
