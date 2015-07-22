/*
 * wrapper.h
 *
 *  Created on: 16/lug/2015
 *      Author: andrea
 */

#ifndef WRAPPER_H_
#define WRAPPER_H_

#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <netdb.h>
#include <regex.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "full_write.h"

/*
 * Fix per la compilazione in eclipse, in realtà basta aver definito _GNU_SOURCE per avere la
 * definizione di RTLD_NEXT; ad ogni modo eclipse sembra non tenere in considerazione questa cosa
 * (soltanto un problema grafico dell'IDE).
 */
#ifndef RTLD_NEXT
#define RTLD_NEXT	((void *) -1l)
#define __RTLD_NEXT_ECLIPSE_BUG__
#endif

#define FALSE 0
#define TRUE  1

#define DNS_PORT (uint16_t) 53
/*
 * Proxy settings
 * TODO: Queste impostazioni devono essere lette da qualche file di configurazione,
 * in modo tale da rederle dinamiche.
 */
#define PROXY_TORDNS_ADDR ((in_addr_t) 0x7f000101) //127.0.1.1
#define PROXY_TORDNS_ADDR_NETWORK_BYTE_ORDER htonl(PROXY_TORDNS_ADDR)
#define PROXY_TORDNS_PORT ((uint16_t) 5553)
#define PROXY_TORDNS_PORT_NETWORK_BYTE_ORDER htons(PROXY_TORDNS_PORT)


#define PROXY_ADDR_NETWORK_BYTE_ORDER htonl(INADDR_ANY)
#define PROXY_PORT ((uint16_t) 4567)
#define PROXY_PORT_NETWORK_BYTE_ORDER htons(PROXY_PORT)

#define HTTP_PORT ((uint16_t) 80)
#define HTTP_PORT_NETWORK_BYTE_ORDER  htons(HTTP_PORT)

#define HTTPS_PORT ((uint16_t) 443)
#define HTTPS_PORT_NETWORK_BYTE_ORDER  htons(HTTPS_PORT)

#define REAL_GETADDRINFO_NAME 		"getaddrinfo"
#define REAL_GETHOSTBYNAME_NAME 	"gethostbyname"
#define REAL_CONNECT_NAME 			"connect"

/** Wrapper per gethostbyname */
static struct hostent *(*real_gethostbyname)(const char *name) = NULL;

/** Wrapper per getaddrinfo */
static int (*real_getaddrinfo)(const char *name, const char *service,
		const struct addrinfo *hints, struct addrinfo **res) = NULL;

/** Wrapper per real_connect */
static int (*real_connect)(int sockfd, const struct sockaddr *addr,
		socklen_t addrlen) = NULL;

#endif /* WRAPPER_H_ */
