import pprint

import simgen.parse_makefile as SPM
import simgen.utils as SU
import simgen.packaging as SPKG

class SIMHBasicSimulator:
    """
    """
    def __init__(self, sim_name, dir_macro, test_name, buildrom, test_args=None):
        self.sim_name = sim_name
        ## self.dir_macro     -> Directory macro (e.g., "${PDP11D}" for source
        self.dir_macro = dir_macro
        self.test_name = test_name
        self.int64 = False
        self.full64 = False
        self.buildrom = buildrom
        ## self.has_display    -> True if there is a specific display used by the simulator.
        self.has_display = False
        ## self.uses_video     -> True if USE_SIM_VIDEO appears in the simulator's preprocessor defn's.
        self.uses_video = False
        ## self.besm6_sdl_hack -> Only set/used by the BESM6 simulator.
        self.besm6_sdl_hack = False
        ## self.uses_aio       -> True if the simulator uses AIO
        self.uses_aio = False
        ## self.test_args      -> Simulator flags to pass to the test phase. Used by ibm1130 to
        ##                        pass "-g" to disable to the GUI. This argument can be a single
        ##                        string or a list.
        self.test_args = test_args

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
            for defn in ['USE_INT64', 'USE_ADDR64']:
                try:
                    self.defines.remove(defn)
                except:
                    pass

        ## Video support:

        self.has_display = any(map(lambda s: 'DISPLAY' in SPM.shallow_expand_vars(s, defs), self.sources))
        if self.has_display:
            for src in ['${DISPLAYL}', '$(DISPLAYL)']:
                try:
                    self.sources.remove(src)
                except:
                    pass

        self.uses_video = 'USE_SIM_VIDEO' in self.defines or self.has_display

        ## AIO support:
        self.uses_aio   = 'SIM_ASYNCH_IO' in self.defines
        if self.uses_aio:
            for defn in ['SIM_ASYNCH_IO', 'USE_READER_THREAD']:
                try:
                    self.defines.remove(defn)
                except:
                    pass

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

    def write_section(self, stream, section, indent, test_label='default', additional_text=[],
                      section_name=None, section_srcs=None, section_incs=None):
        indent4 = ' ' * (indent + 4)
        indent8 = ' ' * (indent + 8)

        pkg_info = SPKG.package_info.get(section_name)
        install_flag = pkg_info.install_flag if pkg_info is not None else None
        pkg_family = pkg_info.family.component_name if pkg_info is not None else None

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
            stream.write('\n' + indent4 + 'FEATURE_INT64')
        if self.full64:
            stream.write('\n' + indent4 + 'FEATURE_FULL64')
        if self.uses_video:
            stream.write('\n' + indent4 + "FEATURE_VIDEO")
        if self.has_display:
            stream.write('\n' + indent4 + "FEATURE_DISPLAY")
        if self.besm6_sdl_hack:
            stream.write('\n' + indent4 + "BESM6_SDL_HACK")
        if self.uses_aio:
            stream.write('\n' + indent4 + "USES_AIO")
        if self.test_args:
            out_args = self.test_args
            if isinstance(self.test_args, str):
                out_args = self.test_args.split()

            out_args = ' '.join('"{0}"'.format(w) for w in out_args)
            stream.write('\n' + indent4 + 'TEST_ARGS {}'.format(out_args))
        if self.buildrom:
            stream.write('\n' + indent4 + "BUILDROMS")
        stream.write('\n' + indent4 + "LABEL " + test_label)
        if install_flag:
            if pkg_family:
                stream.write('\n' + indent4 + "PKG_FAMILY " + pkg_family)
        else:
            stream.write('\n' + indent4 + "NO_INSTALL")
        stream.write('\n' + '\n'.join(additional_text))
        stream.write(')\n')

    def write_simulator(self, stream, indent, test_label='default'):
        ## When writing an individual CMakeList.txt, we can take advantage of the CMAKE_CURRENT_SOURCE_DIR
        ## as a replacement for the SIMH directory macro.
        srcs = [ src.replace(self.dir_macro + '/', '') for src in self.sources]
        incs = [ inc if inc != self.dir_macro else '${CMAKE_CURRENT_SOURCE_DIR}' for inc in self.includes]

        indent4 = ' ' * (indent + 4)

        addl_text = [
            indent4 + "TEST " + self.test_name,
        ]

        self.write_section(stream, 'add_simulator', indent, test_label, additional_text=addl_text,
                           section_name=self.sim_name, section_srcs=srcs, section_incs=incs)

    # Default: Don't generate a unit test CMakeFiles.txt. Yet.
    def write_unit_test(self, stream, indent, test_label='default'):
        pass

    def __repr__(self):
        return '{0}({1},{2},{3},{4})'.format(self.__class__.__name__, self.sim_name.__repr__(),
            self.sources.__repr__(), self.includes.__repr__(), self.defines.__repr__())


