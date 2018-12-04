PROGS = usbctl usbdebug usbstats usbgen 
CFLAGS = -Wall -s

all:	$(PROGS)

man:	usbgen.8
	nroff -mandoc usbgen.8 > usbgen.0

usbctl:		usbctl.c
	cc $(CFLAGS) usbctl.c -o usbctl

usbdebug:	usbdebug.c
	cc $(CFLAGS) usbdebug.c -o usbdebug

usbstats:	usbstats.c
	cc $(CFLAGS) usbstats.c -o usbstats

usbgen:		usbgen.c
	cc $(CFLAGS) usbgen.c -o usbgen

install: $(PROGS)
	install usbctl usbdebug usbstats usbgen $(PREFIX)/sbin

clean:
	rm -f $(PROGS)
