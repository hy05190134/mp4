//
//  netutil.h
//  
//
//  Created by oneoo on 13-9-13.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <errno.h>
#include <time.h>
#include <poll.h>

#ifndef _netutil_h
#define _netutil_h

int
netutil_tcp_connect(const char *addr, int port);
int
netutil_tcp_send(int fd, const char *buf, int size);
int
netutil_tcp_read(int fd, char **pbuf, int *pos, time_t s, int *is_timeout);
char *
netutil_get_remote_server_data(char *server, int port,
                               const char *request, char **pptr,
                               int *pbody_len, int *err);

#endif
