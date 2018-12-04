/*
 * Copyright (c) 1999, 2002 Lennart Augustsson <augustss@netbsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>
#include <dev/usb/usb.h>

#ifndef USB_STACK_VERSION
#define uds_requests requests
#endif

#define USBDEV "/dev/usb"

void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "Usage: %s [-f device]\n", __progname);
	exit(1);
}

void
stats(char *dev, int msg)
{
	int f, r;
	struct usb_device_stats stats;

	f = open(dev, O_RDWR);
	if (f < 0) {
		if (msg)
			err(1, "%s", dev);
		else
			return;
	}
	r = ioctl(f, USB_DEVICESTATS, &stats);
	if (r < 0)
		err(1, "USB_DEVICESTATS");
	if (!msg)
		printf("Controller %s:\n", dev);
	printf("%10lu control\n",     stats.uds_requests[UE_CONTROL]);
	printf("%10lu isochronous\n", stats.uds_requests[UE_ISOCHRONOUS]);
	printf("%10lu bulk\n",        stats.uds_requests[UE_BULK]);
	printf("%10lu interrupt\n",   stats.uds_requests[UE_INTERRUPT]);
}

int
main(int argc, char **argv)
{
	int ch;
	char *dev = 0;

	while ((ch = getopt(argc, argv, "f:")) != -1) {
		switch(ch) {
		case 'f':
			dev = optarg;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (dev)
		stats(dev, 1);
	else {
		int i;
		char buf[20];
		for (i = 0; i < 10; i++) {
			sprintf(buf, "%s%d", USBDEV, i);
			stats(buf, 0);
		}
	}
	exit(0);
}
