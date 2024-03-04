#include <mach/boolean.h>
#include <device/dtb.h>

#define gic_v2_is_compatible(node)					\
	(dtb_node_is_compatible(node, "arm,arm1176jzf-devchip-gic")	\
	|| dtb_node_is_compatible(node, "arm,arm11mp-gic")		\
	|| dtb_node_is_compatible(node, "arm,cortex-a15-gic")		\
	|| dtb_node_is_compatible(node, "arm,cortex-a7-gic")		\
	|| dtb_node_is_compatible(node, "arm,cortex-a9-gic")		\
	|| dtb_node_is_compatible(node, "arm,eb11mp-gic")		\
	|| dtb_node_is_compatible(node, "arm,gic-400")			\
	|| dtb_node_is_compatible(node, "arm,pl390")			\
	|| dtb_node_is_compatible(node, "arm,tc11mp-gic")		\
	|| dtb_node_is_compatible(node, "brcm,brahma-b15-gic")		\
	|| dtb_node_is_compatible(node, "nvidia,tegra210-agic")		\
	|| dtb_node_is_compatible(node, "qcom,msm-8660-qgic")		\
	|| dtb_node_is_compatible(node, "qcom,msm-qgic2"))

extern void gic_v2_init(dtb_node_t node, dtb_ranges_map_t map);
extern void gic_v2_enable(void);

extern void gic_v2_enable_irq(int irq);
extern void gic_v2_disable_irq(int irq);
extern boolean_t gic_v2_check_irq(int irq);
extern void gic_v2_clear_irq(int irq);
