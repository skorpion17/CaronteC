/*
 * full_write.h
 *
 *  Created on: 17/lug/2015
 *      Author: Andrea Mayer
 */

#ifndef FULL_WRITE_H_
#define FULL_WRITE_H_

#include <errno.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

/**
 * Scrittura su socket. Scrive count byte prelevati da buff sul fd.
 */
ssize_t full_write(int fd, const void *buf, size_t count);

#endif /* FULL_WRITE_H_ */
