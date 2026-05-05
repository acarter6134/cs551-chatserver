#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>

struct Client {
  struct sockaddr_in addr;
  socklen_t addr_len;
  int sock;
};

struct ClientQueue {
  struct Client *clients;
  size_t clients_cap;
  size_t write_index;
  size_t read_index;
};

void client_queue_free(struct ClientQueue q);

struct ClientQueue client_queue_new(size_t);
void client_queue_free(struct ClientQueue);
bool client_queue_is_empty(struct ClientQueue);
bool client_queue_is_full(struct ClientQueue);
bool client_queue_pop(struct ClientQueue *, struct Client *);
bool client_queue_push(struct ClientQueue *, struct Client);
int client_handle(struct Client);

#endif
