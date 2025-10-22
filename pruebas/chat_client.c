// chat_client.c - UDP client que lee líneas por teclado y las envía con timestamp UTC.
// Uso: cliente:  ./chat_client -h 127.0.0.1 -p 9999
//       Windows: chat_client.exe -h 127.0.0.1 -p 9999
// Escribí texto y Enter para enviar. "exit" para salir.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "Ws2_32.lib")
#else
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
#endif

#define DEFAULT_PORT 9999
#define LINE_MAX 1600
#define OUT_MAX 2048

static void die(const char* msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

static void iso8601_utc(char* out, size_t out_size) {
  // YYYY-MM-DDTHH:MM:SSZ
  time_t now = time(NULL);
#ifdef _WIN32
  struct tm gmt;
  gmtime_s(&gmt, &now);
  strftime(out, out_size, "%Y-%m-%dT%H:%M:%SZ", &gmt);
#else
  struct tm gmt;
  gmtime_r(&now, &gmt);
  strftime(out, out_size, "%Y-%m-%dT%H:%M:%SZ", &gmt);
#endif
}

static void parse_args(int argc, char** argv, char* host, size_t host_sz, int* port) {
  strncpy(host, "127.0.0.1", host_sz-1); host[host_sz-1] = '\0';
  *port = DEFAULT_PORT;
  for (int i = 1; i < argc; ++i) {
    if ((strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--host") == 0) && i+1 < argc) {
      strncpy(host, argv[++i], host_sz-1); host[host_sz-1] = '\0';
    } else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) && i+1 < argc) {
      *port = atoi(argv[++i]);
    }
  }
}

int main(int argc, char** argv) {
  char host[256];
  int port;
  parse_args(argc, argv, host, sizeof(host), &port);

#ifdef _WIN32
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
    fprintf(stderr, "WSAStartup failed\n");
    return EXIT_FAILURE;
  }
#endif

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) die("socket");

  struct sockaddr_in srv;
  memset(&srv, 0, sizeof(srv));
  srv.sin_family = AF_INET;
  srv.sin_port = htons((uint16_t)port);

  if (inet_pton(AF_INET, host, &srv.sin_addr) != 1) {
    fprintf(stderr, "Host inválido: %s (usa IPv4)\n", host);
#ifdef _WIN32
    closesocket(sockfd); WSACleanup();
#else
    close(sockfd);
#endif
    return EXIT_FAILURE;
  }

  printf("Cliente UDP -> %s:%d\n", host, port);
  printf("Escribí y Enter para enviar. Escribí 'exit' para salir.\n");

  char line[LINE_MAX];
  char packet[OUT_MAX];

  while (1) {
    if (!fgets(line, sizeof(line), stdin)) break;
    // quitar \r\n
    size_t L = strlen(line);
    while (L && (line[L-1] == '\n' || line[L-1] == '\r')) { line[--L] = '\0'; }

    if (strcmp(line, "exit") == 0) break;
    if (L == 0) continue; // no mandar vacíos

    char ts[32];
    iso8601_utc(ts, sizeof(ts));

    // Construye paquete: TS=...;MSG=...
    // Nota: sin escape de caracteres; suficiente para demo básica.
    int n = snprintf(packet, sizeof(packet), "TS=%s;MSG=%s", ts, line);
    if (n < 0 || n >= (int)sizeof(packet)) {
      fprintf(stderr, "Mensaje demasiado largo, truncado/omitido.\n");
      continue;
    }

    int sent = sendto(sockfd, packet, n, 0, (struct sockaddr*)&srv, sizeof(srv));
    if (sent < 0) {
#ifdef _WIN32
      fprintf(stderr, "sendto failed: %d\n", WSAGetLastError());
#else
      perror("sendto");
#endif
    }
  }

#ifdef _WIN32
  closesocket(sockfd); WSACleanup();
#else
  close(sockfd);
#endif
  return 0;
}
