#include "cpu/exec.h"

make_EHelper(mov) {
  operand_write(id_dest, &id_src->val);
  print_asm_template2(mov);
}

make_EHelper(push) {
  rtl_push(&id_dest->val);
  print_asm_template1(push);
}

make_EHelper(push_r) {
  int reg = decoding.opcode & 0x7;
  rtl_lr(&t0, reg, 4);
  rtl_push(&t0);
  print_asm("push %s", reg_name(reg, 4));
}

make_EHelper(pop) {
  rtl_pop(&t0);
  operand_write(id_dest, &t0);
  print_asm_template1(pop);
}

make_EHelper(pop_r) {
  int reg = decoding.opcode & 0x7;
  rtl_pop(&t0);
  rtl_sr(reg, 4, &t0);
  print_asm("pop %s", reg_name(reg, 4));
}

make_EHelper(pusha) {
  rtl_lr_l(&t0, R_ESP);
  rtl_push(&cpu.eax);
  rtl_push(&cpu.ecx);
  rtl_push(&cpu.edx);
  rtl_push(&cpu.ebx);
  rtl_push(&t0);
  rtl_push(&cpu.ebp);
  rtl_push(&cpu.esi);
  rtl_push(&cpu.edi);
  print_asm("pusha");
}

make_EHelper(popa) {
  rtl_pop(&cpu.edi);
  rtl_pop(&cpu.esi);
  rtl_pop(&cpu.ebp);
  rtl_pop(&t0);  // skip ESP
  rtl_pop(&cpu.ebx);
  rtl_pop(&cpu.edx);
  rtl_pop(&cpu.ecx);
  rtl_pop(&cpu.eax);
  print_asm("popa");
}

make_EHelper(leave) {
  rtl_lr_l(&t0, R_EBP);
  rtl_sr_l(R_ESP, &t0);
  rtl_pop(&t0);
  rtl_sr_l(R_EBP, &t0);
  print_asm("leave");
}

make_EHelper(cltd) {
  if (decoding.is_operand_size_16) {
    rtl_lr_w(&t0, R_AX);
    if (t0 & 0x8000) {
      rtl_li(&t1, 0xffff);
      rtl_sr_w(R_DX, &t1);
    } else {
      rtl_sr_w(R_DX, &tzero);
    }
  }
  else {
    rtl_lr_l(&t0, R_EAX);
    if (t0 & 0x80000000) {
      rtl_li(&t1, 0xffffffff);
      rtl_sr_l(R_EDX, &t1);
    } else {
      rtl_sr_l(R_EDX, &tzero);
    }
  }

  print_asm(decoding.is_operand_size_16 ? "cwtl" : "cltd");
}

make_EHelper(cwtl) {
  if (decoding.is_operand_size_16) {
    rtl_lr_b(&t0, R_AL);
    rtl_sext(&t0, &t0, 1);
    rtl_sr_w(R_AX, &t0);
  }
  else {
    rtl_lr_w(&t0, R_AX);
    rtl_sext(&t0, &t0, 2);
    rtl_sr_l(R_EAX, &t0);
  }

  print_asm(decoding.is_operand_size_16 ? "cbtw" : "cwtl");
}

make_EHelper(movsx) {
  rtl_sext(&t0, &id_src->val, id_src->width);
  operand_write(id_dest, &t0);
  print_asm_template2(movsx);
}

make_EHelper(movzx) {
  rtl_li(&t0, id_src->val);
  operand_write(id_dest, &t0);
  print_asm_template2(movzx);
}

make_EHelper(lea) {
  operand_write(id_dest, &id_src->addr);
  print_asm_template2(lea);
}
make_EHelper(movs) {
    int width = decoding.is_operand_size_16 ? 2 : 4;

    rtl_lm(&t0, &cpu.esi, width);
    rtl_sm(&cpu.edi, width, &t0);

    if (cpu.eflags.DF) {
      rtl_subi(&cpu.esi, &cpu.esi, width);
      rtl_subi(&cpu.edi, &cpu.edi, width);
    } else {
      rtl_addi(&cpu.esi, &cpu.esi, width);
      rtl_addi(&cpu.edi, &cpu.edi, width);
    }

    print_asm("movs");
  }

  make_EHelper(stos) {
    int width = decoding.is_operand_size_16 ? 2 : 4;

    rtl_lr_l(&t0, R_EAX);
    rtl_sm(&cpu.edi, width, &t0);

    if (cpu.eflags.DF) {
      rtl_subi(&cpu.edi, &cpu.edi, width);
    } else {
      rtl_addi(&cpu.edi, &cpu.edi, width);
    }

    print_asm("stos");
  }

  make_EHelper(lods) {
    int width = decoding.is_operand_size_16 ? 2 : 4;

    rtl_lm(&t0, &cpu.esi, width);
    rtl_sr_l(R_EAX, &t0);

    if (cpu.eflags.DF) {
      rtl_subi(&cpu.esi, &cpu.esi, width);
    } else {
      rtl_addi(&cpu.esi, &cpu.esi, width);
    }

    print_asm("lods");
  }

  make_EHelper(cmps) {
    int width = decoding.is_operand_size_16 ? 2 : 4;

    rtl_lm(&t0, &cpu.esi, width);
    rtl_lm(&t1, &cpu.edi, width);
    rtl_sub(&t2, &t0, &t1);
    rtl_update_ZFSF(&t2, width);

    if (cpu.eflags.DF) {
      rtl_subi(&cpu.esi, &cpu.esi, width);
      rtl_subi(&cpu.edi, &cpu.edi, width);
    } else {
      rtl_addi(&cpu.esi, &cpu.esi, width);
      rtl_addi(&cpu.edi, &cpu.edi, width);
    }

    print_asm("cmps");
  }

  make_EHelper(scas) {
    int width = decoding.is_operand_size_16 ? 2 : 4;

    rtl_lr_l(&t0, R_EAX);
    rtl_lm(&t1, &cpu.edi, width);
    rtl_sub(&t2, &t0, &t1);
    rtl_update_ZFSF(&t2, width);

    if (cpu.eflags.DF) {
      rtl_subi(&cpu.edi, &cpu.edi, width);
    } else {
      rtl_addi(&cpu.edi, &cpu.edi, width);
    }

    print_asm("scas");
  }
