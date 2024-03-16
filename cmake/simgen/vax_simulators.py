## VAX simulators require extra magic -- notably, 'microvax3900${EXE}' needs
## to be symlinked, hardlinked or copied (in that order) to 'vax${EXE}'.

import simgen.basic_simulator as SBS

class BasicVAXSimulator(SBS.SIMHBasicSimulator):
    """
    """
    def __init__(self, sim_name, dir_macro, test_name, buildrom):
        super().__init__(sim_name, dir_macro, test_name, buildrom)

    def write_unit_test(self, stream, indent, individual=False, test_label='default'):
        stream.write('\n')
        self.write_section(stream, 'add_unit_test', indent, individual=False, test_label=test_label,
                           section_name='vax_cc_{}'.format(self.sim_name),
                           section_srcs=['vax_cc.c'],
                           section_incs=self.includes)

class VAXSimulator(BasicVAXSimulator):
    """
    """
    def __init__(self, sim_name, dir_macro, test_name, buildrom):
        super().__init__(sim_name, dir_macro, test_name, buildrom)

    def write_simulator(self, stream, indent, test_label='VAX'):
        super().write_simulator(stream, indent, test_label)
        stream.write('''
set(vax_binary_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
if (CMAKE_CONFIGURATION_TYPES)
    string(APPEND vax_binary_dir "/$<CONFIG>")
endif (CMAKE_CONFIGURATION_TYPES)

add_custom_command(TARGET vax POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
        -DSRCFILE=vax${CMAKE_EXECUTABLE_SUFFIX}
        -DDSTFILE=microvax3900${CMAKE_EXECUTABLE_SUFFIX}
        -DWORKING_DIR=${vax_binary_dir}
        -P ${CMAKE_SOURCE_DIR}/cmake/file-link-copy.cmake
    COMMENT "Symlink vax${CMAKE_EXECUTABLE_SUFFIX} to microvax3900${CMAKE_EXECUTABLE_SUFFIX}"
    WORKING_DIRECTORY ${vax_binary_dir})

install(
    CODE "
        execute_process(
            COMMAND ${CMAKE_COMMAND}
                -DSRCFILE=vax${CMAKE_EXECUTABLE_SUFFIX}
                -DDSTFILE=microvax3900${CMAKE_EXECUTABLE_SUFFIX}
                -DWORKING_DIR=\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/bin
                -P ${CMAKE_SOURCE_DIR}/cmake/file-link-copy.cmake)"
    COMPONENT vax_family)
''')
        stream.write('\n')

