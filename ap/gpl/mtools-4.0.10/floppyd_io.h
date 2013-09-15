#ifndef MTOOLS_FLOPPYDIO_H
#define MTOOLS_FLOPPYDIO_H

/*  Copyright 1999 Peter Schlaile.
 *  Copyright 1998,2000-2002,2009 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef USE_FLOPPYD

#include "stream.h"

/*extern int ConnectToFloppyd(const char* name, Class_t** ioclass);*/
Stream_t *FloppydOpen(struct device *dev, struct device *dev2,
					  char *name, int mode, char *errmsg,
					  int mode2, int locked);

#define FLOPPYD_DEFAULT_PORT 5703

#define FLOPPYD_PROTOCOL_VERSION_OLD 10
#define FLOPPYD_PROTOCOL_VERSION 11

#define FLOPPYD_CAP_EXPLICIT_OPEN 1 /* explicit open. Useful for 
				     * clean signalling of readonly disks */
#define FLOPPYD_CAP_LARGE_SEEK 2    /* large seeks */

enum FloppydOpcodes {
	OP_READ,
	OP_WRITE,
	OP_SEEK,
	OP_FLUSH,
	OP_CLOSE,
	OP_IOCTL,
	OP_OPRO,
	OP_OPRW
};

enum AuthErrorsEnum {
	AUTH_SUCCESS,
	AUTH_PACKETOVERSIZE,
	AUTH_AUTHFAILED,
	AUTH_WRONGVERSION,
	AUTH_DEVLOCKED,
	AUTH_BADPACKET
};

typedef unsigned long IPaddr_t;

#endif
#endif
