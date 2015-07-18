/*
 * full_write.c
 *
 *  Created on: 17/lug/2015
 *      Author: andrea
 */

#include "full_write.h"

ssize_t full_write(int fd, const void *buf, size_t count) {
	size_t nleft;
	ssize_t nwritten;

	nleft = count;
	while (nleft > 0) { /* repeat until no left */
		if ((nwritten = write(fd, buf, nleft)) < 0) {
			if (errno == EINTR) { /* if interrupted by system call */
				continue; /* repeat the loop */
			} else {
				return (nwritten); /* otherwise exit with error */
			}
		}
		nleft -= nwritten; /* set left to write */
		buf += nwritten; /* set pointer */
	}
	return (nleft);
}
