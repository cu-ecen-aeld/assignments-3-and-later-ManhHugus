#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/stat.h>

#define LISTEN_PORT "9000" 
#define BACKLOG 10 
#define DATA_FILE "/var/tmp/aesdsocketdata"

static volatile sig_atomic_t exit_requested = 0; 

static void handle_signal(int sig)
{
  (void)sig; 
  exit_requested = 1;
}

static void install_signal_handlers(void) 
{
  struct sigaction sa = {0};
  sa.sa_handler = handle_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("sigaction(SIGINT)\r\n");
    exit(EXIT_FAILURE);
  }

  if (sigaction(SIGTERM, &sa, NULL) == -1) {
    perror("sigaction(SIGTERM)\r\n");
    exit(EXIT_FAILURE); 
  }
}

static int create_server_socket(void) 
{
  struct addrinfo *res, *rp; 
  struct addrinfo hints;
  int server_file_descriptor = -1; 
  int rc = 0;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; 
  hints.ai_socktype = SOCK_STREAM; 
  hints.ai_flags = AI_PASSIVE; 

  if ((rc = getaddrinfo(NULL, LISTEN_PORT, &hints, &res)) != 0) {
    syslog(LOG_ERR, "getaddrinfo: %s\r\n", gai_strerror(rc));
    return -1;
  }

  int optval = 1;
  for (rp = res; rp != NULL; rp = rp->ai_next) {
    server_file_descriptor = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (server_file_descriptor < 0) continue; 

    if(setsockopt(server_file_descriptor, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
      close(server_file_descriptor);
      server_file_descriptor = -1; 
      continue; 
    }

    if (bind(server_file_descriptor, rp->ai_addr, rp->ai_addrlen) < 0) {
      close(server_file_descriptor);
      server_file_descriptor = -1;
      continue; 
    }

    if (listen(server_file_descriptor, BACKLOG) < 0) {
      close(server_file_descriptor);
      server_file_descriptor = -1; 
      continue;
    }

    break; 
  }

  freeaddrinfo(res);

  if (server_file_descriptor < 0) {
    syslog(LOG_ERR, "Failed to setup listening server socket");
  }

  return server_file_descriptor;
}

static int append_packet_and_echo(int client_fd)
{
  int df = open(DATA_FILE, O_CREAT | O_WRONLY | O_APPEND, 0644);
  if (df == -1) {
    syslog(LOG_ERR, "Error opening %s: %s\r\n", DATA_FILE, strerror(errno));
    return -1; 
  }

  size_t cap = 1024, len = 0; 
  char *buf = malloc(cap); 
  if (!buf) {
    close(df);
    return -1; 
  }

  while (!exit_requested) {
    char c;
    ssize_t r = recv(client_fd, &c, 1, 0);
    if (r == 0) { // connection closed before newline
        break;
    } else if (r < 0) {
        if (errno == EINTR) continue; 
        syslog(LOG_ERR, "recv: %s", strerror(errno));
        free(buf);
       	close(df);
       	return -1;
    }
    if (len + 1 >= cap) {
        size_t ncap = cap * 2;
        char *tmp = realloc(buf, ncap);
        if (!tmp) { 
	  syslog(LOG_ERR, "realloc failed");
	  free(buf);
	  close(df);
	  return -1;
       	}
        buf = tmp; cap = ncap;
    }
    buf[len++] = c;
    if (c == '\n') break; // end of packet
  }

  if (len > 0) {
    if (write(df, buf, len) != (ssize_t)len) {
      syslog(LOG_ERR, "write data file failed: %s", strerror(errno)); 
    }
  }
  close(df); 

  // Send back full file contents
  df = open(DATA_FILE, O_RDONLY);
  if (df == -1) {
    syslog(LOG_ERR, "Error opening for reading %s: %s\r\n", DATA_FILE, strerror(errno));
    free(buf); 
    return -1; 
  }

  char fbuf[4096];
  ssize_t rd; 
  while ((rd = read(df, fbuf, sizeof(fbuf))) > 0) {
    ssize_t off = 0; 
    while (off < rd) {
      ssize_t wr = send(client_fd, fbuf + off, rd - off, 0);
      if (wr < 0) {
        if (errno == EINTR) continue; 
	syslog(LOG_ERR, "Error sending %s\r\n", strerror(errno)); 
	close(df); 
	free(buf); 
	return -1; 
      }
      off += wr;
    }
  }
  close(df);
  free(buf); 
  return 0;
}

static void log_client_addr(struct sockaddr_in *addr, const char *prefix)
{
  char ipstr[INET_ADDRSTRLEN];
  if (inet_ntop(AF_INET, &addr->sin_addr, ipstr, sizeof(ipstr)) == NULL) {
      strncpy(ipstr, "unknown", sizeof(ipstr));
      ipstr[sizeof(ipstr)-1] = '\0';
  }
  syslog(LOG_INFO, "%s %s", prefix, ipstr);
}

static int daemonize(void)
{
    pid_t pid = fork();
    if (pid < 0) return -1; // fork failed
    if (pid > 0) exit(EXIT_SUCCESS); // parent exits

    if (setsid() == -1) return -1; // new session

    pid = fork();
    if (pid < 0) return -1; // fork 2
    if (pid > 0) exit(EXIT_SUCCESS); // first child exits

    // change working directory to root so we don't block mounts
    if (chdir("/") == -1) return -1;

    // close std descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    // redirect to /dev/null
    int fd = open("/dev/null", O_RDWR);
    if (fd >= 0) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) close(fd);
    }
    return 0;
}

int main(int argc, char const* argv[]) 
{
  int run_as_daemon = 0; 
  if(argc == 2 && strcmp(argv[1], "-d") == 0) {
    run_as_daemon = 1;
  } else if (argc > 1) {
    fprintf(stderr, "Usage: %s [-d]\n", argv[0]);
    return EXIT_FAILURE;
  }

  openlog("aesdsocket", LOG_PID, LOG_USER);

  install_signal_handlers();

  int sfd = create_server_socket();
  if (sfd == -1) {
      closelog();
      return EXIT_FAILURE;
  }

  if (run_as_daemon) {
    if (daemonize() != 0) {
      syslog(LOG_ERR, "Failed to daemonize: %s\r\n", strerror(errno));
      close(sfd); 
      closelog(); 
      return EXIT_FAILURE; 
    }
  }

  while (!exit_requested) {
      struct sockaddr_in caddr; socklen_t caddrlen = sizeof(caddr);
      int cfd = accept(sfd, (struct sockaddr*)&caddr, &caddrlen);
      if (cfd == -1) {
          if (errno == EINTR) continue; 
          syslog(LOG_ERR, "accept: %s", strerror(errno));
          break;
      }
      log_client_addr(&caddr, "Accepted connection from");
      append_packet_and_echo(cfd);
      log_client_addr(&caddr, "Closed connection from");
      close(cfd);
  }

  // cleanup
  remove(DATA_FILE);
  syslog(LOG_INFO, "Caught signal, exiting\r\n");
  if (sfd != -1) close(sfd);
  closelog();
  return EXIT_SUCCESS;
}
