#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/param.h"

static int arg_count = 0;
static char *args[MAXARG];

void
find(char *path, char *target_name, char *cmd)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, O_RDONLY)) < 0){
    fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_DEVICE:
  case T_FILE:
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("find: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("find: cannot stat %s\n", buf);
        continue;
      }
      if(st.type != T_DIR) {
        // check if the file name matches the target name
        if (strcmp(de.name, target_name) == 0) {
          if (cmd) {
            printf("%s %s\n", cmd, buf);
            args[arg_count] = buf;
            args[arg_count+1] = 0;
            int pid = fork();
            if (pid == 0) {
              exec(cmd, args);
            } else {
              wait(&pid);
            }
          } else {
            printf("%s\n", buf);
          }
        }
        continue;
      }
      // avoid infinite loop
      if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
        continue;
      }
      find(buf, target_name, cmd);
    }
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
  if(argc < 3) {
    fprintf(2, "Usage: find <path> <file_name> [-exec] [cmd] [args]...\n");
    exit(1);
  }
  if (argc > 4 && strcmp(argv[3], "-exec") == 0) {
    for (int i = 4; i < argc; i++) {
       args[arg_count++] = argv[i];
    }
    find(argv[1], argv[2], argv[4]);
  } else {
    find(argv[1], argv[2], 0);
  }
  exit(0);
}
