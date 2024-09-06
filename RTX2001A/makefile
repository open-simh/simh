# RTX-2001A
RTX2001AD = ${SIMHD}/RTX2001A/
RTX2001AD_SRC = ${RTX2001AD}src/

RTX2001A_SRC = \
	${RTX2001AD_SRC}rtx2001a_registers.h \
	${RTX2001AD_SRC}rtx2001a_defs.h \
	${RTX2001AD_SRC}rtx2001a_debug.h \
	${RTX2001AD_SRC}rtx2001a_cpu.c \
	${RTX2001AD_SRC}rtx2001a_sys.c \
	${RTX2001AD_SRC}rtx2001a_decode.c \
	${RTX2001AD_SRC}rtx2001a_decode.h \
	${RTX2001AD_SRC}rtx2001a_execute.c \
	${RTX2001AD_SRC}rtx2001a_execute.h \
	${RTX2001AD_SRC}rtx2001a_ab.c \
	${RTX2001AD_SRC}rtx2001a_ab.h \
	${RTX2001AD_SRC}rtx2001a_mb.c \
	${RTX2001AD_SRC}rtx2001a_mb.h \
	${RTX2001AD_SRC}rtx2001a_psb.c \
	${RTX2001AD_SRC}rtx2001a_psb.h \
	${RTX2001AD_SRC}rtx2001a_rsb.c \
	${RTX2001AD_SRC}rtx2001a_rsb.h

# rtx2001a lib
RTX2001A_LIB = rtx2001a.a

${RTX2001AD}${RTX2001A_LIB} : ${SIM} ${RTX2001A_SRC}
	${GCC} -c -fPIC ${CC_STD} -U__STRICT_ANSI__ ${CFLAGS_G} \
		${CFLAGS_O} ${CFLAGS_GIT} ${CFLAGS_I} \
		-DSIM_COMPILER="${COMPILER_NAME}" -DSIM_VERSION_MODE="${SIM_VERSION_MODE}" \
		-DSIM_BUILD_TOOL=simh-makefile  ${OS_CCDEFS} ${ROMS_OPT} ${SIM} ${RTX2001A_SRC}
	ar vrcs ${RTX2001AD}${RTX2001A_LIB} rtx2001a_*.o sim_*.o

# rtx2001a
rtx2001a : ${BIN}rtx2001a${EXE}

#  ${STUB_OPT}
${BIN}rtx2001a${EXE} : ${SIMH_LIB} ${RTX2001AD}${RTX2001A_LIB}
	${MKDIRBIN}
	${CC} scp.o ${SIMH_LIB} ${RTX2001AD}${RTX2001A_LIB} ${CC_OUTSPEC} ${LDFLAGS}

# tests
OS_CCDEFS_TEST = ${OS_CCDEFS}
OS_CCDEFS_TEST += -Wall
OS_CCDEFS_TEST += -Wextra
OS_CCDEFS_TEST += -Wpointer-arith
OS_CCDEFS_TEST += -Wcast-align
OS_CCDEFS_TEST += -Wwrite-strings
OS_CCDEFS_TEST += -Wswitch-default
OS_CCDEFS_TEST += -Wunreachable-code
OS_CCDEFS_TEST += -Winit-self
OS_CCDEFS_TEST += -Wmissing-field-initializers
OS_CCDEFS_TEST += -Wno-unknown-pragmas
OS_CCDEFS_TEST += -Wstrict-prototypes
OS_CCDEFS_TEST += -Wundef
OS_CCDEFS_TEST += -Wold-style-definition

UNITYD_SRC=../Unity/src/

RTX2001AD_TEST = ${RTX2001AD}test/

RTX2001A_TEST_MB_SRC =  \
	${RTX2001AD_TEST}test_mb.c \
	${RTX2001AD_TEST}test_mb_long_store_fetch.c \
	${RTX2001AD_TEST}test_mb_byte_store_fetch.c \
	${RTX2001AD_TEST}test_mb_store_fetch.c \
	${RTX2001AD_TEST}scp_stub.c \
	$(UNITYD_SRC)unity.c

RTX2001A_TEST_AB_SRC =  \
	${RTX2001AD_TEST}test_ab.c \
	${RTX2001AD_TEST}test_ab_gstore.c \
	${RTX2001AD_TEST}test_ab_gfetch.c \
	${RTX2001AD_TEST}scp_stub.c \
	$(UNITYD_SRC)unity.c

