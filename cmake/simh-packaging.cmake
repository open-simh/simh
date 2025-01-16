## The default runtime support component/family:
cpack_add_component(runtime_support
    DISPLAY_NAME "Runtime support"
    DESCRIPTION "Required SIMH runtime support (documentation, shared libraries)"
    REQUIRED
)

## Basic documentation for SIMH
install(FILES doc/simh.doc TYPE DOC COMPONENT runtime_support)

cpack_add_component(altairz80_family
    DISPLAY_NAME "Altair Z80 simulator."
    DESCRIPTION "The Altair Z80 simulator with M68000 support. Simulators: altairz80"
)
cpack_add_component(att3b2_family
    DISPLAY_NAME "ATT&T 3b2 collection"
    DESCRIPTION "The AT&T 3b2 simulator family. Simulators: 3b2, 3b2-700"
)
cpack_add_component(b5500_family
    DISPLAY_NAME "Burroughs 5500"
    DESCRIPTION "The Burroughs 5500 system simulator. Simulators: b5500"
)
cpack_add_component(cdc1700_family
    DISPLAY_NAME "CDC 1700"
    DESCRIPTION "The Control Data Corporation's systems. Simulators: cdc1700"
)
cpack_add_component(decpdp_family
    DISPLAY_NAME "DEC PDP family"
    DESCRIPTION "Digital Equipment Corporation PDP systems. Simulators: pdp1, pdp15, pdp4, pdp6, pdp7, pdp8, pdp9"
)
cpack_add_component(default_family
    DISPLAY_NAME "Default SIMH simulator family."
    DESCRIPTION "The SIMH simulator collection of historical processors and computing systems that do not belong to
any other simulated system family. Simulators: altair, besm6, ssem, tt2500, tx-0"
)
cpack_add_component(dgnova_family
    DISPLAY_NAME "DG Nova and Eclipse"
    DESCRIPTION "Data General NOVA and Eclipse system simulators. Simulators: eclipse, nova"
)
cpack_add_component(experimental
    DISPLAY_NAME "Experimental (work-in-progress) simulators"
    DESCRIPTION "Experimental or work-in-progress simulators not in the SIMH mainline simulator suite. Simulators: alpha, pdq3, sage"
)
cpack_add_component(gould_family
    DISPLAY_NAME "Gould simulators"
    DESCRIPTION "Gould Systems simulators. Simulators: sel32"
)
cpack_add_component(grisys_family
    DISPLAY_NAME "GRI Systems GRI-909"
    DESCRIPTION "GRI Systems GRI-909 system simulator. Simulators: gri"
)
cpack_add_component(honeywell_family
    DISPLAY_NAME "Honeywell H316"
    DESCRIPTION "Honeywell H-316 system simulator. Simulators: h316"
)
cpack_add_component(hp_family
    DISPLAY_NAME "HP 2100, 3000"
    DESCRIPTION "Hewlett-Packard H2100 and H3000 simulators. Simulators: hp2100, hp3000"
)
cpack_add_component(ibm_family
    DISPLAY_NAME "IBM"
    DESCRIPTION "IBM system simulators: i650. Simulators: i1401, i1620, i650, i701, i7010, i704, i7070, i7080, i7090, i7094, ibm1130, s3"
)
cpack_add_component(imlac_family
    DISPLAY_NAME "IMLAC"
    DESCRIPTION "IMLAC system simulators. Simulators: imlac"
)
cpack_add_component(intel_family
    DISPLAY_NAME "Intel"
    DESCRIPTION "Intel system simulators. Simulators: intel-mds, scelbi"
)
cpack_add_component(interdata_family
    DISPLAY_NAME "Interdata"
    DESCRIPTION "Interdata systems simulators. Simulators: id16, id32"
)
cpack_add_component(lgp_family
    DISPLAY_NAME "LGP"
    DESCRIPTION "Librascope systems. Simulators: lgp"
)
cpack_add_component(norsk_family
    DISPLAY_NAME "ND simulators"
    DESCRIPTION "Norsk Data systems simulator family. Simulators: nd100"
)
cpack_add_component(pdp10_family
    DISPLAY_NAME "DEC PDP-10 collection"
    DESCRIPTION "DEC PDP-10 architecture simulators and variants. Simulators: pdp10, pdp10-ka, pdp10-ki, pdp10-kl, pdp10-ks"
)
cpack_add_component(pdp11_family
    DISPLAY_NAME "DEC PDP-11 collection."
    DESCRIPTION "DEC PDP-11 and PDP-11-derived architecture simulators. Simulators: pdp11, uc15"
)
cpack_add_component(sds_family
    DISPLAY_NAME "SDS simulators"
    DESCRIPTION "Scientific Data Systems (SDS) systems. Simulators: sds, sigma"
)
cpack_add_component(swtp_family
    DISPLAY_NAME "SWTP simulators"
    DESCRIPTION "Southwest Technical Products (SWTP) system simulators. Simulators: swtp6800mp-a, swtp6800mp-a2"
)
cpack_add_component(vax_family
    DISPLAY_NAME "DEC VAX simulator collection"
    DESCRIPTION "The Digital Equipment Corporation VAX (plural: VAXen) simulator family. Simulators: infoserver100, infoserver1000, infoserver150vxt, microvax1, microvax2, microvax2000, microvax3100, microvax3100e, microvax3100m80, rtvax1000, vax, vax730, vax750, vax780, vax8200, vax8600, vaxstation3100m30, vaxstation3100m38, vaxstation3100m76, vaxstation4000m60, vaxstation4000vlc"
)
