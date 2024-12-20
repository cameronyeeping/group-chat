#define _DEFAULT_SOURCE
#define _REENTRANT

#include "../include/server.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
// my ip?
#define ADDR "192.168.224.128"

void processMsg(char buf[4][100], char *server_msg, ssize_t output_len);

void convert(uint8_t *buf, char *str, ssize_t size) {
  if (size % 2 == 0)
    size = size / 2 - 1;
  else
    size = size / 2;
  for (int i = 0; i < size; i++)
    sprintf(str + (i * 2), "%02X", buf[i]);
}
static void *send_func(void *arg) {
  int *ptr = (int *)arg;

  int sfd = ptr[0];
  int ttrun = ptr[1];
  uint8_t buf[10];
  char str[10 * 2 + 1];
  char tempBuf[22];
  int i = 0;
  while (i < ttrun) {
    getentropy(buf, 10);
    convert(buf, str, 21);
    sprintf(tempBuf, "0%s\n", str);
    write(sfd, tempBuf, strlen(tempBuf));
    sleep(1);
    i++;
  }
  send(sfd, "1\n", 2, MSG_DONTWAIT);
  return (void *)0;
}
void processMsg(char buf[4][100], char *server_msg, ssize_t output_len) {
  // output, recbuf, strlen(recbuf)
  char *saveptr;
  buf[0][0] = server_msg[0];
  if (server_msg[0] == 1)
    return;

  int i = 0;
  char *token = strtok_r(server_msg, " ", &saveptr);
  while (token != NULL) {
    strcpy(buf[i], token);
    i++;

    token = strtok_r(NULL, " ", &saveptr);
  }
  unsigned long ip = ntohl(atoi(buf[1]));
  uint16_t port = ntohs(atoi(buf[2]));

  struct in_addr ip_addr;
  ip_addr.s_addr = ip;
  strncpy(buf[1], inet_ntoa(ip_addr), strlen(inet_ntoa(ip_addr)));
  sprintf(buf[2], "%hu", port);
}

int main(int argc, char *argv[]) {
  struct sockaddr_in addr;
  int sfd;
  char output[4][BUF_SIZE];
  char recbuf[BUF_SIZE * 2];
  int s;

  int ttrun = atoi(argv[1]);
  pthread_t sender;
  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1)
    handle_error("socket");
  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_port = htons(PORT);
  addr.sin_family = AF_INET;

  if (inet_pton(AF_INET, argv[2], &addr.sin_addr) <= 0)
    handle_error("inet_pton");
  if (connect(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
    handle_error("connect");
  int thread_arg[2] = {sfd, ttrun};
  pthread_create(&sender, NULL, send_func, (void *)thread_arg);

  while (1) {
    s = read(sfd, recbuf, BUF_SIZE * 2);
    if (s > 1) {
      processMsg(output, recbuf, strlen(recbuf));
      if (output[0][0] == '0')
        printf("%-20s%-10s%s", output[1], output[2], output[3]);
      else {
        break;
      }
    }
  }
  pthread_cancel(sender);
  pthread_join(sender, NULL);
  close(sfd);
  exit(EXIT_SUCCESS);
}
