#include "cpu/exec.h"

make_EHelper(mov);
make_EHelper(sub);
make_EHelper(xor);
make_EHelper(push_r);
make_EHelper(pop_r);
make_EHelper(call);
make_EHelper(ret);

make_EHelper(operand_size);

make_EHelper(inv);
make_EHelper(nemu_trap);

make_EHelper(in);
make_EHelper(out);
