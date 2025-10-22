// chat_server.c - UDP server que muestra IP de origen y hora de envío + mensaje.
// Uso: servidor:  ./chat_server -p 9999
//       Windows:  chat_server.exe -p 9999
// Por defecto puerto 9999.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
  #define _WINSOCK_DEPRECATED_NO_WARNINGS
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "Ws2_32.lib")
  typedef int socklen_t;
#else
  #include <unistd.h>
  #include <arpa/inet.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
#endif

#define DEFAULT_PORT 9999
#define BUF_SIZE 2048

static void die(const char* msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

static int parse_port(int argc, char** argv) {
  int port = DEFAULT_PORT;
  for (int i = 1; i < argc; ++i) {
    if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) && i+1 < argc) {
      port = atoi(argv[++i]);
    }
  }
  return port;
}

int main(int argc, char** argv) {
  int port = parse_port(argc, argv);

#ifdef _WIN32
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
    fprintf(stderr, "WSAStartup failed\n");
    return EXIT_FAILURE;
  }
#endif

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) die("socket");

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((uint16_t)port);

  if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
#ifdef _WIN32
    fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
#else
    perror("bind");
#endif
#ifdef _WIN32
    closesocket(sockfd); WSACleanup();
#else
    close(sockfd);
#endif
    return EXIT_FAILURE;
  }

  printf("Servidor UDP escuchando en puerto %d ...\n", port);
  printf("Formato esperado: TS=YYYY-MM-DDTHH:MM:SSZ;MSG=...\n");

  char buf[BUF_SIZE];
  for (;;) {
    struct sockaddr_in src;
    socklen_t srclen = sizeof(src);
    int n = recvfrom(sockfd, buf, BUF_SIZE - 1, 0, (struct sockaddr*)&src, &srclen);
    if (n < 0) {
#ifdef _WIN32
      fprintf(stderr, "recvfrom failed: %d\n", WSAGetLastError());
#else
      perror("recvfrom");
#endif
      continue;
    }
    buf[n] = '\0';

    char ipstr[INET_ADDRSTRLEN] = {0};
#ifdef _WIN32
    // InetNtop está disponible en WinVista+, pero por compat, usamos inet_ntoa
    strncpy(ipstr, inet_ntoa(src.sin_addr), sizeof(ipstr) - 1);
#else
    inet_ntop(AF_INET, &src.sin_addr, ipstr, sizeof(ipstr));
#endif

    // Extraer TS y MSG de la línea recibida
    const char* ts_prefix = "TS=";
    const char* msg_prefix = "MSG=";
    const char* ts = NULL;
    const char* msg = NULL;

    if (strncmp(buf, ts_prefix, 3) == 0) {
      const char* semi = strchr(buf, ';');
      if (semi && strncmp(semi + 1, msg_prefix, 4) == 0) {
        static char tsbuf[64];
        size_t tslen = (size_t)(semi - (buf + 3));
        if (tslen >= sizeof(tsbuf)) tslen = sizeof(tsbuf) - 1;
        memcpy(tsbuf, buf + 3, tslen);
        tsbuf[tslen] = '\0';
        ts = tsbuf;
        msg = semi + 1 + 4; // salta "MSG="
      }
    }

    if (!ts || !msg) {
      // Mensaje no cumple el formato: lo mostramos raw
      printf("[IP %s] [TS ?] %s\n", ipstr, buf);
    } else {
      // Limpieza de fin de línea
      size_t L = strlen(msg);
      while (L && (msg[L-1] == '\n' || msg[L-1] == '\r')) { ((char*)msg)[L-1] = '\0'; L--; }
      printf("[IP %s] [TS %s] %s\n", ipstr, ts, msg);
    }
    fflush(stdout);
  }

#ifdef _WIN32
  closesocket(sockfd); WSACleanup();
#else
  close(sockfd);
#endif
  return 0;
}
