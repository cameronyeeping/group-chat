#include "../include/queue.h"
#include "../include/server.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

static item msg_buffer[MAX_MESSAGES];
static int tail_index;
static int head_index;
void enqueue(char *msg) {

  pthread_mutex_lock(&mtx);

  if ((tail_index + 1) % MAX_MESSAGES == head_index) {
    head_index = (head_index + 1) % MAX_MESSAGES;
  }

  // msg_buffer[tail_index].str = msg;
  strncpy(msg_buffer[tail_index].str, msg, BUF_SIZE);
  int s = strlen(msg_buffer[tail_index].str);
  msg_buffer[tail_index].str[s] = '\0';
  msg_buffer[tail_index].size = strlen(msg_buffer[tail_index].str);
  tail_index = (tail_index + 1) % MAX_MESSAGES;
  pthread_mutex_unlock(&mtx);
}

int dequeue(char *buf) {

  if (head_index == tail_index)
    return -1;
  pthread_mutex_lock(&mtx);
  strncpy(buf, msg_buffer[head_index].str, BUF_SIZE);
  head_index = (head_index + 1) % MAX_MESSAGES;
  pthread_mutex_unlock(&mtx);

  return strlen(buf);
}
