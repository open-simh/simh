import pprint

import simgen.parse_makefile as SPM
import simgen.utils as SU

class SIMHBasicSimulator:
    """
    """
    def __init__(self, sim_name, dir_macro, test_name):
        self.sim_name = sim_name
        self.dir_macro = dir_macro
        self.test_name = test_name
        self.int64 = False
        self.full64 = False
        ## self.has_display -> True if there is a specific display used by the simulator.
        self.has_display = False
        ## self.uses_video   -> True if USE_SIM_VIDEO appears in the simulator's preprocessor defn's.
        self.uses_video = False
        self.sources = []
        self.defines = []
        self.includes = []

    def add_source(self, src):
        if src not in self.sources:
            self.sources.append(src)

    def add_include(self, incl):
        if incl not in self.includes:
            self.includes.append(incl)

    def add_define(self, define):
        if define not in self.defines:
            self.defines.append(define)

    def scan_for_flags(self, defs):
        """Scan for USE_INT64/USE_ADDR64 in the simulator's defines and set the
        'int64' and 'full64' instance variables. If found, these defines are
        removed. Also look for any of the "DISPLAY" make macros, and, if found,
        set the 'video' instance variable.
        """
        use_int64 = 'USE_INT64' in self.defines
        use_addr64 = 'USE_ADDR64' in self.defines
        if use_int64 or use_addr64:
            self.int64  = use_int64 and not use_addr64
            self.full64 = use_int64 and use_addr64
            try:
                self.defines.remove('USE_INT64')
            except:
                pass
            try:
                self.defines.remove('USE_ADDR64')
            except:
                pass

        ## Video support:

        self.has_display = any(map(lambda s: 'DISPLAY' in SPM.shallow_expand_vars(s, defs), self.sources))
        if self.has_display:
            try:
                self.sources.remove('${DISPLAYL}')
            except:
                pass
            try:
                self.sources.remove('$(DISPLAYL)')
            except:
                pass

        self.uses_video = 'USE_SIM_VIDEO' in self.defines

    def cleanup_defines(self):
        """Remove command line defines that aren't needed (because the CMake interface libraries
        already define them.)
        """
        for define in ['USE_SIM_CARD', 'USE_SIM_VIDEO', 'USE_NETWORK', 'USE_SHARED']:
            try:
                self.defines.remove(define)
            except:
                pass

    def get_source_vars(self):
        srcvars = set()
        for src in self.sources:
            srcvars = srcvars.union(set(SPM.extract_variables(src)))
        return srcvars

    def get_include_vars(self):
        incvars = set()
        for inc in self.includes:
            incvars = incvars.union(set(SPM.extract_variables(inc)))
        return incvars

    def write_section(self, stream, section, indent, individual=False, test_label='default',
                      additional_text=[], section_name=None, section_srcs=None, section_incs=None):
        indent4 = ' ' * (indent + 4)
        indent8 = ' ' * (indent + 8)

        stream.write(' ' * indent + '{}({}\n'.format(section, section_name))
        stream.write(' ' * (indent + 4) + 'SOURCES\n')
        stream.write('\n'.join(map(lambda src: indent8 + src, section_srcs)))
        if len(self.includes) > 0:
            stream.write('\n' + indent4 + 'INCLUDES\n')
            stream.write('\n'.join([ indent8 + inc for inc in section_incs]))
        if len(self.defines) > 0:
            stream.write('\n' + indent4 + 'DEFINES\n')
            stream.write('\n'.join(map(lambda dfn: indent8 + dfn, self.defines)))
        if self.int64:
            stream.write('\n' + indent4 + 'INT64')
        if self.full64:
            stream.write('\n' + indent4 + 'FULL64')
        if self.has_display:
            stream.write('\n' + indent4 + "VIDEO")
        if self.uses_video:
            stream.write('\n' + indent4 + "${{{0}_VIDEO}}".format(self.sim_name.upper()))
        stream.write('\n' + indent4 + "LABEL " + test_label)
        stream.write('\n' + '\n'.join(additional_text))
        stream.write(')\n')

    def write_simulator(self, stream, indent, individual=False, test_label='default'):
        if self.has_display or self.uses_video:
            stream.write(' ' * indent + 'if (BUILD_WITH_VIDEO)\n')
            indent += 4

        if self.uses_video:
            stream.write(' ' * indent + 'set({0}_VIDEO "VIDEO")\n'.format(self.sim_name.upper()))
            indent -= 4
            stream.write(' ' * indent + 'else (BUILD_WITH_VIDEO)\n')
            indent += 4
            stream.write(' ' * indent + 'set({0}_VIDEO "")\n'.format(self.sim_name.upper()))
            indent -= 4
            stream.write(' ' * indent + 'endif (BUILD_WITH_VIDEO)\n\n')

        if individual:
            ## When writing an individual CMakeList.txt, we can take advantage of the CMAKE_CURRENT_SOURCE_DIR
            ## as a replacement for the SIMH directory macro.
            srcs = [ src.replace(self.dir_macro + '/', '') for src in self.sources]
            incs = [ inc if inc != self.dir_macro else '${CMAKE_CURRENT_SOURCE_DIR}' for inc in self.includes]
        else:
            ## But when writing out the BF CMakeLists.txt, make everything explicit.
            srcs = self.sources
            incs = self.includes

        indent4 = ' ' * (indent + 4)

        addl_text = [
            indent4 + "TEST " + self.test_name,
        ]

        if not individual:
            addl_text.append(indent4 + "SOURCE_DIR " + self.dir_macro)

        self.write_section(stream, 'add_simulator', indent, individual, test_label, additional_text=addl_text,
                           section_name=self.sim_name, section_srcs=srcs, section_incs=incs)

        if self.has_display:
            indent -= 4
            stream.write(' ' * indent + 'endif (BUILD_WITH_VIDEO)\n')

    # Default: Don't generate a unit test CMakeFiles.txt.
    def write_unit_test(self, stream, indent, individual=False, test_label='default'):
        pass

    def __repr__(self):
        return '{0}({1},{2},{3},{4})'.format(self.__class__.__name__, self.sim_name.__repr__(),
            self.sources.__repr__(), self.includes.__repr__(), self.defines.__repr__())


