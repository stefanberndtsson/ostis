#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "mfp.h"
#include "floppy.h"
#include "mmu.h"

#define FDCSIZE 16
#define FDCBASE 0xff8600

#define FDC_INSTR  0
#define FDC_STATUS 1
#define FDC_TRACK  2
#define FDC_SECTOR 3
#define FDC_DATA   4
#define FDC_DMASEC 5

#define FDCC_READ   ((~fdc_control)&0x100)
#define FDCC_WRITE  (fdc_control&0x100)
#define FDCC_HDC    ((~fdc_control&0x080))
#define FDCC_FDC    (fdc_control&0x080)
#define FDCC_DMAON  ((~fdc_control&0x040))
#define FDCC_DMAOFF (fdc_control&0x040)
#define FDCC_FDCREG ((~fdc_control&0x010))
#define FDCC_DMASEC (fdc_control&0x010)
#define FDCC_FDCXXX ((~fdc_control&0x008))
#define FDCC_HDCXXX (fdc_control&0x008)
#define FDCC_INSTR  ((fdc_control&0x6) == 0)
#define FDCC_TRACK  ((fdc_control&0x6) == 2)
#define FDCC_SECTOR ((fdc_control&0x6) == 4)
#define FDCC_DATA   ((fdc_control&0x6) == 6)

#define FDCI (fdc_reg[FDC_INSTR])

#define FDCI_SEEK0    ((FDCI < 0x80) && ((FDCI&0xf0) == 0x00))
#define FDCI_SEEK     ((FDCI < 0x80) && ((FDCI&0xf0) == 0x10))
#define FDCI_STEP     ((FDCI < 0x80) && ((FDCI&0xe0) == 0x20))
#define FDCI_STEPIN   ((FDCI < 0x80) && ((FDCI&0xe0) == 0x40))
#define FDCI_STEPOUT  ((FDCI < 0x80) && ((FDCI&0xe0) == 0x60))
#define FDCI_READSEC  ((FDCI&0xe0) == 0x80)
#define FDCI_WRITESEC ((FDCI&0xe0) == 0xa0)
#define FDCI_READADDR ((FDCI&0xf0) == 0xc0)
#define FDCI_READTRK  ((FDCI&0xf0) == 0xe0)
#define FDCI_WRITETRK ((FDCI&0xf0) == 0xf0)
#define FDCI_ABORT    ((FDCI&0xf0) == 0xd0)

#define FDCI_UPDTRK   (fdc_reg[FDC_INSTR]&0x10)
#define FDCI_SEEK2MS  ((fdc_reg[FDC_INSTR]&0x03) == 0x00)
#define FDCI_SEEK3MS  ((fdc_reg[FDC_INSTR]&0x03) == 0x01)
#define FDCI_SEEK5MS  ((fdc_reg[FDC_INSTR]&0x03) == 0x02)
#define FDCI_SEEK6MS  ((fdc_reg[FDC_INSTR]&0x03) == 0x03)
#define FDCI_VERIFY   (fdc_reg[FDC_INSTR]&0x04)
#define FDCI_NOSPINUP (fdc_reg[FDC_INSTR]&0x08)

#define FDCI_MULTSEC  (fdc_reg[FDC_INSTR]&0x10)
#define FDCI_DELAY30  (fdc_reg[FDC_INSTR]&0x04)

#define FDCI_ABORTNOW ((fdc_reg[FDC_INSTR]&0x0c) == 0x00)
#define FDCI_INTINDEX ((fdc_reg[FDC_INSTR]&0x0c) == 0x04)
#define FDCI_ABORTINT ((fdc_reg[FDC_INSTR]&0x0c) == 0x08)

#define FDC_STEP_IN 1
#define FDC_STEP_OUT -1

static int fdc_reg[6];
static int fdc_reg_active;

static int fdc_rwmode = 0;
static int fdc_rwlast = 0;
static int fdc_stepdir = FDC_STEP_OUT;
static int fdc_control;
static BYTE fdc_dmastatus;
static LONG fdc_dmaaddr;

static int write_word_flag = 0;

static void fdc_parse_control()
{
  printf("DEBUG: Running fdc_parse_control()\n");
  
  if(FDCC_FDCREG) {
    fdc_reg_active = 1+((fdc_control&0x6)>>1);
    if(fdc_reg_active == FDC_STATUS) fdc_reg_active = FDC_INSTR;
    printf("DEBUG: Selecting register %d\n", fdc_reg_active);
  } else if(FDCC_DMASEC) {
    fdc_reg_active = FDC_DMASEC;
    printf("DEBUG: Selecting sector count register\n");
  }
  if(FDCC_READ) {
    if(fdc_rwlast == 1) fdc_dmastatus = 0x1;
    fdc_rwlast = fdc_rwmode;
    fdc_rwmode = 0;
    printf("DEBUG: Read mode\n");
  } else if(FDCC_WRITE) {
    if(fdc_rwlast == 0) fdc_dmastatus = 0x1;
    fdc_rwlast = fdc_rwmode;
    fdc_rwmode = 1;
    printf("DEBUG: Write mode\n");
  }
}

