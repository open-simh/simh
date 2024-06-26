## cmake_container.py
##
## A container for a collection of SIMH simulators
##
##

import sys
import os
import re
import pprint
import functools

import simgen.parse_makefile as SPM
import simgen.sim_collection as SC
import simgen.utils as SU

## Corresponding special variable uses in the makefile:
_special_sources = frozenset(['${DISPLAYL}', '$(DISPLAYL)'])

## Banner header for individual CMakeLists.txt files:
_individual_header = [
    '##',
    '## This is an automagically generated file. Do NOT EDIT.',
    '## Any changes you make will be overwritten!!',
    '##',
    '## Make changes to the SIMH top-level makefile and then run the',
    '## "cmake/generate.py" script to regenerate these files.',
    '##',
    '##     cd cmake; python -m generate --help',
    '##',
    '## ' + '-' * 60 + '\n'
    ]

## Banner header for individual unit test CMakeLists.txt files:
_unit_test_header = [
    '##',
    '## This is an automagically generated file. Do NOT EDIT.',
    '## Any changes you make will be overwritten!!',
    '##',
    '## If you need to make changes, modify write_unit_test()',
    '## method in the the underlying Python class in ',
    '## cmake/simgen/basic_simulator.py, then execute the',
    '## "cmake/generate.py" script to regenerate these files.',
    '##',
    '##     cd cmake; python -m generate --help',
    '##',
    '## ' + '-' * 60 + '\n'
    ]
## Unset the display variables when not building with video. A +10
## kludge.
_unset_display_vars = '''
if (NOT WITH_VIDEO)
    ### Hack: Unset these variables so that they don't expand if
    ### not building with video:
    set(DISPLAY340 "")
    set(DISPLAYIII "")
    set(DISPLAYNG  "")
    set(DISPLAYVT  "")
endif ()
'''

