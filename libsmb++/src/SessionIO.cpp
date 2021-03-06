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
    Copyright (C) 1999  Nicolas Brodu
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
#include "defines.h"
#include "MAMEoX.h"
//#ifndef USE_SAMBA

// Network related includes
// I haven't checked for portability yet
#include <sys/types.h>   // must be included before in.h on FreeBSD
#ifdef _WIN32
#include <time.h>
#ifdef _XBOX
#include <xtl.h>
#undef errno
#else
#include <winsock2.h>
#endif
#else
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>	// memcpy again...

#include "SessionIO.h"
#include "NMBIO.h"

#ifdef DEBUG
#include <stdio.h>
#include <iostream.h>
#endif

SessionIO::SessionIO(const char *ourName) : NMBIO(ourName)
{
	sock=0;
	port=0;
	hostNBName=0;
	hostName=0;
}

SessionIO::~SessionIO()
{
	if (hostNBName) delete [] hostNBName;
	if (hostName) delete [] hostName;
}


// Connects to host:port and return -1 on error
int SessionIO::connect(const char *hostname, uint16 p)
{
	NBHostEnt *hostList=gethostbyname(hostname);

	if (hostList==0)
	{
		errno=SESSION_ERROR_CALLED_NOT_PRESENT;
#if DEBUG>=1
		cout<<"Host "<<hostname<<" not found !\n";
#endif
		return -1;
	}
	
	if (hostNBName) delete [] hostNBName;
	hostNBName=NULL;
	hostNBName=new char[strlen(hostList->NBName)+1];
	strcpy(hostNBName,hostList->NBName);
	if (hostName) delete [] hostName;
	hostName=NULL;
	hostName=new char[strlen(hostList->name)+1];
	strcpy(hostName,hostList->name);
  PRINTMSG(( T_INFO, hostName ));

	int ret=SessionIO::connect(hostList->ip, p);
	delete hostList;
	hostList=NULL;
	return ret;
}


// Connects to IP:port and return -1 on error
int SessionIO::connect(uint32 IP, uint16 p)
{
	sockaddr_in local;	// local parameters of the connection
	
	port=p;
	
#ifdef _WIN32
	if (sock) closesocket(sock);	// There was already a connection
#else
	if (sock) close(sock);	// There was already a connection
#endif

	if ((sock = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	{
		sock=0;
		errno=SESSION_ERROR;
		return -1;
	}

    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    uint32 bigIP=BIGEND32(IP);
	  memcpy(&local.sin_addr, &bigIP, sizeof(IP));

    if (::connect(sock, (sockaddr*)&local, sizeof(local)) < 0)
	{
		sock=0;
		errno=SESSION_ERROR;
		return -1;
	}
	
	int toggle=1;
	// David: the (void*) cast is required for Solaris
#ifdef _WIN32
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&toggle, sizeof(int));
#else
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (void*)&toggle, sizeof(int));
#endif
/*	discardsock = socket (AF_INET, SOCK_STREAM, 0);
    ::connect(discardsock, (sockaddr*)&local, sizeof(local));
	fcntl(discardsock,F_SETFL,O_NONBLOCK|fcntl(discardsock,F_GETFL,0));
*/
	return 1;
}


int SessionIO::send(SessionPacket *p)
{
	if (!p)
	{
		PRINTMSG(( T_INFO, "SessionIO::send() NULL pointer" ));
		errno=SESSION_ERROR_NOT_ENOUGH_RESOURCES;
		return -1;
	}

	uint8 *packet=p->packet();
	if (!packet)
	{	
		PRINTMSG(( T_INFO, "SessionIO::send() packet=NULL\n" ));
		errno=SESSION_ERROR_NOT_ENOUGH_RESOURCES;
		return -1;
	}
	int ret=p->getLength();
#ifdef _WIN32
	if (::send(sock,(char*)packet,p->getLength(),0) < 0)
#else
	if (write(sock,packet,p->getLength()) < 0)
#endif
	{	
		char szTmp[128];
		sprintf(szTmp,"SessionIO::send() failed %i\n",WSAGetLastError());
		PRINTMSG(( T_INFO, szTmp ));
		errno=SESSION_ERROR;
		ret=-1;
	}
	delete packet;
	return ret;
}

