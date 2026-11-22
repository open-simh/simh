#include "linc_defs.h"

/* The classic LINC had two tape drives.  The micro-LINC could have
 * four, numbered 0, 1, 4, and 5.  For simplicity, we have six units
 * but 2 and 3 are invalid. */
#define TAPE_UNITS  6

#define POS     u3
#define SPEED   u4
#define ACC     u5
#define OFFSET  u6
#define IBZ     us9

#define P (*(uint16 *)cpu_reg[0].loc)
#define C (*(uint16 *)cpu_reg[1].loc)
#define A (*(uint16 *)cpu_reg[2].loc)
#define S (*(uint16 *)cpu_reg[6].loc)
#define B (*(uint16 *)cpu_reg[7].loc)
#define LSW (*(uint16 *)cpu_reg[8].loc)
#define RSW (*(uint16 *)cpu_reg[9].loc)
#define paused (*(int *)cpu_reg[11].loc)
#define CPU_IBZ (*(int *)cpu_reg[12].loc)

#define ACC_START    3
#define ACC_REVERSE  6
#define ACC_STOP     1
#define MAX_SPEED    (ACC_START * 625)   /* 0.1s / 160µs */
#define IBZ_WORDS    5
#define DATA_WORDS   256
#define OTHER_WORDS  7
#define BLOCK_WORDS  (IBZ_WORDS + DATA_WORDS + OTHER_WORDS)
#define START_POS    (ACC_START * (625 + (625 * 625))/2)
#define MAX_BLOCKS   512
#define MAX_POS      ((BLOCK_WORDS * MAX_BLOCKS + IBZ_WORDS) * MAX_SPEED)

#define GOOD_CHECKSUM  07777

#define RDC  0  /* read tape and check */
#define RCG  1  /* read tape group */
#define RDE  2  /* read tape */
#define MTB  3  /* move toward block */
#define WRC  4  /* write tape and check */
#define WCG  5  /* write tape group */
#define WRI  6  /* write tape */
#define CHK  7  /* check tape */

#define DBG            0001
#define DBG_INSN       0002
#define DBG_SEEK       0004
#define DBG_READ       0010
#define DBG_WRITE      0020
#define DBG_MOT        0040
#define DBG_POS        0100

static uint16 BLOCK_GROUP;
static uint16 CURRENT_UNIT;
static t_bool FOUND_BLOCK;
static int16 WANTED_BLOCK;

static t_stat tape_svc(UNIT *uptr);
static t_stat tape_reset(DEVICE *dptr);
static t_stat tape_boot(int32 u, DEVICE *dptr);
static t_stat tape_attach(UNIT *uptr, CONST char *cptr);
static t_stat tape_detach(UNIT *uptr);

#define UNIT_FLAGS (UNIT_IDLE|UNIT_FIX|UNIT_ATTABLE|UNIT_DISABLE|UNIT_ROABLE)
#define CAPACITY (MAX_BLOCKS * DATA_WORDS)

static UNIT tape_unit[TAPE_UNITS] = {
  { UDATA(&tape_svc, UNIT_FLAGS, CAPACITY) },
  { UDATA(&tape_svc, UNIT_FLAGS, CAPACITY) },
  { UDATA(&tape_svc, UNIT_DIS,   0) },
  { UDATA(&tape_svc, UNIT_DIS,   0) },
  { UDATA(&tape_svc, UNIT_FLAGS, CAPACITY) },
  { UDATA(&tape_svc, UNIT_FLAGS, CAPACITY) }
};

static DEBTAB tape_deb[] = {
  { "DBG",      DBG },
  { "INSN",     DBG_INSN },
  { "SEEK",     DBG_SEEK },
  { "READ",     DBG_READ },
  { "WRITE",    DBG_WRITE },
  { "MOTION",   DBG_MOT },
  { "POSITION", DBG_POS },
  { NULL, 0 }
};

DEVICE tape_dev = {
  "TAPE", tape_unit, NULL, NULL,
  TAPE_UNITS, 8, 12, 1, 8, 12,
  NULL, NULL, &tape_reset,
  &tape_boot, &tape_attach, &tape_detach,
  NULL, DEV_DEBUG, 0, tape_deb,
  NULL, NULL, NULL, NULL, NULL, NULL
};

