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
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <dev/usb/usb.h>

/* Backwards compatibility */
#ifndef UE_GET_DIR
#define UE_GET_DIR(a)	((a) & 0x80)
#define UE_DIR_IN	0x80
#define UE_DIR_OUT	0x00
#endif

#ifndef USB_STACK_VERSION
#define uid_config_index config_index
#define uid_interface_index interface_index
#define uid_alt_index alt_index
#define uid_desc desc
#define ued_config_index config_index
#define ued_interface_index interface_index
#define ued_alt_index alt_index
#define ued_endpoint_index endpoint_index
#define ued_desc desc
#define ucd_config_index config_index
#define ucd_desc desc
#define uai_config_index config_index
#define uai_interface_index interface_index
#define udi_product product
#define udi_vendor vendor
#define udi_addr addr
#endif

int verbose;

void
show_device_desc(int indent, usb_device_descriptor_t *d)
{
	printf("\
%*sbLength=%d bDescriptorType=%d bcdUSB=%x.%02x bDeviceClass=%d bDeviceSubClass=%d\n\
%*sbDeviceProtocol=%d bMaxPacketSize=%d\n\
%*sidVendor=0x%04x idProduct=0x%04x bcdDevice=%x\n\
%*siManufacturer=%d iProduct=%d iSerialNumber=%d bNumConfigurations=%d\n",
	       indent, "",
	       d->bLength, d->bDescriptorType, 
	       UGETW(d->bcdUSB) >> 8, UGETW(d->bcdUSB) & 0xff, d->bDeviceClass,
	       d->bDeviceSubClass, 
	       indent, "",
	       d->bDeviceProtocol, d->bMaxPacketSize,
	       indent, "",
	       UGETW(d->idVendor), UGETW(d->idProduct), UGETW(d->bcdDevice), 
	       indent, "",
	       d->iManufacturer, d->iProduct, d->iSerialNumber,
	       d->bNumConfigurations);
}

void
show_config_desc(int indent, usb_config_descriptor_t *d)
{
	printf("\
%*sbLength=%d bDescriptorType=%d wTotalLength=%d bNumInterface=%d\n\
%*sbConfigurationValue=%d iConfiguration=%d bmAttributes=%x bMaxPower=%d mA\n",
	       indent, "",
	       d->bLength, d->bDescriptorType, 
	       UGETW(d->wTotalLength), d->bNumInterface, 
	       indent, "",
	       d->bConfigurationValue, d->iConfiguration,
	       d->bmAttributes, d->bMaxPower*2);
}

void
show_interface_desc(int indent, usb_interface_descriptor_t *d)
{
	printf("\
%*sbLength=%d bDescriptorType=%d bInterfaceNumber=%d bAlternateSetting=%d\n\
%*sbNumEndpoints=%d bInterfaceClass=%d bInterfaceSubClass=%d\n\
%*sbInterfaceProtocol=%d iInterface=%d\n",
	       indent, "",
	       d->bLength, d->bDescriptorType, d->bInterfaceNumber,
	       d->bAlternateSetting, 
	       indent, "",
	       d->bNumEndpoints, d->bInterfaceClass, d->bInterfaceSubClass, 
	       indent, "",
	       d->bInterfaceProtocol, d->iInterface);
}

void
show_endpoint_desc(int indent, usb_endpoint_descriptor_t *d)
{
	printf("\
%*sbLength=%d bDescriptorType=%d bEndpointAddress=%d-%s\n\
%*sbmAttributes=%x wMaxPacketSize=%d bInterval=%d\n",
	       indent, "",
	       d->bLength, d->bDescriptorType,
	       d->bEndpointAddress & UE_ADDR,
	       UE_GET_DIR(d->bEndpointAddress) == UE_DIR_IN ? "in" : "out",
	       indent, "",
	       d->bmAttributes, UGETW(d->wMaxPacketSize), d->bInterval);
}

void
set_conf(int f, int conf)
{
	if (verbose)
		printf("setting configuration %d\n", conf);
	if (ioctl(f, USB_SET_CONFIG, &conf) != 0)
		err(1, "ioctl USB_SET_CONFIG");
}

