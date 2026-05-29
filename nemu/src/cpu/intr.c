#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  // TODO();
  memcpy(&t1,&cpu.eflags,sizeof(cpu.eflags));
  rtl_li(&t0,t1);
  rtl_push(&t0);//eflags
  rtl_push(&cpu.cs);//cs
  rtl_li(&t0,ret_addr);
  rtl_push(&t0);//eip

  //在IDT中找到中断号NO对应的门描述符首地址
  vaddr_t gate_addr=cpu.idtr.base+NO*sizeof(GateDesc);
  Log("gate_addr=%d, cpu.idtr.base=%d, cpu.idtr.limit=%d",gate_addr,cpu.idtr.base,cpu.idtr.limit);
  assert(gate_addr<=cpu.idtr.base+cpu.idtr.limit);//通过

  //根据门描述符的offset计算目标地址，由两部分拼接而成
  uint32_t off_15_0=vaddr_read(gate_addr,2);
  uint32_t off_32_16=vaddr_read(gate_addr+sizeof(GateDesc)-2,2);
  uint32_t target_addr=(off_32_16<<16)+off_15_0;

  decoding.is_jmp=1;
  decoding.jmp_eip=target_addr;
}

void dev_raise_intr() {
}
