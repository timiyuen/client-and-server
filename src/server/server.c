/*
 * This is free software; see the source for copying conditions.  There is NO
 * warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Written by Ni Baozhu: <nibz@qq.com>.
 */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>

#define MAX_EVENTS (0xff)
#define BUFFER_LENGTH (0xff)

int setnonblocking(int fd);
int reads(int fd);
int writes(int fd);
time_t last = 0;
int quit = 0;

int fds[MAX_EVENTS] = {0};
int naccept = 0;
int do_use_fd();
int del_fd(int fd);

int main(int argc, char **argv)
{
	printf("%s:%d:%s\n", __FILE__, __LINE__, __func__);
	int retval = 0;
	do {
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0)
		{
			printf("%s\n", strerror(errno));
			break;
		}

		printf("fd = %d\n", fd);

		struct sockaddr_in addr;
		memset(&addr, 0, sizeof (struct sockaddr_in));

		addr.sin_family = AF_INET;
		addr.sin_port = htons(atoi(argv[2]));
		retval = inet_pton(AF_INET, argv[1], (struct sockaddr *) &addr.sin_addr.s_addr);
		if (retval != 1)
		{
			printf("%s\n", strerror(errno));
			break;
		}

		socklen_t addrlen = sizeof (struct sockaddr_in);
		retval = bind(fd, (struct sockaddr *) &addr, addrlen);
		if (retval < 0)
		{
			printf("%s\n", strerror(errno));
			break;
		}

		int backlog = 5; //may be wrong
		retval = listen(fd, backlog);
		if (retval < 0)
		{
			printf("%s\n", strerror(errno));
			break;
		}

		int efd = epoll_create(MAX_EVENTS);
		if (efd == -1)
		{
			printf("%s\n", strerror(errno));
			break;
		}

		printf("efd = %d\n", efd);

		struct epoll_event ev, events[MAX_EVENTS];
		ev.events = EPOLLIN | EPOLLET; // epoll edge triggered
		ev.data.fd = fd; //bind & listen's fd
		retval = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &ev);
		if (retval == -1)
		{
			printf("%s\n", strerror(errno));
			break;
		}

		int n = 0;
		struct sockaddr_in peer_addr;
		socklen_t peer_addrlen = sizeof (struct sockaddr_in);
		memset(&peer_addr, 0, sizeof (struct sockaddr_in));

		while (1)
		{
			if (quit)
			{
				close(efd);
				close(fd);
				break;
			}

			int nfds = epoll_wait(efd, events, MAX_EVENTS, 1000);
			if (nfds == -1)
			{
				printf("%s\n", strerror(errno));
				break;
			}

			for (n = 0; n < nfds; n++)
			{
				if (events[n].data.fd == fd)
				{
					int afd = accept(fd, (struct sockaddr *) &peer_addr, &peer_addrlen);
					if (afd == -1)
					{
						printf("%s\n", strerror(errno));
						break;
					}

					printf("accept: afd = %d\n", afd);
					//set non blocking
					retval = setnonblocking(afd);
					if (retval < 0)
					{
						break;
					}

					ev.events = EPOLLIN | EPOLLET; //epoll edge triggered
					//                  ev.events = EPOLLIN | EPOLLOUT | EPOLLET; //epoll edge triggered
					//                  ev.events = EPOLLIN | EPOLLOUT; //epoll level triggered (default)
					ev.data.fd = afd;
					retval = epoll_ctl(efd, EPOLL_CTL_ADD, afd, &ev);
					if (retval == -1)
					{
						printf("%s\n", strerror(errno));
						break;
					}

					fds[naccept++] = afd;
				}
				else
				{
					if (events[n].events & EPOLLHUP)
					{
						printf("events[%d].events = 0x%03x\n", n, events[n].events);
						puts("Hang up happened on the associated file descriptor."); // my message
						retval = epoll_ctl(efd, EPOLL_CTL_DEL, events[n].data.fd, &events[n]);
						if (retval == -1)
						{
							printf("%s\n", strerror(errno));
							break;
						}
						close(events[n].data.fd);
						del_fd(events[n].data.fd);
						continue;
					}

					if (events[n].events & EPOLLERR)
					{
						printf("0x%03x\n", events[n].events);
						puts("Error condition happened on the associated file descriptor.  epoll_wait(2) will always wait for this event; it is not necessary to set it in events."); // my message
						retval = epoll_ctl(efd, EPOLL_CTL_DEL, events[n].data.fd, &events[n]);
						if (retval == -1)
						{
							printf("%s\n", strerror(errno));
							break;
						}
						close(events[n].data.fd);
						continue;
					}

					if (events[n].events & EPOLLIN)
					{
						printf("events[%d].events = 0x%03x\n", n, events[n].events);
						puts("The associated file is available for read(2) operations."); // my message
						retval = reads(events[n].data.fd);
						if (retval < 0)
						{
							break;
						}
					}

					if (events[n].events & EPOLLOUT)
					{
						printf("events[%d].events = 0x%03x\n", n, events[n].events);
						puts("The associated file is available for write(2) operations."); // my message
						retval = writes(events[n].data.fd);
						if (retval < 0)
						{
							break;
						}
					}
				}
			}

			do_use_fd();

		}
	} while (0);
	return retval;
}

