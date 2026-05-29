#include "common.h"
#include "syscall.h"
#include "am.h"

_RegSet* do_syscall(_RegSet *r) {
    uint32_t syscall_no = SYSCALL_ARG1(r);
    switch (syscall_no) {
        case SYS_none:
            r->eax = 1;
            break;
        case SYS_exit:
            _halt(SYSCALL_ARG2(r));
            break;
        default:
            panic("Unhandled syscall ID = %d", syscall_no);
    }
    return r;
}
