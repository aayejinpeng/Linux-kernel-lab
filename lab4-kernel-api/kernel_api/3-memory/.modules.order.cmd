cmd_/home/yjp/mylab/linux-kernel-lab/linux/tools/labs/skels/./kernel_api/3-memory/modules.order := {   echo /home/yjp/mylab/linux-kernel-lab/linux/tools/labs/skels/./kernel_api/3-memory/memory.ko; :; } | awk '!x[$$0]++' - > /home/yjp/mylab/linux-kernel-lab/linux/tools/labs/skels/./kernel_api/3-memory/modules.order