"""Makefile parsing and variable expansion.

Read and collect variable, rule and action information from a [Mm]akefile.
This isn't a precise collection; for example, it does not respect GNU Makefile
directives such as 'ifeq' and 'ifneq'.
"""

import re

# Regexes needed for parsing Makefile (and similar syntaxes,
# like old-style Setup files).
_variable_rx = re.compile(r"\s*([A-Za-z][\w_-]+)\s*=\s*(.*)")
_rule_rx     = re.compile(r"(((\$[({])*\w[\w_-]+[)}]*)+)\s*:\s*(.*)")

# Regex that recognizes variables. Group 1 is the variable's name.
_var_pattern = r"[A-Za-z][\w_-]*"
_var_rx      = re.compile(r"^\$[{(](" + _var_pattern + r")[)}]$")
_var_rx2     = re.compile(r"\$[{(]("  + _var_pattern + r")[)}]")
_norm_var_rx = re.compile(r"\$[(]("   + _var_pattern + r")[)]")

def parse_makefile(fn, g_vars=None, g_rules=None, g_actions=None):
    """Parse a Makefile-style file.

    Collects all of the variable definitions, rules and actions associated with rules.

    """
    ## Python 3.11 and onward dropped distuitls, so import our local copy.
    from simgen.text_file import TextFile

    fp = TextFile(fn, strip_comments=1, skip_blanks=1, join_lines=1, errors="surrogateescape")

    if g_vars is None:
        g_vars = {}
    if g_rules is None:
        g_rules = {}
    if g_actions is None:
        g_actions = {}
    done = {}
    rules = {}
    actions = {}

    line = fp.readline()
    while line is not None:
        vmatch = _variable_rx.match(line)
        rmatch = _rule_rx.match(line)
        if vmatch:
            n, v = vmatch.group(1, 2)
            v = v.strip()

            try:
                v = int(v)
            except ValueError:
                # insert literal `$'
                done[n] = v.replace('$$', '$')
            else:
                done[n] = v

            line = fp.readline()
        elif rmatch:
            ## make allows "$(VAR)" and "${VAR}" to be used interchangably, so there's a
            ## possibility that the sim developer used both forms in the target, e.g.:
            ##
            ## foosim: $(BIN)foosim$(EXE)
            ##
            ## ${BIN}foosim${EXE}: stuff that foosim depends on...

            n, v = rmatch.group(1, 4)
            rules[normalize_variables(n)] = normalize_variables(v)

            ## Collect the actions:
            collected = []
            line = fp.readline()
            while line is not None:
                m = _variable_rx.match(line) or _rule_rx.match(line)
                if m is None:
                    collected.append(line.lstrip())
                    line = fp.readline()
                else:
                    break
            actions[n] = collected
        else:
            line = fp.readline()

    fp.close()

    # strip spurious spaces
    for k, v in done.items():
        if isinstance(v, str):
            done[k] = v.strip().replace('\t', ' ')

    # save the results in the global dictionary
    g_vars.update(done)
    g_rules.update(rules)
    g_actions.update(actions)
    return (g_vars, g_rules, g_actions)


def target_dep_list(target, rules, defs):
    return (rules.get(target) or '').split()

def expand_vars(s, defs):
    """Expand Makefile-style variables -- "${foo}" or "$(foo)" -- in
    'string' according to 'defs' (a dictionary mapping variable names to
    values).  Variables not present in 'defs' are silently expanded to the
    empty string.

    Returns a variable-expanded version of 's'.
    """

    # This algorithm does multiple expansion, so if defs['foo'] contains
    # "${bar}", it will expand ${foo} to ${bar}, and then expand
    # ${bar}... and so forth.  This is fine as long as 'defs' comes from
    # 'parse_makefile()', which takes care of such expansions eagerly,
    # according to make's variable expansion semantics.

    while True:
        m = _var_rx2.search(s)
        if m:
            (beg, end) = m.span()
            s = s[0:beg] + (defs.get(m.group(1)) or '') + s[end:]
        else:
            break
    return s


def shallow_expand_vars(s, defs):
    """Expand Makefile-style variables -- "${foo}" or "$(foo)" -- in
    'string' according to 'defs' (a dictionary mapping variable names to
    values).  Variables not present in 'defs' are silently expanded to the
    empty string.

    Returns a variable-expanded version of 's'.
    """

    # This algorithm does multiple expansion, so if defs['foo'] contains
    # "${bar}", it will expand ${foo} to ${bar}, and then expand
    # ${bar}... and so forth.  This is fine as long as 'defs' comes from
    # 'parse_makefile()', which takes care of such expansions eagerly,
    # according to make's variable expansion semantics.

    m = _var_rx2.search(s)
    if m:
        (beg, end) = m.span()
        return s[0:beg] + (defs.get(m.group(1)) or '') + shallow_expand_vars(s[end:], defs)

    return s


def extract_variables(varstr):
    """Extracct all variable references, e.g., "${foo}" or "$(foo)"
    from a string.
    """
    retval = []
    tmp = varstr
    while True:
        m = _var_rx2.search(tmp)
        if m:
            retval.append(m[1])
            tmp = tmp[m.end():]
        else:
            break
    return retval


def normalize_variables(varstr):
    """Convert '$(var)' to '${var}' -- normalizes all variables to a consistent
    form.
    """
    retval = ""
    tmp = varstr
    while tmp:
        m = _norm_var_rx.search(tmp)
        if m:
            retval += tmp[:m.start()] + "${" + m[1] + "}"
            tmp = tmp[m.end():]
        else:
            retval += tmp
            tmp = ""
    return retval


def test_rule_rx():
    result = _rule_rx.match('${BIN}frontpaneltest${EXE} : frontpanel/FrontPanelTest.c sim_sock.c sim_frontpanel.c')
    print('{0}: {1}'.format('${BIN}frontpaneltest${EXE}...', result))
    print(result.groups())


def test_normalize_variables():
    result = normalize_variables('foo: bar baz')
    print('{0}: {1}'.format('foo:...', result))
    result = normalize_variables('$(var): dep1 dep2')
    print('{0}: {1}'.format('$(var)...', result))
    result = normalize_variables('$(var): dep1 ${var2} dep2 $(var3)')
    print('{0}: {1}'.format('$(var)...', result))
