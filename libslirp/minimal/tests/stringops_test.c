#include <stdio.h>
#include <stdint.h>
#include <glib.h>

int main(void)
{
    const char s2_initializer[] = "This is an initializer.";
    const size_t s2_length = sizeof(s2_initializer) / sizeof(s2_initializer[0]);
    const char s3_initializer[] = "0123456789012345678901234567899";
    const size_t s3_length = sizeof(s3_initializer) / sizeof(s3_initializer[0]);

    GString *s1 = g_string_new(NULL);
    GString *s2 = g_string_new(s2_initializer);
    GString *s3 = g_string_new(s3_initializer);

    g_assert(s1->len == 0 && s1->allocated == G_STRING_MIN_ALLOCATION);
    g_assert(s2->len == s2_length - 1 && s2->allocated == G_STRING_MIN_ALLOCATION);
    g_assert(s3->len == s3_length - 1 && s3->allocated >= G_STRING_MIN_ALLOCATION);

    g_assert(g_string_free(s1, TRUE) == NULL);
    g_assert(g_string_free(s2, TRUE) == NULL);

    char *s3_str = g_string_free(s3, FALSE);
    g_assert(strcmp(s3_initializer, s3_str) == 0);
    free(s3_str);

    GString *s4 = g_string_new("something!!");
    g_string_append_printf(s4, " More string with arguments: %s\n", "--special--");
    fputs(s4->str, stdout);
    g_assert(strcmp(s4->str, "something!! More string with arguments: --special--\n") == 0);
    g_string_free(s4, TRUE);

    GString *s5 = g_string_new("a_really_long_prefix: ");
    g_string_append_printf(s5, " string_append_printf %08x %08x %s\n", 0, ~0, "--special--");
    fputs(s5->str, stdout);
    g_assert(strcmp(s5->str, "a_really_long_prefix:  string_append_printf 00000000 ffffffff --special--\n") == 0);
    g_string_free(s5, TRUE);

    g_assert(g_ascii_strcasecmp("This is equal", "this is Equal") == 0);
    g_assert(g_ascii_strcasecmp("One side", "Other side") < 0);
    g_assert(g_ascii_strcasecmp("One side", "One sido") < 0);
    g_assert(g_ascii_strcasecmp("One sido", "One side") > 0);
    g_assert(g_ascii_strcasecmp("1243 One Two three fOuR", "1243 oNe tWO thReE FoUr") == 0);
    
    return 0;
}
