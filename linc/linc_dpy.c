#include "linc_defs.h"
#include "linc_charset.h"

static t_stat dpy_svc(UNIT *uptr);
static t_stat dpy_reset(DEVICE *dptr);
static void scan(void);

/* Debug */
#define DBG             0001

#ifdef CHAR_WIDTH
#undef CHAR_WIDTH
#endif

#define DPY_SIZE      512
#define DOT_SPACING   4
#define DSC_WIDTH     (2 * DOT_SPACING)          //DSC is two dots wide.
#define CHAR_WIDTH    (2 * DSC_WIDTH)            //Two DSC to draw a character.
#define CHAR_SPACING  (CHAR_WIDTH + DOT_SPACING) //One dot between characters.
#define CHAR_HEIGHT   32
#define CHAR_COLUMNS  (DPY_SIZE / CHAR_WIDTH)
#define CHAR_LINES    (DPY_SIZE / CHAR_HEIGHT)

#define UNIT_V_TEXT     (UNIT_V_UF + 0)
#define UNIT_TXT        (1 << UNIT_V_TEXT)

static uint16 bitmap[DPY_SIZE][CHAR_LINES];
static char prev_text[CHAR_LINES][CHAR_COLUMNS + 1];
static char curr_text[CHAR_LINES][CHAR_COLUMNS + 1];

static UNIT dpy_unit = {
  UDATA(&dpy_svc, UNIT_IDLE, 0)
};

static MTAB dpy_mod[] = {
  { UNIT_TXT, UNIT_TXT, "TEXT",   "TEXT",   NULL, NULL, "Read text off display" },
  { UNIT_TXT,        0, "NOTEXT", "NOTEXT", NULL, NULL, "No text" },
  { 0 }
};

static DEBTAB dpy_deb[] = {
  { "DBG",  DBG },
  { NULL, 0 }
};

DEVICE dpy_dev = {
  "DPY", &dpy_unit, NULL, dpy_mod,
  1, 8, 12, 1, 8, 12,
  NULL, NULL, &dpy_reset, NULL, NULL, NULL,
  NULL, DEV_DEBUG, 0, dpy_deb,
  NULL, NULL, NULL, NULL, NULL, NULL
};

static void print(const char *line)
{
  int i;
  for (i = 0; i < CHAR_COLUMNS && *line != 0; i++)
    sim_putchar(*line++);
  sim_putchar('\r');
  sim_putchar('\n');
}

/* Print lines that differ since last scan. */
static void diff(void)
{
  int i;
  for (i = 0; i < CHAR_LINES; i++) {
    if (strncmp(prev_text[i], curr_text[i], CHAR_COLUMNS) == 0)
      continue;
    print(curr_text[i]);
  }
}

static t_stat dpy_svc(UNIT *uptr)
{
  if (uptr->flags & UNIT_TXT) {
    memset(curr_text, 0, sizeof curr_text);
    scan();
    diff();
    memcpy(prev_text, curr_text, sizeof curr_text);
    memset(bitmap, 0, sizeof bitmap);
  }
  sim_activate_after(&dpy_unit, 1000000);
  return SCPE_OK;
}

static t_stat dpy_reset(DEVICE *dptr)
{
  memset(bitmap, 0, sizeof bitmap);
  memset(prev_text, 0, sizeof prev_text);
  if (dptr->flags & DEV_DIS)
    sim_cancel(&dpy_unit);
  else
    sim_activate(&dpy_unit, 1);
  return SCPE_OK;
}

void dpy_dis(uint16 h, uint16 x, uint16 y)
{
  sim_debug(DBG, &dpy_dev, "DIS %u;%03o, A=%03o\n", h, x, y);
  /* Y coordinate +0 and -0 both refer to the same vertical position. */
  if (y < 256)
    y += 255;
  else
    y -= 256;
  crt_point(h, x, y);
}

void dpy_dsc(uint16 x, uint16 y, uint16 z)
{
  int i;
  x &= DMASK;
  y &= DMASK & 0740;
  if (y < 256)
    y += 256;
  else
    y -= 256;
  y /= CHAR_HEIGHT;
  bitmap[x][15 - y] = z;
}

/* Number of bits set in x. */
static int popcount(uint32 x)
{
  int n = 0;
  while (x != 0)
    x ^= (x & -x), n++;
  return n;
}

/* Find character grid matching bits, and return ASCII code for it. */
static int match(uint32 bits, int diff)
{
  int i;
  for (i = 0; i < 63; i++) {
    if (popcount(bits ^ linc_grid[i]) <= diff)
      return linc_ascii[i];
  }
  return 0;
}

/* Find ASCII char for bits. */
static int find(uint32 bits)
{
  int c = match(bits, 0);
  if (c == 0)
    c = match(bits, 1);
  return c;
}

static char * spaces(int n, char *ptr)
{
  memset(ptr, ' ', n);
  return ptr + n;
}

/* Find bits on display at x, y. */
static uint32 look(int x, int y)
{
  uint32 bits = bitmap[x][y];
  return (bits << 12) | bitmap[x + DSC_WIDTH][y];
}

/* Check for similar characters in next location. */
static int similar(int x, int y, int flag, const char *string)
{
  int c;
  if (flag) {
    c = find(look(x + DSC_WIDTH, y));
    if (c != 0)
      return strchr(string, c) != NULL;
  }
  return FALSE;
}
   
/* Scan entire display and convert to ASCII. */
static void scan(void)
{
  char *line;
  uint32 bits;
  int previous;
  int x, y, c;

  for (y = 0; y < CHAR_LINES; y++) {
    previous = 0;
    line = curr_text[y];
    for (x = 0; x < 511 - CHAR_WIDTH; x++) {
      bits = look(x, y);
      if (bits == 0)
        continue;
      c = find(bits);
      if (similar(x, y, c == '|', "HKLU") ||
          similar(x, y, c == ']', "I"))
        /* The left part of HKLU looks like a pipe, and I looks like a bracket. */
        continue;
      if (c != 0) {
        line = spaces((x - previous) / CHAR_SPACING, line);
        previous = x + CHAR_SPACING;
        *line++ = c;
        x += CHAR_WIDTH - 1;
      }
    }
  }
}

/* Called from display library to get data switches. */
void
cpu_get_switches (unsigned long *p1, unsigned long *p2)
{
}

/* Called from display library to set data switches. */
void
cpu_set_switches (unsigned long p1, unsigned long p2)
{
}