class CMakeBuildSystem:
    """A container for collections of SIMH simulators and automagic
    CMakeLists.txt driver.

    This is the top-level container that stores collections of SIMH simulators
    in the 'SIM'
    """

    ## Compile command line elements that are ignored.
    _ignore_compile_elems = frozenset(['${LDFLAGS}', '$(LDFLAGS)',
        '${CC_OUTSPEC}', '$(CC_OUTSPEC)',
        '${SIM}', '$(SIM)',
        '-o', '$@',
        '${NETWORK_OPT}', '$(NETWORK_OPT)',
        '${SCSI}', '$(SCSI)',
        '${DISPLAY_OPT}', '$(DISPLAY_OPT)',
        '${NETWORK_LDFLAGS}', '$(NETWORK_LDFLAGS)',
        '${VIDEO_CCDEFS}', '$(VIDEO_CCDEFS)',
        '${VIDEO_LDFLAGS}', '$(VIDEO_LDFLAGS)',
        '${BESM6_PANEL_OPT}', '$(BESM6_PANEL_OPT)'])

    def __init__(self):
        # "Special" variables that we look for in source code lists and which we'll
        # emit into the CMakeLists.txt files.
        self.vars = SC.ignored_display_macros.copy()
        # Subdirectory -> SimCollection mapping
        self.dirs = {}


    def extract(self, compile_action, test_name, sim_dir, sim_name, defs, buildrom, debug=0, depth=''):
        """Extract sources, defines, includes and flags from the simulator's
        compile action in the makefile.
        """
        sim_dir_path = SPM.expand_vars(sim_dir, defs).replace('./', '')
        simcoll = self.dirs.get(sim_dir_path)
        if simcoll is None:
            simcoll = SC.SimCollection(sim_dir)
            self.dirs[sim_dir_path] = simcoll

        sim = simcoll.get_simulator(sim_name, sim_dir, sim_dir_path, test_name, buildrom)

        # Remove compile command line elements and do one level of variable expansion, split the resulting
        # string into a list.
        all_comps = []
        for comp in [ SPM.normalize_variables(act) for act in compile_action.split() if not self.compile_elems_to_ignore(act) ]:
            all_comps.extend(comp.split())

        if debug >= 3:
            print('{0}all_comps after filtering:'.format(depth))
            pprint.pp(all_comps)

        # Iterate through the final compile component list and extract source files, includes
        # and defines.
        #
        # Deferred: Looking for options that set the i64, z64, video library options.
        while all_comps:
            comp = all_comps[0]
            # print(':: comp {0}'.format(comp))
            if self.is_source(comp):
                # Source file...
                ## sim.add_source(comp.replace(sim_dir + '/', ''))
                sim.add_source(comp)
            elif comp.startswith('-I'):
                all_comps = self.process_flag(all_comps, defs, sim.add_include, depth)
            elif comp.startswith('-D'):
                all_comps = self.process_flag(all_comps, defs, sim.add_define, depth)
            elif comp.startswith('-L') or comp.startswith('-l'):
                ## It's a library path or library. Skip.
                pass
            else:
                # Solitary variable expansion?
                m = SPM._var_rx.match(comp)
                if m:
                    varname = m.group(1)
                    if varname not in SC._special_vars:
                        if not self.is_source_macro(comp, defs):
                            expand = [ SPM.normalize_variables(elem) for elem in SPM.shallow_expand_vars(comp, defs).split()
                                       if not self.source_elems_to_ignore(elem) ]
                            SU.emit_debug(debug, 3, '{0}var expanded {1} -> {2}'.format(depth, comp, expand))
                            all_comps[1:] = expand + all_comps[1:]
                        else:
                            ## Source macro
                            self.collect_source_macros(comp, defs, varname, simcoll, sim, debug, depth)
                    else:
                        sim.add_source(comp)
                else:
                    # Nope.
                    print('{0}unknown component: {1}'.format(depth, comp))
            all_comps = all_comps[1:]

        sim.scan_for_flags(defs)
        sim.cleanup_defines()

        if debug >= 2:
            pprint.pprint(sim)


    def compile_elems_to_ignore(self, elem):
        return (elem in self._ignore_compile_elems or elem.endswith('_LDFLAGS)') or elem.endswith('_LDFLAGS}'))

    def source_elems_to_ignore(self, elem):
        return self.compile_elems_to_ignore(elem) or elem in _special_sources


    def is_source_macro(self, var, defs):
        """Is the macro/variable a list of sources?
        """
        expanded = SPM.expand_vars(var, defs).split()
        # print('is_source_macro {}'.format(expanded))
        return all(map(lambda src: self.is_source(src), expanded))


    def is_source(self, thing):
        return thing.endswith('.c')


    def process_flag(self, comps, defs, process_func, depth):
        if len(comps[0]) > 2:
            # "-Ddef"
            val = comps[0][2:]
        else:
            # "-D def"
            val = comps[1]
            comps = comps[1:]
        m = SPM._var_rx.match(val)
        if m:
            var = m.group(1)
            ## Gracefully deal with undefined variables (ATT3B2M400B2D is a good example)
            if var in defs:
                if var not in self.vars:
                    self.vars[var] = defs[var]
            else:
                print('{0}undefined make macro: {1}'.format(depth, var))

        process_func(val)
        return comps


    def collect_vars(self, defs, debug=0):
        """Add indirectly referenced macros and variables, adding them to the defines dictionary.

        Indirectly referenced macros and variables are macros and variables embedded in existing
        variables, source macros and include lists. For example, SIMHD is an indirect reference
        in "KA10D = ${SIMHD}/ka10" because KA10D might never have been expanded by 'extract()'.
        """

        def scan_var(varset, var):
            tmp = var
            return varset.union(set(SPM.extract_variables(tmp)))

        def replace_simhd(l, v):
            l.append(v.replace('SIMHD', 'CMAKE_SOURCE_DIR'))
            return l

        simvars = set()
        for v in self.vars.values():
            if isinstance(v, list):
                simvars = functools.reduce(scan_var, v, simvars)
            else:
                simvars = scan_var(simvars, v)

        for dir in self.dirs.keys():
            simvars = simvars.union(self.dirs[dir].get_simulator_vars(debug))

        if debug >= 2:
            print('Collected simvars:')
            pprint.pprint(simvars)

        for var in simvars:
            if var not in self.vars:
                if var in defs:
                    self.vars[var] = defs[var]
                else:
                    print('{0}: variable not defined.'.format(var))

        ## Replace SIMHD with CMAKE_SOURCE_DIR
        for k, v in self.vars.items():
            if isinstance(v, list):
                v = functools.reduce(replace_simhd, v, [])
            else:
                v = v.replace('SIMHD', 'CMAKE_SOURCE_DIR')
            self.vars[k] = v

    def collect_source_macros(self, comp, defs, varname, simcoll, sim, debug=0, depth=''):
        def inner_sources(srcmacro):
            for v in srcmacro:
                if not self.source_elems_to_ignore(v):
                    m = SPM._var_rx.match(v)
                    if m is not None:
                        vname = m.group(1)
                        vardef = defs.get(vname)
                        if vardef is not None:
                            ## Convert the macro variable into a list
                            vardef = [ SPM.normalize_variables(v) \
                                       for v in vardef.split() if not self.source_elems_to_ignore(v) ]
                            SU.emit_debug(debug, 3, '{0}source macro: {1} -> {2}'.format(depth, vname, vardef))
                            simcoll.add_source_macro(vname, vardef, sim)
                            ## Continue into the macro variable's definitions
                            inner_sources(vardef)

        vardef = defs.get(varname)
        if vardef is not None:
            vardef = [ SPM.normalize_variables(v) \
                       for v in vardef.split() if not self.source_elems_to_ignore(v) ]
            SU.emit_debug(debug, 3, '{0}source macro: {1} -> {2}'.format(depth, varname, vardef))
            simcoll.add_source_macro(varname, vardef, sim)
            inner_sources(vardef)
            SU.emit_debug(debug, 3, '{0}source added: {1}'.format(depth, comp))
            sim.add_source(comp)
        else:
            print('{0}undefined make macro: {1}'.format(depth, varname))

    def write_vars(self, stream):
        def collect_vars(varlist, var):
            varlist.extend(SPM.extract_variables(var))
            return varlist

        varnames = list(self.vars.keys())
        namewidth = max(map(lambda s: len(s), varnames))
        # vardeps maps the parent variable to its dependents, e.g.,
        # INTELSYSD -> [ISYS8010D, ...]
        vardeps = dict()
        # alldeps is the set of all parent and dependents, which will be
        # deleted from a copy of self.vars. The copy has variables that
        # don't depend on anything (except for CMAKE_SOURCE_DIR, but we
        # know that's defined by CMake.)
        alldeps = set()
        for var in varnames:
            if isinstance(self.vars[var], list):
                mvars = functools.reduce(collect_vars, self.vars[var], [])
            else:
                mvars = SPM.extract_variables(self.vars[var])
            mvars = [mvar for mvar in mvars if mvar != "CMAKE_SOURCE_DIR"]
            if mvars:
                alldeps.add(var)
                for mvar in mvars:
                    if mvar not in vardeps:
                        vardeps[mvar] = []
                    vardeps[mvar].append(var)
                    alldeps.add(mvar)

        nodeps = self.vars.copy()
        ## SIMHD will never be used.
        if 'SIMHD' in nodeps:
            del nodeps['SIMHD']
        for dep in alldeps:
            del nodeps[dep]

        varnames = list(nodeps.keys())
        varnames.sort()
        for var in varnames:
            self.emit_value(var, self.vars[var], stream, namewidth)

        ## Now to emit the dependencies
        depnames = list(vardeps.keys())
        depnames.sort()
        for dep in depnames:
            self.write_dep(dep, vardeps, alldeps, namewidth, stream)

        ## stream.write('\n## ' + '-' * 40 + '\n')

    def write_dep(self, dep, vardeps, alldeps, width, stream):
        # Not the most efficient, but it works
        alldeps.discard(dep)
        for parent in [ v for v in vardeps.keys() if dep in vardeps[v]]:
            if dep in parent.values():
                self.write_dep(parent, vardeps, alldeps, width, stream)

        stream.write('\n')
        self.emit_value(dep, self.vars[dep], stream, width)

        children = vardeps[dep]
        children.sort()
        for child in children:
            if child in alldeps:
                self.emit_value(child, self.vars[child], stream, width)
        alldeps -= set(children)

    def emit_value(self, var, value, stream, width=0):
        if isinstance(value, list):
            stream.write('set({:{width}} {})\n'.format(var, ' '.join(map(lambda s: '"' + s + '"', value)),
                                                       width=width))
        else:
            stream.write('set({:{width}} "{}")\n'.format(var, value, width=width))

    def write_simulators(self, toplevel_dir, debug=0):
        dirnames = list(self.dirs.keys())
        dirnames.sort()

        for subdir in dirnames:
            simcoll = self.dirs[subdir]
            test_label = subdir
            # Group tests under subdirectories together
            has_slash = test_label.find('/')
            if has_slash < 0:
                has_slash = test_label.find('\\')
            if has_slash >= 0:
                test_label = test_label[:has_slash]
            ## Write out individual CMakeLists.txt:
            subdir_cmake = os.path.join(toplevel_dir, subdir, 'CMakeLists.txt')
            print('==== writing to {0}'.format(subdir_cmake))
            with open(subdir_cmake, "w", newline='\r\n') as stream2:
                plural = '' if len(self.dirs[subdir]) == 1 else 's'
                stream2.write('## {} simulator{plural}\n'.format(subdir, plural=plural))
                stream2.write('\n'.join(_individual_header))
                stream2.write('\n')
                stream2.write('if (HAVE_UNITY_FRAMEWORK AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/unit-tests/CMakeLists.txt")\n')
                stream2.write('  add_subdirectory(unit-tests)\n')
                stream2.write('endif ()')
                stream2.write('\n')
                simcoll.write_simulators(stream2, debug=debug, test_label=test_label)

            ## Write out unit tests, if the unit test subdirectory exists:
            subdir_units_dir = os.path.join(toplevel_dir, subdir, 'unit-tests')
            subdir_units = os.path.join(subdir_units_dir, 'CMakeLists.txt')
            if os.path.exists(subdir_units):
                with open(subdir_units, "w") as stream2:
                    plural = '' if len(self.dirs[subdir]) == 1 else 's'
                    stream2.write('## {} simulator{plural}\n'.format(subdir, plural=plural))
                    stream2.write('\n'.join(_unit_test_header))
                    stream2.write('\n')
                    simcoll.write_unit_tests(stream2, debug, subdir)

        simh_subdirs = os.path.join(toplevel_dir, 'cmake', 'simh-simulators.cmake')
        print("==== writing {0}".format(simh_subdirs))
        with open(simh_subdirs, "w") as stream2:
            stream2.write('\n'.join(_individual_header))
            self.write_vars(stream2)
            stream2.write('\n## ' + '-' * 40 + '\n')
            stream2.write(_unset_display_vars)
            stream2.write('\n## ' + '-' * 40 + '\n\n')
            stmts = [ 'add_subdirectory(' + dir + ')' for dir in dirnames ]
            stream2.write('\n'.join(stmts))
            stream2.write('\n')

    ## Representation when printed
    def __repr__(self):
        return '{0}({1}, {2})'.format(self.__class__.__name__, self.dirs.__repr__(), self.vars.__repr__())


if '_dispatch' in pprint.PrettyPrinter.__dict__:
    def cmake_pprinter(pprinter, cmake, stream, indent, allowance, context, level):
        cls = cmake.__class__
        stream.write(cls.__name__ + '(')
        indent += len(cls.__name__) + 1
        pprinter._format(cmake.dirs, stream, indent, allowance + 2, context, level)
        stream.write(',\n' + ' ' * indent)
        pprinter._format(cmake.vars, stream, indent, allowance + 2, context, level)
        stream.write(')')

    pprint.PrettyPrinter._dispatch[CMakeBuildSystem.__repr__] = cmake_pprinter
