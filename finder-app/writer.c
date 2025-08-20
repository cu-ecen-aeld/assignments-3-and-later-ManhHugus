#include <stdio.h>
#include <syslog.h>
#include <string.h>

int main (int argc, char *argv[])
{
  openlog("MySyslog", LOG_CONS | LOG_PID, LOG_USER);

  if (argc != 3) {
    syslog(LOG_ERR, "Only two arguments are required!");
    fprintf(stderr, "Only two arguments are required!\r\n");
    closelog();
    return 1;
  }

  const char *file_path = argv[1];
  const char *file_content = argv[2];

  syslog(LOG_DEBUG, "Writing %s to %s", file_content, file_path);
  FILE *file = fopen(file_path, "w+");
  if (file == NULL) {
    syslog(LOG_ERR, "The file path is not correct!");
    fprintf(stderr, "The file path is not correct!\r\n"); 
    return 1;
  }

  if (fprintf(file, "%s" ,file_content)) {
    syslog(LOG_DEBUG, "Successfully written %s to %s", file_content, file_path);
    fprintf(stderr, "Successfully written %s to %s\r\n", file_content, file_path);
    fclose(file);
  } else {
    syslog(LOG_ERR, "Could not write %s to %s", file_content, file_path);
    fprintf(stderr, "Could not write %s to %s\r\n", file_content, file_path);
    fclose(file);
  }

  closelog();
  return 0;
}
