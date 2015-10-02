/**
 * @file process.c
 * @brief
 *
 * @date Oct 1, 2015
 * @author ncarrier
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#include <signal.h>

#include "log.h"
#include "process.h"

struct io_process_parameters process_default_parameters = {
	.stdout_sep_cb = log_dbg_src_sep_cb,
	.out_sep1 = '\n',
	.out_sep2 = IO_SRC_SEP_NO_SEP2,
	.stderr_sep_cb = log_warn_src_sep_cb,
	.err_sep1 = '\n',
	.err_sep2 = IO_SRC_SEP_NO_SEP2,
	.timeout = 1000,
	.signum = SIGKILL,
};
