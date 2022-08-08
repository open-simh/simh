
import simgen.basic_simulator as SBS

class BasicVAXSimulator(SBS.SIMHBasicSimulator):
    """
    """
    def __init__(self, sim_name, dir_macro, test_name):
        super().__init__(sim_name, dir_macro, test_name)

    def write_unit_test(self, stream, indent, individual=False, test_label='default'):
        stream.write('\n')
        self.write_section(stream, 'add_unit_test', indent, individual=False, test_label=test_label,
                           section_name='vax_cc_{}'.format(self.sim_name),
                           section_srcs=['vax_cc.c'],
                           section_incs=self.includes)

class VAXSimulator(BasicVAXSimulator):
    """
    """
    def __init__(self, sim_name, dir_macro, test_name):
        super().__init__(sim_name, dir_macro, test_name)

    def write_simulator(self, stream, indent, individual=False, test_label='VAX'):
        super().write_simulator(stream, indent, individual, test_label)
        stream.write('\n')
        stream.write('\n'.join([
            'set(vax_symlink_dir_src ${CMAKE_CURRENT_BINARY_DIR})',
            'if (CMAKE_CONFIGURATION_TYPES)',
            '    string(APPEND vax_symlink_dir_src "/$<CONFIG>")',
            'endif (CMAKE_CONFIGURATION_TYPES)',
            ' '.join(['add_custom_command(TARGET vax POST_BUILD',
                      ' '.join(['COMMAND "${CMAKE_COMMAND}" -E copy_if_different',
                                'vax${CMAKE_EXECUTABLE_SUFFIX}',
                                'microvax3900${CMAKE_EXECUTABLE_SUFFIX}']),
                                'COMMENT '
                                  '"Copy vax${CMAKE_EXECUTABLE_SUFFIX} to '
                                  'microvax3900${CMAKE_EXECUTABLE_SUFFIX}"',
                                'WORKING_DIRECTORY ${vax_symlink_dir_src})']),
                      'install(FILES ${vax_symlink_dir_src}/microvax3900${CMAKE_EXECUTABLE_SUFFIX}',
                      '        DESTINATION ${CMAKE_INSTALL_BINDIR})']))
        stream.write('\n')

