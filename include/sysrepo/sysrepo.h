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

#ifndef __SYSREPO_H__
#define __SYSREPO_H__

#include <stdio.h>
#include <arpa/inet.h>

#define SYSREPO_SUCCESS    0

#ifdef __NO_CONFD_LIB__
enum confd_debug_level {
	CONFD_SILENT = 0,
	CONFD_DEBUG  = 1,
	CONFD_TRACE  = 2,      /* trace callback calls */
	CONFD_PROTO_TRACE = 3  /* tailf internal protocol trace */
};

/*
 * Return codes are from ConfD User Guide 
 */

extern int confd_errno;

#define CONFD_OK           0
#define CONFD_ERR_MALLOC  -1
#define CONFD_ERR_OS      -2
#define CONFD_ERR_BADPATH -3
#define CONFD_ERR         -4

/*
 * ConfD Paramaters
 */
#define CONFD_PORT 8800


/*
 * Procedures are from ConfD User Guide 5.3.1
 */
void confd_init(const char *name, FILE *estream, const enum confd_debug_level debug);

// TODO
// It seems that we can live without this function
// 
// int maapi_connect(int sock, const struct sockaddr *srv, int srv_sz);

struct confd_ip {
	int af; /* AF_INET | AF_INET6 */
	union {
		struct in_addr v4;
		struct in6_addr v6;
	} ip;
};

enum confd_proto {
	CONFD_PROTO_UNKNOWN = 0,
	CONFD_PROTO_TCP = 1,
	CONFD_PROTO_SSH = 2,
	CONFD_PROTO_SYSTEM = 3, /* ConfD initiated transactions */
	CONFD_PROTO_CONSOLE = 4,
	CONFD_PROTO_SSL = 5,
	CONFD_PROTO_HTTP = 6,
	CONFD_PROTO_HTTPS = 7,
	CONFD_PROTO_UDP = 8 /* SNMP sessions */
};

int maapi_start_user_session(int sock,
							const char *username,
							const char *context,
							const char **groups, int numgroups,
							const struct confd_ip *src_addr,
							enum confd_proto prot);
enum confd_dbname {
	CONFD_NO_DB = 0,
	CONFD_CANDIDATE = 1,
	CONFD_RUNNING = 2,
	CONFD_STARTUP = 3,
	CONFD_OPERATIONAL = 4,
	CONFD_TRANSACTION = 5 /* trans_in_trans */
};

enum confd_trans_mode {
	CONFD_READ = 1,
	CONFD_READ_WRITE = 2
};

// implement this one in order to lock the tree - not in the demo - in the second loop of implementation
int maapi_start_trans(int sock, enum confd_dbname name, enum confd_trans_mode readwrite);
#endif
// niraj would like this for second demo
int maapi_set_namespace(int sock, int thandle, int hashed_ns);

int maapi_get_str_elem(int sock, int thandle, char *buf, int n, const char *fmt, ...);
int maapi_set_str_elem(int sock, int thandle, char *buf);
int maapi_get_int32_elem(int sock, int thandle, int32_t *rval, const char *fmt, ...);

#endif /* __SYSREPO_H__ */
