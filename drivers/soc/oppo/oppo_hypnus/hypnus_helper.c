/*
 * Copyright (C) 2016 OPPO, Inc.
 * Author: Jie Cheng <jie.cheng@oppo.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/power_supply.h>
#include <linux/mmc/host.h>
#include <soc/oppo/hypnus_helper.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/rcupdate.h>
#include <linux/pm_qos.h>

/* net load */
bool hypnus_get_traffic_status()
{
#ifndef CONFIG_NET
	return 0;
#else
	static u64 rx_total_last, tx_total_last;
	u64 rx_total, tx_total;
	u64 rx_delta, tx_delta;
	struct net_device *dev;

	rx_total = tx_total = 0;
	rx_delta = tx_delta = 0;

	/* we are running as a workqueue task, so we can use an RCU lookup */
	rcu_read_lock();
	for_each_netdev_rcu(&init_net, dev) {
		const struct rtnl_link_stats64 *stats;
		struct rtnl_link_stats64 temp;
		struct in_device *in_dev = __in_dev_get_rcu(dev);
		if (!in_dev || !in_dev->ifa_list){
			continue;
		}
		if (ipv4_is_loopback(in_dev->ifa_list->ifa_local)){
			continue;
		}
		stats = dev_get_stats(dev, &temp);
		rx_total += stats->rx_bytes;
		tx_total += stats->tx_bytes;
	}
	rcu_read_unlock();

	if (rx_total != rx_total_last) {
		if (rx_total > rx_total_last)
			rx_delta = rx_total - rx_total_last;
		rx_total_last = rx_total;
	}
	if (tx_total != tx_total_last) {
		if (tx_total > tx_total_last)
			tx_delta = tx_total - tx_total_last;
		tx_total_last = tx_total;
	}

	if (rx_delta > HEAVY_TRAFFIC || tx_delta > HEAVY_TRAFFIC) {
		pr_debug("hypnus enter heavy traffic mode\n");
		return 1;
	}
	else {
		pr_debug("hypnus exit heavy traffic mode\n");
		return 0;
	}
#endif
}
EXPORT_SYMBOL(hypnus_get_traffic_status);

/*
 * storage: the OPPO ufshcd/mmc clk-scaling hooks this helper used to wrap
 * are absent from this tree; no caller exists either, keep the symbol.
 */
int hypnus_storage_scaling_enable(int index, int scale)
{
	pr_err("hypnus_storage_scaling_enable not supported!\n");
	return -EINVAL;
}
EXPORT_SYMBOL(hypnus_storage_scaling_enable);

/* power */
static struct pm_qos_request hypnus_qos_request;

void lpm_qos_init(void)
{
	pm_qos_add_request(&hypnus_qos_request, PM_QOS_CPU_DMA_LATENCY, PM_QOS_DEFAULT_VALUE);
}
EXPORT_SYMBOL(lpm_qos_init);

void lpm_qos_exit(void)
{
	pm_qos_remove_request(&hypnus_qos_request);
}
EXPORT_SYMBOL(lpm_qos_exit);

void lpm_qos_disable_sleep(bool disable)
{
	if (disable)
		pm_qos_update_request(&hypnus_qos_request, 43);
	else
		pm_qos_update_request(&hypnus_qos_request, PM_QOS_DEFAULT_VALUE);
}
EXPORT_SYMBOL(lpm_qos_disable_sleep);

int hypnus_get_batt_capacity()
{
	union power_supply_propval ret = {0, };
	static struct power_supply *batt_psy;

	if (batt_psy == NULL)
		batt_psy = power_supply_get_by_name("battery");
	if (batt_psy && batt_psy->desc->get_property)
		batt_psy->desc->get_property(batt_psy, POWER_SUPPLY_PROP_CAPACITY, &ret);
	if (ret.intval >= 0 && ret.intval <=100)
		return ret.intval;
	else
		return DEFAULT_CAPACITY;
}
EXPORT_SYMBOL(hypnus_get_batt_capacity);

bool hypnus_get_charger_status()
{
	union power_supply_propval ret = {0,};
	static struct power_supply *usb_psy;

	if (usb_psy == NULL)
		usb_psy = power_supply_get_by_name("usb");
	if (usb_psy && usb_psy->desc->get_property)
		usb_psy->desc->get_property(usb_psy, POWER_SUPPLY_PROP_PRESENT, &ret);
	if (ret.intval)
		return true;
	else
		return false;
}
EXPORT_SYMBOL(hypnus_get_charger_status);

/* sched */
void hypnus_get_nr_running_avg(int *avg, int *iowait_avg, int *big_avg)
{
	int max_nr, big_max_nr;
	sched_get_nr_running_avg(avg, iowait_avg, big_avg, &max_nr, &big_max_nr);
	*avg *= 100;
	*big_avg *= 100;
	*iowait_avg *= 100;
}
EXPORT_SYMBOL(hypnus_get_nr_running_avg);

int sched_set_prefer_idle(unsigned int is_prefer_idle)
{
	return 0;
}
EXPORT_SYMBOL(sched_set_prefer_idle);

int sched_set_small_task(int load_pct)
{
	return 0;
}
EXPORT_SYMBOL(sched_set_small_task);

unsigned int sched_get_small_task(void)
{
	return 10;
}
EXPORT_SYMBOL(sched_get_small_task);

int sched_set_cpu_mostly_idle_load(int cpu, int mostly_idle_pct)
{
	return 0;
}
EXPORT_SYMBOL(sched_set_cpu_mostly_idle_load);

int sched_get_cpu_mostly_idle_load(int cpu)
{
	return 10;
}
EXPORT_SYMBOL(sched_get_cpu_mostly_idle_load);

int sched_set_cpu_mostly_idle_nr_run(int cpu, int nr_run)
{
	return 0;
}
EXPORT_SYMBOL(sched_set_cpu_mostly_idle_nr_run);

int sched_get_cpu_mostly_idle_nr_run(int cpu)
{
	return 1;
}
EXPORT_SYMBOL(sched_get_cpu_mostly_idle_nr_run);
