#include "linc_defs.h"
#include "sim_video.h"

/* Debug */
#define DBG   0001

#define KBD_CASE  023  /* Shift key. */
#define KBD_0     000  /* CASE 0 moves down one page. */
#define KBD_1     001  /* CASE 1 moves down one line. */
#define KBD_Q     044  /* CASE Q moves up one page. */
#define KBD_W     052  /* CASE W moves up one line. */

#define P (*(uint16 *)cpu_reg[0].loc)
#define A (*(uint16 *)cpu_reg[2].loc)
#define paused (*(int *)cpu_reg[11].loc)

static t_stat kbd_svc(UNIT *uptr);
static t_stat kbd_reset(DEVICE *dptr);
static t_bool kbd_from_ascii(unsigned ch);

static UNIT kbd_unit = {
  UDATA(&kbd_svc, UNIT_IDLE, 0)
};

static DEBTAB kbd_deb[] = {
  { "DBG", DBG },
  { NULL, 0 }
};

DEVICE kbd_dev = {
  "KBD", &kbd_unit, NULL, NULL,
  1, 8, 12, 1, 8, 12,
  NULL, NULL, &kbd_reset,
  NULL, NULL, NULL, NULL, DEV_DEBUG, 0, kbd_deb,
  NULL, NULL, NULL, NULL, NULL, NULL
};

/*
CASE  0  1  2  3  4  5  6  7  8  9 DEL
  23 00 01 02 03 04 05 06 07 10 11 13

   Q  W  E  R  T  Y  U  I  O  P i=
  44 52 30 45 47 54 50 34 42 43 15

    A  S  D  F  G  H  J  K  L +. -,
   24 46 27 31 32 33 35 36 37 10 17

 #[  Z  X  C  V  B  N  M pu |⊟ META/EOL
 22 55 53 26 51 25 41 40 16 21 12

             SPACE
              14
*/

static int kbd_pressed = 0;
static uint16 kbd_code;
static uint16 kbd_stroke[3];
static uint16 kbd_strokes = 0;

static t_stat kbd_svc(UNIT *uptr)
{
  t_stat ch;

  /* If key pressed, must wait until it's read by KBD. */
  if (kbd_pressed)
    return sim_activate(&kbd_unit, 3000);

  if (kbd_strokes == 0) {
    /* If no strokes buffered, try polling console. */
    ch = sim_poll_kbd();
    if ((ch & SCPE_KFLAG) == 0 ||
        (ch & SCPE_BREAK) != 0 ||
        !kbd_from_ascii(ch & 0177))
      /* Nothing buffered and nothing from console, try again later. */
      return sim_activate(&kbd_unit, 3000);
  } else {
    sim_debug(DBG, &kbd_dev, "Key struck %02o\n", kbd_code);
    kbd_pressed = 1;
    kbd_strokes--;
    kbd_code = kbd_stroke[0];
    memmove(&kbd_stroke[0], &kbd_stroke[1],
            sizeof kbd_stroke - sizeof kbd_stroke[0]);
  }

  return SCPE_OK;
}

int kbd_struck(void)
{
  if (kbd_pressed)
    sim_debug(DBG, &kbd_dev, "KST\n");
  return kbd_pressed;
}

uint16 kbd_key(uint16 wait)
{
  if (kbd_pressed) {
    sim_debug(DBG, &kbd_dev, "KBD %02o\n", kbd_code);
    kbd_pressed = 0;
    sim_activate(&kbd_unit, 1);
    return kbd_code;
  } else if (wait) {
    sim_debug(DBG, &kbd_dev, "KBD paused\n");
    paused = 1;
  }
  return 0;
}

static void kbd_case(uint16 code)
{
  kbd_code = KBD_CASE;
  kbd_stroke[0] = code;
  kbd_strokes = 1;
}

