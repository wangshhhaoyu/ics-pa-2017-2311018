#include "common.h"
#include "fs.h"
#include "ramdisk.h"

int fs_open(const char *pathname, int flags, int mode) {
  Log("Warning: fs_open called with %s, returning -1", pathname);
  return -1;
}

ssize_t fs_read(int fd, void *buf, size_t len) {
  return -1;
}

ssize_t fs_write(int fd, void *buf, size_t len) {
  if (fd == 1 || fd == 2) {
    for (size_t i = 0; i < len; i++) _putc(((char*)buf)[i]);
    return len;
  }
  return -1;
}

off_t fs_lseek(int fd, off_t offset, int whence) {
  return -1;
}

int fs_close(int fd) {
  return 0;
}

void init_fs() {
  Log("init_fs called (minimal version)");
}

size_t fs_filesz(int fd) {
  return 0;  // dummy 不需要实际大小
}
