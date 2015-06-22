/**
 * @file log.h
 * @brief
 *
 * @date Jun 19, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#ifndef LOG_H_
#define LOG_H_
#include <io_src_sep.h>

void log_warn_src_sep_cb(struct io_src_sep *sep, char *chunk, unsigned len);
void log_dbg_src_sep_cb(struct io_src_sep *sep, char *chunk, unsigned len);

#endif /* LOG_H_ */
