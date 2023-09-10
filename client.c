/* Generic */
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Network */
#include <netdb.h>
#include <sys/socket.h>

#define BUF_SIZE 100

static char *host;
static char *port;
static char *path1;
static char *path2 = 0;

static pthread_barrier_t barrier;
static sem_t semaphore;

// Get host information (used to establishConnection)
struct addrinfo *getHostInfo(char* host, char* port) {
  int r;
  struct addrinfo hints, *getaddrinfo_res;
  // Setup hints
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if ((r = getaddrinfo(host, port, &hints, &getaddrinfo_res))) {
    fprintf(stderr, "[getHostInfo:21:getaddrinfo] %s\n", gai_strerror(r));
    return NULL;
  }

  return getaddrinfo_res;
}

// Establish connection with host
int establishConnection(struct addrinfo *info) {
  if (info == NULL) return -1;

  int clientfd;
  for (;info != NULL; info = info->ai_next) {
    if ((clientfd = socket(info->ai_family,
                           info->ai_socktype,
                           info->ai_protocol)) < 0) {
      perror("[establishConnection:35:socket]");
      continue;
    }

    if (connect(clientfd, info->ai_addr, info->ai_addrlen) < 0) {
      close(clientfd);
      perror("[establishConnection:42:connect]");
      continue;
    }

    freeaddrinfo(info);
    return clientfd;
  }

  freeaddrinfo(info);
  return -1;
}

// Send GET request
void GET(int clientfd, char *host, char *port, char *path) {
  char req[1000] = {0};
  sprintf(req, "GET %s HTTP/1.1\r\nHost: %s:%s\r\n\r\n", path,host,port);
  send(clientfd, req, strlen(req), 0);
}

int doClientStuff(char *path) {
  int clientfd;
  char buf[BUF_SIZE];

  // Establish connection with <hostname>:<port>
  clientfd = establishConnection(getHostInfo(host, port));
  if (clientfd == -1) {
    fprintf(stderr,
            "[main:73] Failed to connect to: %s:%s%s \n",
            host, port, path);
    return 3;
  }

  // Send GET request > stdout
  GET(clientfd, host, port, path);
  while (recv(clientfd, buf, BUF_SIZE, 0) > 0) {
    fputs(buf, stdout);
    memset(buf, 0, BUF_SIZE);
  }

  close(clientfd);
  return 0;
}

void *concur(void *arg) {
  char *path = path1; /* the path to request */
  short which_path = 0; /* which of the 2 paths to request */
  while ( 1 ) {
    pthread_barrier_wait(&barrier);
    if (path2) {
      path = which_path ? path2 : path1;
      which_path = !which_path;
    }
    doClientStuff(path);
  }
  return NULL;
}

void *fifo(void *arg) {
  char *path = path1; /* the path to request */
  short which_path = 0; /* which of the 2 paths to request */
  while ( 1 ) {
    if (path2) {
      path = which_path ? path2 : path1;
      which_path = !which_path;
    }

    int clientfd;
    char buf[BUF_SIZE];

    sem_wait(&semaphore);
    // Establish connection with <hostname>:<port>
    clientfd = establishConnection(getHostInfo(host, port));
    if (clientfd == -1) {
      fprintf(stderr,
            "[main:73] Failed to connect to: %s:%s%s \n",
            host, port, path);
      sem_post(&semaphore);      
      return NULL;
    }
    // Send GET request > stdout
    GET(clientfd, host, port, path);
    sem_post(&semaphore);

    while (recv(clientfd, buf, BUF_SIZE, 0) > 0) {
      fputs(buf, stdout);
      memset(buf, 0, BUF_SIZE);
    }
    close(clientfd);

    pthread_barrier_wait(&barrier);
  }
  return NULL;
}

int main(int argc, char **argv) {
  if (argc != 6 && argc != 7) {
    fprintf(stderr, "USAGE: ./client <hostname> <port> <threads> <schedalg> <filename1> <filename2>\n");
    return 1;
  }

  int i;

  int N = atoi(argv[3]); /* Number of threads */
  host = argv[1];
  port = argv[2];
  path1 = argv[5];
  if (argc == 7) path2 = argv[6];

  pthread_t threadPool[N];

  if (strcmp(argv[4], "CONCUR") == 0 ) {
    /* Concurrent Groups */
    pthread_barrier_init(&barrier, NULL, N);
    for (i = 0; i < N; i++) {
      pthread_create(&threadPool[i], NULL, concur, NULL);
    }
    for (i = 0; i < N; i++) {
      pthread_join(threadPool[i], NULL);
    }
    pthread_barrier_destroy(&barrier);
  } else if (strcmp(argv[4], "FIFO") == 0 ) {
    /* First-In, First-Out */
    // TODO
    sem_init(&semaphore, 0, 1);
    pthread_barrier_init(&barrier, NULL, N);
    for (i = 0; i < N; i++) {
      pthread_create(&threadPool[i], NULL, fifo, NULL);
    }
    for (i = 0; i < N; i++) {
      pthread_join(threadPool[i], NULL);
    }
    pthread_barrier_destroy(&barrier);
    sem_destroy(&semaphore);
  } else {
    fprintf(stderr, "unknown scheduling algorithm: %s\n", argv[4]);
    return 1;
  }

  return 0;
}
