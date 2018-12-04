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
#include <err.h>
#include <errno.h>
#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

#ifndef USB_STACK_VERSION
#define ucr_addr addr
#define ucr_request request
#define ucr_data data
#define ucr_flags flags
#define udi_addr addr
#define udi_class class
#endif

#ifndef UICLASS_HID
#define UICLASS_HID UCLASS_HID
#define UICLASS_AUDIO UCLASS_AUDIO
#define UISUBCLASS_AUDIOCONTROL USUBCLASS_AUDIOCONTROL
#define UISUBCLASS_AUDIOSTREAM USUBCLASS_AUDIOSTREAM
#define UICLASS_CDC UCLASS_CDC
#define UICLASS_HUB UCLASS_HUB
#endif

#define USBDEV "/dev/usb0"

/* Backwards compatibility */
#ifndef UE_GET_DIR
#define UE_GET_DIR(a)	((a) & 0x80)
#define UE_DIR_IN	0x80
#define UE_DIR_OUT	0x00
#endif

#define NSTRINGS

int num = 0;

static int usbf, usbaddr;
void
setupstrings(int f, int addr)
{
	usbf = f;
	usbaddr = addr;
}

void
getstring(int si, char *s)
{
	struct usb_ctl_request req;
	int r, i, n;
	u_int16_t c;
	usb_string_descriptor_t us;

	if (si == 0 || num) {
		*s = 0;
		return;
	}
	req.ucr_addr = usbaddr;
	req.ucr_request.bmRequestType = UT_READ_DEVICE;
	req.ucr_request.bRequest = UR_GET_DESCRIPTOR;
	req.ucr_data = &us;
	USETW2(req.ucr_request.wValue, UDESC_STRING, si);
	USETW(req.ucr_request.wIndex, 0);
#ifdef NSTRINGS
	USETW(req.ucr_request.wLength, sizeof(usb_string_descriptor_t));
	req.ucr_flags = USBD_SHORT_XFER_OK;
#else
	USETW(req.ucr_request.wLength, 1);
	req.ucr_flags = 0;
#endif
	r = ioctl(usbf, USB_REQUEST, &req);
	if (r < 0) {
		fprintf(stderr, "getstring %d failed (error=%d)\n", si, errno);
		*s = 0;
		return;
	}
#ifndef NSTRINGS
	USETW(req.ucr_request.wLength, us.bLength);
	r = ioctl(usbf, USB_REQUEST, &req);
	if (r < 0)
		err(1, "USB_REQUEST");
#endif
	n = us.bLength / 2 - 1;
	for (i = 0; i < n; i++) {
		c = UGETW(us.bString[i]);
		if ((c & 0xff00) == 0)
			*s++ = c;
		else if ((c & 0x00ff) == 0)
			*s++ = c >> 8;
		else {
			sprintf(s, "\\u%04x", c);
			s += 6;
		}
	}
	*s++ = 0;
}

void
prunits(int f)
{
	struct usb_device_info di;
	int r, n, i;

	for(n = i = 0; i < USB_MAX_DEVICES; i++) {
		di.udi_addr = i;
		r = ioctl(f, USB_DEVICEINFO, &di);
		if (r == 0) {
			printf("USB device %d: %d\n", i, di.udi_class);
			n++;
		}
	}
	printf("%d USB devices found\n", n);
}

char *
descTypeName(int t)
{
	static char b[100];
	char *p = 0;

	switch (t) {
	case UDESC_DEVICE: p = "device"; break;
	case UDESC_CONFIG: p = "config"; break;
	case UDESC_STRING: p = "string"; break;
	case UDESC_INTERFACE: p = "interface"; break;
	case UDESC_ENDPOINT: p = "endpoint"; break;
	case 0x20: p = "cs_undefined"; break;
	case UDESC_CS_DEVICE: p = "cs_device"; break;
	case UDESC_CS_CONFIG: p = "cs_config"; break;
	case UDESC_CS_STRING: p = "cs_string"; break;
	case UDESC_CS_INTERFACE: p = "cs_interface"; break;
	case UDESC_CS_ENDPOINT: p = "cs_endpoint"; break;
	}
	if (p)
		sprintf(b, "%s(%d)", p, t);
	else
		sprintf(b, "%d", t);
	return b;
}

#define UDESCSUB_AC_HEADER 1
#define UDESCSUB_AC_INPUT 2
#define UDESCSUB_AC_OUTPUT 3
#define UDESCSUB_AC_MIXER 4
#define UDESCSUB_AC_SELECTOR 5
#define UDESCSUB_AC_FEATURE 6
#define UDESCSUB_AC_PROCESSING 7
#define UDESCSUB_AC_EXTENSION 8

#define UDESCSUB_AS_GENERAL 1
#define UDESCSUB_AS_FORMAT_TYPE 2
#define UDESCSUB_AS_FORMAT_SPECIFIC 3

char *
acSubTypeName(int t)
{
	static char b[100];
	char *p = 0;

	switch (t) {
	case 0: p = "ac_descriptor_undefined"; break;
	case 1: p = "header"; break;
	case 2: p = "input_terminal"; break;
	case 3: p = "output_terminal"; break;
	case 4: p = "mixer_unit"; break;
	case 5: p = "selector_unit"; break;
	case 6: p = "feature_unit"; break;
	case 7: p = "processing_unit"; break;
	case 8: p = "extension_unit"; break;
	}
	if (p)
		sprintf(b, "%s(%d)", p, t);
	else
		sprintf(b, "%d", t);
	return b;
}

char *
asSubTypeName(int t)
{
	static char b[100];
	char *p = 0;

	switch (t) {
	case 0: p = "as_descriptor_undefined"; break;
	case 1: p = "as_general"; break;
	case 2: p = "format_type"; break;
	case 3: p = "format_specific"; break;
	}
	if (p)
		sprintf(b, "%s(%d)", p, t);
	else
		sprintf(b, "%d", t);
	return b;
}

