#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client_handler.h"
#include "common.h"
#include "messages.h"

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

void *client_listen(void *client_p) {
  struct Client c = *(struct Client *)client_p;
  int i = c.thread_no;
  struct Message m;

  // listen for new messages and send them down to the client
  while (true) {
    pthread_mutex_lock(&waiting_messages_mutexes[i]);
    if (message_queue_is_empty(waiting_messages[i])) {
      pthread_cond_wait(&waiting_messages_conds[i], &waiting_messages_mutexes[i]);
    }
    message_queue_pop(&waiting_messages[i], &m);
    pthread_mutex_unlock(&waiting_messages_mutexes[i]);

    char *message_json = message_serialize(m);

    if (send(c.sock, message_json, strlen(message_json), 0) == -1) {
      perror("send()");
    }
    free(message_json);
  }
  return NULL;
}

void client_handle(struct Client c) {
  // create printable client IP address in addr_p
  char addr_p[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(c.addr.sin_addr), addr_p, INET_ADDRSTRLEN);

  printf("handling client connection (%s:%d)\n", addr_p, c.addr.sin_port);

  pthread_t client_listener;
  pthread_create(&client_listener, NULL, client_listen, &c);

  int received;
  char buf[BUFSIZE];

  if ((received = recv(c.sock, buf, BUFSIZE, 0)) == -1) {
    perror("recv()");
    return;
  }

  // wait for messages from the client and put them in the message
  // queue of all threads
  while (received > 0) {
    struct Message m;
    bool deserialize_result = message_deserialize(buf, BUFSIZE, &m);
    m.from_addr = c.addr;
    m.from_addr_len = c.addr_len;

    for (size_t i = 0; deserialize_result && i < MAX_JOINED_CLIENTS; ++i) {
      pthread_mutex_lock(&waiting_messages_mutexes[i]);

      if (message_queue_push(&waiting_messages[i], m)) {
        pthread_cond_signal(&waiting_messages_conds[i]);
      } else {
        fprintf(stderr, "message queue full.\n");
        return;
      }
      pthread_mutex_unlock(&waiting_messages_mutexes[i]);
    }

    if ((received = recv(c.sock, buf, BUFSIZE, 0)) == -1) {
      perror("recv()");
      return;
    }
  }


  printf("closing client connection (%s:%d)\n", addr_p, c.addr.sin_port);
  close(c.sock);
  return;
}