static void fdc_do_instr()
{
  mfp_set_GPIP(5);

  fdc_reg[FDC_STATUS] = 0xc0; /* Motor on, Writeprotected */

  printf("DEBUG: Running command: %08x\n", fdc_reg[FDC_INSTR]);

  if(FDCI_SEEK0) {
    if(FDCI_UPDTRK) fdc_reg[FDC_TRACK] = 0;
    printf("DEBUG: FDCI_SEEK0\n");
    if(floppy_seek(0) == FLOPPY_ERROR) {
      fdc_reg[FDC_STATUS] |= 0x10;
    } else {
      fdc_reg[FDC_STATUS] |= 0x04;
    }
  } else if(FDCI_SEEK) {
    printf("DEBUG: FDCI_SEEK: %d\n", fdc_reg[FDC_DATA]);
    if(floppy_seek(fdc_reg[FDC_DATA]) == FLOPPY_ERROR) {
      fdc_reg[FDC_STATUS] |= 0x10;
    } else {
      if(FDCI_UPDTRK) fdc_reg[FDC_TRACK] = fdc_reg[FDC_DATA];
      if(fdc_reg[FDC_TRACK] < 0) fdc_reg[FDC_TRACK] = 0;
      if(fdc_reg[FDC_TRACK] > 240) fdc_reg[FDC_TRACK] = 240;
      if(fdc_reg[FDC_DATA] == 0)
	fdc_reg[FDC_STATUS] |= 0x04;
    }
  } else if(FDCI_STEP) {
    printf("DEBUG: FDCI_STEP: %d\n", fdc_stepdir);
    if(floppy_seek_rel(fdc_stepdir) == FLOPPY_ERROR) {
      fdc_reg[FDC_STATUS] |= 0x10;
    } else {
      if(FDCI_UPDTRK) fdc_reg[FDC_TRACK] += fdc_stepdir;
      if(fdc_reg[FDC_TRACK] < 0) fdc_reg[FDC_TRACK] = 0;
      if(fdc_reg[FDC_TRACK] > 240) fdc_reg[FDC_TRACK] = 240;
      if((fdc_reg[FDC_TRACK]+fdc_stepdir) <= 0)
	fdc_reg[FDC_STATUS] |= 0x04;
    }
  } else if(FDCI_STEPIN) {
    printf("DEBUG: FDCI_STEPIN\n");
    if(floppy_seek(FDC_STEP_IN) == FLOPPY_ERROR) {
      fdc_reg[FDC_STATUS] |= 0x10;
    } else {
      if(FDCI_UPDTRK) fdc_reg[FDC_TRACK] += FDC_STEP_IN;
      if(fdc_reg[FDC_TRACK] > 240) fdc_reg[FDC_TRACK] = 240;
      if((fdc_reg[FDC_TRACK]+FDC_STEP_IN) <= 0)
	fdc_reg[FDC_STATUS] |= 0x04;
    }
  } else if(FDCI_STEPOUT) {
    printf("DEBUG: FDCI_STEPIN\n");
    if(floppy_seek(FDC_STEP_OUT) == FLOPPY_ERROR) {
      fdc_reg[FDC_STATUS] |= 0x10;
    } else {
      if(FDCI_UPDTRK) fdc_reg[FDC_TRACK] += FDC_STEP_OUT;
      if(fdc_reg[FDC_TRACK] < 0) fdc_reg[FDC_TRACK] = 0;
      if((fdc_reg[FDC_TRACK]+FDC_STEP_OUT) <= 0)
	fdc_reg[FDC_STATUS] |= 0x04;
    }
  } else if(FDCI_READSEC) {
    printf("DEBUG: FDCI_READSEC: 0x%x - %d blocks\n",
	   fdc_dmaaddr, fdc_reg[FDC_DMASEC]);
    fdc_dmastatus = 0x3; /* Sector count != 0, No error */
    if(floppy_read_sector(fdc_dmaaddr, fdc_reg[FDC_DMASEC]) == FLOPPY_ERROR) {
      fdc_dmastatus = 0x0; /* Error */
    } else {
      if(FDCI_MULTSEC) {
	printf("Multisector. Do something here\n");
      }
    }
  } else if(FDCI_WRITESEC) {
    printf("Write sector not supported\n");
  } else if(FDCI_READTRK) {
    printf("Read track not supported\n");
  } else if(FDCI_WRITETRK) {
    printf("Write track not supported\n");
  } else if(FDCI_ABORT) {
    printf("Abort not supported\n");
  }
  mfp_clr_GPIP(5);
}

