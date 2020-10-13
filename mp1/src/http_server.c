/*
** http_server.c -- a stream socket http server demo
*/

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// #define PORT "3490"  // the port users will be connecting to
#define MAXREQUEST 1000

#define BACKLOG 10 // how many pending connections queue will hold

#define STR400 "HTTP/1.1 400 Bad Request\r\n\r\n"
#define STR404 "HTTP/1.1 404 Not Found\r\n\r\n"
#define STR200 "HTTP/1.1 200 OK\r\n\r\n"
#define MAXDATASIZE 2048
void sigchld_handler(int s) {
  (void)s; // quiet unused variable warning

  // waitpid() might overwrite errno, so we save and restore it:
  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
  int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr; // connector's address information
  socklen_t sin_size;
  struct sigaction sa;
  int yes = 1;
  char s[INET6_ADDRSTRLEN];
  int rv;
  FILE *fp;
  if (argc != 2) {
    fprintf(stderr, "usage: http_server port\n If port is less than 1024, "
                    "please run it with root\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE; // use my IP

  if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and bind to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    break;
  }

  freeaddrinfo(servinfo); // all done with this structure

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  sa.sa_handler = sigchld_handler; // reap all dead processes
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  printf("server: waiting for connections...\n");

  while (1) { // main accept() loop
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("server: got connection from %s\n", s);

    if (!fork()) {   // this is the child process
      close(sockfd); // child doesn't need the listener
      // Receive HTTP request
      char request[MAXREQUEST];
      int byte_count = 0;
      byte_count = recv(new_fd, request, MAXREQUEST, 0);
      if (byte_count == -1) {
        perror("server: Receive");
      }
      //             printf("server: receive %d bytes request '%s'", byte_count,
      //             request);
      char method[10] = {'\0'};
      char filename[30] = {'\0'};
      char http_version[10] = {'\0'};
      unsigned long len = strlen(request);
      int slash_pos;
      int end_file;
      for (size_t i = 0; i < len; i++) {
        if (request[i] == '/') {
          slash_pos = i;
          for (size_t j = i + 1; j < len; j++) {
            if (isspace(request[j])) {
              end_file = j;
              break;
            }
          }
          if (end_file) {
            break;
          }
        }
      }
      //            printf("%d %d ", slash_pos, end_file);
      strncpy(method, request, slash_pos);
      strncpy(filename, request + slash_pos + 1, end_file - slash_pos - 1);
      strncpy(http_version, request + end_file + 1, 8);
      printf(
          "request method is: %s\nrequest file is: %s\nhttp version is: %s\n",
          method, filename, http_version);
      // HTTP response
      fp = fopen(filename, "rb");
      if (fp == NULL) {
        perror("open file");
        if (send(new_fd, STR404, strlen(STR404), 0) == -1) {
          perror("send");
        }
        exit(1);
      }

      if (send(new_fd, STR200, strlen(STR200), 0) == -1) {
        perror("send");
      }
      int numbytes = 0;
      char buf[MAXDATASIZE];
      while ((numbytes = fread(buf, sizeof(char), MAXDATASIZE, fp)
              ) > 0) {
        if (send(new_fd, buf, strlen(buf), 0) == -1) {
          perror("send");
          exit(1);
        }
      }
      fclose(fp);
      close(new_fd);
      exit(0);
    }
    close(new_fd); // parent doesn't need this
  }

  return 0;
}
