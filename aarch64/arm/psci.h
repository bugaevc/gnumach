#include <device/dtb.h>

#define psci_is_compatible(node)					\
	(dtb_node_is_compatible(node, "arm,psci")			\
	|| dtb_node_is_compatible(node, "arm,psci-0.2")			\
	|| dtb_node_is_compatible(node, "arm,psci-1.0"))

extern void psci_init(dtb_node_t node);
extern kern_return_t psci_cpu_off(void);
extern kern_return_t psci_system_off(void);
extern kern_return_t psci_system_reset(void);
