/*
 * Copyright 2006 Tail-F Systems AB
 */
/*
 * Copyright 2015 Cisco Systems
 * Allows simulated interfaces using FOPEN macro
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <roxml.h>

#include "sysrepo.h"
//#include "libsysrepo.h"
#include "../../../../src/internal.h"
/* To do: put confd.h include in cdb.h */
/* cdb.h won't compile withouth confd.h */
//#include "confd.h"
//#include "cdb.h"
#include "if.h"
#include "demo.h"
/* Interface to sysrepod */
#include "srd.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT "3510"
#define BACKLOG 10

/*
 * Definitions
 */
/* sysrepo message header parameter */
/* to do: include from sysrepod include file */
#define SYSREPO_HDR_LEN 8

/* sysrepo device simulation */

#define  __SYSREPO_PROC_NET_DEV_SIM__ 1
#ifdef __SYSREPO_PROC_NET_DEV_SIM__
#define MAC16 (int)random()>>23
/* N_DEVICES determines how many interfaces are to be simulated */
const int IFSTATUS_N_DEVICES = 100;
#define FOPEN if_simulate_and_open 
extern FILE* if_simulate_and_open(char* path, char* mode);
extern int confd_errno;
#else
#define FOPEN fopen
#endif

/* sysrepo device simulation */
#define INTERVAL 62

/* /proc/net/dev parsing of leaf counters */
#define GET_COUNTER() {                         \
        if ((p = strtok(NULL, " \t")) == NULL)  \
            continue;                           \
        counter = atoll(p);                     \
    }
#define GET_ELEMENT_NAME() {                    \
        if ((p=strtok(NULL, "/")) == NULL)      \
	  continue;                             \
	elname = p;                             \
  }
/* sysrepod interface */
#define SYSREPO_BUF (BUFSIZ * 1000)
#define CUSTOM_XML_HELO "<xml><protocol>srd</protocol></xml>"

#define HC_SET_OPERATIONAL_DATASTORE "<xml><command>create_opDataStore</command><param1>%s</param1></xml>"
#define HC_USE_OPERATIONAL_DATASTORE "<xml><command>use_opDataStore</command><param1>%s</param1></xml>"
#define HC_REGISTER_CLIENT "<xml><command>register_clientSocket</command><param1>127.0.0.1</param1><param2>3510</param2></xml>"

/* IETF-Interfaces XML configuration string */
char *IETF_INTERFACES_OPERATIONAL_SCHEMA =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
"<data xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"
"  <interfaces-state xmlns=\"urn:ietf:params:xml:ns:yang:ietf-interfaces\">"
"    <interface>"
"      <name/>  "
"      <type/>"
"      <admin-status/>"
"      <oper-status/> "
"      <last-change/>"
"      <if-index/>   "
"      <phys-address/>"
"      <higher-layer-if/>"
"      <lower-layer-if/> "
"      <speed/>         "
"      <statistics>"
"        <discontinuity-time/>"
"        <in-octets/>"
"        <in-unicast-pkts/>"
"        <in-broadcast-pkts/>"
"        <in-multicast-pkts/>"
"        <in-discards/>"
"        <in-errors/>"
"        <in-unknown-protos/>"
"        <out-octets/>"
"        <out-unicast-pkts/>"
"        <out-broadcast-pkts/>"
"        <out-multicast-pkts/>"
"        <out-discards/>"
"        <out-errors/>"
"      </statistics>"
"    </interface>"
"  </interfaces-state>"
"</data>";
/*
 * Globals
 */
int sock = -1;
char message[SYSREPO_BUF];

