#ifndef MESSAGES_H
#define MESSAGES_H

#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>

#include "common.h"

struct Message {
  struct sockaddr_in from_addr;
  socklen_t from_addr_len;
  char *message;
};

struct MessageQueue {
  struct Message *messages;
  size_t messages_cap;
  size_t write_index;
  size_t read_index;
  bool overwrite;
};

pthread_mutex_t waiting_messages_mutexes[MAX_JOINED_CLIENTS];
pthread_cond_t waiting_messages_conds[MAX_JOINED_CLIENTS];
struct MessageQueue waiting_messages[MAX_JOINED_CLIENTS];

struct MessageQueue message_queue_new(size_t, bool);
void message_queue_free(struct MessageQueue);
bool message_queue_is_empty(struct MessageQueue);
bool message_queue_is_full(struct MessageQueue);
bool message_queue_pop(struct MessageQueue *, struct Message *);
bool message_queue_push(struct MessageQueue *, struct Message);
bool message_serialize(void **, size_t *, struct Message);
bool message_deserialize(struct Message *, void *, size_t);

bool waiting_messages_init(void);
void waiting_messages_free(void);

#endif