void tape_op(void)
{
  uint16 u = (C & 050) >> 3;
  UNIT *uptr = &tape_unit[u];

  if ((uptr->flags & UNIT_ATT) == 0)
    return;

  CURRENT_UNIT = u;

  if (uptr->SPEED < 0) {
    if ((C & 7) != MTB) {
      sim_debug(DBG_MOT, &tape_dev, "Unit %u: Reverse to forward\n", u);
      uptr->ACC = ACC_REVERSE;
    }
  } else if (uptr->POS >= MAX_POS) {
    sim_debug(DBG_MOT, &tape_dev, "Unit %u: End zone; reverse\n", u);
    uptr->ACC = -ACC_REVERSE;
  } else if (uptr->SPEED < MAX_SPEED || uptr->ACC < 0) {
    sim_debug(DBG_MOT, &tape_dev, "Unit %u: Speed up\n", u);
    uptr->ACC = ACC_START;
  }
  if (!sim_is_active(uptr))
    sim_activate(uptr, 20);
  paused = 1;
  A = 0;
  WANTED_BLOCK = B & TMASK;

  switch (C & 7) {
  case RDC: case RDE: case WRC: case WRI: case CHK:
    S = 256 * (B >> 9);
    BLOCK_GROUP = 0;
    sim_debug(DBG_INSN, &tape_dev, "Unit %u: Single transfer: S=%04o, BN=%03o\n",
              u, S, WANTED_BLOCK);
    break;
  case RCG: case WCG:
    S = 256 * (B & 7);
    BLOCK_GROUP = B >> 9;
    sim_debug(DBG_INSN, &tape_dev, "Unit %u: Group transfer: S=%04o, BN=%03o/%o\n",
              u, S, WANTED_BLOCK, BLOCK_GROUP+1);
    break;
  case MTB:
    sim_debug(DBG_INSN, &tape_dev, "Unit %u: Move towards block %03o\n",
              u, WANTED_BLOCK);
    break;
  }
}

static t_stat tape_seek(FILE *fileref, t_addr block, t_addr offset)
{
  offset = DATA_WORDS * block + offset;
  offset *= 2;
  if (sim_fseek(fileref, offset, SEEK_SET) == -1)
    return SCPE_IOERR;
  return SCPE_OK;
}

static uint16 read_word(FILE *fileref, t_addr block, t_addr offset)
{
  t_stat stat;
  uint8 data[2];
  uint16 word;

  stat = tape_seek(fileref, block, offset);
  if (stat != SCPE_OK)
    ;
  if (sim_fread(data, 1, 2, fileref) != 2)
    ;
  if (data[1] & 0xF0)
    ;
  word = data[1];
  word <<= 8;
  word |= data[0];
  return word;
}

static void write_word(FILE *fileref, t_addr block, t_addr offset, uint16 word)
{
  t_stat stat;
  uint8 data[2];

  stat = tape_seek(fileref, block, offset);
  if (stat != SCPE_OK)
    ;
  data[0] = word & 0xFF;
  data[1] = word >> 8;
  if (sim_fwrite(data, 1, 2, fileref) != 2)
    ;
}

/*
  IBZ BN G block CS C C G BN IBZ
   5  1  1  256  1  1 1 1 1   5
      ---------------------
            263
      --------------------------
            268
         

  start - 100 ms
  stop - 300 ms
  reverse - 100 ms
  BN to BN at 60 ips - 43 ms
    block length = 43 ms * 60 inch/s = 2.58 inch

  per word - 160 µs
    word length = 0.0096 inch
    words per inch = 104
    words per second = 6250
  end zone to end zone - 23 s
    tape length = 23 * 60 = 1380 inch = 115 feet
    end zone length = 5 feet

 */

