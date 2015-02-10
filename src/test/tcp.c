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
#include <libubox/usock.h>
#include <libubox/ustream.h>

#include "sysrepo/sysrepo.h"
#include "internal.h"
#include "tcp.h"

static void reconnect(struct uloop_timeout *timeout);
static struct uloop_timeout reconnect_timer = { .cb = reconnect };

struct uloop_fd ufd;
static struct ustream_fd usfd;

char *u_tcp_host = NULL;
char *u_tcp_port = NULL;
char *u_read_data = NULL;

static void state_cb(struct ustream *s)
{
	___debug(".");

    usfd.stream.write_error = 1;
	uloop_timeout_set(&reconnect_timer, 1000);
}

static void write_cb(struct ustream *s, int bytes)
{
	___debug("bytes sent: %d", bytes);
	___debug("bytes pending: '%d'", s->w.data_bytes);
}


__unused
static void read_cb(struct ustream *s, int bytes)
{
	struct ustream_buf *buf = s->r.head;

	___debug("got '%d' bytes which were not picked up by any function: '%s'", bytes, buf->data);

	ustream_consume(s, bytes);
}

static void connect_cb(struct uloop_fd *f, unsigned int events) 
{ 
	if (ufd.eof || ufd.error) { 
		_debug("connection to sysrepo failed"); 
		uloop_timeout_set(&reconnect_timer, 1000);
		return; 
	} 

	___debug("connection with sysrepo established"); 
	uloop_fd_delete(&ufd); 

	usfd.stream.string_data = true;
	usfd.stream.notify_read = read_cb;
	usfd.stream.notify_state = state_cb;
	usfd.stream.notify_write = write_cb;
	ustream_fd_init(&usfd, ufd.fd);

    usfd.stream.write_error = 0;

	u_tcp_printf("%07d\n%s\n", strlen(CUSTOM_XML_HELO) + 1, CUSTOM_XML_HELO);
	u_tcp_printf("%07d\n%s\n", strlen(CUSTOM_XML_SET_DATASTORE) + 1, CUSTOM_XML_SET_DATASTORE);
}

int u_tcp_write(const char *buf, int len, bool more)
{
    if (usfd.stream.write_error)
        return -1;

	ustream_write(&usfd.stream, buf, len, more);

	return 0;
}

int u_tcp_printf(const char *format, ...)
{
    va_list arg;
    int ret;

    if (usfd.stream.write_error)
        return -1;

    va_start(arg, format);
    ret = ustream_vprintf(&usfd.stream, format, arg);
    va_end(arg);

    return ret;
}


void u_tcp_connect()
{
    usfd.stream.write_error = 1;

	ufd.fd = usock(USOCK_TCP, u_tcp_host, u_tcp_port);
	if (ufd.fd < 0) {
		_debug("connection to sysrepo failed"); 
		uloop_timeout_set(&reconnect_timer, 1000);
	}

	ufd.cb = connect_cb;
	uloop_fd_add(&ufd, ULOOP_WRITE | ULOOP_EDGE_TRIGGER);
}

static void reconnect(struct uloop_timeout *timeout)
{
	___debug("."); 

	u_tcp_connect();
}
