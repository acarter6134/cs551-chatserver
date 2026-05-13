#ifndef MESSAGES_H
#define MESSAGES_H

#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>

#include "common.h"

struct Message {
  struct sockaddr_in from_addr;
  socklen_t from_addr_len;
  char *from_name;
  char *message;
};

struct MessageQueue {
  struct Message *messages;
  size_t messages_cap;
  size_t write_index;
  size_t read_index;
  bool overwrite;
};

extern pthread_mutex_t waiting_messages_mutexes[];
extern pthread_cond_t waiting_messages_conds[];
extern struct MessageQueue waiting_messages[];

struct MessageQueue message_queue_new(size_t, bool);
void message_queue_free(struct MessageQueue);
bool message_queue_is_empty(struct MessageQueue);
bool message_queue_is_full(struct MessageQueue);
bool message_queue_pop(struct MessageQueue *, struct Message *);
bool message_queue_push(struct MessageQueue *, struct Message);
char *message_serialize(struct Message);
bool message_deserialize(char *, size_t, struct Message *);

bool waiting_messages_init(void);
void waiting_messages_free(void);

#endif
