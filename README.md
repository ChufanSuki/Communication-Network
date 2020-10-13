# MP0
Remember, any thing is a **file** under Unix.

## Step by Step
0. On every step do error checking on return value and  `perror()` it.
1. `getaddrinfo()`
Fill the `struct addrinfo`
2. `socket()`
Get the file descriptor
3. `bind()`
Associate the socket with a port on your **local machine**
4. `connect()`
Connect to a **remote host**.
5. `listen()`
Wait for incoming connections and set `backlog`
6. `accept()`
Handle the connections and get new socket descriptor(fork a new socket for every connection).
7. `send()` and `recv()`
Talk to me, baby and be careful about the return value.
8. `sendto()` and `recvfrom()`
Talk to me, DGRAM-style.Used for **unconnected datagram** sockets
9. `close()` and `shutdown()`
`Shutdown()` change socket's usability while `close()` free a socket descriptor.
10. `getpeername()` and `gethostname()`
Who are you? Who am I?



## Struct

| struct |function  |notes|
|--|--|--|
| `struct addrinfo` | `getaddrinfo()` and `freeaddrinfo()` | `struct sockaddr *ai_addr`
|`struct sockaddr` | |can be cast into/back `sockaddr_in` or `sockaddr_in6`
|`struct sockaddr_in`|`inet_pton(AF_INET, ipv4 string, struct sockaddr_in *)` and its brother `inet_ntop`|`struct in_addr sin_addr`
|`struct sockaddr_in6`|`inet_pton(AF_INET6, ipv6 string, struct sockaddr_in6 *)` and its brother `inet_ntop`|`struct in6_addr sin6_addr`
|`struct sockaddr_storage`||check `ss_familt` and cast it to according socket address struct

## Notes
1. If you see something like this: `Makefile:66: *** missing separator.  Stop.` Makefile expect command is indented by tab other than space, so you maybe use \t or use tab.
2. TCP and UDP can use same port.

# MP1
- Use strlen() instead of sizeof().
- CRLF = CR LF
- HTTP Response
  - Request line, such as GET /logo.gif HTTP/1.1 or Status line, such as HTTP/1.1 200 OK,
  - Headers
  - An empty line
  - Optional HTTP message body data

The request/status line and headers must all end with <CR><LF> (that is, a carriage return followed by a line feed). The empty line must consist of only <CR><LF> and no other whitespace.
- Don't set const too small