#define MAXSTR (127*6)
void
prdevd(usb_device_descriptor_t *d)
{
	char man[MAXSTR], prod[MAXSTR], ser[MAXSTR];
	getstring(d->iManufacturer, man);
	getstring(d->iProduct, prod);
	getstring(d->iSerialNumber, ser);
	if (d->bDescriptorType != UDESC_DEVICE) printf("weird descriptorType, should be %d\n", UDESC_DEVICE);
	printf("\
bLength=%d bDescriptorType=%s bcdUSB=%x.%02x bDeviceClass=%d bDeviceSubClass=%d\n\
bDeviceProtocol=%d bMaxPacketSize=%d idVendor=0x%04x idProduct=0x%04x bcdDevice=%x\n\
iManufacturer=%d(%s) iProduct=%d(%s) iSerialNumber=%d(%s) bNumConfigurations=%d\n",
	       d->bLength, descTypeName(d->bDescriptorType), 
	       UGETW(d->bcdUSB) >> 8, UGETW(d->bcdUSB) & 0xff, d->bDeviceClass,
	       d->bDeviceSubClass, d->bDeviceProtocol, d->bMaxPacketSize,
	       UGETW(d->idVendor), UGETW(d->idProduct), UGETW(d->bcdDevice), 
	       d->iManufacturer, man,
	       d->iProduct, prod, d->iSerialNumber, ser,
	       d->bNumConfigurations);
}

void
prconfd(usb_config_descriptor_t *d)
{
	char conf[MAXSTR];
	getstring(d->iConfiguration, conf);
	if (d->bDescriptorType != UDESC_CONFIG) printf("weird descriptorType, should be %d\n", UDESC_CONFIG);
	printf("\
bLength=%d bDescriptorType=%s wTotalLength=%d bNumInterface=%d\n\
bConfigurationValue=%d iConfiguration=%d(%s) bmAttributes=%x bMaxPower=%d mA\n",
	       d->bLength, descTypeName(d->bDescriptorType), 
	       UGETW(d->wTotalLength),
	       d->bNumInterface, d->bConfigurationValue, d->iConfiguration,
	       conf,
	       d->bmAttributes, d->bMaxPower*2);
}

void
prifcd(usb_interface_descriptor_t *d)
{
	char ifc[MAXSTR];
	getstring(d->iInterface, ifc);
	if (d->bDescriptorType != UDESC_INTERFACE) printf("weird descriptorType, should be %d\n", UDESC_INTERFACE);
	printf("\
bLength=%d bDescriptorType=%s bInterfaceNumber=%d bAlternateSetting=%d\n\
bNumEndpoints=%d bInterfaceClass=%d bInterfaceSubClass=%d\n\
bInterfaceProtocol=%d iInterface=%d(%s)\n",
	       d->bLength, descTypeName(d->bDescriptorType), d->bInterfaceNumber,
	       d->bAlternateSetting, d->bNumEndpoints, d->bInterfaceClass,
	       d->bInterfaceSubClass, d->bInterfaceProtocol, 
	       d->iInterface, ifc);
}

char *xfernames[] = { "control", "isochronous", "bulk", "interrupt" };
char *xfertypes[] = { "", "-async", "-adaptive", "-sync" };

void
prendpd(usb_endpoint_descriptor_t *d)
{
	if (d->bDescriptorType != UDESC_ENDPOINT) printf("weird descriptorType, should be %d\n", UDESC_ENDPOINT);
	printf("\
bLength=%d bDescriptorType=%s bEndpointAddress=%d-%s\n\
bmAttributes=%s%s wMaxPacketSize=%d bInterval=%d\n",
	       d->bLength, descTypeName(d->bDescriptorType),
	       d->bEndpointAddress & UE_ADDR,
	       UE_GET_DIR(d->bEndpointAddress) == UE_DIR_IN ? "in" : "out",
	       xfernames[d->bmAttributes & UE_XFERTYPE],
	       xfertypes[(d->bmAttributes >> 2) & UE_XFERTYPE],
	       UGETW(d->wMaxPacketSize), d->bInterval);
}

void
prhubd(usb_hub_descriptor_t *d)
{
	if (d->bDescriptorType != UDESC_HUB) printf("weird descriptorType, should be %d\n", UDESC_HUB);
	printf("\
bDescLength=%d bDescriptorType=%s bNbrPorts=%d wHubCharacteristics=%02x\n\
bPwrOn2PwrGood=%d bHubContrCurrent=%d DeviceRemovable=%x\n",
	       d->bDescLength, descTypeName(d->bDescriptorType), d->bNbrPorts,
	       UGETW(d->wHubCharacteristics), d->bPwrOn2PwrGood, d->bHubContrCurrent,
	       d->DeviceRemovable[0]);
}

void
prhidd(usb_hid_descriptor_t *d)
{
	int i;

	printf("\
bLength=%d bDescriptorType=%s bcdHID=%x.%02x bCountryCode=%d bNumDescriptors=%d\n",
	       d->bLength, descTypeName(d->bDescriptorType), 
	       UGETW(d->bcdHID) >> 8,
	       UGETW(d->bcdHID) & 0xff, d->bCountryCode,
	       d->bNumDescriptors);
	for(i = 0; i < d->bNumDescriptors; i++) {
		printf("bDescriptorType[%d]=%s, wDescriptorLength[%d]=%d\n",
		       i, descTypeName(d->descrs[i].bDescriptorType),
		       i, UGETW(d->descrs[i].wDescriptorLength));
	}
}