static BYTE fdc_read_byte(LONG addr)
{
  switch(addr) {
  case 0xff8604:
    return 0xff;
  case 0xff8605:
    printf("DEBUG: Reading register %d == %02x\n", fdc_reg_active,
	   fdc_reg[fdc_reg_active]);
    if(fdc_reg_active == 0) {
      printf("DEBUG: Returning Status: %02x\n", fdc_reg[FDC_STATUS]);
      return fdc_reg[FDC_STATUS];
    } else {
      printf("DEBUG: Returning Register: %02x\n", fdc_reg[FDC_STATUS]);
      return fdc_reg[fdc_reg_active];
    }
  case 0xff8606:
    return 0;
  case 0xff8607:
    return fdc_dmastatus;
  case 0xff8609:
    return (fdc_dmaaddr&0xff0000)>>16;
  case 0xff860b:
    return (fdc_dmaaddr&0xff00)>>8;
  case 0xff860d:
    return fdc_dmaaddr&0xff;
  default:
    return 0;
  }
}

static WORD fdc_read_word(LONG addr)
{
  return (fdc_read_byte(addr)<<8)|fdc_read_byte(addr+1);
}

static LONG fdc_read_long(LONG addr)
{
  return ((fdc_read_byte(addr)<<24)|
	  (fdc_read_byte(addr+1)<<16)|
	  (fdc_read_byte(addr+2)<<8)|
	  (fdc_read_byte(addr+3)));
}

static void fdc_write_byte(LONG addr, BYTE data)
{
  switch(addr) {
  case 0xff8605:
    printf("Writing register %d with %02x\n", fdc_reg_active, data);
    fdc_reg[fdc_reg_active] = data;
    if(fdc_reg_active == FDC_INSTR)
      fdc_do_instr();
    break;
  case 0xff8606:
    printf("Writing control with %02x--\n", data);
    fdc_control = (fdc_control&0xff)|(data<<8);
    if(!write_word_flag)
      fdc_parse_control();
    break;
  case 0xff8607:
    printf("Writing control with --%02x\n", data);
    fdc_control = (fdc_control&0xff00)|data;
    if(!write_word_flag)
      fdc_parse_control();
    break;
  case 0xff8609:
    printf("Writing DMA with %02x----\n", data);
    fdc_dmaaddr = (fdc_dmaaddr&0xffff)|(data<<16);
    break;
  case 0xff860b:
    printf("Writing DMA with --%02x--\n", data);
    fdc_dmaaddr = (fdc_dmaaddr&0xff00ff)|(data<<8);
    break;
  case 0xff860d:
    printf("Writing DMA with ----%02x\n", data);
    fdc_dmaaddr = (fdc_dmaaddr&0xffff00)|data;
    break;
  }
}

static void fdc_write_word(LONG addr, WORD data)
{
  write_word_flag = 1;
  fdc_write_byte(addr, (data&0xff00)>>8);
  fdc_write_byte(addr+1, (data&0xff));
  write_word_flag = 0;
  if(addr == 0xff8606)
    fdc_parse_control();
}

static void fdc_write_long(LONG addr, LONG data)
{
#if 0
  fdc_write_byte(addr, (data&0xff000000)>>24);
  fdc_write_byte(addr+1, (data&0xff0000)>>16);
  fdc_write_byte(addr+2, (data&0xff00)>>8);
  fdc_write_byte(addr+3, (data&0xff));
#endif
  fdc_write_word(addr, (data&0xffff0000)>>16);
  fdc_write_word(addr+2, data&0xffff);
}

void fdc_init()
{
  struct mmu *fdc;

  fdc = (struct mmu *)malloc(sizeof(struct mmu));
  if(!fdc) {
    return;
  }
  fdc->start = FDCBASE;
  fdc->size = FDCSIZE;
  fdc->name = strdup("FDC");
  fdc->read_byte = fdc_read_byte;
  fdc->read_word = fdc_read_word;
  fdc->read_long = fdc_read_long;
  fdc->write_byte = fdc_write_byte;
  fdc->write_word = fdc_write_word;
  fdc->write_long = fdc_write_long;

  fdc_reg[FDC_STATUS] = 0xc0;

  mmu_register(fdc);
}
