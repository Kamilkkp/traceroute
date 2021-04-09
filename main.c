// Kamil Puchacz
// 309593
#include "traceroute.h"

int traceroute(char *ip_arg) {
  struct sockaddr_in addr;

  // Konwersja IP
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  if (inet_pton(AF_INET, ip_arg, &(addr.sin_addr)) == 0) {
    printf("Error: invalid ip address: %s. Pass an ipv4 address.\n", ip_arg);
    return EXIT_FAILURE;
  }

  int socketFD = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

  if (socketFD < 0) {
    fprintf(stderr, "socket error: %s\n", strerror(errno));
    return EXIT_FAILURE;
  }

  bool reached_target = false;
  for (int i = 1; i <= 30 && !reached_target; ++i) {
    trace_response responses;
    bzero(&responses, sizeof(responses));
    error(pingRouters(socketFD, &addr, (uint32_t)i, &responses));
    printf("%d. ", i);

    // brak odpowiedzi
    if (responses.response_count == 0)
      printf("*\n");
    else {
      // Lista adresów ip
      for (unsigned j = 0; j < responses.ip_count; ++j) {
        printf("%s ", responses.ip_addr[j]);
        // sprawdzenie, czy docelowy adres ip został osiągniety
        if (strcmp(responses.ip_addr[j], ip_arg) == 0)
          reached_target = true;
      }
      // brak kompletu odpowiedzi
      if (responses.response_count < 3)
        printf("???\n");
      else {
        double avg =
            (responses.response_times[0] + responses.response_times[1] +
             responses.response_times[2]) /
            3000.0;
        printf("%.0lfms\n", avg);
      }
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  // Sprawdzenie liczby podanych argumentów
  if (argc != 2) {
    printf("Error: wrong number of arguments.\n Example usage (required root "
           "privileges):\ntraceroute 127.0.0.1\n");
    return 0;
  }
  // SPrawdzenie czy aplikacja ma ma uprawnienia root
  if (getuid()) {
    printf("Error: you cannot perform this operation unless you are root.\n");
    return -1;
  }

  return traceroute(argv[1]);
}