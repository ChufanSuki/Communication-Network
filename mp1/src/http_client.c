/*
** http_client.c -- a stream socket http client demo
*/

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <arpa/inet.h>

#define PORT "80" // the default port client will be connecting to

#define MAXDATASIZE 100000 // max number of bytes we can get at once
#define MAXFILESIZE 100
#define MAXINPUTSIZE 10000

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
  int sockfd, numbytes;
  char buf[MAXDATASIZE];
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  if (argc != 2) {
    fprintf(stderr, "usage: http_client http://hostname[:port]/path/to/file\n");
    exit(1);
  }

  // prase input parameter
  char input[MAXINPUTSIZE];
  strcpy(input, argv[1]);
  char protocol[8] = "http://";
  char port[6] = {'\0'};
  char host_name[MAXINPUTSIZE] = {'\0'};
  char file[MAXINPUTSIZE] = {'\0'};
  int port_flag = 0;
  int port_pos = 0;
  int slash_pos = 0;
  if (strncmp(protocol, input, 7) != 0) {
    fprintf(stderr, "protocol error!\nusage: http_client "
                    "http://hostname[:port]/path/to/file\n");
    exit(1);
  }
  for (int i = 7; i < strlen(input); i++) {
    if (input[i] == ':') {
      port_flag = 1;
      port_pos = i;
    }
    if (input[i] == '/') {
      slash_pos = i;
      break;
    }
  }
  if (port_flag) {
    strncpy(port, input + port_pos + 1, slash_pos - port_pos - 1);
    strncpy(host_name, input + 7, port_pos - 7);
  } else {
    strncpy(host_name, input + 7, slash_pos - 7);
    strcpy(port, "80");
  }
  strncpy(file, input + slash_pos + 1, MAXFILESIZE);
  //  printf("DEBUG\n http://%s:%s/%s\n", host_name, port, file);
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(host_name, port, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and connect to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("client: connect");
      close(sockfd);
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s,
            sizeof s);
  printf("client: connecting to %s\n", s);

  freeaddrinfo(servinfo); // all done with this structure

  // Send HTTP request
  char request[MAXINPUTSIZE] = {'\0'};
  strcat(request, "GET /");
  strcat(request, file);
  strcat(request, " HTTP/1.1\r\n\r\n");
  // printf("DEBUG: HTTP request:\n %s", request);

  send(sockfd, request, sizeof(char) * strlen(request), 0);

  FILE *fp = fopen("output", "wb");
  if (fp == NULL) {
    perror("file open");
    exit(1);
  }

  numbytes = recv(sockfd, buf, MAXDATASIZE, 0);
  while (numbytes > 0) {
    if (numbytes == -1) {
      perror("recv");
      exit(1);
    }

    fwrite(buf, sizeof(char), numbytes, fp);
    //        printf("DEBUG: client: received '%d' bytes\n %s\n", numbytes,
    //        buf); printf("%s", buf);
    numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0);
  }
  //    fprintf(stdin, "\n%s\n", buf);
  fclose(fp);
  close(sockfd);

  return 0;
}
