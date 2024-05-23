// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2014, The Linux Foundation. All rights reserved.
 */

#include <linux/device.h>
#include <linux/clk-provider.h>
#include <linux/regmap.h>
#include <linux/export.h>
#include <linux/pm_runtime.h>

#include "clk-regmap.h"

static LIST_HEAD(clk_regmap_list);
static DEFINE_SPINLOCK(clk_regmap_lock);

/**
 * clk_is_enabled_regmap - standard is_enabled() for regmap users
 *
 * @hw: clk to operate on
 *
 * Clocks that use regmap for their register I/O can set the
 * enable_reg and enable_mask fields in their struct clk_regmap and then use
 * this as their is_enabled operation, saving some code.
 */
int clk_is_enabled_regmap(struct clk_hw *hw)
{
	struct clk_regmap *rclk = to_clk_regmap(hw);
	unsigned int val;
	int ret;

	ret = regmap_read(rclk->regmap, rclk->enable_reg, &val);
	if (ret != 0)
		return ret;

	if (rclk->enable_is_inverted)
		return (val & rclk->enable_mask) == 0;
	else
		return (val & rclk->enable_mask) != 0;
}
EXPORT_SYMBOL_GPL(clk_is_enabled_regmap);

/**
 * clk_enable_regmap - standard enable() for regmap users
 *
 * @hw: clk to operate on
 *
 * Clocks that use regmap for their register I/O can set the
 * enable_reg and enable_mask fields in their struct clk_regmap and then use
 * this as their enable() operation, saving some code.
 */
int clk_enable_regmap(struct clk_hw *hw)
{
	struct clk_regmap *rclk = to_clk_regmap(hw);
	unsigned int val;

	if (rclk->enable_is_inverted)
		val = 0;
	else
		val = rclk->enable_mask;

	return regmap_update_bits(rclk->regmap, rclk->enable_reg,
				  rclk->enable_mask, val);
}
EXPORT_SYMBOL_GPL(clk_enable_regmap);

/**
 * clk_disable_regmap - standard disable() for regmap users
 *
 * @hw: clk to operate on
 *
 * Clocks that use regmap for their register I/O can set the
 * enable_reg and enable_mask fields in their struct clk_regmap and then use
 * this as their disable() operation, saving some code.
 */
void clk_disable_regmap(struct clk_hw *hw)
{
	struct clk_regmap *rclk = to_clk_regmap(hw);
	unsigned int val;

	if (rclk->enable_is_inverted)
		val = rclk->enable_mask;
	else
		val = 0;

	regmap_update_bits(rclk->regmap, rclk->enable_reg, rclk->enable_mask,
			   val);
}
EXPORT_SYMBOL_GPL(clk_disable_regmap);

/**
 * clk_is_regmap_clk - Checks if clk is a regmap clk
 *
 * @hw: clk to check on
 *
 * Iterate over maintained clk regmap list to know
 * if concern clk is regmap
 *
 * Returns true on success, false otherwise.
 */
bool clk_is_regmap_clk(struct clk_hw *hw)
{
	struct clk_regmap *rclk;
	bool is_regmap_clk = false;

	if (hw) {
		spin_lock(&clk_regmap_lock);
		list_for_each_entry(rclk, &clk_regmap_list, list_node) {
			if (&rclk->hw == hw) {
				is_regmap_clk = true;
				break;
			}
		}
		spin_unlock(&clk_regmap_lock);
	}

	return is_regmap_clk;
}
EXPORT_SYMBOL(clk_is_regmap_clk);

/**
 * devm_clk_register_regmap - register a clk_regmap clock
 *
 * @dev: reference to the caller's device
 * @rclk: clk to operate on
 *
 * Clocks that use regmap for their register I/O should register their
 * clk_regmap struct via this function so that the regmap is initialized
 * and so that the clock is registered with the common clock framework.
 */
int devm_clk_register_regmap(struct device *dev, struct clk_regmap *rclk)
{
	if (dev && dev_get_regmap(dev, NULL))
		rclk->regmap = dev_get_regmap(dev, NULL);
	else if (dev && dev->parent)
		rclk->regmap = dev_get_regmap(dev->parent, NULL);

	return devm_clk_hw_register(dev, &rclk->hw);
}
EXPORT_SYMBOL_GPL(devm_clk_register_regmap);

int clk_runtime_get_regmap(struct clk_regmap *rclk)
{
	int ret;

	if (pm_runtime_enabled(rclk->dev)) {
		ret = pm_runtime_get_sync(rclk->dev);
		if (ret < 0)
			return ret;
	}

	return 0;
}
EXPORT_SYMBOL(clk_runtime_get_regmap);

void clk_runtime_put_regmap(struct clk_regmap *rclk)
{
	if (pm_runtime_enabled(rclk->dev))
		pm_runtime_put_sync(rclk->dev);
}
EXPORT_SYMBOL(clk_runtime_put_regmap);