SessionPacket *SessionIO::receive()
{
	SessionPacket *ret=0;
	uint8 type;
//	uint32 queue=0;
//	uint8 *buf=0;
	struct timeval tv;
	fd_set rfds;	// from man select...
	FD_ZERO(&rfds);
	FD_SET(sock,&rfds);
	tv.tv_sec = 2;	//wait max 2 sec for server
	tv.tv_usec = 0;
	// timeout=>exit loop
	if (!select(sock+1, &rfds, 0, 0, &tv)) 
	{
		PRINTMSG(( T_INFO, "SessionIO::receive select() failed" ));
		errno=SESSION_ERROR_CALLED_NOT_PRESENT;
		return 0;
	}
#ifdef _WIN32
	::recv(sock,(char *)&type,1,0);
#else
	read(sock,&type,1);
#endif
	if ((type!=SESSION_MESSAGE) && (type!=POSITIVE_SESSION_RESPONSE)
		&& (type!=NEGATIVE_SESSION_RESPONSE) && (type!=RETARGET_SESSION_RESPONSE)
		&& (type!=SESSION_KEEP_ALIVE) && (type!=SESSION_REQUEST))
	{
		PRINTMSG(( T_INFO, "SessionIO::receive Unknown NetBIOS packet type" ));
		errno=SESSION_ERROR;
		return 0;
	}
	uint8 lengthField[3];
	FD_ZERO(&rfds);
	FD_SET(sock,&rfds);
	tv.tv_sec = 2;	//wait 2 sec
	tv.tv_usec = 0;
	// timeout=>exit loop
	if (!select(sock+1, &rfds, 0, 0, &tv)) 
	{
		PRINTMSG(( T_INFO, "SessionIO::receive select() #2 failed" ));
		errno=SESSION_ERROR; 
		return 0;
	}
#ifdef _WIN32
	if (recv(sock,(char*)lengthField,3,0)<0) 
	{
		PRINTMSG(( T_INFO, "SessionIO::receive recv() failed" ));
		errno=SESSION_ERROR; 
		return 0;
	}
#else
	if (read(sock,lengthField,3)<0) 
	{
		PRINTMSG(( T_INFO, "SessionIO::receive read() failed" ));
		errno=SESSION_ERROR; 
		return 0;
	}
#endif
	int32 length=((uint32)lengthField[0])&1
			| ((uint32)lengthField[1])<<8
			| ((uint32)lengthField[2]);
	
	int32 alreadyRead=0, numRead=0;
	if (length>0)
	{
		uint8* data = new uint8[length];
		while (alreadyRead<length) {
			FD_ZERO(&rfds);
			FD_SET(sock,&rfds);
			tv.tv_sec = 2;	//wait 2 sec
			tv.tv_usec = 0;
			// timeout=>exit loop
			if (!select(sock+1, &rfds, 0, 0, &tv)) 
			{
				PRINTMSG(( T_INFO, "SessionIO::receive select()#3 failed" ));
				errno=SESSION_ERROR;
				delete data;
				return 0;
			}
#ifdef _WIN32
			if ((numRead=recv(sock,(char*)(data+alreadyRead),length,0))>0)
#else
			if ((numRead=read(sock,data+alreadyRead,length))>0)
#endif
				alreadyRead+=numRead;
			else 
			{
				PRINTMSG(( T_INFO, "SessionIO::receive recv()#2 failed" ));
				errno=SESSION_ERROR;
				delete data;
				return 0;
			}
		}
		ret = new SessionPacket(type,data,length);
		delete data;
	}
	else ret = new SessionPacket(type,0,0);
	
	return ret;
}


// Returns -1 and a NetBIOS Session error code in errno on error
int SessionIO::openSession(const char *hostname)
{
	
	if (connect(hostname, 139)==-1)
	{
		errno=SESSION_ERROR_CALLED_NOT_LISTENING;
		return -1; // and even maybe not present
	}
	
	SessionRequestPacket *req=new SessionRequestPacket(hostNBName, ourNBName);
	if (send(req)==-1) {delete req; return -1;} // errno already set
	SessionPacket *retp=receive();
	if (!retp) {delete req; return -1;} // errno already set
	
	// create with dummy numbers
	RetargetSessionResponsePacket *retarget=new RetargetSessionResponsePacket(0,0);
	while ((uint8)(retp->getType())==(uint8)RETARGET_SESSION_RESPONSE)
	{
		// parse received answer
		retarget->parse(retp);
		if (connect(retarget->getIP(), retarget->getPort())==-1)
		{
			delete req;
			delete retp;
			delete retarget;
			return -1; // errno already set
		}
		send(req);
		delete retp;
		retp=receive();
		if (!retp)
		{
			delete req;
			delete retarget;
			return -1; // errno already set
		}
	}
	if (retarget) delete retarget;
	if (req) delete req;
	
	if ((uint8)(retp->getType())!=(uint8)POSITIVE_SESSION_RESPONSE)
	{
		errno=SESSION_ERROR;
		delete retp;
		return -1;
	}
	
	delete retp;
	return 1;	// Gooooooooooooood !!!!!!!! %~}
}

void SessionIO::closeSession()
{
	// There doesn't seem to be anything in the protocol
	// to tell the called host that we leave
#ifdef _WIN32
	if (sock) closesocket(sock);
#else
	if (sock) close(sock);
#endif
	sock=0;
	if (hostNBName) {delete [] hostNBName; hostNBName=0;}
	if (hostName) {delete [] hostName; hostName=0;}
}

//#endif

#pragma code_seg()
#pragma data_seg()
#pragma bss_seg()
#pragma const_seg()

