/**
 * @file log.c
 * @brief
 *
 * @date Jun 19, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#define ULOG_TAG firmwared_log
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared_log);

#include "log.h"

static void log_cb_implem(struct io_src_sep *sep, char *chunk, unsigned len,
		int level)
{
	if (len == 0)
		return;

	chunk[len - 1] = '\0';
	ULOG_PRI(level, "%s", chunk);
}

void log_warn_src_sep_cb(struct io_src_sep *sep, char *chunk, unsigned len)
{
	log_cb_implem(sep, chunk, len, ULOG_WARN);
}

void log_dbg_src_sep_cb(struct io_src_sep *sep, char *chunk, unsigned len)
{
	log_cb_implem(sep, chunk, len, ULOG_DEBUG);
}