int srd_hello()
{
	struct sockaddr_in server;
	int flags;
	int len;
	int i;

	sock = socket(AF_INET , SOCK_STREAM , 0);
	if (sock == -1) 
	  {
	    perror("Socket creation attempt:");
	    return -1;
	  }

	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_family = AF_INET;
	server.sin_port = htons(3500);

	if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) 
	  {
	    perror("Socket connect attempt:");
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
	len = strlen(message);
	if (send(sock , message , strlen(message) , 0) < 0) {
	  perror("Socket send HELLO attempt: ");
	  return -1;
	}

	char *xml = (char*)calloc(len + 1, sizeof(char));
	if (!xml) 
	  {
	    perror("XML string allocation attempt: ");
	    return -1;
	  }

	memset(message, 0, SYSREPO_BUF);
	for (i = 0; i < 8; i++) {
		recv(sock, message + i, 1, 0);
	}

	len = atoi(message);
	___debug("received: %s", message);

	memset(message, 0, SYSREPO_BUF);
	for (i = 0; i < len; i++) {
		recv(sock, message + i, 1, 0);
	}
	___debug("received (%d): %s", len, message);

	return 0;
}

int hc_data()
{
	int i, len;
	char buf[BUFSIZ];
	char *datastore = "urn:ietf:params:xml:ns:yang:ietf-interfaces:interfaces-state";

	// HC_SET_OPERATIONAL_DATASTORE
	sprintf(buf, HC_SET_OPERATIONAL_DATASTORE, datastore);
	sprintf(message, "%07d %s", (int) strlen(buf), buf);
	___debug("sending: %s", message);
	if (send(sock , message , strlen(message) , 0) < 0) {
		return -1;
	}

	memset(message, 0, SYSREPO_BUF);
	for (i = 0; i < 8; i++) {
		recv(sock, message + i, 1, 0);
	}

	len = atoi(message);
	___debug("received: %s", message);

	memset(message, 0, SYSREPO_BUF);
	for (i = 0; i < len; i++) {
		recv(sock, message + i, 1, 0);
	}
	___debug("received (%d): %s", len, message);


	// HC_USE_OPERATIONAL_DATASTORE
	sprintf(buf, HC_USE_OPERATIONAL_DATASTORE, datastore);
	sprintf(message, "%07d %s", (int) strlen(buf), buf);
	___debug("sending: %s", message);
	if (send(sock , message , strlen(message) , 0) < 0) {
		return -1;
	}

	memset(message, 0, SYSREPO_BUF);
	for (i = 0; i < 8; i++) {
		recv(sock, message + i, 1, 0);
	}

	len = atoi(message);
	___debug("received: %s", message);

	memset(message, 0, SYSREPO_BUF);
	for (i = 0; i < len; i++) {
		recv(sock, message + i, 1, 0);
	}
	___debug("received (%d): %s", len, message);


	// HC_REGISTER_CLIENT
	sprintf(message, "%07d %s", (int) strlen(HC_REGISTER_CLIENT), HC_REGISTER_CLIENT);
	___debug("sending: %s", message);
	if (send(sock , message , strlen(message) , 0) < 0) {
		return -1;
	}

	memset(message, 0, SYSREPO_BUF);
	for (i = 0; i < 8; i++) {
		recv(sock, message + i, 1, 0);
	}

	len = atoi(message);
	___debug("received: %s", message);

	memset(message, 0, SYSREPO_BUF);
	for (i = 0; i < len; i++) {
		recv(sock, message + i, 1, 0);
	}
	___debug("received (%d): %s", len, message);

	return 0;
}