#define UDESCSUB_CDC_HEADER	0
#define UDESCSUB_CDC_CM		1 /* Call Management */
#define UDESCSUB_CDC_ACM	2 /* Abstract Control Model */
#define UDESCSUB_CDC_DLM	3 /* Direct Line Management */
#define UDESCSUB_CDC_TRF	4 /* Telephone Ringer */
#define UDESCSUB_CDC_TCLSR	5 /* Telephone Call ... */
#define UDESCSUB_CDC_UNION	6
#define UDESCSUB_CDC_CS		7 /* Country Selection */
#define UDESCSUB_CDC_TOM	8 /* Telephone Operational Modes */
#define UDESCSUB_CDC_USBT	9 /* USB Terminal */

char *
descCDCSubtypeName(int s)
{
	static char buf[20];

	switch (s) {
	case UDESCSUB_CDC_HEADER: return "header";
	case UDESCSUB_CDC_CM: return "Call_Management";
	case UDESCSUB_CDC_ACM: return "Abstract_Control_Model";
	case UDESCSUB_CDC_UNION: return "union";
	default:
		sprintf(buf, "CDC_subtype_%d", s);
		return buf;
	}
}

struct usb_cdc_header_descriptor {
	uByte		bLength;
	uByte		bDescriptorType;
	uByte		bDescriptorSubtype;
	uWord		bcdCDC;
};

struct usb_cdc_cm_descriptor {
	uByte		bLength;
	uByte		bDescriptorType;
	uByte		bDescriptorSubtype;
	uByte		bmCapabilities;
	uByte		bDataInterface;
};

struct usb_cdc_acm_descriptor {
	uByte		bLength;
	uByte		bDescriptorType;
	uByte		bDescriptorSubtype;
	uByte		bmCapabilities;
};

struct usb_cdc_union_descriptor {
	uByte		bLength;
	uByte		bDescriptorType;
	uByte		bDescriptorSubtype;
	uByte		bMasterInterface;
	uByte		bSlaveInterface[1];
};

void
prcdcd(usb_descriptor_t *ud)
{
	if (ud->bDescriptorType != UDESC_CS_INTERFACE)
		printf("prcdcd: strange bDescriptorType=%d\n", 
		       ud->bDescriptorType);
	switch (ud->bDescriptorSubtype) {
	case UDESCSUB_CDC_HEADER:
	{
		struct usb_cdc_header_descriptor *d = (void *)ud;
		printf("\
bLength=%d bDescriptorType=%s bDescriptorSubtype=%s\n\
bcdCDC=%x.%02x\n",
		       d->bLength, 
		       descTypeName(d->bDescriptorType), 
		       descCDCSubtypeName(d->bDescriptorSubtype), 
		       UGETW(d->bcdCDC) >> 8,
		       UGETW(d->bcdCDC) & 0xff);
		break;
	}
	case UDESCSUB_CDC_CM:
	{
		struct usb_cdc_cm_descriptor *d = (void *)ud;
		printf("\
bLength=%d bDescriptorType=%s bDescriptorSubtype=%s\n\
bmCapabilities=0x%x bDataInterface=%d\n",
		       d->bLength, 
		       descTypeName(d->bDescriptorType), 
		       descCDCSubtypeName(d->bDescriptorSubtype), 
		       d->bmCapabilities,
		       d->bDataInterface);
		break;
	}
	case UDESCSUB_CDC_ACM:
	{
		struct usb_cdc_acm_descriptor *d = (void *)ud;
		printf("\
bLength=%d bDescriptorType=%s bDescriptorSubtype=%s\n\
bmCapabilities=0x%x\n",
		       d->bLength, 
		       descTypeName(d->bDescriptorType), 
		       descCDCSubtypeName(d->bDescriptorSubtype), 
		       d->bmCapabilities);
		break;
	}
	case UDESCSUB_CDC_UNION:
	{
		struct usb_cdc_union_descriptor *d = (void *)ud;
		int i;
		printf("\
bLength=%d bDescriptorType=%s bDescriptorSubtype=%s\n\
bMasterInterface=%d",
		       d->bLength, 
		       descTypeName(d->bDescriptorType), 
		       descCDCSubtypeName(d->bDescriptorSubtype), 
		       d->bMasterInterface);
		for (i = 0; i < d->bLength - 4; i++)
			printf(" bSlaveInterface%d=%d", 
			       i, d->bSlaveInterface[i]);
		printf("\n");
		break;
	}
	default:
		printf("prcdcd: unknown bDescriptorSubtype=%d\n",
		       ud->bDescriptorSubtype);
		break;
	}
}

void
prbits(int bits, char **strs, int n)
{
	int i;

	for(i = 0; i < n; i++, bits >>= 1)
		if (strs[i*2])
			printf("%s%s", i == 0 ? "" : ", ", strs[i*2 + (bits&1)]);
}

