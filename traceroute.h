// Kamil Puchacz
// 309593
#include <assert.h>
#include <errno.h>
#include <netinet/ip_icmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <arpa/inet.h>

#define error(v)                                                               \
  if (v < 0)                                                                   \
  return -1

typedef struct trace_response {
  uint32_t ttl;
  uint32_t ip_count;
  uint32_t response_count;
  uint64_t response_times[3];
  char ip_addr[3][20];
} trace_response;

int pingRouters(int socketfd, struct sockaddr_in *addr, uint32_t ttl,
                trace_response *responses);