#define XML_HEADER "<xml></xml>"
char *update_status_and_generate_xml(node_t **root)
{
    node_t *child, *child_data;
    char *xml = NULL;
    FILE *proc;
    int i;
    char buf[BUFSIZ];
    char macaddr[BUFSIZ];
    static char counterstr[N_COUNTERS][BUFSIZ];
    char *ifname, *p;
    unsigned long long counter;

    /* when simulated, write out and open a demo/proc/net/dev file */
    if ((proc = FOPEN("/proc/net/dev", "r")) == NULL)
      {
	fprintf(stderr,"Cannot open /proc/net/dev\n");
	return NULL;
      }
    bzero(&buf, sizeof(buf));
    *root = roxml_load_buf(XML_HEADER);
    child_data = roxml_add_node(roxml_get_chld(*root, NULL, 0), 0, ROXML_ELM_NODE, "ok", NULL);
    child_data = roxml_add_node(child_data, 0, ROXML_ELM_NODE, "data", NULL);
    i = 0;

    while (fgets(buf, sizeof(buf), proc) != NULL) {
        if ((p = strchr(buf, ':')) == NULL)
            continue;
        *p = ' ';
        if ((ifname = strtok(buf, " \t")) == NULL)
            continue;
	child = child_data;
	child = roxml_add_node(child, 0, ROXML_ELM_NODE, "interfaces-state", NULL);

	child = roxml_add_node(child, 0, ROXML_ELM_NODE, "interface", NULL);
	strcpy(counterstr[RX_BYTES], strtok(NULL, " \t"));
	strcpy(counterstr[RX_PACKETS], strtok(NULL, " \t"));

	/* Get /proc/net/dev  RX Errors */
	strcpy(counterstr[RX_ERRS], strtok(NULL, " \t"));
	counter = atoi(counterstr[RX_ERRS]);

	/* Next, get RX Dropped and Missed Errors */
	strcpy(counterstr[RX_DROPPED], strtok(NULL, " \t"));

        /* RX FIFO Erros */
	strcpy(counterstr[RX_ERRS], strtok(NULL, " \t"));
	counter += atoi(counterstr[RX_ERRS]);

	/* RX Length, Overflow, CRC and Frame Errs */
	strcpy(counterstr[RX_ERRS], strtok(NULL, " \t"));
	counter += atoi(counterstr[RX_ERRS]);
	sprintf(counterstr[RX_ERRS], "%llu", counter);
	strcpy(counterstr[RX_COMPRESSED], strtok(NULL, " \t"));
	strcpy(counterstr[MULTICAST], strtok(NULL, " \t"));
	strcpy(counterstr[TX_BYTES], strtok(NULL, " \t"));
	strcpy(counterstr[TX_PACKETS], strtok(NULL, " \t"));

	/* /proc/net/dev TX Errors */
	strcpy(counterstr[TX_ERRS], strtok(NULL, " \t"));
	counter = atoi(counterstr[TX_ERRS]);

	/* TX Dropped Errors */
	strcpy(counterstr[TX_DROPPED], strtok(NULL, " \t"));
	counter += atoi(counterstr[TX_DROPPED]);

	/* TX FIFO Errors added into TX_ERRS */
	strcpy(counterstr[TX_ERRS], strtok(NULL, " \t"));
	counter += atoi(counterstr[TX_ERRS]);

	/* Collisions */
	strcpy(counterstr[TX_ERRS], strtok(NULL, " \t"));
	counter += atoi(counterstr[TX_ERRS]);	

	/* TX Carrier Errors from /proc/net/dev */
	strcpy(counterstr[TX_ERRS], strtok(NULL, " \t"));
	counter += atoi(counterstr[TX_ERRS]);	
	sprintf(counterstr[TX_ERRS], "%llu", counter);
	strcpy(counterstr[TX_COMPRESSED], strtok(NULL, " \t"));
	sprintf(counterstr[INTERFACE_INDEX], "%i", i + 1);

	/* Simulate a MAC address */
	sprintf(macaddr,"%02X:%02X:%02X:%02X:%02X:%02X", MAC16, MAC16, MAC16, MAC16, MAC16, MAC16);

	roxml_add_node(child, 0, ROXML_ELM_NODE, "name", ifname);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "type", "1000BASET");
	roxml_add_node(child, 0, ROXML_ELM_NODE, "admin-status", NULL);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "oper-status", NULL);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "last-status", NULL);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "if-index", counterstr[INTERFACE_INDEX]);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "phys-address", macaddr);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "higher-layer-if", NULL);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "lower-layer-if", NULL);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "speed", NULL);
	child = roxml_add_node(child, 0, ROXML_ELM_NODE, "statistics", NULL);

	roxml_add_node(child, 0, ROXML_ELM_NODE, "discontinuity-time", NULL);
        roxml_add_node(child, 0, ROXML_ELM_NODE, "in-octets", counterstr[RX_BYTES]);
        roxml_add_node(child, 0, ROXML_ELM_NODE, "in-unicast-pkts", counterstr[RX_PACKETS]);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "in-broadcast-pkts", NULL);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "in-multicast-pkts", counterstr[MULTICAST]);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "in-discards", counterstr[RX_DROPPED]);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "in-errors", 	counterstr[RX_ERRS]);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "in-unknown-protos", NULL);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "out-octets", counterstr[TX_BYTES]);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "out-unicast-pkts", counterstr[TX_PACKETS]);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "out-broadcast-pkts", NULL);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "out-multicast-pkts",counterstr[MULTICAST]);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "out-discards", counterstr[TX_DROPPED]);
	roxml_add_node(child, 0, ROXML_ELM_NODE, "out-errors", counterstr[TX_ERRS]);

	i++;
    }
    roxml_commit_changes(*root, NULL, &xml, 0);
   
    fclose(proc);
    proc = fopen("../dev.xml", "w");
    if (proc == NULL)
      {
	fprintf(stderr, "Unable to write to ../dev.xml\n");
      } 
    else
      {
	fprintf(proc,"%s\n",xml);
	fclose(proc);
      }
    return xml;
}

