#include <stdio.h>
#include <glib.h>

int main(void)
{
    GRand *rnd = g_rand_new();
    GRand *rnd2 = g_rand_new();
    int j;

    puts("Random range: [10,30)");
    for (j = 0; j < 40; ++j) {
        int i = g_rand_int_range(rnd, 10, 30);

        printf("rnd: %2d\n", i);
        g_assert(i >= 10 && i < 30);
    }

    puts("Random range: [-12,99)");
    for (j = 0; j < 200; ++j) {
        int i = g_rand_int_range(rnd, -12, 99);

        printf("rnd: %2d\n", i);
        g_assert(i >= -12 && i < 99);
    }

    g_rand_free(rnd);
    g_rand_free(rnd2);

    return 0;
}
