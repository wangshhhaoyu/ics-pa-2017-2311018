#ifndef RAMDISK_H
#define RAMDISK_H

#include <stddef.h>   // size_t, off_t

void ramdisk_read(void *buf, off_t offset, size_t len);
size_t get_ramdisk_size(void);

#endif
