#ifndef SERVER_H
#define SERVER_H

// contains all macros
#define BUF_SIZE 100
#define PORT 8000
#define LISTEN_BACKLOG 32
#define MAX_EVENTS 10
#define MAX_MESSAGES 25
#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#endif
