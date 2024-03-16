import os
import functools

## Initialize package_info to an empty dictionary here so
## that it's visible to write_packaging().
package_info = {}


class SIMHPackaging:
    def __init__(self, family, install_flag = True) -> None:
        self.family = family
        self.processed = False
        self.install_flag = install_flag

    def was_processed(self) -> bool:
        return self.processed == True
    
    def encountered(self) -> None:
        self.processed = True

class PkgFamily:
    def __init__(self, component_name, display_name, description) -> None:
        self.component_name = component_name
        self.display_name   = display_name
        self.description    = description

    def write_component_info(self, stream, indent) -> None:
        pkg_description = self.description
        if pkg_description[-1] != '.':
            pkg_description += '.'
        sims = []
        for sim, pkg in package_info.items():
            if pkg.family is self and pkg.was_processed():
                sims.append(sim)

        if len(sims) > 0:
            sims.sort()
            pkg_description += " Simulators: " + ', '.join(sims)
            indent0 = ' ' * indent
            indent4 = ' ' * (indent + 4)
            stream.write(indent0 + "cpack_add_component(" + self.component_name + "\n")
            stream.write(indent4 + "DISPLAY_NAME \"" + self.display_name + "\"\n")
            stream.write(indent4 + "DESCRIPTION \"" + pkg_description + "\"\n")
            stream.write(indent0 + ")\n")

    def __lt__(self, obj):
        return self.component_name < obj.component_name
    def __eq__(self, obj):
        return self.component_name == obj.component_name
    def __gt__(self, obj):
        return self.component_name > obj.component_name
    def __hash__(self):
        return hash(self.component_name)

def write_packaging(toplevel_dir) -> None:
    families = set([sim.family for sim in package_info.values()])
    pkging_file = os.path.join(toplevel_dir, 'cmake', 'simh-packaging.cmake')
    print("==== writing {0}".format(pkging_file))
    with open(pkging_file, "w") as stream:
        ## Runtime support family:
        stream.write("""## The default runtime support component/family:
cpack_add_component(runtime_support
    DISPLAY_NAME "Runtime support"
    DESCRIPTION "Required SIMH runtime support (documentation, shared libraries)"
    REQUIRED
)

## Basic documentation for SIMH
install(FILES doc/simh.doc TYPE DOC COMPONENT runtime_support)

""")

        ## Simulators:
        for family in sorted(families):
            family.write_component_info(stream, 0)


default_family = PkgFamily("default_family", "Default SIMH simulator family.",
    """The SIMH simulator collection of historical processors and computing systems that do not belong to
any other simulated system family"""
)
    
att3b2_family = PkgFamily("att3b2_family", "ATT&T 3b2 collection",
    """The AT&T 3b2 simulator family"""
)

vax_family = PkgFamily("vax_family", "DEC VAX simulator collection",
    """The Digital Equipment Corporation VAX (plural: VAXen) simulator family."""
)

pdp10_family = PkgFamily("pdp10_family", "DEC PDP-10 collection",
    """DEC PDP-10 architecture simulators and variants."""
)

pdp11_family = PkgFamily("pdp11_family", "DEC PDP-11 collection.",
   """DEC PDP-11 and PDP-11-derived architecture simulators"""
)

experimental_family = PkgFamily("experimental", "Experimental (work-in-progress) simulators",
    """Experimental or work-in-progress simulators not in the SIMH mainline simulator suite."""
)

altairz80_family = PkgFamily("altairz80_family", "Altair Z80 simulator.",
    """The Altair Z80 simulator with M68000 support."""
)

b5500_family = PkgFamily("b5500_family", "Burroughs 5500",
    """The Burroughs 5500 system simulator""")

cdc1700_family = PkgFamily("cdc1700_family", "CDC 1700",
    """The Control Data Corporation's systems"""
)

dgnova_family = PkgFamily("dgnova_family", "DG Nova and Eclipse",
    """Data General NOVA and Eclipse system simulators"""
)

grisys_family = PkgFamily("grisys_family", "GRI Systems GRI-909",
    """GRI Systems GRI-909 system simulator"""
)

honeywell_family = PkgFamily("honeywell_family", "Honeywell H316",
    """Honeywell H-316 system simulator"""
)

hp_family = PkgFamily("hp_family", "HP 2100, 3000",
    """Hewlett-Packard H2100 and H3000 simulators""")

ibm_family = PkgFamily("ibm_family", "IBM",
    """IBM system simulators: i650"""
)

imlac_family = PkgFamily("imlac_family", "IMLAC",
    """IMLAC system simulators"""
)

intel_family = PkgFamily("intel_family", "Intel",
    """Intel system simulators"""
)

interdata_family = PkgFamily("interdata_family", "Interdata",
    """Interdata systems simulators"""
)

lgp_family = PkgFamily("lgp_family", "LGP",
    """Librascope systems"""
)

decpdp_family = PkgFamily("decpdp_family", "DEC PDP family",
    """Digital Equipment Corporation PDP systems"""
)

sds_family = PkgFamily("sds_family", "SDS simulators",
    """Scientific Data Systems (SDS) systems"""
)

gould_family = PkgFamily("gould_family", "Gould simulators",
    """Gould Systems simulators"""
)

swtp_family = PkgFamily("swtp_family", "SWTP simulators",
    """Southwest Technical Products (SWTP) system simulators"""
)

norsk_family = PkgFamily("norsk_family", "ND simulators",
    """Norsk Data systems simulator family""")