void
dump_idesc(int f, int all, int cindex, int iindex, int aindex)
{
	struct usb_interface_desc idesc;
	struct usb_endpoint_desc edesc;
	int e;

	idesc.uid_config_index = cindex;
	idesc.uid_interface_index = iindex;
	idesc.uid_alt_index = aindex;
	/*printf("*** idesc %d %d %d\n", cindex, iindex, aindex);*/
	if (ioctl(f, USB_GET_INTERFACE_DESC, &idesc) != 0)
		err(1, "ioctl USB_GET_INTERFACE_DESC");
	if (all) {
		printf("  INTERFACE descriptor index %d, alt index %d:\n",
		       iindex, aindex);
	} else {
		printf("  INTERFACE descriptor index %d:\n", iindex);
	}
	show_interface_desc(2, &idesc.uid_desc);
	printf("\n");
	
	edesc.ued_config_index = cindex;
	edesc.ued_interface_index = iindex;
	edesc.ued_alt_index = aindex;
	for (e = 0; e < idesc.uid_desc.bNumEndpoints; e++) {
		edesc.ued_endpoint_index = e;
		if (ioctl(f, USB_GET_ENDPOINT_DESC, &edesc) != 0)
			err(1, "ioctl USB_GET_ENDPOINT_DESC");
		printf("    ENDPOINT descriptor index %d:\n", e);
		show_endpoint_desc(4, &edesc.ued_desc);
		printf("\n");
	}
}

void
dump_cdesc(int f, int all, int cindex)
{
	struct usb_config_desc cdesc;
	struct usb_alt_interface ai;
	int i, a;

	cdesc.ucd_config_index = cindex;
	if (ioctl(f, USB_GET_CONFIG_DESC, &cdesc) != 0)
		err(1, "ioctl USB_GET_CONFIG_DESC");
	if (all)
		printf("CONFIGURATION descriptor index %d:\n", cindex);
	else
		printf("CONFIGURATION descriptor:\n");
	show_config_desc(0, &cdesc.ucd_desc);
	printf("\n");
	
	for (i = 0; i < cdesc.ucd_desc.bNumInterface; i++) {
		if (all) {
#if 0
			if (ioctl(f, USB_GET_ALTINTERFACE, &ai) != 0)
				err(1, "USB_GET_ALTINTERFACE");
			printf("Current alternative %d\n", ai->alt_no);
#endif
			ai.uai_config_index = cindex;
			ai.uai_interface_index = i;
			if (ioctl(f, USB_GET_NO_ALT, &ai) != 0)
				err(1, "USB_GET_NO_ALT");
			/*printf("*** %d alts\n", ai.alt_no);*/
			for (a = 0; a < ai.uai_alt_no; a++)
				dump_idesc(f, all, cindex, i, a);
		} else {
			dump_idesc(f, all, cindex, i, USB_CURRENT_ALT_INDEX);
		}
	}
}

void
dump_desc(int f, int all)
{
	usb_device_descriptor_t ddesc;
	int c, co;

	if (verbose)
		printf("Dumping %s descriptors\n", all ? "all" : "current");
	if (ioctl(f, USB_GET_DEVICE_DESC, &ddesc) != 0)
		err(1, "ioctl USB_GET_DEVICE_DESC");
	printf("DEVICE descriptor:\n");
	show_device_desc(0, &ddesc);
	printf("\n");

	if (all) {
		if (ioctl(f, USB_GET_CONFIG, &co) != 0)
			err(1, "ioctl USB_GET_CONFIG");
		printf("Current configuration is number %d\n\n", co);
		for (c = 0; c < ddesc.bNumConfigurations; c++)
			dump_cdesc(f, all, c);
	} else
		dump_cdesc(f, all, USB_CURRENT_CONFIG_INDEX);
}

void
dump_deviceinfo(int f)
{
	struct usb_device_info di;

	if (ioctl(f, USB_GET_DEVICEINFO, &di) != 0)
		err(1, "USB_GET_DEVICEINFO");
	printf("Product: %s\n", di.udi_product);
	printf("Vendor:  %s\n", di.udi_vendor);
	printf("address %d\n", di.udi_addr);
}

void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "Usage: %s [-c configno] [-d] [-D] [-i] -f device [-v]\n", __progname);
	exit(1);
}

int
main(int argc, char **argv)
{
	char *dev = 0;
	char devbuf[1024];
	int f, ch, i;

	/* Find device first */
	for (i = 1; i < argc-1; i++) {
		if (strcmp(argv[i], "-f") == 0) {
			dev = argv[i+1];
			break;
		}
	}
	if (!dev)
		usage();

	f = open(dev, O_RDWR);
	if (f < 0) {
		if (dev[0] != '/') {
			sprintf(devbuf, "/dev/%s", dev);
			f = open(devbuf, O_RDWR);
		} else
			strcpy(devbuf, dev);
		if (f < 0) {
			if (!strchr(devbuf, '.')) {
				strcat(devbuf, ".00");
				f = open(devbuf, O_RDWR);
			}
		}
	}
	if (f < 0)
		err(1, "%s", dev);

	while ((ch = getopt(argc, argv, "c:dDf:iv")) != -1) {
		switch(ch) {
		case 'c':
			set_conf(f, atoi(optarg));
			break;
		case 'd':
			dump_desc(f, 0);
			break;
		case 'D':
			dump_desc(f, 1);
			break;
		case 'f':
			break;
		case 'i':
			dump_deviceinfo(f);
			printf("\n");
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	exit(0);
}
