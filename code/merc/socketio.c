#include "merc.h"
#include <sys/time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include "telnet.h"
#include "socketio.h"


/** imports */
extern int gettimeofday(struct timeval *tp, struct timezone *tzp);
extern int close(int fd);


/** exports */


/** locals */


void disconnect(int descriptor)
{
    close(descriptor);
}

void deafen_port(int listen_control)
{
    close(listen_control);
}

void init_descriptor(int control, handle_new_connection *new_connection_handler)
{
    struct sockaddr_in sock;
    struct hostent *from;
    int descriptor;
    socklen_t size;

    size = sizeof(sock);
    getsockname(control, (struct sockaddr *)&sock, &size);
    if ((descriptor = accept(control, (struct sockaddr *)&sock, &size)) < 0) {
	perror("New_descriptor: accept");
	return;
    }

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

    if (fcntl(descriptor, F_SETFL, FNDELAY) == -1) {
	perror("descriptor_new: fcntl: FNDELAY");
	return;
    }


    size = sizeof(sock);
    if (getpeername(descriptor, (struct sockaddr *)&sock, &size) < 0) {
	perror("descriptor_new: getpeername");
	new_connection_handler(descriptor, 0, "unknown"); 
    } else {
	/*
	 * Would be nice to use inet_ntoa here but it takes a struct arg,
	 * which ain't very compatible between gcc and system libraries.
	 */
	int addr;

	addr = ntohl(sock.sin_addr.s_addr);
	from = gethostbyaddr((char *)&sock.sin_addr, sizeof(sock.sin_addr), AF_INET);
	new_connection_handler(descriptor, addr, from->h_name); 
    }

}

int listen_on_port(int port)
{
    static struct sockaddr_in sa_zero;
    struct sockaddr_in sa;
    int x = 1;
    int fd;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("Init_socket: socket");
	raise(SIGABRT);
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&x, sizeof(x)) < 0) {
	perror("Init_socket: SO_REUSEADDR");
	disconnect(fd);
	raise(SIGABRT);
    }

#if defined(SO_DONTLINGER) && !defined(SYSV)
    {
	struct linger ld;

	ld.l_onoff = 1;
	ld.l_linger = 1000;

	if (setsockopt(fd, SOL_SOCKET, SO_DONTLINGER, (char *)&ld, sizeof(ld)) < 0) {
	    perror("Init_socket: SO_DONTLINGER");
	    disconnect(fd);
	    raise(SIGABRT);
	}
    }
#endif

    sa = sa_zero;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
	perror("Init socket: bind");
	disconnect(fd);
	raise(SIGABRT);
    }


    if (listen(fd, 3) < 0) {
	perror("Init socket: listen");
	disconnect(fd);
	raise(SIGABRT);
    }

    return fd;
}

int read_from_descriptor(int descriptor, char *inbuf)
{
    int iStart;

    /* Check for overflow. */
    iStart = (int)strlen(inbuf);

    if (iStart >= (int)(sizeof(inbuf) - 10)) {
	return DESC_READ_RESULT_OVERFLOW;
    }

    for (;;) {
	int nRead;

	nRead = read(descriptor, inbuf + iStart, sizeof(inbuf) - 10 - iStart);

	if (nRead > 0) {
	    iStart += nRead;
	    if (inbuf[iStart - 1] == '\n' || inbuf[iStart - 1] == '\r')
		break;
	} else if (nRead == 0) {
	    return DESC_READ_RESULT_EOF;
	} else if (errno == EWOULDBLOCK) {
	    break;
	} else {
	    perror("read_from_descriptor");
	    return DESC_READ_RESULT_IOERROR;
	}
    }

    inbuf[iStart] = '\0';
    return DESC_READ_RESULT_OK;
}

/**
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks(like 'ofind all'),
 *   try lowering the max block size.
 */
bool write_to_descriptor(int descriptor, char *txt, int length)
{
    int iStart;
    int nWrite;
    int nBlock;


    if (length <= 0)
	length = (int)strlen(txt);

    for (iStart = 0; iStart < length; iStart += nWrite) {
	nBlock = UMIN(length - iStart, 8192);
	if ((nWrite = (int)write(descriptor, txt + iStart, (size_t)nBlock)) < 0) {
	    perror("Write_to_descriptor");
	    return false;
	}
    }

    return true;
}
