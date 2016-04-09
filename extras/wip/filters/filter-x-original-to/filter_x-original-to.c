/*      $OpenBSD$   */

/*
 * Copyright (c) 2016 Thomas Schneider <qsuscs@qsuscs.de>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "includes.h"

#include <string.h>
#include <sys/types.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "smtpd-defines.h"
#include "smtpd-api.h"
#include "log.h"

struct udata {
	int             line;
	char           *rcpt;
};

static int
on_rcpt(uint64_t id, struct mailaddr *rcpt)
{
	log_debug("debug: on_rcpt");
	struct udata   *u = malloc(sizeof(*u));
	filter_api_set_udata(id, u);
	u->line = 0;
	u->rcpt = strdup(filter_api_mailaddr_to_text(rcpt));
	return filter_api_accept(id);
}

static void
on_dataline(uint64_t id, const char *line)
{
	log_debug("debug: on_dataline");
	struct udata   *u = filter_api_get_udata(id);
	if (u->line++ == 0) {
		char           *ret = NULL;
		asprintf(&ret, "X-Original-To: %s", u->rcpt);
		filter_api_writeln(id, ret);
		free(ret);
	}
	filter_api_writeln(id, line);
}

static void
on_disconnect(uint64_t id)
{
	log_debug("debug: on_disconnect");
	struct udata   *u = filter_api_get_udata(id);
	free(u->rcpt);
	free(u);
}

int
main(int argc, char **argv)
{
	int             ch, d = 0, v = 0;

	log_init(1);

	while ((ch = getopt(argc, argv, "dv")) != -1) {
		switch (ch) {
		case 'd':
			d = 1;
			break;
		case 'v':
			v |= TRACE_DEBUG;
			break;
		default:
			log_warnx("warn: bad option");
			return 1;
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	log_init(d);
	log_verbose(v);

	log_debug("debug: starting...");

	filter_api_on_rcpt(on_rcpt);
	filter_api_on_dataline(on_dataline);
	filter_api_on_disconnect(on_disconnect);

	filter_api_loop();
	log_debug("debug: exiting");

	return 1;
}