void
prreportd(u_char *d, int len)
{
	int ind;
	u_char *p;

#if 0
	for(i = 0; i < len; i++)
		printf("%02x ", d[i]);
	printf("\n");
#endif

	ind = 0;
	for(p = d; p < d + len;) {
		int bTag, bType, bSize;
		u_char *data;
		long dval;
		static char *gstr[] = {
			"Usage Page", 
			"Logical Min", "Logical Max",
			"Physical Min", "Physical Max",
			"Unit Exponent", "Unit",
			"Report size", "Report ID", 
			"Report count", 
			"Push", "Pop", 
			"??12", "??13", "??14", "??15"};
		static char *lstr[] = {
			"Usage",
			"Usage Min", "Usage Max",
			"Designator index",
			"Designator Min", "Designator Max",
			"??6", "String index",
			"String Min", "String Max",
			"Set delimiter",
			"??11", "??12", "??13", "??14", "??15"
		};
		static char *inputbits[] = {
			"Data", "Constant",
			"Array", "Variable",
			"Absolute", "Relative",
			"No wrap", "Wrap",
			"Linear", "Non linear",
			"Preferred state", "No Preferred",
			"No null position", "Null position",
			0, 0,
			"Bit field", "Bufferred bytes"
		};
		static char *outputbits[] = {
			"Data", "Constant",
			"Array", "Variable",
			"Absolute", "Relative",
			"No wrap", "Wrap",
			"Linear", "Non linear",
			"Preferred state", "No Preferred",
			"No null position", "Null position",
			"Non volatile", "Volatile",
			"Bit field", "Bufferred bytes"
		};
		static char *colls[] = {
			"Physical", "Application", "Logical"
		};

		/*printf("pos = %d\n", p - d);*/
		bSize = *p++;
		if (bSize == 0xfe) {
			/* long item */
			bSize = *p++;
			bSize |= *p++ << 8;
			bTag = *p++;
			data = p;
			p += bSize;
		} else {
			/* short item */
			bTag = bSize >> 4;
			bType = (bSize >> 2) & 3;
			bSize &= 3;
			if (bSize == 3) bSize = 4;
			data = p;
			p += bSize;
		}
		switch(bSize) {
		case 0:
			dval = 0;
			break;
		case 1:
			dval = *data++;
			break;
		case 2:
			dval = *data++;
			dval |= *data++ << 8;
			dval = dval;
			break;
		case 4:
			dval = *data++;
			dval |= *data++ << 8;
			dval |= *data++ << 16;
			dval |= *data++ << 24;
			break;
		default:
			printf("BAD LENGTH %d\n", bSize);
			break;
		}
#define INDENT printf("%*s", ind * 3, "")
		switch (bType) {
		case 0:		/* Main */
			switch (bTag) {
			case 8:
				INDENT;
				printf("Input (");
				prbits(dval, inputbits, 9);
				printf(")\n");
				break;
			case 9:
				INDENT;
				printf("Output (");
				prbits(dval, outputbits, 9);
				printf(")\n");
				break;
			case 10:
				INDENT;
				if (dval >= 0 && dval <= 2)
					printf("Collection (%s)\n", colls[dval]);
				else
					printf("Collection (%ld)\n", dval);
				ind++;
				break;
			case 11:
				INDENT;
				printf("Feature (");
				prbits(dval, outputbits, 9);
				printf(")\n");
				break;
			case 12:
				ind--;
				INDENT;
				printf("End Collection\n");
				break;
			default:
				INDENT;
				printf("??Main bType=%d\n", bTag);
				break;
			}
			break;
		case 1:		/* Global */
			INDENT;
			printf("%s(%ld)\n", gstr[bTag], dval);
			break;
		case 2:		/* Local */
			INDENT;
			printf("%s(%ld)\n", lstr[bTag], dval);
			break;
		default:
			INDENT;
			printf("default\n");
			break;
		}
	}
}

void
gethubdesc(int f, usb_hub_descriptor_t *d, int addr)
{
	struct usb_ctl_request req;
	int r;

	req.ucr_addr = addr;
	req.ucr_request.bmRequestType = UT_READ_CLASS_DEVICE;
	req.ucr_request.bRequest = UR_GET_DESCRIPTOR;
	USETW(req.ucr_request.wValue, 0);
	USETW(req.ucr_request.wIndex, 0);
	USETW(req.ucr_request.wLength, USB_HUB_DESCRIPTOR_SIZE);
	req.ucr_data = d;
	req.ucr_flags = 0;
	r = ioctl(f, USB_REQUEST, &req);
	if (r < 0)
		err(1, "USB_REQUEST");
}

void
getdevicedesc(int f, usb_device_descriptor_t *d, int addr)
{
	struct usb_ctl_request req;
	int r;

	req.ucr_addr = addr;
	req.ucr_request.bmRequestType = UT_READ_DEVICE;
	req.ucr_request.bRequest = UR_GET_DESCRIPTOR;
	USETW2(req.ucr_request.wValue, UDESC_DEVICE, 0);
	USETW(req.ucr_request.wIndex, 0);
	USETW(req.ucr_request.wLength, USB_DEVICE_DESCRIPTOR_SIZE);
	req.ucr_data = d;
	req.ucr_flags = 0;
	r = ioctl(f, USB_REQUEST, &req);
	if (r < 0)
		err(1, "USB_REQUEST");
}

void
getconfigdesc(int f, int i, usb_config_descriptor_t *d, int size, int addr)
{
	struct usb_ctl_request req;
	int r;

	req.ucr_addr = addr;
	req.ucr_request.bmRequestType = UT_READ_DEVICE;
	req.ucr_request.bRequest = UR_GET_DESCRIPTOR;
	USETW2(req.ucr_request.wValue, UDESC_CONFIG, i);
	USETW(req.ucr_request.wIndex, 0);
	USETW(req.ucr_request.wLength, USB_CONFIG_DESCRIPTOR_SIZE);
	req.ucr_data = d;
	req.ucr_flags = 0;
	r = ioctl(f, USB_REQUEST, &req);
	if (r < 0)
		err(1, "USB_REQUEST");
	req.ucr_addr = addr;
	req.ucr_request.bmRequestType = UT_READ_DEVICE;
	req.ucr_request.bRequest = UR_GET_DESCRIPTOR;
	USETW2(req.ucr_request.wValue, UDESC_CONFIG, i);
	USETW(req.ucr_request.wIndex, 0);
	USETW(req.ucr_request.wLength, UGETW(d->wTotalLength));
	req.ucr_data = d;
	r = ioctl(f, USB_REQUEST, &req);
	if (r < 0)
		err(1, "USB_REQUEST");
}

