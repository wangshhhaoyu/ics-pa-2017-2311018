#include "common.h"
#include "fs.h"
#include "ramdisk.h"

#define DEFAULT_ENTRY ((void *)0x4000000)

uintptr_t loader(_Protect *as, const char *filename) {
  // 直接从 ramdisk 读取整个镜像到 DEFAULT_ENTRY
  size_t ramdisk_size = get_ramdisk_size();
  ramdisk_read((void *)DEFAULT_ENTRY, 0, ramdisk_size);
  return (uintptr_t)DEFAULT_ENTRY;
}
