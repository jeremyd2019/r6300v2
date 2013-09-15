/*  Copyright 1999 Peter Schlaile.
 *  Copyright 1999-2002,2006,2007,2009 Alain Knaff.
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
 *
 * Small install-test utility to check if a floppyd-server is running on the
 * X-Server-Host.
 *
 * written by:
 *
 * Peter Schlaile
 *
 * udbz@rz.uni-karlsruhe.de
 *
 */

#include "sysincludes.h"
#include "stream.h"
#include "mtools.h"
#include "msdos.h"
#include "scsi.h"
#include "partition.h"
#include "floppyd_io.h"

#ifdef USE_FLOPPYD
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

/* ######################################################################## */

typedef unsigned char Byte;
typedef unsigned long Dword;

const char* AuthErrors[] = {
	"Auth success!",
	"Auth failed: Packet oversized!",
	"Auth failed: X-Cookie doesn't match!",
	"Auth failed: Wrong transmission protocol version!",
	"Auth failed: Device locked!"
};

#include "byte_dword.h"
#include "read_dword.h"

static void write_dword(int handle, Dword parm) 
{
	Byte val[4];

	dword2byte(parm, val);

	write(handle, val, 4);
}


/* ######################################################################## */

static int authenticate_to_floppyd(char fullauth, int sock, char *display, 
				   int protoversion)
{
	off_t filelen=0;
	Byte buf[16];
	const char *command[] = { "xauth", "xauth", "extract", "-", 0, 0 };
	char *xcookie = NULL;
	Dword errcode;
	int bytesRead;

	if (fullauth) {
		command[4] = display;

		filelen=strlen(display);
		filelen += 100;

		xcookie = (char *) safe_malloc(filelen+4);
		filelen = safePopenOut(command, xcookie+4, filelen);
		if(filelen < 1)
		    return AUTH_AUTHFAILED;
	}
	dword2byte(4,buf);
	dword2byte(protoversion,buf+4);
	write(sock, buf, 8);

	bytesRead = read_dword(sock);

	if (bytesRead != 4 && bytesRead != 12) {
		return AUTH_WRONGVERSION;
	}


	errcode = read_dword(sock);

	if (errcode != AUTH_SUCCESS) {
		return errcode;
	}


	if(bytesRead == 8) {
	    protoversion = read_dword(sock);
	    read_dword(sock);
	}
	
	fprintf(stderr, "Protocol Version=%d\n", protoversion);

	if (fullauth) {
		dword2byte(filelen, (Byte *) xcookie);
		write(sock, xcookie, filelen+4);

		if (read_dword(sock) != 4) {
			return AUTH_PACKETOVERSIZE;
		}

		errcode = read_dword(sock);
	}

	return errcode;

}


/* ######################################################################## */

static int get_host_and_port(const char* name, char** hostname, char **display,
			     short* port)
{
	char* newname = strdup(name);
	char* p;
	char* p2;

	p = newname;
	while (*p != '/' && *p) p++;
	p2 = p;
	if (*p) p++;
	*p2 = 0;
	
	*port = atoi(p);
	if (*port == 0) {
		*port = FLOPPYD_DEFAULT_PORT;	
	}

	*display = strdup(newname);

	p = newname;
	while (*p != ':' && *p) p++;
	p2 = p;
	if (*p) p++;
	*p2 = 0;

	*port += atoi(p);  /* add display number to the port */

	if (!*newname || strcmp(newname, "unix") == 0) {
		free(newname);
		newname = strdup("localhost");
	}

	*hostname = newname;
	return 1;
}

/*
 *  * Return the IP address of the specified host.
 *  */
static IPaddr_t getipaddress(char *ipaddr)
{
	
	struct hostent  *host;
	IPaddr_t        ip;
	
	if (((ip = inet_addr(ipaddr)) == INADDR_NONE) &&
	    (strcmp(ipaddr, "255.255.255.255") != 0)) {
		
		if ((host = gethostbyname(ipaddr)) != NULL) {
			memcpy(&ip, host->h_addr, sizeof(ip));
		}
		
		endhostent();
	}
	
#ifdef DEBUG
	fprintf(stderr, "IP lookup %s -> 0x%08lx\n", ipaddr, ip);
#endif
	  
	return (ip);
}

/*
 *  * Connect to the floppyd server.
 *  */
static int connect_to_server(IPaddr_t ip, short port)
{
	
	struct sockaddr_in      addr;
	int                     sock;
	
	/*
	 * Allocate a socket.
	 */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		return (-1);
	}
	
	/*
	 * Set the address to connect to.
	 */
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = ip;
	
        /*
	 * Connect our socket to the above address.
	 */
	if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		return (-1);
	}

        /*
	 * Set the keepalive socket option to on.
	 */
	{
		int             on = 1;
		setsockopt(STDIN_FILENO, SOL_SOCKET, SO_KEEPALIVE, 
			   (char *)&on, sizeof(on));

	}
	
	return (sock);
}

int main (int argc, char** argv) 
{
	char* hostname;
	char* display;
	char* name;
	short port;
	int sock;
	int reply;
	int rval;
	int protoversion;
	char fullauth = 0;
	Byte opcode = OP_CLOSE;

        if (argc < 2) {
	       puts("Usage: floppyd_installtest [-f] Connect-String\n"
		    "-f\tDo full X-Cookie-Authentication");
	       return -1;
	}

	name = argv[1];
	if (strcmp(name, "-f") == 0) {
		fullauth = 1;
		name = argv[2];
	}

	rval = get_host_and_port(name, &hostname, &display, &port);
	
	if (!rval) return -1;

	sock = connect_to_server(getipaddress(hostname), port);

	if (sock == -1) {
		fprintf(stderr,
			"Can't connect to floppyd server on %s, port %i!\n",
			hostname, port);
		return -1;
	}
	
	protoversion = FLOPPYD_PROTOCOL_VERSION;
	while(1) {
	    reply = authenticate_to_floppyd(fullauth, sock, display,
					    protoversion);
	    if(protoversion == FLOPPYD_PROTOCOL_VERSION_OLD)
		break;
	    if(reply == AUTH_WRONGVERSION) {
		/* fall back on old version */
		protoversion = FLOPPYD_PROTOCOL_VERSION_OLD;
		continue;
	    }
	    break;
	}

	if (reply != 0) {
		fprintf(stderr, 
			"Connection to floppyd failed:\n"
			"%s\n", AuthErrors[reply]);
		return -1;
	}
	
	free(hostname);
	free(display);

	write_dword(sock, 1);
	write(sock, &opcode, 1);

	close(sock);

	return 0;
}
#endif