class BESM6Simulator(SIMHBasicSimulator):
    """The (fine Communist) BESM6 simulator needs some extra code
    in the CMakeLists.txt to detect a suitable font that supports
    Cyrillic.
    """
    def __init__(self, sim_name, dir_macro, test_name, buildrom):
        super().__init__(sim_name, dir_macro, test_name, buildrom)

    def scan_for_flags(self, defs):
        super().scan_for_flags(defs)

    def write_simulator(self, stream, indent, test_label='besm6'):
        ## Fixups... :-)
        for macro in ['FONTFILE=$(FONTFILE)', 'FONTFILE=${FONTFILE}']:
            try:
                self.defines.remove(macro)
            except:
                pass

        ## Add the search for a font file.
        stream.write('\n'.join([
          'set(besm6_font)',
          'set(cand_fonts',
          '      "DejaVuSans.ttf"',
          '      "LucidaSansRegular.ttf"',
          '      "FreeSans.ttf"',
          '      "AppleGothic.ttf"',
          '      "tahoma.ttf")',
          'set(cand_fontdirs',
          '      "/usr/share/fonts"',
          '      "/usr/lib/jvm"',
          '      "/Library/Fonts"',
          '      "/System/Library/Fonts"',
          '      "/System/Library/Frameworks/JavaVM.framework/Versions"',
          '      "$ENV{WINDIR}/Fonts")',
          '',
          'foreach (fdir ${cand_fontdirs})',
          '    foreach (font ${cand_fonts})',
          '        if (EXISTS ${fdir}/${font})',
          '            get_filename_component(fontfile ${fdir}/${font} ABSOLUTE)',
          '            list(APPEND besm6_font ${fontfile})',
          '        endif ()',
          '',
          '        file(GLOB besm6_font_cand_1 LIST_DIRECTORIES FALSE "${fdir}/*/${font}")',
          '        file(GLOB besm6_font_cand_2 LIST_DIRECTORIES FALSE "${fdir}/*/*/${font}")',
          '        file(GLOB besm6_font_cand_3 LIST_DIRECTORIES FALSE "${fdir}/*/*/*/${font}")',
          '        list(APPEND besm6_font ${besm6_font_cand_1} ${besm6_font_cand_2} ${besm6_font_cand_3})',
          '    endforeach()',
          'endforeach()',
          '',
          'if (besm6_font)',
          '    set(besm6_found_fonts "BESM6: Fonts found")',
          '    foreach(bfont ${besm6_font})',
          '        string(APPEND besm6_found_fonts "\n      .. ${bfont}")',
          '    endforeach ()',
          '    message(STATUS ${besm6_found_fonts})',
          '    unset(besm6_found_fonts)',
          '    list(GET besm6_font 0 besm6_font)',
          '    message(STATUS "BESM6: Using ${besm6_font}")',
          'else ()',
          '    set(besm6_no_fonts "BESM6: No applicable Cyrillic fonts found.")',
          '    string(APPEND besm6_no_fonts "\n    Font names tried:")',
          '    foreach (font ${cand_fonts})',
          '        string(APPEND besm6_no_fonts "\n      ..  ${font}")',
          '    endforeach ()',
          '    string(APPEND besm6_no_fonts "\n\n    Looked in:")',
          '    foreach (fdir ${cand_fontdirs})',
          '        string(APPEND besm6_no_fonts "\n      ..  ${fdir}")',
          '    endforeach()',
          '    string(APPEND besm6_no_fonts "\n\nBESM6: Not building with panel display.")',
          '    message(STATUS ${besm6_no_fonts})',
          '    unset(besm6_no_fonts)',
          'endif ()',
          '',
          'if (NOT (besm6_font AND WITH_VIDEO))\n']))
        super().write_simulator(stream, indent + 4, test_label)
        stream.write('else ()\n')
        self.defines.append("FONTFILE=${besm6_font}")
        self.has_display = True
        self.uses_video = True
        self.besm6_sdl_hack = True
        super().write_simulator(stream, indent + 4, test_label)
        stream.write('\n'.join([
            'endif()',
            'unset(cand_fonts)',
            'unset(cand_fontdirs)\n']))

class IBM650Simulator(SIMHBasicSimulator):
    '''The IBM650 simulator creates relatively deep stacks, which will fail on Windows.
    Adjust target simulator link flags to provide a 8M stack, similar to Linux.
    '''
    def __init__(self, sim_name, dir_macro, test_name, buildrom):
        super().__init__(sim_name, dir_macro, test_name, buildrom)
        self.stack_size = 8 * 1024 * 1024

    def write_simulator(self, stream, indent, test_label='ibm650'):
        super().write_simulator(stream, indent, test_label)
        stream.write('\n')
        ## Link i650 with a 8M stack on windows
        stream.write('\n'.join([
            'if (WIN32)',
            '    if (MSVC)',
            '        set(I650_STACK_FLAG "/STACK:{0}")'.format(self.stack_size),
            '    elseif (CMAKE_C_COMPILER_ID MATCHES ".*Clang")',
            '        set(I650_STACK_FLAG "-Xlinker" "/STACK:{0}")'.format(self.stack_size),
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
