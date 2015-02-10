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

#ifndef __INTERNAL_H__
#define __INTERNAL_H__

#define PROJECT_NAME "libsysrepo"

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#define _log(fmt, ...) do { \
		syslog(0, fmt, ## __VA_ARGS__); \
		fprintf(stderr, "%s: "fmt, PROJECT_NAME, ## __VA_ARGS__); \
	} while (0)

#define _error(fmt, ...) do { \
		syslog(0, fmt, ## __VA_ARGS__); \
		fprintf(stderr, "%s: "fmt, PROJECT_NAME, ## __VA_ARGS__); \
	} while (0)

#define _debug(fmt, ...) do { \
		fprintf(stderr, "%s: %s: %s (%d): " fmt "\n", PROJECT_NAME, __FILE__, __FUNCTION__, __LINE__, ## __VA_ARGS__); \
	} while (0)

#define __debug(fmt, ...) do { \
		fprintf(stderr, "%s: %s: %s (%d): " fmt "\n", PROJECT_NAME, __FILE__, __FUNCTION__, __LINE__, ## __VA_ARGS__); \
	} while (0)

#define ___debug(fmt, ...) do { \
		fprintf(stderr, "%s: %s: %s (%d): " fmt "\n", PROJECT_NAME, __FILE__, __FUNCTION__, __LINE__, ## __VA_ARGS__); \
	} while (0)

#ifndef __unused
#define __unused __attribute__((unused))
#endif

#endif /* __INTERNAL_H__ */
