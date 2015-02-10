/*
 * Copyright (C) 2015 Cisco Systems, Inc.
 *
 * Author: Luka Perkov <luka.perkov@sartura.hr>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <libubox/uloop.h>

#include "sysrepo/sysrepo.h"
#include "../internal.h"

#include "ubus.h"
#include "tcp.h"

int main()
{
	int rc;

	rc = uloop_init();
	if (rc) {
		_debug("uloop init failed");
		goto exit;
	}

	rc = u_ubus_init();
	if (rc) {
		_debug("ubus init failed");
		goto exit;
	}

#if 0
	u_tcp_host = "127.0.0.1";
	u_tcp_port = "3000";

	u_tcp_connect();
#endif

	uloop_run();

	rc = EXIT_SUCCESS;
exit:
	u_ubus_done();
	uloop_done();

	return rc;
}
