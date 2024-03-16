## KA10 simulator: Add the PANDA_LIGHTS and the PIDP10 options for frontpanel
## code.

import simgen.basic_simulator as SBS

class KA10Simulator(SBS.SIMHBasicSimulator):
    def __init__(self, sim_name, dir_macro, test_name, buildrom):
        super().__init__(sim_name, dir_macro, test_name, buildrom)

    def write_simulator(self, stream, indent, test_label='ka10'):
        ## Keep the CMake options separate, just in case they are needed for more
        ## than just the KA10 (and add a class variable so they are only written
        ## once.)
        stream.write('''
option(PANDA_LIGHTS
       "Enable (=1)/disable (=0) KA-10/KI-11 simulator\'s Panda display. (def: disabled)"
       FALSE)
option(PIDP10
       "Enable (=1)/disable (=0) PIDP10 display options (def: disabled)"
       FALSE)

### Ensure that the options are mutually exclusive:
if (PANDA_LIGHTS AND PIDP10)
  message(FATAL_ERROR "PANDA_LIGHTS and PIDP10 options are mutually exclusive. Choose one.")
endif ()


''')
        ## Emit the simulator:
        super().write_simulator(stream, indent, test_label)
        ## Update the display sources for PANDA_LIGHTS or PIDP10:
        stream.write('\n'.join([
            '',
            'if (PANDA_LIGHTS)',
            '  target_sources({0} PUBLIC {1}/kx10_lights.c)'.format(self.sim_name, self.dir_macro),
            '  target_compile_definitions({0} PUBLIC PANDA_LIGHTS)'.format(self.sim_name),
            '  target_link_libraries({0} PUBLIC usb-1.0)'.format(self.sim_name),
            'endif ()',
            'if (PIDP10)',
            '  target_sources({0} PUBLIC {1}/ka10_pipanel.c)'.format(self.sim_name, self.dir_macro),
            '  target_compile_definitions({0} PUBLIC PIDP10=1)'.format(self.sim_name),
            'endif ()'
        ]))
        stream.write('\n')

