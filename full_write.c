/*
 * full_write.c
 *
 *  Created on: 17/lug/2015
 *      Author: Andrea Mayer
 */

#include "full_write.h"

/**
 * Implementazione della write per scrivere sulla socket il cui
 * descrittore è identificato da fd, il buffer di lettura è buf
 * e la quantità totale di byte da leggere è pari a count.
 */
ssize_t full_write(int fd, const void *buf, size_t count) {
	size_t nleft;
	ssize_t nwritten;

	nleft = count;
	/* Ripeti fino a quando ci sono byte da leggere. */
	while (nleft > 0) {
		if ((nwritten = write(fd, buf, nleft)) < 0) {
			if (errno == EINTR) {
				/* Nel caso in cui ci sia una interruzione da syscall, ripeti il loop.*/
				continue; /* repeat the loop */
			} else {
				/**
				 * Per tutti gli altri messaggi di errore si interrompe la lettura
				 * e si ritorna il numero di byte letti.
				 */
				return (nwritten);
			}
		}
		nleft -= nwritten;
		buf += nwritten;
	}
	return (nleft);
}
