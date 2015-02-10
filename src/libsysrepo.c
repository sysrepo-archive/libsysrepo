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

#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libubox/uloop.h>
#include <libubox/usock.h>
#include <roxml.h>

#include "sysrepo/sysrepo.h"

#include "internal.h"

#define CUSTOM_XML_APPLY_XPATH "<xml><command>apply_xpath</command></xml>"

#define CUSTOM_XML_APPLY_OP_XPATH "<xml><command>apply_xpathOpDataStore</command><param1>urn:ietf:params:xml:ns:yang:ietf-interfaces:interfaces-state</param1></xml>"

#define CUSTOM_XML_HELO "<xml><protocol>srd</protocol></xml>"
#define CUSTOM_XML_CREATE_DATASTORE "<xml><command>create_dataStore</command><param1>runtime</param1><param2></param2></xml>"
#define CUSTOM_XML_SET_DATASTORE "<xml><command>set_dataStore</command><param1>urn:ietf:params:xml:ns:yang:ietf-interfaces:interfaces-state</param1></xml>"
#define CUSTOM_XML_USE_OP_DATASTORE "<xml><command>use_opDataStore</command><param1>urn:ietf:params:xml:ns:yang:ietf-interfaces:interfaces-state</param1></xml>"

#define SYSREPO_BUF 10000

int sock = -1;
char message[SYSREPO_BUF];

static int sysrepo_connect()
{
	struct sockaddr_in server;
	int flags;
	int len;

	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1) return -1;

	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(3500);

	if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
		return -1;
	}

	flags = fcntl(sock, F_GETFL, 0);
	if (flags == -1)
		return -1;

	flags &= ~O_NONBLOCK;
	fcntl(sock, F_SETFL, flags);

	___debug("connected");

	sprintf(message, "%07d %s", (int) strlen(CUSTOM_XML_HELO), CUSTOM_XML_HELO);
	___debug("sending: %s", message);
	if (send(sock , message , strlen(message) , 0) < 0) {
		return -1;
	}

	memset(message, 0, SYSREPO_BUF);
	for (int i = 0; i < 8; i++) {
		recv(sock, message + i, 1, 0);
	}

	len = atoi(message);
	___debug("received: %s", message);

	memset(message, 0, SYSREPO_BUF);
	for (int i = 0; i < len; i++) {
		recv(sock, message + i, 1, 0);
	}
	___debug("received (%d): %s", len, message);

#if 0
	memset(message, 0, SYSREPO_BUF);
	snprintf(message, SYSREPO_BUF, "%07d %s", (int) strlen(CUSTOM_XML_CREATE_DATASTORE), CUSTOM_XML_CREATE_DATASTORE);
	___debug("sending: %s", message);
	if (send(sock, message, strlen(message), 0) < 0) {
		return -1;
	}

	memset(message, 0, SYSREPO_BUF);
	for (int i = 0; i < 8; i++) {
		recv(sock, message + i, 1, 0);
	}

	len = atoi(message);
	___debug("received: %s", message);

	memset(message, 0, SYSREPO_BUF);
	for (int i = 0; i < len; i++) {
		recv(sock, message + i, 1, 0);
	}
	___debug("received (%d): %s", len, message);

	memset(message, 0, SYSREPO_BUF);
	snprintf(message, SYSREPO_BUF, "%07d %s", (int) strlen(CUSTOM_XML_SET_DATASTORE), CUSTOM_XML_SET_DATASTORE);
	___debug("sending: %s", message);
	if (send(sock, message, strlen(message), 0) < 0) {
		return -1;
	}

	memset(message, 0, SYSREPO_BUF);
	for (int i = 0; i < 8; i++) {
		recv(sock, message + i, 1, 0);
	}

	len = atoi(message);
	___debug("received: %s", message);

	memset(message, 0, SYSREPO_BUF);
	for (int i = 0; i < len; i++) {
		recv(sock, message + i, 1, 0);
	}
	___debug("received (%d): %s", len, message);
#endif


#if 0
	memset(message, 0, SYSREPO_BUF);
	snprintf(message, SYSREPO_BUF, "%07d %s", (int) strlen(CUSTOM_XML_USE_OP_DATASTORE), CUSTOM_XML_USE_OP_DATASTORE);
	___debug("sending: %s", message);
	if (send(sock, message, strlen(message), 0) < 0) {
		return -1;
	}

	memset(message, 0, SYSREPO_BUF);
	for (int i = 0; i < 8; i++) {
		recv(sock, message + i, 1, 0);
	}

	len = atoi(message);
	___debug("received: %s", message);

	memset(message, 0, SYSREPO_BUF);
	for (int i = 0; i < len; i++) {
		recv(sock, message + i, 1, 0);
	}
	___debug("received (%d): %s", len, message);