class BESM6Simulator(SIMHBasicSimulator):
    """The (fine Communist) BESM6 simulator needs some extra code
    in the CMakeLists.txt to detect a suitable font that supports
    Cyrillic.
    """
    def __init__(self, sim_name, dir_macro, test_name):
        super().__init__(sim_name, dir_macro, test_name)
        self.defines.append("FONTFILE=${besm6_font}")

    def scan_for_flags(self, defs):
        super().scan_for_flags(defs)
        self.has_display = True

    def write_simulator(self, stream, indent, individual=False, test_label='besm6'):
        ## Fixups... :-)
        try:
            self.defines.remove('FONTFILE=$(FONTFILE)')
        except:
            pass
        try:
            self.defines.remove('FONTFILE=${FONTFILE}')
        except:
            pass

        ## Add the search for a font file.
        stream.write('\n'.join([
            'set(besm6_font)',
            'foreach (fdir IN ITEMS',
            '           "/usr/share/fonts" "/Library/Fonts" "/usr/lib/jvm"',
            '           "/System/Library/Frameworks/JavaVM.framework/Versions"',
            '           "$ENV{WINDIR}/Fonts")',
            '    foreach (font IN ITEMS',
            '                "DejaVuSans.ttf" "LucidaSansRegular.ttf"',
            '                "FreeSans.ttf" "AppleGothic.ttf" "tahoma.ttf")',
            '        if (EXISTS ${fdir}/${font})',
            '            get_filename_component(fontfile ${fdir}/${font} ABSOLUTE)',
            '            list(APPEND besm6_font ${fontfile})',
            '        endif ()',
            '    endforeach()',
            'endforeach()',
            '',
            'if (besm6_font)',
            '    list(LENGTH besm6_font besm6_font_len)',
            '    if (besm6_font_len GREATER 1)',
            '        message(STATUS "BESM6: Fonts found ${besm6_font}")',
            '    endif ()',
            '    list(GET besm6_font 0 besm6_font)',
            '    message(STATUS "BESM6: Using ${besm6_font}")',
            'endif (besm6_font)',
            '',
            'if (besm6_font)\n']))
        super().write_simulator(stream, indent + 4, individual, test_label)
        stream.write('endif ()\n')