static void tape_done(UNIT *uptr, uint16 block)
{
  uint16 u = uptr - tape_unit;
  sim_debug(DBG_INSN, &tape_dev, "Unit %u: Done with block\n", u);

  switch (C & 7) {
  case RDC: case RCG: case RDE: case CHK:
    A = GOOD_CHECKSUM;
    break;
  case WRI:
    A = (A ^ 07777) + 1;
    A &= WMASK;
    break;
  case MTB:
    A = (WANTED_BLOCK + ~block);
    A += A >> 12;
    A &= WMASK;
    break;
  }

  switch (C & 7) {
  case RDC:
    if (A != GOOD_CHECKSUM) {
      sim_debug(DBG_INSN, &tape_dev, "Unit %u: Check failed; read again\n", u);
      S &= ~0377;
    } else {
      sim_debug(DBG_INSN, &tape_dev, "Unit %u: Check passed\n", u);
      paused = 0;
    }
    break;
  case WRC:
    sim_debug(DBG_INSN, &tape_dev, "Unit %u: Block written, go back and check\n", u);
    // For now, done.
    A = GOOD_CHECKSUM;
    paused = 0;
    break;
  case RCG: case WCG:
    if (BLOCK_GROUP == 0) {
      sim_debug(DBG_INSN, &tape_dev, "Unit %u: Done with group\n", u);
      paused = 0;
    } else {
      sim_debug(DBG_INSN, &tape_dev, "Unit %u: Blocks left in group: %d\n",
                u, BLOCK_GROUP);
      BLOCK_GROUP--;
    }
    WANTED_BLOCK = (WANTED_BLOCK + 1) & TMASK;
    break;
  case RDE: case WRI:
    sim_debug(DBG_INSN, &tape_dev, "Unit %u: Transfer done\n", u);
    paused = 0;
    break;
  case MTB:
    sim_debug(DBG_INSN, &tape_dev, "Unit %u: Move towards block done, result %04o\n",
              u, A);
    paused = 0;
    break;
  case CHK:
    sim_debug(DBG_INSN, &tape_dev, "Unit %u: Check done\n", u);
    paused = 0;
    break;
  }

  if (paused)
    ;
  else if ((C & IMASK) == 0) {
    sim_debug(DBG_MOT, &tape_dev, "Unit %u: Instruction done, stop tape\n", u);
    uptr->ACC = uptr->SPEED > 0 ? -ACC_STOP : ACC_STOP;
  } else {
    sim_debug(DBG_MOT, &tape_dev, "Unit %u: Instruction done, keep moving\n", u);
  }
}

static void tape_word(UNIT *uptr, uint16 block, uint16 offset)
{
  uint16 u = uptr - tape_unit;

  switch (C & 7) {
  case RDC: case RCG: case RDE: case CHK:
    B = read_word(uptr->fileref, block - uptr->OFFSET, offset);
    sim_debug(DBG_READ, &tape_dev,
              "Unit %u: Read block %03o offset %03o data %04o address %04o\n",
              u, block, offset, B, S);
    if ((C & 7) != CHK)
      M[S] = B;
    break;
  case WRC: case WCG: case WRI:
    B = M[S];
    sim_debug(DBG_WRITE, &tape_dev,
              "Unit %u: Write block %03o offset %03o data %04o address %04o\n",
              u, block, offset, B, S);
    write_word(uptr->fileref, block - uptr->OFFSET, offset, B);
    break;
  }
  S = (S+1) & AMASK;
  A += B;
  A &= WMASK;
}

