## generate.py
##
## Generate the simulator CMakeLists.txt from the top-level makefile.
##
## This is the top-level driver: process options, search for the
## makefile, parse the makefile and walk its dependencies, and,
## finally, output the CMakeLists.txt(s) and simh-simulators.cmake.
##
## Author: B. Scott Michel
## ("scooter me fecit")

import sys
import os.path
import argparse
import re

GEN_SCRIPT_DIR = os.path.dirname(__file__)
GEN_SCRIPT_NAME = os.path.basename(__file__)

import pprint

import simgen.cmake_container as SCC
import simgen.parse_makefile as SPM
import simgen.packaging as SPKG


def process_makefile(makefile_dir, debug=0):
    the_makefile = os.path.join(makefile_dir, "makefile")
    print('{0}: Processing {1}'.format(GEN_SCRIPT_NAME, the_makefile))

    (defs, rules, actions) = SPM.parse_makefile(the_makefile)
    if debug >= 4:
        pprint.pp(defs)

    all_rule = rules.get('all') or rules.get('ALL')
    if all_rule is None:
        print('{0}: "all" rule not found. Cannot proceed.'.format(GEN_SCRIPT_NAME))

    simulators = SCC.CMakeBuildSystem()
    for all_targ in SPM.shallow_expand_vars(all_rule, defs).split():
        print("{0}: all target {1}".format(GEN_SCRIPT_NAME, all_targ))
        walk_target_deps(all_targ, defs, rules, actions, simulators, debug=debug)

    experimental_rule = rules.get('experimental')
    if experimental_rule is not None:
        for experimental_targ in SPM.shallow_expand_vars(experimental_rule, defs).split():
            print("{0}: exp target {1}".format(GEN_SCRIPT_NAME, experimental_targ))
            walk_target_deps(experimental_targ, defs, rules, actions, simulators, debug=debug)

    simulators.collect_vars(defs, debug=debug)
    return simulators


## Makefile target dependencies to filter out.
_ignored_deps = [
    '${SIM}',
    '${BUILD_ROMS}'
]

## Simulator compile/link action pattern
_compile_act_rx = re.compile(r"\$[({]CC[)}]\s*(.*)")
_test_name_rx    = re.compile(r"\$@\s*\$\(call\s+find_test,\s*(.*),(.*)\)\s+\$")

def walk_target_deps(target, defs, rules, actions, simulators, depth='', debug=0):
    """ Recursively walk a target's dependencies, i.e., the right hand side of a make rule.
    Descend into each dependency to find something that looks like a simulator's
    source code list. Once source code list is found, extract simulator defines, includes,
    source files and set flags.
    """
    if debug >= 1:
        print('{0}-- target: {1}'.format(depth, target))

    target_deps = SPM.target_dep_list(target, rules, defs)
    if debug >= 2:
        print('{0}target deps: {1}'.format(depth, target_deps))

    has_buildrom = any(filter(lambda dep: dep == '${BUILD_ROMS}', target_deps))
    if debug >= 1:
        print('{0}   has_buildrom {1}', has_buildrom)

    deps = [dep for dep in target_deps if dep not in _ignored_deps]
    targ_actions = actions.get(target)
    if targ_actions:
        depth3 = depth + '   '
        if debug >= 2:
            print('{0}deps {1}'.format(depth3, deps))

        # Are the dependencies a source code list?
        expanded_deps = [l for slist in [ SPM.shallow_expand_vars(dep, defs).split() for dep in deps ] for l in slist]
        if debug >= 3:
            print('{0}expanded_deps {1}'.format(depth3, expanded_deps))

        if any(filter(lambda f: f.endswith('.c'), expanded_deps)):
            if debug >= 1:
                print('{0}sim sources {1}'.format(depth3, deps))
            if debug >= 2:
                print('{0}targ_actions {1}'.format(depth3, targ_actions))

            # The simulators' compile and test actions are very regular and easy to find:
            compile_act = None
            test_name   = None
            sim_dir     = None
            for act in targ_actions:
                m_cact = _compile_act_rx.match(act)
                m_test = _test_name_rx.match(act)
                if m_cact:
                    compile_act = m_cact.group(1)
                elif m_test:
                    (sim_dir, test_name) = m_test.group(1, 2)

            if debug >= 2:
                print('{0}sim_dir     {1}'.format(depth3, sim_dir))
                print('{0}compile_act {1}'.format(depth3, compile_act))
                print('{0}test_name   {1}'.format(depth3, test_name))

            if compile_act and test_name and sim_dir:
                sim_name = target.replace("${BIN}", "").replace("${EXE}", "")
                # Just in case there are vestiges of old-style make variables
                sim_name = sim_name.replace("$(BIN)", "").replace("$(EXE)", "")
                if debug >= 2:
                    print('{0}sim_name    {1}'.format(depth3, sim_name))

                simulators.extract(compile_act, test_name, sim_dir, sim_name, defs, has_buildrom, debug, depth+'  ')
    else:
        # No actions associated with the dependency(ies), which means that the dependency(ies)
        # are meta-targets. Continue to walk.
        for dep in deps:
            walk_target_deps(dep, defs, rules, actions, simulators, depth=depth+'  ', debug=debug)


