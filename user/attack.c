#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define DATASIZE (32*4096)


int printable(char c) {
    return c >= 32 && c < 127;
}

char data[DATASIZE];

int
main(int argc, char *argv[])
{
  data[DATASIZE-1] = 0;
  for (int i = 0; i < DATASIZE; i++) {
    if (!printable(data[i])) {
      continue;
    }
    if (memcmp(&data[i], "secret", 6) == 0) {
      // found it, skip "secret\0"
      printf("%s\n", data+i+7);
    }
  }
  exit(0);
}
