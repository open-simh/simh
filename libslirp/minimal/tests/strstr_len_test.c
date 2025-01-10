#include <stdio.h>
#include <stdint.h>
#include <glib.h>

char *test1 = "This string tests a test pattern.";
const char *test1_patterns[] = {
    "This",
    "his ",
    "a test",
    "pattern.",
    "ern.",
    ".",
    "tests a test",
    "ng tes",
    "This string tests a test pattern."
};

char *test2 =
    "DESCRIPTION         top\n"
    "       The strstr() function finds the first occurrence of the substring\n"
    "       needle in the string haystack.  The terminating null bytes ('\\0')\n"
    "       are not compared.\n"
    "\n"
    "       The strcasestr() function is like strstr(), but ignores the case\n"
    "       of both arguments.\n"
    "RETURN VALUE         top\n"
    "       These functions return a pointer to the beginning of the located\n"
    "       substring, or NULL if the substring is not found.\n"
    "\n"
    "       If needle is the empty string, the return value is always\n"
    "       haystack itself.\n";

int main(void)
{
    size_t i;

    for (i = 0; i < sizeof(test1_patterns) / sizeof(test1_patterns[0]); ++i) {
        g_assert(g_strstr_len(test1, strlen(test1), test1_patterns[i]) != NULL);
    }

    g_assert(g_strstr_len(test2, strlen(test2), "always\n") != NULL);
    g_assert(g_strstr_len(test2, strlen(test2), "       If needle is the empty") != NULL);
    g_assert(g_strstr_len(test2, strlen(test2), "") == test2);

    return 0;
}