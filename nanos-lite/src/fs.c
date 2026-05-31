#include "fs.h"
#include <sys/types.h>   // for off_t

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void ramdisk_read(void *buf, off_t offset, size_t len);
void ramdisk_write(const void *buf, off_t offset, size_t len);
size_t events_read(void *buf, size_t len);
void dispinfo_read(void *buf, off_t offset, size_t len);
void fb_write(const void *buf, off_t offset, size_t len);

int fs_open(const char *pathname, int flags, int mode)
{
  (void)flags;
  (void)mode;

  int idx = 0;
  while (idx < NR_FILES) {
    if (strcmp(pathname, file_table[idx].name) == 0) {
      file_table[idx].open_offset = 0;
      return idx;
    }
    idx++;
  }
  assert(0);
  return -1;
}

size_t fs_filesz(int fd)
{
  assert(fd >= 0 && fd < NR_FILES);
  return file_table[fd].size;
}

size_t fs_read(int fd, void *buf, size_t len)
{
  assert(fd >= 0 && fd < NR_FILES);
  assert(buf != NULL);

  switch (fd) {
    case FD_STDIN:
    case FD_STDOUT:
    case FD_STDERR:
      return 0;
    case FD_EVENTS:
      return events_read(buf, len);
    case FD_DISPINFO: {
      size_t max_len = file_table[fd].size - file_table[fd].open_offset;
      len = (len < max_len) ? len : max_len;
      dispinfo_read(buf, file_table[fd].open_offset, len);
      file_table[fd].open_offset += len;
      return len;
    }
    default:
      break;
  }

  size_t max_len = file_table[fd].size - file_table[fd].open_offset;
  len = (len < max_len) ? len : max_len;

  ramdisk_read(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  file_table[fd].open_offset += len;
  return len;
}

size_t fs_write(int fd, const void *buf, size_t len)
{
  assert(fd >= 0 && fd < NR_FILES);
  assert(buf != NULL);

  switch (fd) {
    case FD_STDOUT:
    case FD_STDERR: {
      const char *ptr = buf;
      size_t cnt = 0;
      while (cnt < len) {
        _putc(ptr[cnt]);
        cnt++;
      }
      return cnt;
    }
    case FD_STDIN:
    case FD_EVENTS:
    case FD_DISPINFO:
      return 0;
    case FD_FB: {
      size_t max_len = file_table[fd].size - file_table[fd].open_offset;
      len = (len < max_len) ? len : max_len;
      fb_write(buf, file_table[fd].open_offset, len);
      file_table[fd].open_offset += len;
      return len;
    }
    default:
      break;
  }

  size_t max_len = file_table[fd].size - file_table[fd].open_offset;
  len = (len < max_len) ? len : max_len;

  ramdisk_write(buf, file_table[fd].disk_offset + file_table[fd].open_offset, len);
  file_table[fd].open_offset += len;
  return len;
}

off_t fs_lseek(int fd, off_t offset, int whence)
{
  assert(fd >= 0 && fd < NR_FILES);

  // 对设备文件，允许任意偏移，不进行边界检查
  if (fd == FD_STDIN || fd == FD_STDOUT || fd == FD_STDERR ||
      fd == FD_FB || fd == FD_EVENTS || fd == FD_DISPINFO) {
    off_t new_offset = 0;   // 初始化避免警告
    switch (whence) {
      case SEEK_SET:
        new_offset = offset;
        break;
      case SEEK_CUR:
        new_offset = file_table[fd].open_offset + offset;
        break;
      case SEEK_END:
        new_offset = file_table[fd].size + offset;
        break;
      default:
        assert(0);
        new_offset = 0;   // 不会执行，但消除警告
    }
    if (new_offset < 0) new_offset = 0;
    file_table[fd].open_offset = new_offset;
    return new_offset;
  }

  // 普通文件的原有逻辑
  off_t base = 0;
  switch (whence) {
    case SEEK_SET:
      base = 0;
      break;
    case SEEK_CUR:
      base = file_table[fd].open_offset;
      break;
    case SEEK_END:
      base = file_table[fd].size;
      break;
    default:
      assert(0);
  }

  off_t new_offset = base + offset;
  assert(new_offset >= 0);
  assert(new_offset <= file_table[fd].size);
  file_table[fd].open_offset = new_offset;
  return new_offset;
}

int fs_close(int fd)
{
  assert(fd >= 0 && fd < NR_FILES);
  file_table[fd].open_offset = 0;
  return 0;
}

void init_fs() {
  file_table[FD_FB].size = _screen.width * _screen.height * sizeof(uint32_t);
  file_table[FD_EVENTS].size = 4096;   // 为 /dev/events 设置一个合理大小
}