static void tape_motion(UNIT *uptr)
{
  uint16 u = uptr - tape_unit;

  uptr->SPEED += uptr->ACC;
  if (uptr->SPEED >= MAX_SPEED) {
    uptr->SPEED = MAX_SPEED;
    if (uptr->ACC != 0)
      sim_debug(DBG_MOT, &tape_dev, "Unit %u: Tape up to speed\n", u);
    uptr->ACC = 0;
  }
  else if (uptr->SPEED <= -MAX_SPEED) {
    uptr->SPEED = -MAX_SPEED;
    if (uptr->ACC != 0)
      sim_debug(DBG_MOT, &tape_dev, "Unit %u: Tape up to speed\n", u);
    uptr->ACC = 0;
  } else if (uptr->SPEED == 0 && (uptr->ACC == ACC_STOP || uptr->ACC == -ACC_STOP))
    uptr->ACC = 0;
  uptr->POS += uptr->SPEED;
  sim_debug(DBG_POS, &tape_dev,
            "Unit %u: Speed %d, position %d (block %03o)\n",
            u, uptr->SPEED, uptr->POS,
            uptr->POS / MAX_SPEED / BLOCK_WORDS + uptr->OFFSET);

  if (uptr->POS < 0 && uptr->ACC <= 0 && uptr->SPEED < 0) {
    sim_debug(DBG_MOT, &tape_dev, "Unit %u: End zone; stop tape\n", u);
    uptr->ACC = ACC_STOP;
  } else if(uptr->POS >= MAX_POS && uptr->ACC >= 0 && uptr->SPEED > 0) {
    sim_debug(DBG_MOT, &tape_dev, "Unit %u: End zone; stop tape\n", u);
    uptr->ACC = -ACC_STOP;
  }

  if (uptr->SPEED != 0)
    /* The tape takes 160 microseconds between words.  This is
       approximately 20 memory cycles, 8 microseconds each. */
    sim_activate(uptr, 20);
  else
    sim_debug(DBG_MOT, &tape_dev, "Unit %u: Tape stopped\n", u);

  if (uptr->SPEED > -MAX_SPEED && uptr->SPEED < MAX_SPEED) {
    if (uptr->SPEED > 0 && uptr->ACC > 0)
      sim_debug(DBG_MOT, &tape_dev, "Unit %u: Not up to speed yet\n", u);
    else if (uptr->SPEED < 0 && uptr->ACC < 0)
      sim_debug(DBG_MOT, &tape_dev, "Unit %u: Not up to speed yet\n", u);
    else if (uptr->ACC != 0)
      sim_debug(DBG_MOT, &tape_dev, "Unit %u: Stopping\n", u);
  }
}

static void tape_ibz(UNIT *uptr, long offset)
{
  uint16 u = uptr - tape_unit;
  int i;

  /* IBZ is sensed even if the tape isn't up to speed */
  uptr->IBZ = offset < IBZ_WORDS;
  if (uptr->IBZ)
    sim_debug(DBG_INSN, &tape_dev, "Unit %u: Interblock zone\n", u);

  /* IBZ is set if any tape unit is inside an interblock zone. */
  CPU_IBZ = 0;
  for (i = 0; i < TAPE_UNITS; i++)
    CPU_IBZ |= tape_unit[i].IBZ;
}

static void tape_data(UNIT *uptr, int16 block, long offset)
{
  uint16 u = uptr - tape_unit;

  if (uptr->SPEED > 0) {
    if (offset == 5) {
      /* Forward block number. */
      FOUND_BLOCK = WANTED_BLOCK == block;
      sim_debug(DBG_SEEK, &tape_dev,
                "Unit %u: Found block number %03o; looking for %03o\n",
                u, block, WANTED_BLOCK);
      if (block > WANTED_BLOCK) {
        sim_debug(DBG_SEEK, &tape_dev,
                  "Unit %u: Reverse to find lower block numbers\n", u);
        uptr->ACC = -ACC_REVERSE;
      }
      if ((C & 7) == MTB)
        tape_done(uptr, block);
    /* Word 6 is a guard. */
    } else if (offset >= 7 && offset < 263 && FOUND_BLOCK)
      tape_word(uptr, block, (uint16)(offset - 7));
    else if (offset == 263 && FOUND_BLOCK) {
      /* Checksum here. */
      tape_done(uptr, block);
      FOUND_BLOCK = 0;
    }
  }
  /* Word 264-265 are "C". */
  /* Word 266 is a guard. */
  else if (offset == 267 && uptr->SPEED < 0) {
    /* Reverse block number. */
    sim_debug(DBG_SEEK, &tape_dev,
              "Unit %u: Found reverse block number %03o; looking for %03o\n",
              u, block, WANTED_BLOCK);
    if (block <= WANTED_BLOCK) {
      sim_debug(DBG_SEEK, &tape_dev,
                "Unit %u: Reverse to find higher block numbers\n", u);
      uptr->ACC = ACC_REVERSE;
      uptr->POS -= MAX_SPEED * BLOCK_WORDS;
    }
    if ((C & 7) == MTB)
      tape_done(uptr, block);
  }
}

