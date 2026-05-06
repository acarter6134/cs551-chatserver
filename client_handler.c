#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client_handler.h"

#define BUFSIZE 4096

struct ClientQueue client_queue_new(size_t cap) {
  // NOTE: capacity is cap + 1 because ring-buffers need a dummy space
  // to distinguish between full and empty states
  return (struct ClientQueue){.clients = calloc(cap + 1, sizeof(struct Client)),
                              .clients_cap = cap + 1,
                              .write_index = 0,
                              .read_index = 0};
}

void client_queue_free(struct ClientQueue q) { free(q.clients); }

bool client_queue_is_empty(struct ClientQueue q) {
  return q.write_index == q.read_index;
}

bool client_queue_is_full(struct ClientQueue q) {
  return (q.write_index + 1) % q.clients_cap == q.read_index;
}

bool client_queue_pop(struct ClientQueue *q, struct Client *c) {
  if (client_queue_is_empty(*q)) {
    return false;
  }
  *c = q->clients[q->read_index];
  q->read_index = (q->read_index + 1) % q->clients_cap;
  return true;
}

bool client_queue_push(struct ClientQueue *q, struct Client c) {
  if (client_queue_is_full(*q)) {
    return false;
  }
  q->clients[q->write_index] = c;
  q->write_index = (q->write_index + 1) % q->clients_cap;
  return true;
}

int client_handle(struct Client client) {
  // create printable client IP address in addr_p
  char addr_p[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(client.addr.sin_addr), addr_p, INET_ADDRSTRLEN);

  printf("handling client connection (%s:%d)\n", addr_p, client.addr.sin_port);

  int received;
  char buf[BUFSIZE];
  char *prefix = "ECHO! ";

  if ((received = recv(client.sock, buf, BUFSIZE, 0)) == -1) {
    perror("recv()");
    return 1;
  }

  while (received > 0) {
    if (send(client.sock, prefix, strlen(prefix), 0) == -1) {
      perror("send()");
      return 1;
    }

    if (send(client.sock, buf, received, 0) == -1) {
      perror("send()");
      return 1;
    }

    if ((received = recv(client.sock, buf, BUFSIZE, 0)) == -1) {
      perror("recv()");
      return 1;
    }
  }

  printf("closing client connection (%s:%d)\n", addr_p, client.addr.sin_port);
  close(client.sock);
  return 0;
}
