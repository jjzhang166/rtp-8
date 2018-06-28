/*
 * Copyright (c) 2018 Jan stary <hans@stare.cz>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <err.h>

#include "format-dump.h"

/* Read over the DUMPHDR line.
 * Check that the version is there, ignore the addr/port.
 * Return bytes read, or -1 on error. */
ssize_t
read_dumpline(int fd, void *buf, size_t len)
{
	char p;
	ssize_t r;
	if ((read(fd, buf, DUMPLINELEN) != DUMPLINELEN)
	||  (strncmp(buf, DUMPLINE, DUMPLINELEN) != 0)
	||  (strncmp(buf+9, "1.0", 3) != 0)) {
		warnx("Invalid dump line");
		return -1;
	}
	r = DUMPLINELEN;
	while (read(fd, &p, 1) == 1) {
		r++;
		if (p == '\n')
			break;
	}
	if (p != '\n') {
		warnx("Invalid dump file header");
		return -1;
	}
	return r;
}

void
print_dumphdr(struct dumphdr *dumphdr)
{
	if (dumphdr == NULL)
		return;
	printf("dump starts on %u:%u\n",
		dumphdr->time.sec, dumphdr->time.usec);
}

/* Read the binary dumphdr.
 * Return bytes read, or -1 on error. */
ssize_t
read_dumphdr(int fd, void *buf, size_t len)
{
	struct dumphdr *dumphdr = (struct dumphdr*) buf;
	if (read(fd, buf, DUMPHDRSIZE) != DUMPHDRSIZE) {
		warnx("Broken dump header");
		return -1;
	}
	dumphdr->time.sec = ntohl(dumphdr->time.sec);
	dumphdr->time.usec = ntohl(dumphdr->time.usec);
	dumphdr->addr = ntohl(dumphdr->addr);
	dumphdr->port = ntohs(dumphdr->port);
	return DUMPHDRSIZE;
}

void
print_dpkthdr(struct dpkthdr *dpkthdr)
{
	if (dpkthdr == NULL)
		return;
	printf("%08u ", dpkthdr->usec);
	if (dpkthdr->plen) {
		printf("RTP %u bytes (%lu captured)",
			dpkthdr->plen, dpkthdr->dlen - DPKTHDRSIZE);
	} else {
		printf("RTCP");
	}
	putchar('\n');
}

ssize_t
read_dpkthdr(int fd, void *buf, size_t len)
{
	ssize_t r = 0;
	struct dpkthdr *dpkthdr;
	if (len < DPKTHDRSIZE) {
		warnx("Bufer full");
		return -1;
	}
	if ((r = read(fd, buf, DPKTHDRSIZE)) == 0) {
		return 0;
	} else if (r != DPKTHDRSIZE) {
		warnx("Error reading dumped packet header");
		return -1;
	}
	dpkthdr = (struct dpkthdr*) buf;
	dpkthdr->dlen = ntohs(dpkthdr->dlen);
	dpkthdr->plen = ntohs(dpkthdr->plen);
	dpkthdr->usec = ntohl(dpkthdr->usec);
	return r;
}

/* Read the next packet stored in a dump file into a buffer.
 * Return bytes read, or -1 on error. */
ssize_t
read_dump(int fd, void *buf, size_t len)
{
	ssize_t r, s = 0;
	struct dpkthdr *dpkthdr;
	if ((r = read_dpkthdr(fd, buf, len)) == -1)
		return -1;
	dpkthdr = (struct dpkthdr*) buf;
	s += r, len -= r;
	if (len < dpkthdr->dlen - DPKTHDRSIZE) {
		warnx("Buffer full");
		return -1;
	}
	if ((r = read(fd, buf+s, dpkthdr->dlen - DPKTHDRSIZE)) == -1)
		return -1;
	s += r, len -= r;
	print_dpkthdr(dpkthdr);
	return s;
}

ssize_t
write_dump(int fd, void *buf, size_t len)
{
	return write(fd, buf, len);
}