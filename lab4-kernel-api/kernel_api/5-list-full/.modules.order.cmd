cmd_/home/yjp/mylab/linux-kernel-lab/linux/tools/labs/skels/./kernel_api/5-list-full/modules.order := {   echo /home/yjp/mylab/linux-kernel-lab/linux/tools/labs/skels/./kernel_api/5-list-full/list-full.ko; :; } | awk '!x[$$0]++' - > /home/yjp/mylab/linux-kernel-lab/linux/tools/labs/skels/./kernel_api/5-list-full/modules.order