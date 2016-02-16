/*
 * wrapper.c
 *
 *  Created on: 16/lug/2015
 *      Author: Andrea Mayer
 */

#include "wrapper.h"

#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/un.h>

/**
 * Calcoa il timestamp.
 */
long long __current_timestamp() {
	struct timeval te;
	gettimeofday(&te, NULL);
	/* Ottiene il calcolo in millisecondi del tempo attuale. */
	const long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
	return milliseconds;
}

/**
 * Stampa sulla console il timestamp al momento dell'invocazione della funzione.
 */
void __print_current_timestamp() {
	const long long milliseconds = __current_timestamp();
	printf("\t @@@ %lld @@@ \n", milliseconds);
}

/**
 * Cambia la destinazione e la porta della sockaddr passata come parametro.
 */
void __change_sockaddr(struct sockaddr * const saddr, const in_addr_t s_addr,
		const in_port_t sin_port) {
	/*
	 * Casting a sockaddr_in da sockaddr
	 */
	struct sockaddr_in * const addr_in = (struct sockaddr_in *) saddr;
	/*
	 * Imposto l'indirizzo e la porta che sono in network byte order
	 */
	addr_in->sin_addr.s_addr = s_addr;
	addr_in->sin_port = sin_port;
}

/**
 * Stampa a video l'indirizzo addr
 */
void __print_addr(const struct sockaddr *addr) {
	struct sockaddr_in *addr_in = (struct sockaddr_in *) addr;
	if (addr->sa_family == AF_INET) {
		/* Stampa a video se è IPV4, altrimenti non fa nulla. */
		char *s = NULL;
		if ((s = calloc(1, INET_ADDRSTRLEN)) == NULL) {
			/* Non è stato possibile allocare la memoria, errore fatale. */
			perror("calloc fatal error");
			exit(1);
		}
		/*
		 * Converte da decimale a stringa
		 */
		inet_ntop(AF_INET, &(addr_in->sin_addr), s, INET_ADDRSTRLEN);
		printf("\t>>> Wrapped connect: connection to address (%s:%d) <<< \n", s,
				ntohs(addr_in->sin_port));
		free(s);
	}
}

/**
 * Risoluzione dell'host.
 *
 * @Deprecated
 */
struct hostent *gethostbyname(const char *name) {
	printf("\n\t>>> Wrapped gethostbyname <<< \n");
	printf("\t>>> gethostbyname hostname:%s <<< \n", name);
	real_gethostbyname = dlsym(RTLD_NEXT, REAL_GETHOSTBYNAME_NAME);
	return real_gethostbyname(name);
}

/**
 *  Risoluzione dell'host.
 */
int getaddrinfo(const char *hostname, const char *service,
		const struct addrinfo *hints, struct addrinfo **res) {
	printf("\n\t>>> Wrapped getaddrinfo <<< \n");
	printf("\t>>> getaddrinfo hostname:%s, service:%s <<< \n", hostname,
			service);
	real_getaddrinfo = dlsym(RTLD_NEXT, REAL_GETADDRINFO_NAME);
	// printf("\t>>> before real_getaddrinfo <<< \n");
	const int ret = real_getaddrinfo(hostname, service, hints, res);
	printf("\t>>> getaddrinfo hostname: %s, service %s returned <<< \n",
			hostname, service);
	return ret;
}

/**
 * Verifica se l'hostname è un hidden service identificato dal suffisso .onion.
 * Ritorna 0 se hostname risulta essere un hidden service -1 altrimenti.
 *
 * @Deprecated
 */
int __is_onion_hostname(const char *hostname) {
	regex_t regex;
	int ret_regexc = -1;
	int retcode = -1;

	/* Si compila l'espressione regolare (questa cosa potrebbe essere fatta una sola volta) */
	if (regcomp(&regex, ".+\\.onion", REG_EXTENDED) != 0) {
		printf("Could not compile regex\n");
		/*
		 * Questo è un errore fatale, e si esce dalla applicazione.
		 */
		exit(1);
	}
	/* Si esegue il match */
	ret_regexc = regexec(&regex, hostname, 0, NULL, 0);
	switch (ret_regexc) {
	case 0:
		/* Successo */
		retcode = 0;
		break;

	case REG_NOMATCH:
		/* Non c'è stato il match */
		retcode = -1;
		break;

	default:
		/* Erorri fatali non gestiti */
		printf("regexec fatal error.\n");
		exit(1);
		break;
	}

	/*
	 * Libero la memoria e ritorno il risultato del matching con l'hostname.
	 */
	regfree(&regex);
	return retcode;
}

