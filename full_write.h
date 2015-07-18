/*
 * full_write.h
 *
 *  Created on: 17/lug/2015
 *      Author: andrea
 */

#ifndef FULL_WRITE_H_
#define FULL_WRITE_H_

#include <errno.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>

/**
 * Scrittura su socket. Scrive count byte prelevati da buff sul fd.
 * 
 * Gestione completa degli errori.
 */
ssize_t full_write(int fd, const void *buf, size_t count);

#endif /* FULL_WRITE_H_ */
