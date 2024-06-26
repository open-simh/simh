# libslirp-version.awk: Generate the libslirp-version.h header:
#
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in
#  all copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
#  THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
#  Except as contained in this notice, the name of the author shall not be
#  used in advertising or otherwise to promote the sale, use or other dealings
#  in this Software without prior written authorization from the author.

/ +version : '.*'/ {
    vstr = $3;
    sub(/'/, "", vstr);
    sub(/',/, "", vstr);
    split(vstr, vparts, ".");
    print "#ifndef LIBSLIRP_VERSION_H_";
    print "#define LIBSLIRP_VERSION_H_";
    print "";
    print "#ifdef __cplusplus";
    print "extern \"C\" {";
    print "#endif";
    print "";
    print "#define SLIRP_MAJOR_VERSION ", vparts[1];
    print "#define SLIRP_MINOR_VERSION ", vparts[2];
    print "#define SLIRP_MICRO_VERSION ", vparts[3];
    print "#define SLIRP_VERSION_STRING \"" vstr "\"";
    print "";
    print "#define SLIRP_CHECK_VERSION(major,minor,micro)                           \\";
    print "    (SLIRP_MAJOR_VERSION > (major) ||                                    \\";
    print "    (SLIRP_MAJOR_VERSION == (major) && SLIRP_MINOR_VERSION > (minor)) || \\";
    print "    (SLIRP_MAJOR_VERSION == (major) && SLIRP_MINOR_VERSION == (minor) && \\";
    print "    SLIRP_MICRO_VERSION >= (micro)))";
    print "";
    print "#ifdef __cplusplus";
    print "} /* extern \"C\" */";
    print "#endif";
    print "";
    print "#endif /* LIBSLIRP_VERSION_H_ */";
}
