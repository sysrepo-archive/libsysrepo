/*
 * Copyright (C) 2015 Cisco Systems, Inc.
 *
 * Author: Nobody <nbd@sysrepo.org>
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

#ifndef __DEMO_H__
#define __DEMO_H__

/* MAX_DEVICES is the maximum number of simulated interfaces */
#define MAX_DEVICES 500000
/* net_device_stats orders the named counters below in       */
/* "computational order" but the following defines names the */
/* the counters for use in devs[][].                         */
/* See the printf and fprintf  statements below for the      */
/* Linux /proc/net/dev formatting order.                     */
#define RX_BYTES    0
#define TX_BYTES    1
#define RX_PACKETS  2
#define TX_PACKETS  3
#define RX_ERRS     4
#define TX_ERRS     5 
#define RX_DROPPED  6
#define TX_DROPPED  7
#define MULTICAST   8 
#define COLLISIONS  9
#define RX_LENGTH_ERRS   10
#define RX_OVER_ERRS     11
#define RX_CRC_ERRS      12
#define RX_FRAME_ERRS    13
#define RX_FIFO_ERRS     14
#define RX_MISSED_ERRS   15
#define TX_ABORTED_ERRS  16
#define TX_CARRIER_ERRS  17
#define TX_FIFO_ERRS     18
#define TX_HEARTBEAT_ERRS 19
#define TX_WINDOW_ERRS    20
#define RX_COMPRESSED       21
#define TX_COMPRESSED       22
#define INTERFACE_INDEX     23
/* N_COUNTERS is the number of net_device_stats members */
#define N_COUNTERS 24

#define DEFAULT_LOCAL_IPV4_ADDRESS "127.0.0.1"
#define DEFAULT_LOCAL_PORT "36000"
#endif
