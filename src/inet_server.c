#include "../include/queue.h"
#include "../include/server.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Invalid number of arguments\n");
  }

  int max_client = atoi(argv[1]);
  int clientnum = 0;

  struct sockaddr_in addr, remote_addr;
  int sfd, cfd, epollfd;
  int nfds;
  ssize_t num_read;
  socklen_t addrlen = sizeof(struct sockaddr_in);
  char buf[BUF_SIZE];
  struct epoll_event ev, events[MAX_EVENTS];
  int msg_type;
  unsigned long remote_ip;
  uint16_t remote_port;

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1)
    handle_error("socket");

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
    handle_error("bind");

  if (listen(sfd, LISTEN_BACKLOG) == -1)
    handle_error("listen");

  epollfd = epoll_create1(0); // new epoll instance
  if (epollfd == -1)
    handle_error("epoll_create1");

  ev.events = EPOLLIN | EPOLLOUT;
  ev.data.fd = sfd; // save the accept socket
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sfd, &ev) == -1)
    handle_error("epoll_ctl");

  for (;;) {
    nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1); // returns available fd
    if (nfds == -1)
      handle_error("epoll_wait");

    for (int i = 0; i < nfds; ++i) {
      if (events[i].data.fd == sfd) {
        if (clientnum < max_client) {
          memset(&remote_addr, 0, sizeof(struct sockaddr_in));
          cfd = accept(sfd, (struct sockaddr *)&remote_addr,
                       &addrlen); // remote address is client

          if (cfd == -1)
            handle_error("accept");
          clientnum++;
          // Set O_NONBLOCK
          int flags = fcntl(cfd, F_GETFL, 0);
          if (flags == -1)
            handle_error("fcntl");
          flags |= O_NONBLOCK;
          if (fcntl(cfd, F_SETFL, flags) == -1)
            handle_error("fcntl");

          ev.events = EPOLLIN | EPOLLOUT;
          ev.data.fd = cfd;
          // add client to epoll instance
          if (epoll_ctl(epollfd, EPOLL_CTL_ADD, cfd, &ev) == -1) {
            perror("epoll_ctl: conn_sock");
            exit(EXIT_FAILURE);
          }
        } else {
          if (close(sfd) == -1)
            handle_error("close");
        }
      } else {
        // if already connected
        while ((num_read = read(events[i].data.fd, buf, BUF_SIZE)) > 0) {
          if (buf[0] == '0')
            msg_type = 0;
          else
            msg_type = 1;
          if (msg_type == 0) {
            if (getpeername(events[i].data.fd, (struct sockaddr *)&remote_addr,
                            &addrlen) != 0) {
              handle_error("getpeername");
            }

            enqueue(buf);

            remote_ip = ntohl(remote_addr.sin_addr.s_addr);
            remote_port = ntohs(remote_addr.sin_port);

            char format_buf[200];
            snprintf(format_buf, 200, "%d %lu %hu %s\n", msg_type, remote_ip,
                     remote_port, buf);
            while (dequeue(buf) != -1) {
              for (int j = 0; j < nfds; j++) {

                send(events[j].data.fd, format_buf, strlen(format_buf),
                     MSG_DONTWAIT);
              }
              write(STDOUT_FILENO, format_buf, strlen(format_buf));
              memset(format_buf, '\0', strlen(format_buf));
              memset(buf, '\0', strlen(buf));
            }

            if (num_read == -1)
              handle_error("read");
          } else if (msg_type == 1) {
            // terminate
            char str[4];
            clientnum--;
            snprintf(str, 4, "%d\n", clientnum);
            write(STDOUT_FILENO, str, strlen(str));
            if (clientnum == 0) {
              char *endmsg =
                  "Received type 1 from all clients. Terminating now\n";
              write(STDOUT_FILENO, endmsg, strlen(endmsg));
              for (int j = 0; j < nfds; j++) {
                send(events[j].data.fd, "1\n", 2, MSG_DONTWAIT);
              }
              close(sfd);
              close(cfd);
              exit(0);
            }
          }
        }
      }
    }
  }
}
