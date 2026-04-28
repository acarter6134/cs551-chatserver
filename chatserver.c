#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFSIZE 4096
#define PORT 10551

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

int handle(struct Client client) {
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

int main(void) {
  struct Server s;
  struct Client c;

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

  for (;;) {
    if ((c.sock = accept(s.sock, (struct sockaddr *)&c.addr, &c.addr_len)) == -1) {
      perror("accept()");
      return 1;
    }

    pid_t c_pid;
    if ((c_pid = fork()) == -1) {
      perror("fork()");
      return 1;
    }

    if (c_pid == 0) {
      return handle(c);
    }
  }
  return 0;
}
