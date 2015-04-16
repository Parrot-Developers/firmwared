/**
 * @file main.c
 * @brief
 *
 * @date Apr 16, 2015
 * @author nicolas.carrier@parrot.com
 * @copyright Copyright (C) 2015 Parrot S.A.
 */
#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <string.h>
#include <libgen.h>
#include <stdlib.h>
#include <stdio.h>

#define ULOG_TAG firmwared
#include <ulog.h>
ULOG_DECLARE_TAG(firmwared);



int main(int argc, char *argv[])
{
	ULOGI("%s[%jd] starting\n", basename(argv[0]), (intmax_t)getpid());

	return EXIT_SUCCESS;
}
