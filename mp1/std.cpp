#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>

#define MAXDATASIZE 1000
#define BACKLOG 100 // how many pending connections queue will hold

class PacketParser {
private:
  std::string method = "";
  std::string file_path = "";

public:
  PacketParser(std::string packet) {
    auto first_space_pos = packet.find_first_of(" ");
    method = packet.substr(0, first_space_pos);
    auto sec_space_pos = packet.substr(first_space_pos + 1).find_first_of(" ");
    file_path = packet.substr(first_space_pos + 1, sec_space_pos);
  }
  std::string getMethod() { return method; }
  std::string getFilePath() { return file_path; }
};

void sigchld_handler(int s) {
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
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

  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    return 2;
  }

  freeaddrinfo(servinfo); // all done with this structure

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

      char msg_buf[MAXDATASIZE];
      recv(new_fd, msg_buf, MAXDATASIZE, 0);
      std::string string_msg(msg_buf);
      std::cout << string_msg;
      PacketParser parser(string_msg);

      if (parser.getMethod() != "GET") {
        // invalid method type
        std::string bad_request = "HTTP/1.1 400 Bad Request\r\n";
        send(new_fd, bad_request.c_str(), bad_request.size(), 0);
        exit(1);
      }

      std::ifstream files(parser.getFilePath().substr(1).c_str(),
                          std::ios::binary | std::ios::in);
      if (!files.good()) {
        // request file does not exist
        std::string not_found =
            "HTTP/1.1 404 Not Found\r\n\r\nwhoops, file not found!\n";
        send(new_fd, not_found.c_str(), not_found.size(), 0);
      } else {
        // request file exists
        std::string OK = "HTTP/1.1 200 OK\r\n\r\n";
        send(new_fd, OK.c_str(), OK.size(), 0);
        char buf[2048];
        auto fp = fopen(parser.getFilePath().substr(1).c_str(), "rb");
        int numbytes = 0;
        while ((numbytes = fread(buf, sizeof(char), 2048, fp)) > 0) {
          send(new_fd, buf, numbytes, 0);
        }
        fclose(fp);
      }
      close(new_fd);
      exit(0);
    }
    close(new_fd); // parent doesn't need this
  }

  return 0;
}
