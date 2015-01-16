//
//  netutil.c
//
//
//  Created by oneoo on 13-9-13.
//
//

#ifdef __cplusplus
extern "C"
{
#endif

#include "netutil.h"

static int
netutil_tcp_wait_ready(int fd, const struct timeval *timeout)
{
    struct pollfd wfd[1];
    long msec;

    msec = -1;
    wfd[0].fd = fd;
    wfd[0].events = POLLOUT;

    /* Only use timeout when not NULL */
    if(timeout != NULL) {
        if(timeout->tv_usec > 1000000) {
            close(fd);
            return -1;
        }

        msec = (timeout->tv_sec * 1000) + ((timeout->tv_usec + 999) / 1000);

        if(msec < 0) {
            msec = 3;
        }
    }

    if(errno == EINPROGRESS) {
        int res;

        if((res = poll(wfd, 1, msec)) == -1) {
            close(fd);
            return -1;

        } else if(res == 0) {
            errno = ETIMEDOUT;
            close(fd);
            return -1;
        }

        int err = 0;
        socklen_t errlen = sizeof(err);

        if(getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen) == -1) {
            close(fd);
            return -1;
        }

        if(err) {
            errno = err;
            close(fd);
            return -1;
        }

        return fd;
    }

    close(fd);
    return -1;
}

/* begin of network functions */
int
netutil_tcp_connect(const char *addr, int port)
{
    int s = -1, flags;
    struct sockaddr_in sa;

    if((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return -1;
    }

    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);

    if(inet_aton(addr, &sa.sin_addr) == 0) {
        struct hostent *he;

        if((he = gethostbyname(addr)) == NULL || he->h_addr == NULL) {
            close(s);
            return -1;
        }

        memcpy(&sa.sin_addr, he->h_addr, sizeof(struct in_addr));
    }

    if((flags = fcntl(s, F_GETFL)) == -1) {
        close(s);
        return -1;
    }

    if(fcntl(s, F_SETFL, flags | O_NONBLOCK) == -1) {
        close(s);
        return -1;
    }

    if(connect(s, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
        struct timeval tv;
        tv.tv_sec = 3;
        tv.tv_usec = 0;
        return netutil_tcp_wait_ready(s, &tv);
    }

    return s;
}


int
netutil_tcp_send(int fd, const char *buf, int size)
{
    if(size <= 0 || buf == NULL) {
        return 1;
    }

    int nwrite = 0, sent = 0;
    time_t s = time(NULL);

    while(sent != size) {
        nwrite = send(fd, buf + sent, size - sent, MSG_DONTWAIT);

        if(nwrite == -1) {
            if(errno == EAGAIN || errno == EINTR) {
                if(time(NULL) - s > 3) {
                    return 0;
                }

                usleep(5000);
                continue;

            } else {
                return 0;
            }
        }

        sent += nwrite;
    }

    return 1;
}

int
netutil_tcp_read(int fd, char **pbuf, int *pos, time_t s, int *is_timeout)
{
    char buf[4096];
    int nread = 0;

    while(1) {
        memset(buf, 0, sizeof(buf));
        nread = read(fd, buf, 4096);

        if(nread == -1) {
            if(errno == EAGAIN || errno == EINTR) {
                if(time(NULL) - s > 3) {
                    *is_timeout = 1;
                    return 0;
                }

                usleep(5000);
                break;

            } else {
                return 0;
            }

        } else if(nread == 0) {
            /* client closed */
            return 0;
        }

        if(*pbuf == NULL) {
            *pbuf = (char *) malloc(nread + 1);

        } else {
            *pbuf = (char *) realloc(*pbuf, *pos + nread + 1);
        }

        memcpy(*pbuf + *pos, buf, nread);
        *pos += nread;
        (*pbuf)[*pos] = 0;
    }

    return 1;
}


char *
strnstr(const char *s, const char *find, size_t slen)
{
    char c, sc;
    size_t len;

    if((c = *find++) != '\0') {
        len = strlen(find);

        do {
            do {
                if(slen-- < 1 || (sc = *s++) == '\0') {
                    return (NULL);
                }
            } while(sc != c);

            if(len > slen) {
                return (NULL);
            }
        } while(strncmp(s, find, len) != 0);

        s--;
    }

    return ((char *)s);
}

char *
netutil_get_remote_server_data(char *server, int port,
                               const char *request, char **pptr,
                               int *pbody_len, int *err)
{
    int socketfd = -1;
    (*err) = 0;

    if((socketfd = netutil_tcp_connect((const char *) server, port)) == -1) {
        if((socketfd = netutil_tcp_connect((const char *) server, port)) == -1) {
            if((socketfd = netutil_tcp_connect((const char *) server, port)) == -1) {
                (*err) = 1;
                return NULL;
            }
        }
    }

    char *data = NULL;
    int data_len = 0, body_len = 0;

    if(socketfd != -1) {
        if(netutil_tcp_send(socketfd, request, strlen(request))
           == 1) {
            /* DONE: read request data */

        } else {
            (*err) = 2;
            close(socketfd);

            return NULL;
        }
    }

    char *ptr = NULL;

    time_t s = time(NULL);

    int is_timeout = 0;

    while(netutil_tcp_read(socketfd, &data, &data_len, s, &is_timeout) != 0);

    close(socketfd);

    if(data) {
        int i = 0;

        for(i = 0; i + 3 < data_len; i++) {
            if(data[i] == '\n' && data[i + 2] == '\n') {
                ptr = data + i + 3;
                body_len = data_len - i - 3;
                break;
            }
        }
    }

    if(!ptr || body_len < 1) {
        free(data);
        data = NULL;
        ptr = NULL;
        body_len = 0;

    } else {
        char b[20] = {0};
        sprintf(b, "%d\r\n", body_len);

        if(is_timeout == 1 || !strnstr(data, b, data_len - body_len) || !strnstr(data, " 206 ", data_len - body_len)) {
            free(data);
            data = NULL;
            ptr = NULL;
            body_len = 0;
        }
    }

    (*pptr) = ptr;
    (*pbody_len) = body_len;

    return data;
}
/* end of network functions */

#ifdef __cplusplus
}
#endif
