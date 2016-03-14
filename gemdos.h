#ifndef GEMDOS_H
#define GEMDOS_H

#include "common.h"
#include "cpu.h"

#define GEMDOS_RESUME_CALL 1
#define GEMDOS_ABORT_CALL 0

int gemdos_hd_drive();
int gemdos_hd(struct cpu *);
void gemdos_print(struct cpu *);

#endif
