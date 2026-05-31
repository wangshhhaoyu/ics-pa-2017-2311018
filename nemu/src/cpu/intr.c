#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  vaddr_t gate_addr = cpu.idtr.base + NO * sizeof(GateDesc);
  
  assert(gate_addr + sizeof(GateDesc) - 1 <= cpu.idtr.base + cpu.idtr.limit);

  uint32_t gate_lo = vaddr_read(gate_addr, 4);
  uint32_t gate_hi = vaddr_read(gate_addr + 4, 4);
  vaddr_t intr_addr = (gate_lo & 0xffff) | (gate_hi & 0xffff0000);

  rtlreg_t cs = cpu.cs;
  rtl_push(&cpu.eflags, 4);
  rtl_push(&cs, 4);
  rtl_push(&ret_addr, 4);

  decoding.is_jmp = 1;
  decoding.jmp_eip = intr_addr;
}

void dev_raise_intr() {
  const uint8_t IRQ_TIMER = 32;
  
  if (cpu.Eflags.IF) {
    raise_intr(IRQ_TIMER, cpu.eip);
  }
}