static t_bool kbd_from_ascii(unsigned ch)
{
  switch (ch) {
  case '0':  kbd_code = KBD_0; break;
  case '1':  kbd_code = KBD_1; break;
  case '2':  kbd_code = 002; break;
  case '3':  kbd_code = 003; break;
  case '4':  kbd_code = 004; break;
  case '5':  kbd_code = 005; break;
  case '6':  kbd_code = 006; break;
  case '7':  kbd_code = 007; break;
  case '8':  kbd_code = 010; break;
  case '9':  kbd_code = 011; break;
  case '\n':
  case '\r': kbd_code = 012; break;
  case '\177':
  case '\b': kbd_code = 013; break;
  case ' ':  kbd_code = 014; break;
  case 'i':  kbd_code = 015; break;
  case 'p':  kbd_code = 016; break;
  case '-':  kbd_code = 017; break;
  case '+':  kbd_code = 020; break;
  case '|':  kbd_code = 021; break;
  case '#':  kbd_code = 022; break;
  case '\t': kbd_code = KBD_CASE; break;
  case 'a':
  case 'A':  kbd_code = 024; break;
  case 'b':
  case 'B':  kbd_code = 025; break;
  case 'c':
  case 'C':  kbd_code = 026; break;
  case 'd':
  case 'D':  kbd_code = 027; break;
  case 'e':
  case 'E':  kbd_code = 030; break;
  case 'f':
  case 'F':  kbd_code = 031; break;
  case 'g':
  case 'G':  kbd_code = 032; break;
  case 'h':
  case 'H':  kbd_code = 033; break;
  case 'I':  kbd_code = 034; break;
  case 'j':
  case 'J':  kbd_code = 035; break;
  case 'k':
  case 'K':  kbd_code = 036; break;
  case 'l':
  case 'L':  kbd_code = 037; break;
  case 'm':
  case 'M':  kbd_code = 040; break;
  case 'n':
  case 'N':  kbd_code = 041; break;
  case 'o':
  case 'O':  kbd_code = 042; break;
  case 'P':  kbd_code = 043; break;
  case 'q':
  case 'Q':  kbd_code = KBD_Q; break;
  case 'r':
  case 'R':  kbd_code = 045; break;
  case 's':
  case 'S':  kbd_code = 046; break;
  case 't':
  case 'T':  kbd_code = 047; break;
  case 'U':  kbd_code = 050; break;
  case 'v':
  case 'V':  kbd_code = 051; break;
  case 'w':
  case 'W':  kbd_code = KBD_W; break;
  case 'x':
  case 'X':  kbd_code = 053; break;
  case 'y':
  case 'Y':  kbd_code = 054; break;
  case 'z':
  case 'Z':  kbd_code = 055; break;
  case '\033': kbd_code = 056; break;
  case '?':  kbd_case(014); break;
  case '=':  kbd_case(015); break;
  case 'u':  kbd_case(016); break;
  case ',':  kbd_case(017); break;
  case '.':  kbd_case(020); break;
  case '$':
  case '@':  kbd_case(021); break;
  case '[':  kbd_case(022); break;
  case '_':  kbd_case(023); break;
  case '"':  kbd_case(024); break;
  case '\'': kbd_case(025); break;
  case '<':  kbd_case(026); break;
  case '>':  kbd_case(027); break;
  case ']':  kbd_case(030); break;
  case '*':  kbd_case(031); break;
  case ':':  kbd_case(032); break;
  case '\020': kbd_case(KBD_W); break; //Control-P
  case '\016': kbd_case(KBD_1); break; //Control-N
  default: return 0;
  }
  sim_debug(DBG, &kbd_dev, "Key struck '%c' -> %02o\n", ch, kbd_code);
  kbd_pressed = 1;
  paused = 0;
  return 1;
}