void
gethiddesc(int f, int i, usb_hid_descriptor_t *d, int size, int addr)
{
	struct usb_ctl_request req;
	int r;

	req.ucr_addr = addr;
	req.ucr_request.bmRequestType = UT_READ_INTERFACE;
	req.ucr_request.bRequest = UR_GET_DESCRIPTOR;
	USETW2(req.ucr_request.wValue, UDESC_HID, 0);
	USETW(req.ucr_request.wIndex, i);
	USETW(req.ucr_request.wLength, size);
	req.ucr_data = d;
	req.ucr_flags = 0;
	r = ioctl(f, USB_REQUEST, &req);
	if (r < 0)
		err(1, "USB_REQUEST");
}

void
getreportdesc(int f, int ifc, int no, char *d, int size, int addr)
{
	struct usb_ctl_request req;
	int r;

	req.ucr_addr = addr;
	req.ucr_request.bmRequestType = UT_READ_INTERFACE;
	req.ucr_request.bRequest = UR_GET_DESCRIPTOR;
	USETW2(req.ucr_request.wValue, UDESC_REPORT, no);
	USETW(req.ucr_request.wIndex, ifc);
	USETW(req.ucr_request.wLength, size);
	req.ucr_data = d;
	req.ucr_flags = 0;
	r = ioctl(f, USB_REQUEST, &req);
	if (r < 0)
		err(1, "USB_REQUEST");
}

void
getportstatus(int f, int i, usb_port_status_t *d, int addr)
{
	struct usb_ctl_request req;
	int r;

	req.ucr_addr = addr;
	req.ucr_request.bmRequestType = UT_READ_CLASS_OTHER;
	req.ucr_request.bRequest = UR_GET_STATUS;
	USETW(req.ucr_request.wValue, 0);
	USETW(req.ucr_request.wIndex, i);
	USETW(req.ucr_request.wLength, 4);
	req.ucr_data = d;
	req.ucr_flags = 0;
	r = ioctl(f, USB_REQUEST, &req);
	if (r < 0)
		err(1, "USB_REQUEST");
}

void
gethubstatus(int f, usb_hub_status_t *d, int addr)
{
	struct usb_ctl_request req;
	int r;

	req.ucr_addr = addr;
	req.ucr_request.bmRequestType = UT_READ_CLASS_DEVICE;
	req.ucr_request.bRequest = UR_GET_STATUS;
	USETW(req.ucr_request.wValue, 0);
	USETW(req.ucr_request.wIndex, 0);
	USETW(req.ucr_request.wLength, 4);
	req.ucr_data = d;
	req.ucr_flags = 0;
	r = ioctl(f, USB_REQUEST, &req);
	if (r < 0)
		err(1, "USB_REQUEST");
}

void
getconfiguration(int f, u_int8_t *d, int addr)
{
	struct usb_ctl_request req;
	int r;

	req.ucr_addr = addr;
	req.ucr_request.bmRequestType = UT_READ_DEVICE;
	req.ucr_request.bRequest = UR_GET_CONFIG;
	USETW(req.ucr_request.wValue, 0);
	USETW(req.ucr_request.wIndex, 0);
	USETW(req.ucr_request.wLength, 1);
	req.ucr_data = d;
	req.ucr_flags = 0;
	r = ioctl(f, USB_REQUEST, &req);
	if (r < 0)
		err(1, "USB_REQUEST");
}

void
getdevicestatus(int f, usb_status_t *d, int addr)
{
	struct usb_ctl_request req;
	int r;

	req.ucr_addr = addr;
	req.ucr_request.bmRequestType = UT_READ_DEVICE;
	req.ucr_request.bRequest = UR_GET_STATUS;
	USETW(req.ucr_request.wValue, 0);
	USETW(req.ucr_request.wIndex, 0);
	USETW(req.ucr_request.wLength, 2);
	req.ucr_data = d;
	req.ucr_flags = 0;
	r = ioctl(f, USB_REQUEST, &req);
	if (r < 0)
		err(1, "USB_REQUEST");
}

void
getinterfacestatus(int f, usb_status_t *d, int addr, int ifc)
{
	struct usb_ctl_request req;
	int r;

	req.ucr_addr = addr;
	req.ucr_request.bmRequestType = UT_READ_INTERFACE;
	req.ucr_request.bRequest = UR_GET_STATUS;
	USETW(req.ucr_request.wValue, 0);
	USETW(req.ucr_request.wIndex, ifc);
	USETW(req.ucr_request.wLength, 2);
	req.ucr_data = d;
	req.ucr_flags = 0;
	r = ioctl(f, USB_REQUEST, &req);
	if (r < 0)
		err(1, "USB_REQUEST");
}

void
getendpointstatus(int f, usb_status_t *d, int addr, int endp)
{
	struct usb_ctl_request req;
	int r;

	req.ucr_addr = addr;
	req.ucr_request.bmRequestType = UT_READ_ENDPOINT;
	req.ucr_request.bRequest = UR_GET_STATUS;
	USETW(req.ucr_request.wValue, 0);
	USETW(req.ucr_request.wIndex, endp);
	USETW(req.ucr_request.wLength, 2);
	req.ucr_data = d;
	req.ucr_flags = 0;
	r = ioctl(f, USB_REQUEST, &req);
	if (r < 0)
		err(1, "USB_REQUEST");
}

void
usage(void)
{
	extern char *__progname;

	fprintf(stderr, "Usage: %s [-a addr] [-f device] [-d]\n", __progname);
	exit(1);
}

struct usb_audio_control_descriptor {
	uByte	bLength;
	uByte	bDescriptorType;
	uByte	bDescriptorSubtype;
	uWord	bcdADC;
	uWord	wTotalLength;
	uByte	bInCollection;
	uByte	baInterfaceNr[1];
};