static t_stat tape_svc(UNIT *uptr)
{
  long pos, block, offset;
  uint16 u = uptr - tape_unit;

  tape_motion(uptr);

  pos = uptr->POS / MAX_SPEED;
  block = pos / BLOCK_WORDS;
  offset = pos % BLOCK_WORDS;

  tape_ibz(uptr, offset);

  /* The tape contents are ignored for any of these conditions. */
  /* In an end zone. */
  if (pos < 0 || block >= MAX_BLOCKS)
    return SCPE_OK;
  /* Not up to speed. */
  if (uptr->SPEED > -MAX_SPEED && uptr->SPEED < MAX_SPEED)
    return SCPE_OK;
  /* Not paused. */
  if (!paused)
    return SCPE_OK;
  /* Not the unit currently attended to. */
  if (u != CURRENT_UNIT)
    return SCPE_OK;

  tape_data(uptr, block + uptr->OFFSET, offset);

  return SCPE_OK;
}

static t_stat tape_reset(DEVICE *dptr)
{
  int i;

  for (i = 0; i < TAPE_UNITS; i++) {
    if (tape_unit[i].flags & UNIT_DIS)
      tape_unit[i].IBZ = 0;
  }

  return SCPE_OK;
}

static t_stat tape_boot(int32 unit_num, DEVICE *dptr)
{
  uint16 block = 0300;
  uint16 blocks = 8;
  uint16 quarter = 0;
  t_stat stat;

  if (unit_num >= 2 && unit_num <= 3)
    return SCPE_ARG;
  if (blocks == 0)
    return SCPE_ARG;
    
  if (blocks == 1)
    LSW = RDC;
  else
    LSW = RCG, quarter = blocks - 1;
  LSW |= 0700 | (unit_num << 3);
  RSW = (quarter << 9) | block;
  stat = cpu_do();
  if (stat != SCPE_OK)
    return stat;
  P = 020;
  return SCPE_OK;
}

t_stat tape_metadata(FILE *fileref, uint16 *block_size, int16 *forward_offset, int16 *reverse_offset)
{
  t_offset size = sim_fsize(fileref);
  if (size == MAX_BLOCKS * DATA_WORDS * 2) {
    /* Plain image. */
    *block_size = DATA_WORDS;
    *forward_offset = 0;
    *reverse_offset = 0;
  } else if ((size % (2 * DATA_WORDS)) == 6) {
    /* Extended image with additional meta data. */
    uint16 metadata = (uint16)(size / (2 * DATA_WORDS));
    *block_size = read_word(fileref, metadata, 0);
    *forward_offset = (int16)read_word(fileref, metadata, 1);
    *reverse_offset = (int16)read_word(fileref, metadata, 2);
  } else
    return SCPE_FMT;
  return SCPE_OK;
}

static t_stat tape_attach(UNIT *uptr, CONST char *cptr)
{
  t_stat stat;
  uint16 block_size;
  uint16 u = uptr - tape_unit;
  int16 forward_offset, reverse_offset;

  if (uptr - tape_unit >= 2 && uptr - tape_unit <= 3)
    return SCPE_ARG;
  stat = attach_unit(uptr, cptr);
  if (stat != SCPE_OK)
    return stat;
  stat = tape_metadata(uptr->fileref, &block_size, &forward_offset, &reverse_offset);
  if (stat != SCPE_OK)
    return stat;
  sim_debug(DBG, &tape_dev,
            "Unit %u: Tape image with block size %o, block offset %d/%d\r\n",
            u, block_size, forward_offset, reverse_offset);
  if (block_size != DATA_WORDS)
    return SCPE_FMT;
  if (forward_offset != reverse_offset)
    return SCPE_FMT;
  uptr->OFFSET = forward_offset;
    
  uptr->POS = -2 * START_POS;
  uptr->SPEED = 0;
  uptr->IBZ = 0;
  return SCPE_OK;
}

static t_stat tape_detach(UNIT *uptr)
{
  if (uptr - tape_unit >= 2 && uptr - tape_unit <= 3)
    return SCPE_ARG;
  if ((uptr->flags & UNIT_ATT) == 0)
    return SCPE_OK;
  if (sim_is_active(uptr))
    sim_cancel(uptr);
  return detach_unit(uptr);
}
