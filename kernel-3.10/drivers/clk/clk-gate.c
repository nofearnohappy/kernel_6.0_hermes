/*
 * Copyright (C) 2010-2011 Canonical Ltd <jeremy.kerr@canonical.com>
 * Copyright (C) 2011-2012 Mike Turquette, Linaro Ltd <mturquette@linaro.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Gated clock implementation
 */

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/string.h>

#if !defined(CONFIG_MTK_LEGACY) /* FIXME: only for bring up */
#define MT_CCF_DEBUG	1
#define MT_CCF_BRINGUP	0 /* FIXME: only for bring up */
#endif /* !defined(CONFIG_MTK_LEGACY) */

/**
 * DOC: basic gatable clock which can gate and ungate it's ouput
 *
 * Traits of this clock:
 * prepare - clk_(un)prepare only ensures parent is (un)prepared
 * enable - clk_enable and clk_disable are functional & control gating
 * rate - inherits rate from parent.  No clk_set_rate support
 * parent - fixed parent.  No clk_set_parent support
 */

#define to_clk_gate(_hw) container_of(_hw, struct clk_gate, hw)

/*
 * It works on following logic:
 *
 * For enabling clock, enable = 1
 *	set2dis = 1	-> clear bit	-> set = 0
 *	set2dis = 0	-> set bit	-> set = 1
 *
 * For disabling clock, enable = 0
 *	set2dis = 1	-> set bit	-> set = 1
 *	set2dis = 0	-> clear bit	-> set = 0
 *
 * So, result is always: enable xor set2dis.
 */
static void clk_gate_endisable(struct clk_hw *hw, int enable)
{
	struct clk_gate *gate = to_clk_gate(hw);
	int set = gate->flags & CLK_GATE_SET_TO_DISABLE ? 1 : 0;
	unsigned long flags = 0;
	u32 reg;

	set ^= enable;

	if (gate->lock)
		spin_lock_irqsave(gate->lock, flags);

	reg = readl(gate->reg);

	if (set)
		reg |= BIT(gate->bit_idx);
	else
		reg &= ~BIT(gate->bit_idx);

	writel(reg, gate->reg);

	if (gate->lock)
		spin_unlock_irqrestore(gate->lock, flags);
#if !defined(CONFIG_MTK_LEGACY) /* FIXME: only for bring up */
#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: %s, enable=%d, reg=%u, idx=%u\n", __func__,
	       __clk_get_name(hw->clk), enable, reg, gate->bit_idx);
#endif /* MT_CCF_DEBUG */
#endif /* !defined(CONFIG_MTK_LEGACY) */
}

static int clk_gate_enable(struct clk_hw *hw)
{
#if !defined(CONFIG_MTK_LEGACY) /* FIXME: only for bring up */
#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: %s\n", __func__, __clk_get_name(hw->clk));
#endif /* MT_CCF_DEBUG */
#if MT_CCF_BRINGUP
	return 0;
#endif /* MT_CCF_BRINGUP */
#endif /* !defined(CONFIG_MTK_LEGACY) */
	clk_gate_endisable(hw, 1);

	return 0;
}

static void clk_gate_disable(struct clk_hw *hw)
{
#if !defined(CONFIG_MTK_LEGACY) /* FIXME: only for bring up */
#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: %s\n", __func__, __clk_get_name(hw->clk));
#endif /* MT_CCF_DEBUG */
#if MT_CCF_BRINGUP
	return;
#else /* !MT_CCF_BRINGUP */
	/*if (!strcmp(__clk_get_name(hw->clk), "mm_sel") ||
	    !strcmp(__clk_get_name(hw->clk), "vdec_sel") ||
	    !strcmp(__clk_get_name(hw->clk), "mfg_sel") ||
	    !strcmp(__clk_get_name(hw->clk), "msdc30_1_sel")) {
		pr_debug("[CCF] %s: %s is skip!\n", __func__,
		       __clk_get_name(hw->clk));
		return;
	}*/
#endif /* MT_CCF_BRINGUP */
#endif /* !defined(CONFIG_MTK_LEGACY) */
	clk_gate_endisable(hw, 0);
}

static int clk_gate_is_enabled(struct clk_hw *hw)
{
	u32 reg;
	struct clk_gate *gate = to_clk_gate(hw);
#if !defined(CONFIG_MTK_LEGACY) /* FIXME: only for bring up */
#if MT_CCF_DEBUG
	pr_debug("[CCF] %s: %s\n", __func__, __clk_get_name(hw->clk));
#endif /* MT_CCF_DEBUG */
#if MT_CCF_BRINGUP
	return 1;
#endif /* MT_CCF_BRINGUP */
#endif /* !defined(CONFIG_MTK_LEGACY) */

	reg = readl(gate->reg);

	/* if a set bit disables this clk, flip it before masking */
	if (gate->flags & CLK_GATE_SET_TO_DISABLE)
		reg ^= BIT(gate->bit_idx);

	reg &= BIT(gate->bit_idx);

	return reg ? 1 : 0;
}

const struct clk_ops clk_gate_ops = {
	.enable = clk_gate_enable,
	.disable = clk_gate_disable,
	.is_enabled = clk_gate_is_enabled,
};
EXPORT_SYMBOL_GPL(clk_gate_ops);

/**
 * clk_register_gate - register a gate clock with the clock framework
 * @dev: device that is registering this clock
 * @name: name of this clock
 * @parent_name: name of this clock's parent
 * @flags: framework-specific flags for this clock
 * @reg: register address to control gating of this clock
 * @bit_idx: which bit in the register controls gating of this clock
 * @clk_gate_flags: gate-specific flags for this clock
 * @lock: shared register lock for this clock
 */
struct clk *clk_register_gate(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 bit_idx,
		u8 clk_gate_flags, spinlock_t *lock)
{
	struct clk_gate *gate;
	struct clk *clk;
	struct clk_init_data init;

	/* allocate the gate */
	gate = kzalloc(sizeof(struct clk_gate), GFP_KERNEL);
	if (!gate) {
		pr_err("%s: could not allocate gated clk\n", __func__);
		return ERR_PTR(-ENOMEM);
	}

	init.name = name;
	init.ops = &clk_gate_ops;
	init.flags = flags | CLK_IS_BASIC;
	init.parent_names = (parent_name ? &parent_name: NULL);
	init.num_parents = (parent_name ? 1 : 0);

	/* struct clk_gate assignments */
	gate->reg = reg;
	gate->bit_idx = bit_idx;
	gate->flags = clk_gate_flags;
	gate->lock = lock;
	gate->hw.init = &init;

	clk = clk_register(dev, &gate->hw);

	if (IS_ERR(clk))
		kfree(gate);

	return clk;
}
