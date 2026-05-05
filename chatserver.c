#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFSIZE 4096
#define PORT 10551

#define MAX_JOINED_CLIENTS 10
#define MAX_WAITING_CLIENTS 10

struct Server {
  struct sockaddr_in addr;
  socklen_t addr_len;
  int sock;
};

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

struct ClientQueue waiting_clients;

pthread_mutex_t client_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t client_is_waiting = PTHREAD_COND_INITIALIZER;

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

int handle_client(struct Client client) {
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

void *handle_connection(void *_unused) {
  (void)_unused; // silence 'unused parameter' compiler warning

  struct Client c;
  while (true) {
    pthread_mutex_lock(&client_queue_mutex);
    if (client_queue_is_empty(waiting_clients)) {
      pthread_cond_wait(&client_is_waiting, &client_queue_mutex);
    }
    client_queue_pop(&waiting_clients, &c);
    pthread_mutex_unlock(&client_queue_mutex);
    handle_client(c);
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

    pthread_mutex_lock(&client_queue_mutex);
    if (client_queue_push(&waiting_clients, c)) {
      pthread_cond_signal(&client_is_waiting);
    } else {
      fprintf(stderr, "closing client connection. waiting queue full.\n");
      close(c.sock);
    }
    pthread_mutex_unlock(&client_queue_mutex);
  }

  client_queue_free(waiting_clients);
  return 0;
}
