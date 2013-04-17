#include "vichess.h"

// Assorted general-purpose functions.

void debug(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

void error(const char *msg)
{
  printf("error:  %s\n", msg);
  exit(-1);
}

void *get_in_addr(struct sockaddr *sa) 
{
  if (sa->sa_family == AF_INET)
  {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// malloc()ate a socket, connect() to it, and return the file descriptor
// n.b.: client must free()
long *get_socket_fd(const char *server, const char *port)
{
  struct addrinfo hints, *servinfo, *p;
  long *sock_fd = malloc(sizeof(*sock_fd));
  if(sock_fd == NULL) { error("get_socket_fd"); }
  int status = -1;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family   = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  memset(&servinfo, 0, sizeof(servinfo));
  if((status = getaddrinfo(server, port, &hints, &servinfo )))
  {
    error("getaddrinfo"); 
  }

  for(p = servinfo; p != NULL; p = p->ai_next)
  {
    if((*sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
    {
      error("socket");
      continue;
    }
    if(connect(*sock_fd, p->ai_addr, p->ai_addrlen) == -1)
    {
      close(*sock_fd);
      error("connect");
      continue;
    }
    break; // valid connection
  }
  freeaddrinfo(servinfo);

  if(p == NULL)
  {
    error("Failed to connect");
    return (long*) -1;
  }

  //inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof(s));
  return (long*) sock_fd;
}

// return malloc()ed message queue fd
// n.b.: client must free()
mqd_t *get_mq_fd(const char* name, int o_flags)
{
  mqd_t *mqd = malloc(sizeof(*mqd));
  if (mqd == NULL) { error("get_mq_fd"); }

  if ((*mqd = mq_open(name, o_flags, S_IRUSR | S_IWUSR, NULL)) == -1) { 
    perror("error:");
    error("mq_open");
  }
  return mqd;
}

// N.B.:  callers must always terminate message strings with "\n"
void send_message(int mq_fd, char *fmt, ...)
{
  char msg[MAX_LINE_SIZE];
  memset(msg, 0, MAX_LINE_SIZE);
  va_list args;
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  if(mq_send(mq_fd, msg, strlen(msg), PRIORITY) == -1) { error("send_message"); }
  va_end(args);
}

bool even(int z)
{
  return z % 2 == 0;
}

bool odd(int z)
{
  return ! even(z);
}

bool begins_with(char *this, char *that)
{
  return strncmp(that, this, strlen(that)) == 0;
}

bool equals(char *a, char *b)
{
  return strcmp(a, b) == 0;
}

bool contains(char *haystack, char *needle)
{
  return strstr(haystack, needle) != NULL;
}

void swap(char **a, char **b)
{
  char *cp = *b;
  *b = *a;
  *a = cp;
}

bool equal(char *a, char *b)
{
  return strncmp(a, b, strlen(a)) == 0;
}

unsigned int centered(char *s)
{
  return ((COLS - strlen(s)) / 2);
}

void ms_to_hh_mm_ss_ms(int _ms, char *hh_mm_ss_ms)
{
  unsigned int hh, mm, ss, ms;
  if (_ms < 1)
  {
    ms = hh = mm = ss = 0;
  }
  else
  {
    ms = _ms % 1000;
    hh = floor( _ms / 3600000         );
    mm = floor( (_ms / (60000) ) % 60  );
    ss = floor( (_ms / 1000) % 60     );

    memset(hh_mm_ss_ms, 0, 10);
  }
  sprintf( hh_mm_ss_ms, 
      "%02d:%02d:%02d.%03d", hh, mm, ss, ms );
}
