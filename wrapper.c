/*
 * wrapper.c
 *
 *  Created on: 16/lug/2015
 *      Author: andrea
 */

#include "wrapper.h"

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
	char *s = NULL;
	struct sockaddr_in *addr_in = (struct sockaddr_in *) addr;
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

/**
 * Risoluzione dell'host.
 * FIXME: TODO da mandare al proxy per la risoluzione del DNS attraverso Tor.
 *
 * @Deprecated
 */
struct hostent *gethostbyname(const char *name) {
	printf("\t>>> Wrapped gethostbyname <<< \n");
	real_gethostbyname = dlsym(RTLD_NEXT, REAL_GETHOSTBYNAME_NAME);
	return real_gethostbyname(name);
}

/**
 *  Risoluzione dell'host.
 *  FIXME: TODO da mandare al proxy per la risoluzione del DNS attraverso Tor.
 */
int getaddrinfo(const char *name, const char *service,
		const struct addrinfo *hints, struct addrinfo **res) {
	printf("\t>>> Wrapped getaddrinfo <<< \n");
	real_getaddrinfo = dlsym(RTLD_NEXT, REAL_GETADDRINFO_NAME);
	return real_getaddrinfo(name, service, hints, res);
}

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
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
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
			case HTTP_PORT:
			case HTTPS_PORT:
				do {
					forward_to_proxy = TRUE;
					printf("\t>>> Forward to proxy <<< \n");
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

				printf("\t>>> Destionation host and port are "
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
		/* TODO: Non ancora implementato */
		break;
	}
	return -1;
}

