/* sim_debtab.c:
  ------------------------------------------------------------------------------

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of the author shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from the author.

  ------------------------------------------------------------------------------

   This module provides DEBTAB debug table manipulation functions.

*/

#include "sim_defs.h"
#include "scp.h"

/* Count the elements in the debugging table. */
size_t sim_debtab_nelems(const DEBTAB *debtab)
{
    const DEBTAB *p;

    for (p = debtab; p->name != NULL; ++p)
        /* NOP */ ;

    return (p  - debtab);
}

/* Combine two DEBTAB debug tables, returning the resulting combined
 * table. Both tables have NULL as the last element's name. */
DEBTAB *sim_combine_debtabs(const DEBTAB *tab1, const DEBTAB *tab2)
{
    size_t n_combined = 0;
    const DEBTAB *p;
    DEBTAB *combined;

    n_combined = sim_debtab_nelems(tab1) + sim_debtab_nelems(tab2) + 1;
    combined = (DEBTAB *) malloc(n_combined * sizeof(DEBTAB));
    n_combined = 0;

    for (p = tab1; p->name != NULL; ++p, n_combined++)
        combined[n_combined] = *p;
    for (p = tab2; p->name != NULL; ++p, n_combined++)
        combined[n_combined] = *p;

    combined[n_combined].name = NULL;
    return combined;
}

/* Fill debug mask holes in the debugging table: If an entry's mask == 0,
 * find an unused bit in the existing debugging table's mask and assign
 * that unused bit to the entry's mask. */
void sim_fill_debtab_flags(DEBTAB *debtab)
{
    uint32_t dbg_bitmask = 0, new_flag;
    DEBTAB *p;
    const unsigned int n_bits = sizeof(dbg_bitmask) * 8;
    unsigned int i;

    /* Augment the device's debugging table with the poll and socket flags,
     * so that debugging looks correct in the output (see _get_dbg_verb() in
     * scp.c) */
    dbg_bitmask = 0;
    for (p = debtab; p->name != NULL; ++p)
        dbg_bitmask |= p->mask;

    p = debtab;
    new_flag = 1;
    i = 0;
    while (p->name != NULL && i < n_bits) {
        if ((new_flag & dbg_bitmask) == 0) {
            /* Find a home for the new flag */
            for (/* empty */; p->name != NULL; ++p) {
                if (p->mask == 0) {
                  p->mask = new_flag;
                  break;
                }
            }
        }

        new_flag <<= 1;
        ++i;
    }
}