#endif

	return 0;
}

static int sysrepo_write(const char *fmt, ...)
{
	node_t *root, *child;
	char *xml = NULL, buffer[BUFSIZ];
	va_list ap;
	char server_reply[2000];

	root = roxml_load_buf(CUSTOM_XML_APPLY_OP_XPATH);
	if (!root) return -1;

	va_start(ap, fmt);
	vsnprintf(buffer, BUFSIZ, fmt, ap);
	va_end(ap);

	child = roxml_get_chld(root, "xml", 0);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "param2", buffer);

	roxml_commit_changes(root, NULL, &xml, 0);

	sprintf(message, "%07d %s", (int) strlen(xml), xml);
	___debug("sending: %s", message);
	if (send(sock, message, strlen(message) , 0) < 0) {
		return -1;
	}

 	roxml_close(root);
	roxml_release(RELEASE_ALL);

	free(xml);


	return 0;
}

static int sysrepo_read(node_t **root)
{
	int len = 0;
	bool data_received = 0;

	char *xml = calloc(len + 1, sizeof(char));
	if (!xml) {
		return -1;
	}

	memset(message, 0, SYSREPO_BUF);
	for (int i = 0; i < 8; i++) {
		recv(sock, message + i, 1, 0);
	}

	len = atoi(message);
	___debug("received: %s", message);

	memset(message, 0, SYSREPO_BUF);
	for (int i = 0; i < len; i++) {
		recv(sock, message + i, 1, 0);
	}
	___debug("received (%d): %s", len, message);

	*root = roxml_load_buf(message + 8);
	free(xml);
	if (!(*root))
		return -1;

	return 0;
}

int maapi_get_str_elem(int sock, int thandle, char *buf, int n, const char *fmt, ...)
{
	va_list ap;
	int rc;

	rc = sysrepo_connect();
	if (rc) return rc;

	va_start(ap, fmt);
	sysrepo_write(fmt, ap);
	va_end(ap);

	node_t *root = NULL;
	rc = sysrepo_read(&root);
	if (rc)
		goto exit;

	char *xml;
	roxml_commit_changes(root, NULL, &xml, 0);

	strncpy(buf, xml, n);
	free(xml);

	rc = 0;

exit:
 	roxml_close(root);
	roxml_release(RELEASE_ALL);

	return rc;
}

int maapi_set_str_elem(int sock, int thandle, char *buf)
{
	va_list ap;
	int rc;

	rc = sysrepo_connect();
	if (rc) return rc;

#if 0
	sprintf(message, "%07d %s", (int) strlen(xml), xml);
	___debug("sending: %s", message);
	if (send(sock, message, strlen(message) , 0) < 0) {
		return -1;
	}
#endif

	rc = 0;

	return rc;
}

int maapi_get_int32_elem(int sock, int thandle, int32_t *rval, const char *fmt, ...)
{
// FIXME: check implementation when sysrepod is ready
#if 0
	va_list ap;
	int rc;

	va_start(ap, fmt);
	sysrepo_write(fmt, ap);
	va_end(ap);

	node_t *root = NULL;
	rc = sysrepo_read(&root);
	if (rc)
		return -1;

	char *xml;
	roxml_commit_changes(root, NULL, &xml, 0);

	free(xml);
	roxml_release(RELEASE_ALL);
 	roxml_close(root);
#endif

	return 0;
}
#if 0
int cdb_connect(int sock, enum cdb_sock_type type, const struct sockaddr* srv, int srv_sz)
{
	struct sockaddr_in server;
	int flags;
	int len;

	sprintf(message, "%07d %s", (int) strlen(CUSTOM_XML_HELO), CUSTOM_XML_HELO);
	___debug("sending: %s", message);
	if (send(sock , message , strlen(message) , 0) < 0) {
	        ___debug("error sending: %s", message);
		return -1;
	}

	memset(message, 0, SYSREPO_BUF);
	for (int i = 0; i < 8; i++) {
		recv(sock, message + i, 1, 0);
	}

	len = atoi(message);
	___debug("received: %s", message);

	memset(message, 0, SYSREPO_BUF);
	for (int i = 0; i < len; i++) {
		recv(sock, message + i, 1, 0);
	}
	___debug("received (%d): %s", len, message);

	return 0;
}
#endif
