/*

 PMSDR command line manager (experimental)

 Control a PMSDR hardware on Linux
 Modified to comply to firmware version 2.1.x

 Copyright (C) 2008,2009,2010  Andrea Montefusco IW0HDV, Martin Pernter IW3AUT

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/


#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <fcntl.h>               /* for nonblocking */
#include        <netdb.h>
#include        <stdlib.h>
#include        <string.h>

#include        "udp_cmd.h" 




static int sockfd;
static struct sockaddr cliaddr;
static socklen_t sa_len = sizeof(cliaddr);


int udpListen (short port)
{
	struct sockaddr_in	servaddr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(port);

	return bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
}

int udpGetNextCommand (char *szBuf, int len)
{
	return recvfrom (sockfd, szBuf, len, 0, &cliaddr, &sa_len);
}

int udpSendAnswer (const char *psz)
{
    return sendto (sockfd, psz, strlen(psz), 0, &cliaddr, sa_len);
}