static void kbd_convert(uint32 key, VID_DISPLAY *vptr)
{
  switch (key) {
  case SIM_KEY_0: /* 0 Q */
  case SIM_KEY_BACKQUOTE:
    kbd_code = KBD_0;
    break;
  case SIM_KEY_1: /* 1 R */
    kbd_code = KBD_1;
    break;
  case SIM_KEY_2: /* 2 S */
    kbd_code = 002;
    break;
  case SIM_KEY_3: /* 3 T */
    kbd_code = 003;
    break;
  case SIM_KEY_4: /* 4 U */
    kbd_code = 004;
    break;
  case SIM_KEY_5: /* 5 V */
    kbd_code = 005;
    break;
  case SIM_KEY_6: /* 6 W */
    kbd_code = 006;
    break;
  case SIM_KEY_7: /* 7 X */
    kbd_code = 007;
    break;
  case SIM_KEY_8: /* 8 Y */
    kbd_code = 010;
    break;
  case SIM_KEY_9: /* 9 Z */
    kbd_code = 011;
    break;
  case SIM_KEY_ENTER:
    kbd_code = 012;
    break;
  case SIM_KEY_BACKSPACE:
  case SIM_KEY_DELETE:
    kbd_code = 013;
    break;
  case SIM_KEY_SPACE: /* Space ? */
  case SIM_KEY_SLASH:
    kbd_code = 014;
    break;
  case SIM_KEY_EQUALS: /* i = */
    kbd_code = 015;
    break;
  case SIM_KEY_F1: /* p u */
    kbd_code = 016;
    break;
  case SIM_KEY_MINUS: /* - , */
  case SIM_KEY_COMMA:
    kbd_code = 017;
    break;
  case SIM_KEY_PERIOD: /* + . */
    kbd_code = 020;
    break;
  case SIM_KEY_BACKSLASH: /* | ⊟ */
    kbd_code = 021;
    break;
  case SIM_KEY_LEFT_BRACKET: /* # [ */
  case SIM_KEY_LEFT_BACKSLASH:
    kbd_code = 022;
    break;
  case SIM_KEY_SHIFT_L: /* CASE _ */
  case SIM_KEY_SHIFT_R:
  case SIM_KEY_ESC:
    kbd_code = KBD_CASE;
    break;
  case SIM_KEY_A: /* A " */
  case SIM_KEY_SINGLE_QUOTE:
    kbd_code = 024;
    break;
  case SIM_KEY_B: /* B „ */
    kbd_code = 025;
    break;
  case SIM_KEY_C: /* C < */
    kbd_code = 026;
    break;
  case SIM_KEY_D: /* D > */
    kbd_code = 027;
    break;
  case SIM_KEY_E: /* E ] */
  case SIM_KEY_RIGHT_BRACKET:
    kbd_code = 030;
    break;
  case SIM_KEY_F: /* F ˣ */
    kbd_code = 031;
    break;
  case SIM_KEY_G: /* G : */
  case SIM_KEY_SEMICOLON:
    kbd_code = 032;
    break;
  case SIM_KEY_H: /* H */
    kbd_code = 033;
    break;
  case SIM_KEY_I: /* I */
    kbd_code = 034;
    break;
  case SIM_KEY_J: /* J */
    kbd_code = 035;
    break;
  case SIM_KEY_K: /* K */
    kbd_code = 036;
    break;
  case SIM_KEY_L: /* L */
    kbd_code = 037;
    break;
  case SIM_KEY_M: /* M */
    kbd_code = 040;
    break;
  case SIM_KEY_N: /* N */
    kbd_code = 041;
    break;
  case SIM_KEY_O: /* O */
    kbd_code = 042;
    break;
  case SIM_KEY_P: /* P */
    kbd_code = 043;
    break;
  case SIM_KEY_Q: /* Q */
    kbd_code = KBD_Q;
    break;
  case SIM_KEY_R: /* R */
    kbd_code = 045;
    break;
  case SIM_KEY_S: /* S */
    kbd_code = 046;
    break;
  case SIM_KEY_T: /* T */
    kbd_code = 047;
    break;
  case SIM_KEY_U: /* U */
    kbd_code = 050;
    break;
  case SIM_KEY_V: /* V */
    kbd_code = 051;
    break;
  case SIM_KEY_W: /* W */
    kbd_code = KBD_W;
    break;
  case SIM_KEY_X: /* X */
    kbd_code = 053;
    break;
  case SIM_KEY_Y: /* Y */
    kbd_code = 054;
    break;
  case SIM_KEY_Z: /* Z */
    kbd_code = 055;
    break;
  case SIM_KEY_ALT_L: /* META? */
  case SIM_KEY_ALT_R:
    kbd_code = 056;
    break;
    // → 57
    // ? 60
    // = 61
    // u 62
    // , 63
    // . 64
    // ⊟ 65
    // [ 66
    // _ 67
    // " 70
    // „ 71
    // < 72
    // > 73
    // ] 74
    // ˣ 75
    // : 76
    // ʸ 77
  case SIM_KEY_F2:  /* START 20 */
    P = 020;
    break;
  case SIM_KEY_F4:  /* START 400 */
    P = 0400;
    break;
  case SIM_KEY_F11:
    crt_toggle_fullscreen(vptr);
    return;
  case SIM_KEY_UP:
    kbd_case(KBD_W);
    break;
  case SIM_KEY_DOWN:
    kbd_case(KBD_1);
    break;
  case SIM_KEY_PAGE_UP:
    kbd_case(KBD_Q);
    break;
  case SIM_KEY_PAGE_DOWN:
    kbd_case(KBD_0);
    break;
  default:
    return;
  }
  sim_debug(DBG, &kbd_dev, "Key struck %s -> %02o\n",
            vid_key_name(key), kbd_code);
  if (paused)
    A = kbd_code;
  else {
    kbd_pressed = 1;
    sim_cancel(&kbd_unit);
  }
  paused = 0;
}

void
kbd_event(SIM_KEY_EVENT *ev)
{
  if (ev->state == SIM_KEYPRESS_DOWN) {
    sim_debug(DBG, &kbd_dev, "Key pressed, window %p\n", ev->vptr);
    kbd_convert(ev->key, ev->vptr);
  }
}

static t_stat
kbd_reset(DEVICE *dptr)
{
  if (sim_switches & SWMASK ('P'))
    return SCPE_OK;

  if (dptr->flags & DEV_DIS)
    return sim_cancel(&kbd_unit);

  return sim_activate(&kbd_unit, 1);
}
