/*
 * wrapper.h
 *
 *  Created on: 16/lug/2015
 *      Author: Andrea Mayer Mayer
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

/**
 * DEFINIZIONE DELLE VARIABILI D'AMBIENTE
 */
#define ENV_TOR_DNS_PORT "TOR_DNS_PORT"
#define ENV_TOR_DNS_ADDR "TOR_DNS_ADDR"

#define ENV_WEB_PROXY_ADDR "WEB_PROXY_ADDR"
#define ENV_WEB_PROXY_PORT "WEB_PROXY_PORT"

/**
 * Se definita questa macro permette di catturare il traffico su tutte le porte
 * ed inviarlo al Web Proxy. Il traffico sulla porta del servizio DNS, viene valutata
 * prima di questa regola di default.
 */
#define FORWARD_ALL

/* ##################################### */

#define FALSE 0
#define TRUE  1

/* Porte di servizio per HTTP e HTTPS */
#define HTTP_PORT ((uint16_t) 80)
#define HTTP_PORT_PROXY ((uint16_t) 8080)
#define HTTPS_PORT ((uint16_t) 443)

/* Porta di servizio per il DNS di sistema. */
#define DNS_PORT (uint16_t) 53

/* Inidirizzi di default del TOR_DNS Server */
#define PROXY_TORDNS_ADDR ((in_addr_t) 0x7f000101) //127.0.1.1
#define PROXY_TORDNS_PORT ((uint16_t) 5553)

#define PROXY_TORDNS_ADDR_NETWORK_BYTE_ORDER htonl( \
	read_uint32_t_from_getenv(ENV_TOR_DNS_ADDR, PROXY_TORDNS_ADDR))

#define PROXY_TORDNS_PORT_NETWORK_BYTE_ORDER htons( \
	read_uint32_t_from_getenv(ENV_TOR_DNS_PORT, PROXY_TORDNS_PORT))

/* Indirizzo di default del Web Proxy */
#define PROXY_ADDR ((in_addr_t) INADDR_ANY)
#define PROXY_PORT ((uint16_t) 4567)

#define PROXY_ADDR_NETWORK_BYTE_ORDER htonl( \
		read_uint32_t_from_getenv(ENV_WEB_PROXY_ADDR, PROXY_ADDR))

#define PROXY_PORT_NETWORK_BYTE_ORDER htons( \
		read_uint32_t_from_getenv(ENV_WEB_PROXY_PORT, PROXY_PORT))

//#define HTTP_PORT_NETWORK_BYTE_ORDER  htons(HTTP_PORT)
//#define HTTPS_PORT_NETWORK_BYTE_ORDER  htons(HTTPS_PORT)

/*
 * Permette di leggere il contenuto di una variabile d'ambiente sottoforma di stringa.
 * Se la variabile d'ambiente non è settata allora ritorna NULL, altrimenti il puntatore
 * alla stringa. NB: Non deve essere fatta la free del puntatore ritornato.
 */
#define read_from_getenv(varname) ({ 	\
		char * s = getenv(varname); 	\
		s;								\
	})

/**
 * Permette di leggere un intero diverso da 0 dalla variabile di ambiente specificata
 * da varname. Se il valore letto dalla variabile d'ambiente non è impostato
 * o si è verificato un errore durante la conversione in uint32_t il si ritorna
 * il valore di defaut che viene passato alla funzione.
 */
#define read_uint32_t_from_getenv(varname, default_value) ({	\
		int v = default_value;	 								\
		do { 													\
			char * s = read_from_getenv(varname);				\
			if (s != NULL) { 									\
				uint32_t ret = strtoul(s, NULL, 10);			\
				if (ret > 0) {									\
					v = ret;									\
				}												\
			}													\
		} while(0); 											\
		v; 														\
})

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
