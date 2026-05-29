#include "common.h"
#include "syscall.h"

_RegSet* do_syscall(_RegSet *r);  // forward declaration

static _RegSet* do_event(_Event e, _RegSet* r) {
  switch (e.event) {
    case _EVENT_SYSCALL:
      return do_syscall(r);
    default:
      Log("Unhandled event ID = %d, ignoring", e.event);
      return r;
  }
}

void init_irq(void) {
  _asye_init(do_event);
}
