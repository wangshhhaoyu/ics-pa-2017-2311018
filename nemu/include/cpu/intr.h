#ifndef __CPU_INTR_H__
#define __CPU_INTR_H__

#include "common.h"

/* 触发中断/异常，NO 为中断向量号，ret_addr 为中断返回后应该继续执行的地址（通常为当前指令的下一条） */
void raise_intr(uint8_t NO, vaddr_t ret_addr);

#endif
