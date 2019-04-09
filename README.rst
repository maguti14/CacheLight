Patching Genode with CacheLight
###################################################
CacheLight is a lightweight defense against memeory inspection resistant rootkits that can currently be added to the Genode OS Framework for the i.MX53.
To implement the CacheLight defense, first, obtain Genode 18.02 from their online Github repository and move into
your new Genode working directory.

In order to map the Normal World memory region to Secure World and be able to 
successfully obtian virtual address translations, replace the original files
from the Genode 18.02 source tree with their respective patched files provided:

/repos/base-hw/src/core/platform.h             // from the core directory

/repos/base-hw/src/core/platform.cc			   // from the core directory

/repos/base-hw/src/bootstrap/platform.h

/repos/base-hw/src/bootstrap/platform.cc

/repos/base-hw/src/bootstrap/spec/imx53_qsb/platform_trustzone.cc

/repos/base-hw/src/lib/hw/boot_info.h

/repos/base-hw/src/lib/hw/memory_map.h

/repos/base-hw/src/lib/hw/spec/32bit/memory_map.cc

/repos/base-hw/src/lib/hw/spec/arm/page_table.h

Then to get the actual CacheLight approach added to Genode, replace the following 
files:

/repos/os/src/server/tz_vmm/spec/imx53/main.cc

/repos/base-hw/src/core/spec/arm_v7/trustzone/kernel/vm.cc

Alternatively, apply the provided patch to Genode.

To read more about CacheLight, please find out paper published in ASHES '18 Proceedings of the 2018 Workshop on Attacks and Solutions in Hardware Security at:
https://dl.acm.org/citation.cfm?id=3266449


