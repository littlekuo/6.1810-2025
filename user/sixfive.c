#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"


#define IO_BUF_SIZE 1024

enum State {
    STATE_SCANNING,  // try to find a digit
    STATE_IN_NUMBER, // already in number (reading digits)
    STATE_IN_JUNK    // in junk (skipping until next delimiter)
};

int is_delimiter(char c) {
  return c == ' ' || c == '-' || c == '\r' || c == '\t' || c == '\n' || c == '.' || c == '/' || c == ',';
}

int is_digit(char c) {
  return c >= '0' && c <= '9';
}

void process_number(char *buf, int len, int mod_three) {
    if (len == 0) return;
    buf[len] = '\0';
    char last_char = buf[len - 1];
    int idx = 0;
    // skip leading zeros
    while (idx < len && buf[idx] == '0') {
        idx++;
    }
    if ((mod_three == 0 && (last_char - '0') % 2 == 0) || (last_char == '0' || last_char == '5')) {
        printf("%s\n", buf+idx);
    }
}

void process_file(int fd) {
    char io_buf[IO_BUF_SIZE];
    int bytes_in_buf = 0;
    int buf_idx = 0;
    
    enum State cur_state = STATE_SCANNING;
    char *num_buffer = 0;
    int buffer_size = 0;
    int buffer_capacity = 0;
    int mod_three = 0;
  
    while (1) {
      if (buf_idx >= bytes_in_buf) {
          // read more data
          bytes_in_buf = read(fd, io_buf, sizeof(io_buf));
          if (bytes_in_buf <= 0) {
              break;
          }
          buf_idx = 0;
      }
  
      char c = io_buf[buf_idx++];
      switch (cur_state) {
          case STATE_SCANNING:
              if (is_digit(c)) {
                  cur_state = STATE_IN_NUMBER;
                  buffer_size = 0;
                  mod_three = 0;
                  // back up one char to reprocess this char
                  buf_idx--;
              } else if (is_delimiter(c)) {
                  // do nothing
              } else {
                  cur_state = STATE_IN_JUNK;
              }
              break;
          case STATE_IN_NUMBER:
              if (is_digit(c)) {
                  if (buffer_size + 1 >= buffer_capacity) {
                      int new_capacity = (buffer_capacity == 0) ? 16 : buffer_capacity * 2;
                      char *new_buffer = malloc(new_capacity);
                      if (new_buffer == 0) {
                          printf("malloc failed\n");
                          free(num_buffer);
                          exit(1);
                      }
                      if (num_buffer != 0) {
                          // copy old buffer to new buffer
                          memcpy(new_buffer, num_buffer, buffer_size);
                          free(num_buffer);
                      }
                      num_buffer = new_buffer;
                      buffer_capacity = new_capacity;
                  }
                  num_buffer[buffer_size++] = c;
                  mod_three = (mod_three + c - '0') % 3;
              } else if (is_delimiter(c)) {
                  process_number(num_buffer, buffer_size, mod_three);
                  cur_state = STATE_SCANNING;
              } else {
                  // not a digit, so we're in junk
                  cur_state = STATE_IN_JUNK;
              }
              break;
          case STATE_IN_JUNK:
              if (is_delimiter(c)) {
                  cur_state = STATE_SCANNING;
              }
              break;
      }
    }
  
    if (cur_state == STATE_IN_NUMBER) {
      process_number(num_buffer, buffer_size, mod_three);
    }
    free(num_buffer);
}

int
main(int argc, char *argv[])
{

  if(argc < 2){
    fprintf(2, "Usage: sixfive filename...\n");
    exit(1);
  }

  int fd;
  for(int i = 1; i < argc; i++){
    fd = open(argv[i], O_RDONLY);
    if(fd < 0){
      fprintf(2, "sixfive: cannot open %s\n", argv[i]);
      exit(1);
    }
    process_file(fd);
    close(fd);
  }
  exit(0);
}