class KA10Simulator(SIMHBasicSimulator):
    def __init__(self, sim_name, dir_macro, test_name):
        super().__init__(sim_name, dir_macro, test_name)

    def write_simulator(self, stream, indent, individual=False, test_label='ka10'):
        super().write_simulator(stream, indent, individual, test_label)
        stream.write('\n')
        stream.write('\n'.join([
            'if (PANDA_LIGHTS)',
            '  target_sources({0} PUBLIC {1}/ka10_lights.c)'.format(self.sim_name, self.dir_macro),
            '  target_compile_definitions({0} PUBLIC PANDA_LIGHTS)'.format(self.sim_name),
            '  target_link_libraries({0} PUBLIC usb-1.0)'.format(self.sim_name),
            'endif (PANDA_LIGHTS)'
        ]))
        stream.write('\n')

class IBM650Simulator(SIMHBasicSimulator):
    '''The IBM650 simulator creates relatively deep stacks, which will fail on Windows.
    Adjust target simulator link flags to provide a 8M stack, similar to Linux.
    '''
    def __init__(self, sim_name, dir_macro, test_name):
        super().__init__(sim_name, dir_macro, test_name)
        self.stack_size = 8 * 1024 * 1024

    def write_simulator(self, stream, indent, individual=False, test_label='ibm650'):
        super().write_simulator(stream, indent, individual, test_label)
        stream.write('\n')
        ## Link i650 with a 8M stack on windows
        stream.write('\n'.join([
            'if (WIN32)',
            '    if (MSVC)',
            '        set(I650_STACK_FLAG "/STACK:{0}")'.format(self.stack_size),
            '    else ()',
            '        set(I650_STACK_FLAG "-Wl,--stack,{0}")'.format(self.stack_size),
            '    endif ()',
            '    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.13")',
            '        target_link_options({0} PUBLIC "${{I650_STACK_FLAG}}")'.format(self.sim_name),
            '    else ()',
            '        set_property(TARGET {0} LINK_FLAGS " ${{I650_STACK_FLAG}}")'.format(self.sim_name),
            '    endif ()',
            'endif()'
        ]))


if '_dispatch' in pprint.PrettyPrinter.__dict__:
    def sim_pprinter(pprinter, sim, stream, indent, allowance, context, level):
        cls = sim.__class__
        stream.write(cls.__name__ + '(')
        indent += len(cls.__name__) + 1
        pprinter._format(sim.sim_name, stream, indent, allowance + 2, context, level)
        stream.write(',')
        pprinter._format(sim.dir_macro, stream, indent, allowance + 2, context, level)
        stream.write(',')
        pprinter._format(sim.int64, stream, indent, allowance + 2, context, level)
        stream.write(',')
        pprinter._format(sim.full64, stream, indent, allowance + 2, context, level)
        stream.write(',')
        pprinter._format(sim.has_display, stream, indent, allowance + 2, context, level)
        stream.write(',')
        pprinter._format(sim.sources, stream, indent, allowance + 2, context, level)
        stream.write(',\n' + ' ' * indent)
        pprinter._format(sim.defines, stream, indent, allowance + 2, context, level)
        stream.write(',\n' + ' ' * indent)
        pprinter._format(sim.includes, stream, indent, allowance + 2, context, level)
        stream.write(')')

    pprint.PrettyPrinter._dispatch[SIMHBasicSimulator.__repr__] = sim_pprinter