/**
 * Ridefinizione della connect di libc. La connect è utilizzata sia per la risoluzione
 * dei nomi di dominio (DNS) che per la connessione delle socket.
 * Queste versione di connect permette, in base a delle porte specificate, di connettersi
 * tramite un protocollo s-SOCKS al Web Proxy per la navigazione anonima attraverso
 * la rete Tor. Per tutte le altre porte che non sono abilitate a dirottare il traffico sul "Web Proxy"
 * il comportamento della connect è identico a quello della medesima funzione della libc.
 */
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	//__print_current_timestamp();
	/*
	 * A seconda del tipo di family (IPV4, IPV6)
	 */
	switch (addr->sa_family) {
	case AF_INET:
		do {
			/* DEBUG */
			__print_addr(addr);
			/**
			 * Ottengo l'indirizzo ip e la porta remota a cui il client si vuole
			 * connettere (tutto in Network Byte Order).
			 */
			struct sockaddr_in *addr_in = (struct sockaddr_in *) addr;
			const in_port_t remote_port_network_byte_order = addr_in->sin_port;
			const in_addr_t remote_address = addr_in->sin_addr.s_addr;
			uint8_t forward_to_proxy = FALSE;

			/*
			 * A seconda della porta di destinazione per la connessione, si verifica
			 * se esiste la regola di redirezione al proxy server
			 */
			switch (ntohs(remote_port_network_byte_order)) {

			/* DNS */
			case DNS_PORT :
				/*
				 * Se stiamo facendo una risoluzione, allora bisogna redirigere la richiesta
				 * al servizio di resolution offerto dal PROXY Server
				 */
				do {
					printf("\t>>> Forward to TorDNS <<< \n");
					/*
					 * Viene cambiato l'indirizzo di destinazione e la porta di destinazione in modo tale che la
					 * risoluzione DNS punti al TorDNS server anzichè al DNS Server locale.
					 */
					__change_sockaddr((struct sockaddr *) addr,
					PROXY_TORDNS_ADDR_NETWORK_BYTE_ORDER,
					PROXY_TORDNS_PORT_NETWORK_BYTE_ORDER);

					/* DEBUG */
					__print_addr(addr);

					printf("\t>>> Sockaddr has been changed <<< \n");
				} while (0);
				break;

#ifndef FORWARD_ALL
				/* solo servizi in ascolto su porte note HTTP, HTTP_PROXY e HTTPS  */
			case HTTP_PORT:
			case HTTP_PORT_PROXY:
			case HTTPS_PORT:
#else
				/**
				 * Il caso default permette di gestire un qualsiasi numero di porta.
				 * In questo modo non si è limitati soltanto alle porte dei servizi noti.
				 */
				default:
#endif
				do {
					forward_to_proxy = TRUE;
					printf("\t>>> Forward to Web Proxy <<< \n");
					/*
					 * Viene cambiato l'indirizzo di destinazione e la porta di destinazione in modo tale che
					 * questi puntino al proxy locale.
					 */
					__change_sockaddr((struct sockaddr *) addr,
					PROXY_ADDR_NETWORK_BYTE_ORDER,
					PROXY_PORT_NETWORK_BYTE_ORDER);

					/* DEBUG */
					__print_addr(addr);

					printf("\t>>> Sockaddr has been changed <<< \n");
				} while (0);
				break;
			}

			printf("\t>>> resolution of CONNECT address <<< \n");
			/**
			 * Viene risolta la connect originale e viene effettuata la chiamata.
			 */
			real_connect = dlsym(RTLD_NEXT, REAL_CONNECT_NAME);

			printf("\t>>> call to CONNECT <<< \n");

			/* Viene eseguita la connect */
			int connect_return_code = -1;
			uint8_t non_blocking = FALSE;
			if ((connect_return_code = real_connect(sockfd, addr, addrlen))
					< 0) {
				switch (errno) {
				case EINPROGRESS:
					do {
						non_blocking = TRUE;
						connect_return_code = 0;
					} while (0);
					break;
				default:
					/* Errore nella connect che non è stato gestito */
					return -1;
				}
			}

			if (forward_to_proxy == TRUE) {
				/*
				 * Appena apro la socket, scrivo su questa subito l'indirizzo di
				 * destinazione e la porta
				 */
				if (non_blocking == TRUE) {
					int result_getsockopt = 0;
					/* La socket è non bloccante, bisogna attendere che diventi "scrivibile" */
					socklen_t result_len_getsockop = sizeof(result_getsockopt);
					if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR,
							&result_getsockopt, &result_len_getsockop) < 0) {
						/*
						 * C'è stato un errore sulla socket,
						 * ritorno immediatamente con l'errore.
						 */
						return -1;
					}
				}

				/* Arrivata qui la socket è scribile */
				printf("\t>>> Socket is writable <<< \n");

				/*
				 * La connessione che viene aperta col proxy server è simile ad un SOCKS.
				 * Infatti la nuova connect invia:
				 * 		1) AF_FAM che indica la famiglia, IPV4 o IPV6
				 * 		2) IP_ADDR_NTB che indica l'indirizzo di destinazione in network byte order.
				 * 		3) PORT_NTB che indica la porta di destinazione in network byte order.
				 *
				 * 	AF_FAM sono 2 byte (sa_family_t).
				 * 	IP_ADDR_NTB dipende dalla AF_FAMILY
				 * 	PORT_NTB sono 2 byte.
				 *
				 * +--------+-------------+----------+
				 * | AF_FAM | IP_ADDR_NTB | PORT_NTB |
				 * +--------+-------------+----------+
				 */

				/* Anche la sa_family viene convertita in network_byte_order */
				const sa_family_t sa_family_network_byte_order = htons(
						addr->sa_family);
				if (full_write(sockfd, (void *) &sa_family_network_byte_order,
						sizeof(sa_family_t)) != 0) {
					/* C'è stato un problema nella scrittura della SA_FAMILY (network_byte_order) */
					return -1;
				}
				if (full_write(sockfd, (void *) &remote_address,
						sizeof(in_addr_t)) != 0) {
					/* C'è stato un problema nella scrittura dell'indirizzo (network byte order) */
					return -1;
				}
				if (full_write(sockfd, (void *) &remote_port_network_byte_order,
						sizeof(in_port_t)) != 0) {
					/* C'è stato un problema nella scrittura della porta remota (network byte order) */
					return -1;
				}

				printf("\t>>> Destination host and port are "
						"sent to proxy server <<<\n");
			}

			printf("\t>>> CONNECT is done <<< \n");
			/*
			 * Se siamo arrivati qui e abbiamo scritto con successo <host_remoto,porta_remota> allora ritorniamo con la connect su 0
			 * che indica successo.
			 */
			return 0;
		} while (0);
		break;

	case AF_INET6:
		/* Non implementato. */
		break;
	}
	return -1;
}

