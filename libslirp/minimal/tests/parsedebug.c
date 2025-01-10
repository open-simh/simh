#include <stdio.h>
#include <stdint.h>
#include <glib.h>

#define FLAG_0 0x00000001
#define FLAG_1 0x00000002
#define FLAG_2 0x00000004
#define FLAG_3 0x00000100

typedef struct ParseDebug
{
    const char *flags;
    uint32_t expected;
} ParseDebug;

ParseDebug trials[] = {
    { "all", FLAG_0|FLAG_1|FLAG_2|FLAG_3 },
    { "help", 0 },
    { " all", FLAG_0|FLAG_1|FLAG_2|FLAG_3 },
    { " all;", FLAG_0|FLAG_1|FLAG_2|FLAG_3 },
    { " all  ;;  ; ", FLAG_0|FLAG_1|FLAG_2|FLAG_3 },
    { "one;three", FLAG_1|FLAG_3 },
    { "one;three ;;", FLAG_1|FLAG_3 },
    { "one;;three", FLAG_1|FLAG_3 },
    { "all;one", FLAG_0|FLAG_2|FLAG_3 },
    { "one;all", FLAG_0|FLAG_2|FLAG_3 },
    { "zero;all;one", FLAG_2|FLAG_3 },
};

const size_t n_trials = sizeof(trials) / sizeof(trials[0]);

GDebugKey dkeys[] = {
    { "zero", FLAG_0 },
    { "one", FLAG_1 },
    { "two", FLAG_2 },
    { "three", FLAG_3 },
};

static size_t n_dkeys = sizeof(dkeys) / sizeof(dkeys[0]);

int main(void)
{
    size_t i;
    int success = 0, errs = 0;

    for (i = 0; i < n_trials; ++i) {
        uint32_t val = g_parse_debug_string(trials[i].flags, dkeys, n_dkeys);

        if (val != trials[i].expected) {
            fprintf(stderr, "flags: \"%s\" expected: %08x val: %08X\n",
                    trials[i].flags, trials[i].expected, val);
            ++errs;
        } else
            ++success;
    }

    if (!errs) {
        fputs("All tests passed.\n", stderr);
    } else {
        fprintf(stderr, "%d tests passed, %d failed.\n", success, errs);
    }
    return 0;
}
