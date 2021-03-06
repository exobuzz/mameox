#pragma code_seg("CSAMBA")
#pragma data_seg("DSAMBA")
#pragma bss_seg("BSAMBA")
#pragma const_seg("KSAMBA")
#pragma comment(linker, "/merge:DSAMBA=SAMBA")
#pragma comment(linker, "/merge:CSAMBA=SAMBA")
#pragma comment(linker, "/merge:BSAMBA=SAMBA")
#pragma comment(linker, "/merge:KSAMBA=SAMBA")
/*
    This file is part of the smb++ library
    Copyright (C) 2000  Nicolas Brodu
    nicolas.brodu@free.fr

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program, see the file COPYING; if not, write
    to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
    MA 02139, USA.
*/

/*
	Implements NameQueryInterface using the native code
*/
#include "defines.h"
#ifndef USE_SAMBA

#include <sys/types.h> // needed on FreeBSD
#ifdef _WIN32
#ifdef _XBOX
#include <xtl.h>
#undef errno
#else
#include <winsock2.h>
#endif
#else
#include <netdb.h>
#include <sys/socket.h>  // for AF_INET
#endif
#include "NativeNMB.h"
#include "NMBIO.h"
#include "strtool.h"

hostent *NativeNMB::gethostbyname(const char *name, bool groupquery)
{
	NBHostEnt *foo = nmbio->gethostbyname(name, groupquery);
	if (!foo) return 0;
	newstrcpy(returnValue.h_name, foo->name);
	memcpy(returnValue.h_addr_list[0], &(foo->ip), 4);
	return &returnValue;
}


NativeNMB::NativeNMB()
{
	nmbio = new NMBIO();
	returnValue.h_name = 0;
	returnValue.h_aliases = 0;
	returnValue.h_addrtype = AF_INET;
	returnValue.h_length = 4;
#ifdef _WIN32
	returnValue.h_addr_list = new char*[2];
#else
	returnValue.h_addr_list = new (char*)[2];
#endif
	returnValue.h_addr_list[0] = new char[4];
	returnValue.h_addr_list[1] = 0;
}

NativeNMB::~NativeNMB()
{
	if (nmbio) delete nmbio;
	if (returnValue.h_name) delete returnValue.h_name;
	if (returnValue.h_addr_list) {
		if (returnValue.h_addr_list[0]) delete returnValue.h_addr_list[0];
		delete returnValue.h_addr_list;
	}
}

int NativeNMB::setNBNSAddress(uint32 addr)
{
	return nmbio->setNBNSAddress(addr);
}

int NativeNMB::setNBNSAddress(const char *addr)
{
	return nmbio->setNBNSAddress(addr);
}


#endif
#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

