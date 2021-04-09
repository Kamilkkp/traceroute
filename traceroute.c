// Kamil Puchacz
// 309593
#include "traceroute.h"

u_int16_t compute_icmp_checksum(const void *buff, int length) {

  u_int32_t sum;
  const u_int16_t *ptr = buff;
  assert(length % 2 == 0);
  for (sum = 0; length > 0; length -= 2)
    sum += *ptr++;

  sum = (sum >> 16) + (sum & 0xffff);
  return (u_int16_t)(~(sum + (sum >> 16)));
}

ssize_t sendICMPPacket(int socketFD, struct sockaddr_in addr, int ttl,
                       uint16_t pid) {
  // tworzenie nagłówka ICMP
  struct icmp header;
  header.icmp_type = ICMP_ECHO;
  header.icmp_code = 0;
  header.icmp_id = pid;
  header.icmp_seq = ttl;
  header.icmp_cksum = 0;
  header.icmp_cksum =
      compute_icmp_checksum((u_int16_t *)&header, sizeof(header));

  // Ustawianie TTL
  setsockopt(socketFD, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));

  // Wysyłanie pakietu
  ssize_t bytes_sent = sendto(socketFD, &header, sizeof(header), 0,
                              (struct sockaddr *)&addr, sizeof(addr));
  return bytes_sent;
}

int pingRouters(int socketFD, struct sockaddr_in *addr, uint32_t ttl,
                trace_response *responses) {
  const uint16_t pid = getpid() & 0xffff;

  // Wysyłanie 3 pakietów
  for (int i = 0; i < 3; ++i) {
    if (sendICMPPacket(socketFD, *addr, ttl, pid) < 0) {
      fprintf(stderr, "sendto error: %s\n", strerror(errno));
      return -1;
    }
  }

  // Zapisanie czasu wysłania
  struct timeval send_time;
  gettimeofday(&send_time, NULL);

  struct timeval time_now;
  struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000000;

  // Sprawdzanie odpowiedzi dopóki nie minie 1s lub nie otrzymamy 3 odpowiedzi
  // na wysłane pakiety
  while (responses->response_count < 3 && tv.tv_usec > 0) {

    fd_set descriptors;
    FD_ZERO(&descriptors);
    FD_SET(socketFD, &descriptors);
    int ready = select(socketFD + 1, &descriptors, NULL, NULL, &tv);

    // kontrola zwracanych wartosci przez selecet(błąd, koniec czasu)
    if (ready < 0) {
      printf("Error: select\n");
      return -1;
    } else if (ready == 0) {
      return EXIT_SUCCESS;
    }

    // wydobywanie pakietów nadesłanyxh do gniazda
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    u_int8_t buffer[IP_MAXPACKET];
    ssize_t packet_len = recvfrom(socketFD, buffer, IP_MAXPACKET, MSG_DONTWAIT,
                                  (struct sockaddr *)&sender, &sender_len);
    while (packet_len > 0) {
      gettimeofday(&time_now, NULL);
      long time_passed = (1000000 * time_now.tv_sec + time_now.tv_usec) -
                    (1000000 * send_time.tv_sec + send_time.tv_usec);

      // pobranie adresu wysyłającego
      char sender_ip_str[20];
      inet_ntop(AF_INET, &(sender.sin_addr), sender_ip_str,
                sizeof(sender_ip_str));

      // pobranie nagłówka ICMP
      struct ip *ip_header = (struct ip *)buffer;
      ssize_t ip_header_len = 4 * ip_header->ip_hl;
      struct icmp *icmp_header = (struct icmp *)(buffer + ip_header_len);
      ssize_t icmp_header_len = 8;

      // Type 8 - loopback (form localhost, etc.)
      bool skip = false;
      if (icmp_header->icmp_type == 8) {
        skip = true;
      }

      // Type 11 - TTL exceeded, extract the original request IP, ICMP data
      if (icmp_header->icmp_type == ICMP_TIME_EXCEEDED) {
        ssize_t inner_packet = ip_header_len + icmp_header_len;
        ip_header = (struct ip *)(buffer + inner_packet);
        ip_header_len = 4 * ip_header->ip_hl;
        icmp_header = (struct icmp *)(buffer + inner_packet + ip_header_len);
      }

      // pobranie icmp packet id, seq;
      uint16_t id = icmp_header->icmp_hun.ih_idseq.icd_id;
      uint16_t seq = icmp_header->icmp_hun.ih_idseq.icd_seq;

      // sprawdzenie czy pakiet jest odpowiedzią na ostatnie 3 wysłane pakiety
      if (id == pid && seq == ttl && !skip) {
        // sprawdzenie czy nadwca ma inny adres ip niż obencne
        bool unique = true;
        for (unsigned i = 0; i < responses->ip_count; ++i)
          if (strcmp(responses->ip_addr[i], sender_ip_str) == 0)
            unique = false;
        if (unique) {
          strcpy(responses->ip_addr[responses->ip_count], sender_ip_str);
          responses->ip_count++;
        }
        // aktualizacja response_time i response_count
        responses->response_times[responses->response_count] = time_passed;
        responses->response_count++;
      }

      packet_len = recvfrom(socketFD, buffer, IP_MAXPACKET, MSG_DONTWAIT,
                            (struct sockaddr *)&sender, &sender_len);
    }

    if (packet_len < 0 && errno != EWOULDBLOCK) {
      printf("Error: recvfrom, msg: %s\n", strerror(errno));
      return -1;
    }
  }
  return EXIT_SUCCESS;
}