void sigchld_handler(int s)
{
	while(waitpid(-1, NULL, WNOHANG) > 0);
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int hc_server(void)
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	int i, rv, result;
	int pid;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		return 2;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");

	while(1) {  // main accept() loop
	        int len = 0;
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept: ");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);

		/* The first field is the length header, receive it */
		memset(message, 0, SYSREPO_BUF);
		for (i = 0; (i < SYSREPO_HDR_LEN) && (i < SYSREPO_BUF); i++) {
		  result = recv(new_fd, message + i, 1, 0);
		  if (result < 1)
		    {
		      perror("recv sysrepo message header: ");
		      close(new_fd);
		      continue;
		    }
		}
		result = sscanf(message,"%d", &len);
		if ((result != 1) || (len > SYSREPO_BUF) || (len < 1))
		  { /* SYSREPO_BUF is set too small or too large */
		    fprintf(stderr,"Message header has invalid length: %i\n", len);
		    //continue;
		  }
		___debug("received: %s", message);

		/* Next, receive the xpath */
		memset(message, 0, SYSREPO_BUF);
		for (i = 0; i < len; i++) {
		  result = recv(new_fd, message + i, 1, 0);
		  if (result < 1)
		    {
		      perror("recv sysrepo xpath: ");
		      break;
		    }
		}
		___debug("received (%d): %s", len, message);

		/* spawn thread */
		if (!(pid=fork())) 
		  { // this is the child process
	           node_t *root;
		   char *xml = NULL;         // The XML string returned from update_status_and_generate_xml
		   node_t **node_set;        // The node set returned from roxml_xpath
		   int n = 0;                // The number of nodes returned from roxml_xpath
		   char *xpath = "/*";
		   char message [SYSREPO_BUF];

		   close(sockfd); // child doesn't need the listener

		   xml = update_status_and_generate_xml(&root);
		   node_set = roxml_xpath(root, xpath, &n);
		   //TODO: In general, n>1, which happens to be not true for the demo.
		   //      So we only treat the case of node_set[0] below.
		   if (node_set == NULL) {
		     fprintf(stderr,"NULL node set from XPATH: %s\n", xpath);
		     fflush(stderr);
		     roxml_close(root);
		     roxml_release(RELEASE_ALL);
		     free(xml);
		     close(new_fd);
		     exit(0);
		   }
		   roxml_commit_changes(node_set[0], NULL, &xml, 0);
		   sprintf(message, "%07d %s", (int) strlen(xml), xml);

		   ___debug("sending: %s", message);
		   if (send(new_fd, message , strlen(message) , 0) < 0) {
		    return -1;
		   }
		   roxml_close(root);
		   roxml_release(RELEASE_ALL);
		   free(xml);

		   close(new_fd);
		   exit(0);
		  }
	}
	return 0;
}

int main(int argc, char **argv)
{
    int interval = 0;

    if (argc > 1)
        interval = atoi(argv[1]);
    if (interval == 0)
        interval = INTERVAL;

    srandom(10);
    if (srd_hello() < 0)
      {
	fprintf(stderr, "srd_hello failed\n");
	exit(0);
      }
    hc_data();
    hc_server();

    exit(0);

}
