#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client.h"

#define PORT 10551

#define MAX_JOINED_CLIENTS 10
#define MAX_WAITING_CLIENTS 10

struct Server {
  struct sockaddr_in addr;
  socklen_t addr_len;
  int sock;
};

struct ClientQueue waiting_clients;

pthread_mutex_t waiting_clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t client_is_waiting = PTHREAD_COND_INITIALIZER;

void *handle_connection(void *_unused) {
  (void)_unused; // silence 'unused parameter' compiler warning

  struct Client c;
  while (true) {
    pthread_mutex_lock(&waiting_clients_mutex);
    if (client_queue_is_empty(waiting_clients)) {
      pthread_cond_wait(&client_is_waiting, &waiting_clients_mutex);
    }
    client_queue_pop(&waiting_clients, &c);
    pthread_mutex_unlock(&waiting_clients_mutex);
    client_handle(c);
  }
  return NULL;
}

int main(void) {
  struct Server s;
  waiting_clients = client_queue_new(MAX_WAITING_CLIENTS);
  pthread_t client_threads[MAX_JOINED_CLIENTS];

  s.addr.sin_family = AF_INET;
  s.addr.sin_port = htons(PORT);
  s.addr.sin_addr.s_addr = INADDR_ANY;

  if ((s.sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
    perror("socket()");
    return 1;
  }

  if (bind(s.sock, (struct sockaddr *)&s.addr, sizeof(s.addr)) == -1) {
    perror("bind()");
    return 1;
  }

  if (listen(s.sock, 1) == -1) {
    perror("listen()");
    return 1;
  }

  for (size_t i = 0; i < MAX_JOINED_CLIENTS; ++i) {
    pthread_create(&client_threads[i], NULL, handle_connection, NULL);
  }

  while (true) {
    struct Client c;
    if ((c.sock = accept(s.sock, (struct sockaddr *)&c.addr, &c.addr_len)) == -1) {
      perror("accept()");
      return 1;
    }

    pthread_mutex_lock(&waiting_clients_mutex);
    if (client_queue_push(&waiting_clients, c)) {
      pthread_cond_signal(&client_is_waiting);
    } else {
      fprintf(stderr, "closing client connection. waiting queue full.\n");
      close(c.sock);
    }
    pthread_mutex_unlock(&waiting_clients_mutex);
  }

  client_queue_free(waiting_clients);
  return 0;
}
