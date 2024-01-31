/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KVM_MONITOR_H__
#define __KVM_MONITOR_H__

#define POWER_OF_BUCKETS_NUM 7

struct record_node {
    unsigned long long addr;
    unsigned long long global_data;
    // void *heap_data;
    struct hlist_node node;
};

int new_global_data(void *var_addr, unsigned long long val);
int set_global_data(void *shared_data, unsigned long long val);
void set_ull_node(void *shared_data, unsigned long long val, struct record_node *rec_data);
unsigned long long get_global_data(void *shared_data);
int init_global_record_data(void *kvm_createvm_count, void *kvm_active_vms);
void restore_record_data_to_global_var(void *kvm_createvm_count, void *kvm_active_vms);
#endif