struct usb_audio_streaming_interface_descriptor {
	uByte	bLength;
	uByte	bDescriptorType;
	uByte	bDescriptorSubtype;
	uByte	bTerminalLink;
	uByte	bDelay;
	uWord	wFormatTag;
};

struct usb_audio_streaming_endpoint_descriptor {
	uByte	bLength;
	uByte	bDescriptorType;
	uByte	bDescriptorSubtype;
	uByte	bmAttributes;
	uByte	bLockDelayUnits;
	uWord	wLockDelay;
};

struct usb_audio_descriptor {
	uByte	bLength;
	uByte	bDescriptorType;
	uByte	bDescriptorSubtype;
};	

struct usb_audio_streaming_type1_descriptor {
	uByte	bLength;
	uByte	bDescriptorType;
	uByte	bDescriptorSubtype;
	uByte	bFormatType;
	uByte	bNrChannels;
	uByte	bSubFrameSize;
	uByte	bBitResolution;
	uByte	bSamFreqType;
	uByte	tSamFreq[3];
};
	
struct usb_audio_input_terminal {
	uByte	bLength;
	uByte	bDescriptorType;
	uByte	bDescriptorSubtype;
	uByte	bTerminalId;
	uWord	wTerminalType;
	uByte	bAssocTerminal;
	uByte	bNrChannels;
	uWord	wChannelConfig;
	uByte	iChannelNames;
	uByte	iTerminal;
};

struct usb_audio_output_terminal {
	uByte	bLength;
	uByte	bDescriptorType;
	uByte	bDescriptorSubtype;
	uByte	bTerminalId;
	uWord	wTerminalType;
	uByte	bAssocTerminal;
	uByte	bSourceId;
	uByte	iTerminal;
};

struct usb_audio_feature_unit {
	uByte	bLength;
	uByte	bDescriptorType;
	uByte	bDescriptorSubtype;
	uByte	bUnitId;
	uByte	bSourceId;
	uByte	bControlSize;
	uByte	bmaControls[1];
};

struct usb_audio_mixer_unit {
	uByte	bLength;
	uByte	bDescriptorType;
	uByte	bDescriptorSubtype;
	uByte	bUnitId;
	uByte	bNrInPins;
	uByte	baSourceID[1];
	/* ... and more */
};

struct usb_audio_extension_unit {
	uByte	bLength;
	uByte	bDescriptorType;
	uByte	bDescriptorSubtype;
	uByte	bUnitId;
	uWord	wExtensionCode;
	uByte	bNrInPins;
	uByte	baSourceID[1];
	/* ... and more */
};

void
pracdesc(struct usb_audio_control_descriptor *d)
{
	int i;

	printf("\
bLength=%d bDescriptorType=%s bDescriptorSubtype=%s bcdADC=%x.%02x\n\
wTotalLength=%d bInCollection=%x\n",
	       d->bLength, descTypeName(d->bDescriptorType), 
	       acSubTypeName(d->bDescriptorSubtype),
	       UGETW(d->bcdADC) >> 8, UGETW(d->bcdADC) & 0xff,
	       UGETW(d->wTotalLength), d->bInCollection);
	for (i = 0; i < d->bLength - 8; i++)
		printf("baInterfaceNr[%d]=%d\n", i, d->baInterfaceNr[i]);
}

void
prasigd(struct usb_audio_streaming_interface_descriptor *d)
{
	printf("\
bLength=%d bDescriptorType=%s bDescriptorSubtype=%s\n\
bTerminalLink=%d bDelay=%d wFormatTag=%d\n",
	       d->bLength, descTypeName(d->bDescriptorType),
	       asSubTypeName(d->bDescriptorSubtype),
	       d->bTerminalLink, d->bDelay,
	       UGETW(d->wFormatTag));
}

void
prasiepd(struct usb_audio_streaming_endpoint_descriptor *d)
{
	printf("\
bLength=%d bDescriptorType=%s bDescriptorSubtype=%s bmAttributes=%x\n\
bLockDelayUnits=%d wLockDelay=%d\n",
	       d->bLength, descTypeName(d->bDescriptorType),
	       asSubTypeName(d->bDescriptorSubtype),
	       d->bmAttributes, d->bLockDelayUnits,
	       UGETW(d->wLockDelay));
}


void
prast1d(struct usb_audio_streaming_type1_descriptor *d)
{
	int i, f;
	u_char *p;

	printf("\
bLength=%d bDescriptorType=%s bDescriptorSubtype=%s\n\
bFormatType=%d bNrChannels=%d bSubFrameSize=%d\n\
bBitResolution=%d bSamFreqType=%d\n",
	       d->bLength, descTypeName(d->bDescriptorType), 
	       asSubTypeName(d->bDescriptorSubtype),
	       d->bFormatType, d->bNrChannels, d->bSubFrameSize,
	       d->bBitResolution, d->bSamFreqType);
	p = d->tSamFreq;
#define GETSAMP(f,p) f = p[0] | (p[1] << 8) | (p[2] << 16), p+=3
	if (d->bSamFreqType == 0) {
		GETSAMP(f, p);
		printf("tSampLo=%d\n", f);
		GETSAMP(f, p);
		printf("tSampHi=%d\n", f);
	} else {
		for (i = 0; i < d->bSamFreqType; i++) {
			GETSAMP(f, p);
			printf("tSamFreq[%d]=%d\n", i, f);
		}
	}
}

