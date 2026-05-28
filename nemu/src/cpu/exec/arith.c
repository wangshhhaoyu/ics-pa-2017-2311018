#include "cpu/exec.h"

make_EHelper(add) {
  rtl_add(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  // Update CF
  rtl_sltu(&t0, &t2, &id_dest->val);
  rtl_set_CF(&t0);

  // Update OF: overflow if both operands have same sign but result has different sign
  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_not(&t0);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template2(add);
}

make_EHelper(sub) {
  rtl_sub(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  // Update CF
  rtl_sltu(&t0, &id_dest->val, &id_src->val);
  rtl_set_CF(&t0);

  // Update OF
  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template2(sub);
}

make_EHelper(cmp) {
  rtl_sub(&t2, &id_dest->val, &id_src->val);
  rtl_update_ZFSF(&t2, id_dest->width);

  // Update CF
  rtl_sltu(&t0, &id_dest->val, &id_src->val);
  rtl_set_CF(&t0);

  // Update OF
  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template2(cmp);
}

make_EHelper(inc) {
  rtl_addi(&t2, &id_dest->val, 1);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  // inc does not affect CF
  // Update OF: overflow if dest was positive and result is negative
  rtl_xor(&t0, &id_dest->val, &t2);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template1(inc);
}

make_EHelper(inc_r) {
  int reg = decoding.opcode & 0x7;
  rtl_lr(&t1, reg, 4);
  rtl_addi(&t2, &t1, 1);
  rtl_sr(reg, 4, &t2);
  rtl_update_ZFSF(&t2, 4);

  rtl_xor(&t0, &t1, &t2);
  rtl_msb(&t0, &t0, 4);
  rtl_set_OF(&t0);

  print_asm("inc %s", reg_name(reg, 4));
}

make_EHelper(dec) {
  rtl_subi(&t2, &id_dest->val, 1);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  // dec does not affect CF
  // Update OF: overflow if dest was negative and result is positive
  rtl_xor(&t0, &id_dest->val, &t2);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template1(dec);
}

make_EHelper(dec_r) {
  int reg = decoding.opcode & 0x7;
  rtl_lr(&t1, reg, 4);
  rtl_subi(&t2, &t1, 1);
  rtl_sr(reg, 4, &t2);
  rtl_update_ZFSF(&t2, 4);

  rtl_xor(&t0, &t1, &t2);
  rtl_msb(&t0, &t0, 4);
  rtl_set_OF(&t0);

  print_asm("dec %s", reg_name(reg, 4));
}

make_EHelper(neg) {
  rtl_sub(&t2, &tzero, &id_dest->val);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  // Update CF: CF = (dest != 0)
  rtl_neq0(&t0, &id_dest->val);
  rtl_set_CF(&t0);

  // Update OF: overflow if dest is minimum negative value
  rtl_xor(&t0, &id_dest->val, &t2);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template1(neg);
}

make_EHelper(adc) {
  rtl_add(&t2, &id_dest->val, &id_src->val);
  rtl_sltu(&t3, &t2, &id_dest->val);
  rtl_get_CF(&t1);
  rtl_add(&t2, &t2, &t1);
  rtl_sltu(&t0, &t2, &t1);
  rtl_or(&t3, &t3, &t0);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  // Update CF
  rtl_set_CF(&t3);

  // Update OF
  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_not(&t0);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template2(adc);
}

make_EHelper(sbb) {
  rtl_sub(&t2, &id_dest->val, &id_src->val);
  rtl_sltu(&t3, &id_dest->val, &t2);
  rtl_get_CF(&t1);
  rtl_sub(&t2, &t2, &t1);
  rtl_sltu(&t0, &id_dest->val, &t1);
  rtl_or(&t3, &t3, &t0);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  // Update CF
  rtl_set_CF(&t3);

  // Update OF
  rtl_xor(&t0, &id_dest->val, &id_src->val);
  rtl_xor(&t1, &id_dest->val, &t2);
  rtl_and(&t0, &t0, &t1);
  rtl_msb(&t0, &t0, id_dest->width);
  rtl_set_OF(&t0);

  print_asm_template2(sbb);
}

make_EHelper(mul) {
  rtl_lr_l(&t0, R_EAX);
  rtl_mul(&t2, &t3, &t0, &id_dest->val);
  rtl_sr_l(R_EAX, &t3);
  rtl_sr_l(R_EDX, &t2);

  print_asm_template2(mul);
}

make_EHelper(imul1) {
  rtl_lr_l(&t0, R_EAX);
  rtl_imul(&t2, &t3, &t0, &id_dest->val);
  rtl_sr_l(R_EAX, &t3);
  rtl_sr_l(R_EDX, &t2);

  print_asm_template2(imul);
}

make_EHelper(imul2) {
  rtl_imul(&t0, &t1, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t1);

  print_asm_template3(imul);
}

make_EHelper(imul3) {
  rtl_sext(&t0, &id_src->val, id_src->width);
  rtl_imul(&t1, &t2, &id_dest->val, &t0);
  operand_write(id_dest, &t1);

  print_asm_template3(imul);
}

make_EHelper(div) {
  rtl_lr_l(&t0, R_EAX);
  rtl_lr_l(&t1, R_EDX);
  rtl_div(&t2, &t3, &t1, &t0, &id_dest->val);
  rtl_sr_l(R_EAX, &t2);
  rtl_sr_l(R_EDX, &t3);

  print_asm_template2(div);
}

make_EHelper(idiv) {
  rtl_lr_l(&t0, R_EAX);
  rtl_lr_l(&t1, R_EDX);
  rtl_idiv(&t2, &t3, &t1, &t0, &id_dest->val);
  rtl_sr_l(R_EAX, &t2);
  rtl_sr_l(R_EDX, &t3);

  print_asm_template2(idiv);
}
