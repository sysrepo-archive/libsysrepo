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

// ubus call umaapi ping '{ "payload": "baaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaar\n" }'
// ubus call umaapi get '{ "xpath": "/foo/bar", "type": "string" }'
// ubus call umaapi get '{ "xpath": "/foo/foo", "type": "int32" }'
// ubus call umaapi get '{ "xpath": "/*", "type": "string" }'

#include <stdio.h>
#include <unistd.h>
#include <libubus.h>

#include "sysrepo/sysrepo.h"
#include "../internal.h"

#include "ubus.h"
#include "tcp.h"

static struct ubus_context *ubus = NULL;
static struct blob_buf b;

enum ping {
	PING_PAYLOAD,
	__PING_MAX
};

static const struct blobmsg_policy ping_policy[] = {
	[PING_PAYLOAD] = { .name = "payload", .type = BLOBMSG_TYPE_STRING },
};

static int
handle_ping(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
    struct blob_attr *tb[__PING_MAX];

    blobmsg_parse(ping_policy, ARRAY_SIZE(ping_policy), tb,
              blob_data(msg), blob_len(msg));

#if 0
    if (!tb[PING_PAYLOAD]) {
		u_tcp_write("\n", 1, false);
	} else {
		char *payload = blobmsg_data(tb[PING_PAYLOAD]);
		u_tcp_printf("%s", payload);
		free(payload);
	}
#endif

    return 0;
}

enum get {
	GET_XPATH,
	GET_TYPE,
	__GET_STR_MAX
};

static const struct blobmsg_policy get_policy[] = {
	[GET_XPATH] = { .name = "xpath", .type = BLOBMSG_TYPE_STRING },
	[GET_TYPE] = { .name = "type", .type = BLOBMSG_TYPE_STRING },
};

static int
handle_get(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
    struct blob_attr *tb[__GET_STR_MAX];
	int rc = 0;

    blobmsg_parse(get_policy, ARRAY_SIZE(get_policy), tb,
              blob_data(msg), blob_len(msg));

    if (!tb[GET_XPATH])
		return UBUS_STATUS_INVALID_ARGUMENT;

    if (!tb[GET_TYPE])
		return UBUS_STATUS_INVALID_ARGUMENT;

	char *xpath = blobmsg_data(tb[GET_XPATH]);
	char *type = blobmsg_data(tb[GET_TYPE]);
	char buf[100000];

    if (!strcmp("string", type))
		rc = maapi_get_str_elem(0, 0, buf, 100000, xpath);

    if (!strcmp("int32", type))
		rc = maapi_get_int32_elem(0, 0, NULL, xpath);

	if (rc)
		return UBUS_STATUS_UNKNOWN_ERROR;

	blob_buf_init(&b, 0);

	blobmsg_add_string(&b, "data", buf);

	ubus_send_reply(ctx, req, b.head);

	blob_buf_free(&b);

    return 0;
}

enum set {
	SET_XPATH,
	SET_VALUE,
	__SET_STR_MAX
};

static const struct blobmsg_policy set_policy[] = {
	[SET_XPATH] = { .name = "xpath", .type = BLOBMSG_TYPE_STRING },
	[SET_VALUE] = { .name = "value", .type = BLOBMSG_TYPE_STRING },
};

static int
handle_set(struct ubus_context *ctx, struct ubus_object *obj,
			struct ubus_request_data *req, const char *method,
			struct blob_attr *msg)
{
    struct blob_attr *tb[__SET_STR_MAX];
	int rc = 0;

    blobmsg_parse(set_policy, ARRAY_SIZE(set_policy), tb,
              blob_data(msg), blob_len(msg));

    if (!tb[SET_XPATH])
		return UBUS_STATUS_INVALID_ARGUMENT;

    if (!tb[SET_VALUE])
		return UBUS_STATUS_INVALID_ARGUMENT;

	char *xpath = blobmsg_data(tb[GET_XPATH]);
	char *value = blobmsg_data(tb[GET_TYPE]);
	char buf[2000];

	// rc = maapi_get_int32_elem(0, 0, NULL, xpath);

//	if (rc)
//		return UBUS_STATUS_UNKNOWN_ERROR;

    return 0;
}

static const struct ubus_method u_methods[] = {
	UBUS_METHOD("ping", handle_ping, ping_policy),
	UBUS_METHOD("get", handle_get, get_policy),
	UBUS_METHOD("set", handle_set, set_policy)
};

static struct ubus_object_type main_object_type =
	UBUS_OBJECT_TYPE("umaapi", u_methods);

static struct ubus_object main_object =
{
	.name = "umaapi",
	.type = &main_object_type,
	.methods = u_methods,
	.n_methods = ARRAY_SIZE(u_methods),
};

int u_ubus_init(void)
{
	int rc;

	ubus = ubus_connect(NULL);
	if (!ubus)
		return -1;

	ubus_add_uloop(ubus);

	rc = ubus_add_object(ubus, &main_object);

	return rc;
}

void u_ubus_done(void)
{
	if (ubus) ubus_free(ubus);
}
