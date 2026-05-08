#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "client_handler.h"
#include "common.h"
#include "messages.h"

#define PORT 10551

struct Server {
  struct sockaddr_in addr;
  socklen_t addr_len;
  int sock;
};

struct ClientQueue waiting_clients;

pthread_mutex_t waiting_clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waiting_clients_cond = PTHREAD_COND_INITIALIZER;

void *handle_connection(void *thread_no_p) {
  struct Client c;
  int thread_no = *(int *)thread_no_p;
  while (true) {
    pthread_mutex_lock(&waiting_clients_mutex);
    if (client_queue_is_empty(waiting_clients)) {
      pthread_cond_wait(&waiting_clients_cond, &waiting_clients_mutex);
    }
    client_queue_pop(&waiting_clients, &c);
    c.thread_no = thread_no;
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

  if (!waiting_messages_init()) {
    return 1;
  }

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
    int *thread_no = malloc(sizeof(int));
    *thread_no = i;
    if (pthread_create(&client_threads[i], NULL, handle_connection, thread_no) != 0) {
      fputs("failed to create thread", stderr);
      return 1;
    }
  }

  while (true) {
    struct Client c = {.addr_len = sizeof(c.addr)};
    if ((c.sock = accept(s.sock, (struct sockaddr *)&c.addr, &c.addr_len)) == -1) {
      perror("accept()");
      return 1;
    }

    pthread_mutex_lock(&waiting_clients_mutex);
    if (client_queue_push(&waiting_clients, c)) {
      pthread_cond_signal(&waiting_clients_cond);
    } else {
      fprintf(stderr, "closing client connection. waiting queue full.\n");
      close(c.sock);
    }
    pthread_mutex_unlock(&waiting_clients_mutex);
  }

  waiting_messages_free();
  client_queue_free(waiting_clients);
  return 0;
}
