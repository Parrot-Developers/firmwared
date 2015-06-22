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

#include <io_mon.h>
#include <io_src.h>
#include <io_src_sig.h>

#ifndef FIRMWARED_GROUP
#define FIRMWARED_GROUP "firmwared"
#endif /* FIRMWARED_GROUP */

int firmwared_init(void);
void firmwared_run(void);
void firmwared_stop(void);
int firmwared_notify(uint32_t msgid, const char *fmt, ...);
#define firmwared_answer(c, m, f, ...) pomp_conn_send(c, pomp_msg_get_id(m), \
		f, __VA_ARGS__)
struct io_mon *firmwared_get_mon(void);

void firmwared_clean(void);

#endif /* FIRMWARED_H_ */