int reads(int fd)
{
	printf("%s:%d:%s\n", __FILE__, __LINE__, __func__);
	int retval = 0;
	do {
		char buffer[BUFFER_LENGTH];
		memset(buffer, 0, sizeof buffer);
		retval = read(fd, buffer, BUFFER_LENGTH);
		if (retval < 0)
		{
			printf("%s\n", strerror(errno));
			break;
		}
		printf("fd = %d, READ [%s]\n", fd, buffer);
	} while (0);
	return retval;
}

int writes(int fd)
{
	printf("%s:%d:%s\n", __FILE__, __LINE__, __func__);
	int retval = 0;
	do {
		char buffer[BUFFER_LENGTH];
		memset(buffer, 0, sizeof buffer);
		sprintf(buffer, "send");
		size_t length = strlen(buffer);
		retval = write(fd, buffer, length);
		if (retval < 0)
		{
			printf("%s\n", strerror(errno));
			break;
		}
		printf("fd = %d, write [%s]\n", fd, buffer);
	} while (0);
	return retval;
}

int setnonblocking(int fd)
{
	printf("%s:%d:%s\n", __FILE__, __LINE__, __func__);
	int retval = 0;
	do {
		int flags = fcntl(fd, F_GETFL);    
		if (flags == -1)
		{
			retval = flags;
			printf("%s\n", strerror(errno));
			break;
		}
		flags |= O_NONBLOCK; //non block
		retval = fcntl(fd, F_SETFL, flags);
		if (retval == -1)
		{
			printf("%s\n", strerror(errno));
			break;
		}
	} while (0);
	return retval;
}

int do_use_fd()
{
	printf("%s:%d:%s\n", __FILE__, __LINE__, __func__);
	int retval = 0;
	do {
		time_t t;
		time(&t);
		if (t - last < 3)
		{
			break;
		}
		last = t;

		int n;
		for (n = 0; n < naccept; n++)
		{
			retval = writes(fds[n]);
			if (retval < 0)
			{
				break;
			}
		}

	} while (0);
	return retval;
}

int del_fd(int fd)
{
	printf("%s:%d:%s\n", __FILE__, __LINE__, __func__);
	int retval = 0;
	do {
		int n;
		for (n = 0; n < naccept; n++)
		{
			if (fds[n] == fd)
			{
				printf("Remove fd = %d\n", fd);
				fds[n] = 0;
				for (; n < naccept - 1; n++)
				{
					fds[n] = fds[n + 1];
				}
				fds[n] = 0;
				naccept--;
			}
		}
	} while (0);
	return retval;
}