package_info["3b2"] = SIMHPackaging(att3b2_family)
package_info["3b2-700"] = SIMHPackaging(att3b2_family)
package_info["altair"] = SIMHPackaging(default_family)
package_info["altairz80"] = SIMHPackaging(altairz80_family)
package_info["b5500"] = SIMHPackaging(b5500_family)
package_info["besm6"] = SIMHPackaging(default_family)
package_info["cdc1700"] = SIMHPackaging(cdc1700_family)
package_info["eclipse"] = SIMHPackaging(dgnova_family)
package_info["gri"] = SIMHPackaging(grisys_family)
package_info["h316"] = SIMHPackaging(honeywell_family)
package_info["hp2100"] = SIMHPackaging(hp_family)
package_info["hp3000"] = SIMHPackaging(hp_family)
package_info["i1401"] = SIMHPackaging(ibm_family)
package_info["i1620"] = SIMHPackaging(ibm_family)
package_info["i650"] = SIMHPackaging(ibm_family)
package_info["i701"] = SIMHPackaging(ibm_family)
package_info["i7010"] = SIMHPackaging(ibm_family)
package_info["i704"] = SIMHPackaging(ibm_family)
package_info["i7070"] = SIMHPackaging(ibm_family)
package_info["i7080"] = SIMHPackaging(ibm_family)
package_info["i7090"] = SIMHPackaging(ibm_family)
package_info["i7094"] = SIMHPackaging(ibm_family)
package_info["ibm1130"] = SIMHPackaging(ibm_family)
package_info["id16"] = SIMHPackaging(interdata_family)
package_info["id32"] = SIMHPackaging(interdata_family)
package_info["imlac"] = SIMHPackaging(imlac_family)
package_info["infoserver100"] = SIMHPackaging(vax_family)
package_info["infoserver1000"] = SIMHPackaging(vax_family)
package_info["infoserver150vxt"] = SIMHPackaging(vax_family)
package_info["intel-mds"] = SIMHPackaging(intel_family)
package_info["lgp"] = SIMHPackaging(lgp_family)
package_info["microvax1"] = SIMHPackaging(vax_family)
package_info["microvax2"] = SIMHPackaging(vax_family)
package_info["microvax2000"] = SIMHPackaging(vax_family)
package_info["microvax3100"] = SIMHPackaging(vax_family)
package_info["microvax3100e"] = SIMHPackaging(vax_family)
package_info["microvax3100m80"] = SIMHPackaging(vax_family)
package_info["nd100"] = SIMHPackaging(norsk_family)
package_info["nova"] = SIMHPackaging(dgnova_family)
package_info["pdp1"] = SIMHPackaging(decpdp_family)
## Don't install pdp10 per Rob Cromwell
package_info["pdp10"] = SIMHPackaging(pdp10_family, install_flag=False)
package_info["pdp10-ka"] = SIMHPackaging(pdp10_family)
package_info["pdp10-ki"] = SIMHPackaging(pdp10_family)
package_info["pdp10-kl"] = SIMHPackaging(pdp10_family)
package_info["pdp10-ks"] = SIMHPackaging(pdp10_family)
package_info["pdp11"] = SIMHPackaging(pdp11_family)
package_info["pdp15"] = SIMHPackaging(decpdp_family)
package_info["pdp4"] = SIMHPackaging(decpdp_family)
package_info["pdp6"] = SIMHPackaging(decpdp_family)
package_info["pdp7"] = SIMHPackaging(decpdp_family)
package_info["pdp8"] = SIMHPackaging(decpdp_family)
package_info["pdp9"] = SIMHPackaging(decpdp_family)
package_info["rtvax1000"] = SIMHPackaging(vax_family)
package_info["s3"] = SIMHPackaging(ibm_family)
package_info["scelbi"] = SIMHPackaging(intel_family)
package_info["sds"] = SIMHPackaging(sds_family)
package_info["sel32"] = SIMHPackaging(gould_family)
package_info["sigma"] = SIMHPackaging(sds_family)
package_info["ssem"] = SIMHPackaging(default_family)
package_info["swtp6800mp-a"] = SIMHPackaging(swtp_family)
package_info["swtp6800mp-a2"] = SIMHPackaging(swtp_family)
package_info["tt2500"] = SIMHPackaging(default_family)
package_info["tx-0"] = SIMHPackaging(default_family)
package_info["uc15"] = SIMHPackaging(pdp11_family)
package_info["vax"] = SIMHPackaging(vax_family)
package_info["vax730"] = SIMHPackaging(vax_family)
package_info["vax750"] = SIMHPackaging(vax_family)
package_info["vax780"] = SIMHPackaging(vax_family)
package_info["vax8200"] = SIMHPackaging(vax_family)
package_info["vax8600"] = SIMHPackaging(vax_family)
package_info["vaxstation3100m30"] = SIMHPackaging(vax_family)
package_info["vaxstation3100m38"] = SIMHPackaging(vax_family)
package_info["vaxstation3100m76"] = SIMHPackaging(vax_family)
package_info["vaxstation4000m60"] = SIMHPackaging(vax_family)
package_info["vaxstation4000vlc"] = SIMHPackaging(vax_family)

## Experimental simulators:
package_info["alpha"] = SIMHPackaging(experimental_family)
package_info["pdq3"] = SIMHPackaging(experimental_family)
package_info["sage"] = SIMHPackaging(experimental_family)
