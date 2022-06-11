You fill in sim_instr do the instruction decoding and execution.

sim_load will typically take some input file and put it in memory.

cpu_reg should contain all machine state.

parse_sym/fprint_sym is to assemble (with DEPOSIT) and disassemble (EXAMINE-M) instructions.

sim_devices is an array of DEVICE * for peripherals.  Something like
build_dev_tab will go through the array and initialize data structures
at run time.

Refer to this: https://github.com/open-simh/simh/blob/master/doc/simh.doc

Make use of asynchronous events.  sim_activate posts a future event.
The "svc" routine will be called.