void
pratd(struct usb_audio_descriptor *d)
{
	struct usb_audio_input_terminal *it;
	struct usb_audio_output_terminal *ot;
	struct usb_audio_feature_unit *fu;
	struct usb_audio_mixer_unit *mu;
	struct usb_audio_extension_unit *eu;
	char msg[1024];

	sprintf(msg, "\
bLength=%d bDescriptorType=%s bDescriptorSubtype=%d\n",
	       d->bLength, descTypeName(d->bDescriptorType), d->bDescriptorSubtype);
	switch (d->bDescriptorSubtype) {
	case UDESCSUB_AC_INPUT:
		it = (void *)d;
		printf("Input terminal descriptor\n%s", msg);
		printf("\
bTerminalId=%d wTerminalType=%d bAssocTerminal=%d\n\
bNrChannels=%d wChannelConfig=%04x\n\
iChannelNames=%d iTerminal=%d\n",
		       it->bTerminalId, UGETW(it->wTerminalType),
		       it->bAssocTerminal, it->bNrChannels, 
		       UGETW(it->wChannelConfig), it->iChannelNames,
		       it->iTerminal);
		break;
	case UDESCSUB_AC_OUTPUT:
		ot = (void *)d;
		printf("Output terminal descriptor\n%s", msg);
		printf("\
bTerminalId=%d wTerminalType=%d bAssocTerminal=%d\n\
bSourceId=%d iTerminal=%d\n",
		       ot->bTerminalId, UGETW(ot->wTerminalType),
		       ot->bAssocTerminal,
		       ot->bSourceId, ot->iTerminal);
		break;
	case UDESCSUB_AC_MIXER:
		mu = (void *)d;
		printf("Mixer unit descriptor\n%s", msg);
		printf("bUnitId=%d bNrInPins=%d\n",
		       mu->bUnitId, mu->bNrInPins);
		{
			u_char *src = mu->baSourceID;
			int i;
			printf("baSourceID=");
			for (i = 0; i < mu->bNrInPins; i++)
				printf(" %d", src[i]);
			printf("\n");
		}
		break;
	case UDESCSUB_AC_FEATURE:
		fu = (void *)d;
		printf("Feature unit descriptor\n%s", msg);
		printf("bUnitId=%d bSourceId=%d bControlSize=%d\n",
		       fu->bUnitId, fu->bSourceId, fu->bControlSize);
		{
			u_char *ctl = fu->bmaControls;
			int i, j, s;
			s = (fu->bLength - 6) / fu->bControlSize;
			for (i = 0; i < s; i++) {
				printf("bmaControls[%d]=", i);
				for (j = 0; j < fu->bControlSize; j++)
					printf("%02x", ctl[fu->bControlSize-j-1]);
				ctl += fu->bControlSize;
				printf("\n");
			}
		}
		break;
	case UDESCSUB_AC_EXTENSION:
		eu = (void *)d;
		printf("Extension unit descriptor\n%s", msg);
		printf("bUnitId=%d bNrInPins=%d wExtensionCode=%d\n",
		       eu->bUnitId, eu->bNrInPins, UGETW(eu->wExtensionCode));
		{
			u_char *src = eu->baSourceID;
			int i;
			printf("baSourceID=");
			for (i = 0; i < eu->bNrInPins; i++)
				printf(" %d", src[i]);
			printf("\n");
		}
		break;
	default:
		printf("Descriptor\n%s   ...\n", msg);
		break;
	}
}

int globf, globaddr;

void *
prdesc(void *p, int *class, int *subclass, int *iface, int conf)
{
	usb_descriptor_t *d = p;
	struct usb_audio_descriptor *ad;
	usb_interface_descriptor_t *id;
	
	switch (d->bDescriptorType) {
	case UDESC_DEVICE:
		printf("DEVICE descriptor:\n");
		prdevd(p);
		break;
	case UDESC_CONFIG:
		printf("CONFIGURATION descriptor:\n");
		prconfd(p);
		*iface = -1;
		break;
	case UDESC_INTERFACE:
		printf("INTERFACE descriptor %d:\n", ++*iface);
		prifcd(p);
		id = p;
		if (id->bInterfaceClass != 0) {
			*class = id->bInterfaceClass;
			*subclass = id->bInterfaceSubClass;
		}
		break;
	case UDESC_ENDPOINT:
		printf("ENDPOINT descriptor:\n");
		prendpd(p);
		break;
#if 0
	case UDESC_HUB:
		prhubd(p);
		break;
#endif
	case UDESC_CS_DEVICE:
		if (*class == UICLASS_HID) {
			usb_hid_descriptor_t *hid = p;
			int k;
			
			printf("HID descriptor:\n");
			prhidd(p);
			printf("\n");
			for(k = 0; k < hid->bNumDescriptors; k++) {
				int type, len;
				u_char buf[256];

				type = hid->descrs[k].bDescriptorType;
				len = UGETW(hid->descrs[k].wDescriptorLength);
				if (type == UDESC_REPORT) {
					getreportdesc(globf, *iface, k, buf, len, globaddr);
					printf("Report descriptor\n");
					prreportd(buf, len);
				} else if (type == UDESC_PHYSICAL) {
					printf("Physical descriptor ...\n");
				} else {
					printf("Unknown HID descriptor type %d\n", type);
				}
				printf("\n");
			}
		} else
			goto def;
		break;
	case UDESC_CS_INTERFACE:
		ad = p;
		if (*class == UICLASS_AUDIO) {
			if (*subclass == UISUBCLASS_AUDIOCONTROL) {
				switch (ad->bDescriptorSubtype) {
				case UDESCSUB_AC_HEADER:
					printf("AC interface descriptor\n");
					pracdesc(p);
					break;
				case UDESCSUB_AC_INPUT:
				case UDESCSUB_AC_OUTPUT:
				case UDESCSUB_AC_FEATURE:
				case UDESCSUB_AC_MIXER:
				case UDESCSUB_AC_EXTENSION:
					printf("AC unit descriptor\n");
					pratd(p);
					break;
				default:
					goto def;
				}
			} else if (*subclass == UISUBCLASS_AUDIOSTREAM) {
				switch (ad->bDescriptorSubtype) {
				case UDESCSUB_AS_GENERAL:
					prasigd(p);
					break;
				case UDESCSUB_AS_FORMAT_TYPE:
					prast1d(p);
					break;
				default:
					goto def;
				}
			} else
				goto def;
		} else if (*class == UICLASS_CDC) {
			switch (ad->bDescriptorSubtype) {
			case UDESCSUB_CDC_HEADER:
			case UDESCSUB_CDC_CM:
			case UDESCSUB_CDC_ACM:
			case UDESCSUB_CDC_UNION:
				printf("CDC INTERFACE descriptor:\n");
				prcdcd(p);
				break;
			default:
				goto def;
			}
		} else
			goto def;
		break;
	case UDESC_CS_ENDPOINT:
		ad = p;
		if (*class == UICLASS_AUDIO) {
			if (*subclass == UISUBCLASS_AUDIOCONTROL) {
				printf("CONTROL %d\n", ad->bDescriptorSubtype);
				goto def;
			} else if (*subclass == UISUBCLASS_AUDIOSTREAM) {
				switch (ad->bDescriptorSubtype) {
				case UDESCSUB_AS_GENERAL:
					prasiepd(p);
					break;
				default:
					goto def;
				}
			} else
				goto def;
		} else
			goto def;
		break;
	default:
	def:
		printf("Unknown descriptor (class %d/%d):\n", *class, *subclass);
		printf("bLength=%d bDescriptorType=%d bDescriptorSubtype=%d ...\n", d->bLength, 
		       d->bDescriptorType, d->bDescriptorSubtype
		       );
		break;
	}
	return (u_char *)p + d->bLength;
}
	