if __name__ == '__main__':
    args = argparse.ArgumentParser(description="SIMH simulator CMakeLists.txt generator.")
    args.add_argument('--debug', nargs='?', const=1, default=0, type=int,
                      help='Debug level (0-3, 0 == off)')
    args.add_argument('--srcdir', default=None,
                      help='makefile source directory.')
    args.add_argument('--skip-orphans', action='store_true',
                      help='Skip the check for packaging orphans')

    flags = vars(args.parse_args())

    debug_level = flags.get('debug')
    makefile_dir = flags.get('srcdir')

    found_makefile = True
    if makefile_dir is None:
        ## Find the makefile, which should be one directory up from this Python
        ## module
        makefile_dir = GEN_SCRIPT_DIR
        print('{0}: Looking for makefile, starting in {1}'.format(GEN_SCRIPT_NAME, makefile_dir))
        the_makefile = ''
        while makefile_dir:
            the_makefile = os.path.join(makefile_dir, "makefile")
            if os.path.exists(the_makefile):
                break
            else:
                makefile_dir = os.path.dirname(makefile_dir)
                print('{0}: Looking for makefile, trying {1}'.format(GEN_SCRIPT_NAME, makefile_dir))

        if not the_makefile:
            found_makefile = False
    else:
        the_makefile = os.path.join(makefile_dir, "makefile")
        if not os.path.exists(the_makefile):
            found_makefile = False

    if not found_makefile:
        print('{0}: SIMH top-level makefile not found, relative to {1}'.format(GEN_SCRIPT_NAME, GEN_SCRIPT_DIR))
        sys.exit(1)

    sims = process_makefile(makefile_dir, debug=debug_level)

    ## Sanity check: Make sure that all of the simulators in SPKG.package_info have
    ## been encountered
    for simdir in sims.dirs.keys():
        if debug_level >= 2:
            print('{}: simdir {}'.format(GEN_SCRIPT_NAME, simdir))
        for sim in sims.dirs[simdir].simulators.keys():
            SPKG.package_info[sim].encountered()

    if not flags.get('skip_orphans'):
        print('{0}: Expecting to emit {1} simulators.'.format(GEN_SCRIPT_NAME, len(SPKG.package_info.keys())))

        orphans = [ sim for sim, pkg_info in SPKG.package_info.items() if not pkg_info.was_processed() ]
        if len(orphans) > 0:
            print('{0}: Simulators not extracted from makefile:'.format(GEN_SCRIPT_NAME))
            for orphan in orphans:
                print('{0}{1}'.format(' ' * 4, orphan))
            sys.exit(1)
        else:
            print('{0}: All simulators present and accounted for!'.format(GEN_SCRIPT_NAME))

    if debug_level >= 1:
        pp = pprint.PrettyPrinter()
        pp.pprint(sims)

    ## Emit all of the individual CMakeLists.txt
    sims.write_simulators(makefile_dir, debug=debug_level)
    ## Emit the packaging data
    SPKG.write_packaging(makefile_dir)