RTX2001A_TEST_PSB_SRC =  \
	${RTX2001AD_TEST}test_psb.c \
	${RTX2001AD_TEST}scp_stub.c \
	$(UNITYD_SRC)unity.c

RTX2001A_TEST_RSB_SRC =  \
	${RTX2001AD_TEST}test_rsb.c \
	${RTX2001AD_TEST}scp_stub.c \
	$(UNITYD_SRC)unity.c

RTX2001A_TEST_DECODE_SRC =  \
	${RTX2001AD_TEST}test_decode.c \
	${RTX2001AD_TEST}scp_stub.c \
	$(UNITYD_SRC)unity.c

RTX2001A_TEST_EXECUTE_SRC =  \
	${RTX2001AD_TEST}test_execute.c \
	${RTX2001AD_TEST}scp_stub.c \
	$(UNITYD_SRC)unity.c

rtx2001a_test : \
	${BIN}test_mb${EXE} \
	${BIN}test_ab${EXE} \
	${BIN}test_psb${EXE} \
	${BIN}test_rsb${EXE} \
	${BIN}test_decode${EXE} \
	${BIN}test_execute${EXE}

${BIN}test_mb${EXE} : ${RTX2001AD}${RTX2001A_LIB} ${RTX2001A_TEST_MB_SRC} ${RTX2001A_SRC}
	${MKDIRBIN}
	${CC} -I${UNITYD_SRC} -I${RTX2001AD_SRC} \
	${RTX2001A_TEST_MB_SRC} \
	${LDFLAGS} \
	${RTX2001AD}${RTX2001A_LIB} \
	${CC_OUTSPEC}

${BIN}test_ab${EXE} : ${RTX2001AD}${RTX2001A_LIB} ${RTX2001A_TEST_AB_SRC} ${RTX2001A_SRC}
	${MKDIRBIN}
	${CC} -I${UNITYD_SRC} -I${RTX2001AD_SRC} \
	${RTX2001A_TEST_AB_SRC} \
	${LDFLAGS} \
	${RTX2001AD}${RTX2001A_LIB} \
	${CC_OUTSPEC}

${BIN}test_psb${EXE} : ${RTX2001AD}${RTX2001A_LIB} ${RTX2001A_TEST_PSB_SRC} ${RTX2001A_SRC}
	${MKDIRBIN}
	${CC} -I${UNITYD_SRC} -I${RTX2001AD_SRC} \
	${RTX2001A_TEST_PSB_SRC} \
	${LDFLAGS} \
	${RTX2001AD}${RTX2001A_LIB} \
	${CC_OUTSPEC}

${BIN}test_rsb${EXE} : ${RTX2001AD}${RTX2001A_LIB} ${RTX2001A_TEST_RSB_SRC} ${RTX2001A_SRC}
	${MKDIRBIN}
	${CC} -I${UNITYD_SRC} -I${RTX2001AD_SRC} \
	${RTX2001A_TEST_RSB_SRC} \
	${LDFLAGS} \
	${RTX2001AD}${RTX2001A_LIB} \
	${CC_OUTSPEC}

${BIN}test_decode${EXE} : ${RTX2001AD}${RTX2001A_LIB} ${RTX2001A_TEST_DECODE_SRC} ${RTX2001A_SRC}
	${MKDIRBIN}
	${CC} -I${UNITYD_SRC} -I${RTX2001AD_SRC} \
	${RTX2001A_TEST_DECODE_SRC} \
	${LDFLAGS} \
	${RTX2001AD}${RTX2001A_LIB} \
	${CC_OUTSPEC}

${BIN}test_execute${EXE} : ${RTX2001AD}${RTX2001A_LIB} ${RTX2001A_TEST_EXECUTE_SRC} ${RTX2001A_SRC}
	${MKDIRBIN}
	${CC} -I${UNITYD_SRC} -I${RTX2001AD_SRC} \
	${RTX2001A_TEST_EXECUTE_SRC} \
	${LDFLAGS} \
	${RTX2001AD}${RTX2001A_LIB} \
	${CC_OUTSPEC}

# clean
rtx2001a_clean:
	${RM} -rf ${BIN} ${SIMH}/*.o ${RTX2001AD}$(RTX2001A_LIB)