int
main(int argc, char **argv)
{
	int f, r, i;
	char *dev = USBDEV;
	usb_device_descriptor_t dd;
#define USB_CONFIGSPACE 1024
	struct usb_getconfigdesc {
		usb_config_descriptor_t ucd;
		u_char	filler[USB_CONFIGSPACE];
	} cd;
	u_char *p, *enddata;
	usb_hub_descriptor_t hd;
	usb_port_status_t ps;
	usb_hub_status_t hs;
	int ch;
	extern char *optarg;
	extern int optind;
	int disconly = 0, nodisc = 0;
	struct usb_device_info di;
	int addr;
	int doaddr = -1, si = -1;
	u_int8_t cconf;
	int iface;

	while ((ch = getopt(argc, argv, "a:f:dmns:")) != -1) {
		switch(ch) {
		case 'a':
			nodisc = 1;
			doaddr = atoi(optarg);
			break;
		case 'f':
			dev = optarg;
			break;
		case 'd':
			disconly = 1;
			break;
		case 'n':
			nodisc = 1;
			break;
		case 'm':
			num = 1;
			break;
		case 's':
			si = atoi(optarg);
			break;
		case '?':
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	f = open(dev, O_RDWR);
	if (f < 0)
		err(1, "%s", dev);
	globf = f;

	if (doaddr > 0 && si >= 0) {
		char buf[128];
		setupstrings(f, doaddr);
		getstring(si, buf);
		printf("string %d = '%s'\n", si, buf);
		exit(0);
	}

	if (!doaddr)
		prunits(f);
	if (!nodisc) {
		r = ioctl(f, USB_DISCOVER);
		if (r < 0)
			err(1, "USB_DISCOVER");
		prunits(f);
		if (disconly)
			exit(0);
	}

	for(addr = 0; addr < USB_MAX_DEVICES; addr++) {
		if (doaddr != -1 && addr != doaddr)
			continue;
		di.udi_addr = addr;
		r = ioctl(f, USB_DEVICEINFO, &di);
		if (r)
			continue;

		globaddr = addr;
		printf("DEVICE addr %d\n", addr);
		setupstrings(f, addr);
		getdevicedesc(f, &dd, addr);
		printf("DEVICE descriptor:\n");
		prdevd(&dd);
		printf("\n");
		/*getdevicestatus(f, &status, addr);
		 printf("Device status %04x\n", status);*/

		for(i = 0; i < dd.bNumConfigurations; i++) {
			int class, subclass;
			getconfigdesc(f, i, &cd.ucd, sizeof cd, addr);
			printf("CONFIGURATION descriptor %d:\n", i);
			prconfd(&cd.ucd);
			printf("\n");
			p = (char *)&cd + cd.ucd.bLength;
			enddata = (char *)&cd + UGETW(cd.ucd.wTotalLength);

			class = dd.bDeviceClass;
			subclass = dd.bDeviceSubClass;

			iface = -1;
			while (p < enddata) {
				p = prdesc(p, &class, &subclass, &iface, i);
				printf("\n");
			}

		}
		getconfiguration(f, &cconf, addr);
		printf("current configuration %d\n\n", cconf);
#if 1
		if (dd.bDeviceClass == UICLASS_HUB) {
			printf("HUB descriptor:\n");
			gethubdesc(f, &hd, addr);
			prhubd(&hd);
			printf("\n");
			gethubstatus(f, &hs, addr);
			printf("Hub status %04x %04x\n\n",
			       UGETW(hs.wHubStatus), UGETW(hs.wHubChange));
			for(i = 1; i <= hd.bNbrPorts; i++) {
				getportstatus(f, i, &ps, addr);
				printf("Port %d status=%04x change=%04x\n\n", i,
				       UGETW(ps.wPortStatus), UGETW(ps.wPortChange));
			}
		}
#endif
		printf("----------\n");
	}
	exit(0);
}
