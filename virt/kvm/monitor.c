#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/hashtable.h>
#include <linux/hash.h>
#include <asm-generic/errno.h>
#include "monitor.h"

#define GET_VAR_NAME(var) #var

DEFINE_HASHTABLE(record_htable, POWER_OF_BUCKETS_NUM);

void set_ull_node(void *var_addr, unsigned long long val,struct record_node *rec_data) 
{
    u32 key;

    rec_data->addr = (unsigned long long) var_addr;
    rec_data->global_data = val;

    key = hash_ptr(var_addr, POWER_OF_BUCKETS_NUM);

    hash_add(record_htable, &rec_data->node, key);

}

int new_global_data(void *shared_data, unsigned long long val)
{
    struct record_node *rec_data;

    rec_data = kzalloc(sizeof(struct record_node), GFP_KERNEL);
    if (!rec_data)
			return -ENOMEM;
    set_ull_node(shared_data, val, rec_data);

    return 0;
}

int set_global_data(void *shared_data, unsigned long long val) 
{
    struct record_node *rec_data = NULL;
    u32 key;
    key = hash_ptr(shared_data, POWER_OF_BUCKETS_NUM);

    hash_for_each_possible(record_htable, rec_data, node, key) {
        if (rec_data->addr == (unsigned long long)shared_data ) {
            break;
        }
    }

    if (rec_data == NULL && hash_empty(record_htable)) {
        return new_global_data(shared_data, val);
        
    }

    set_ull_node(shared_data, val, rec_data);

    return 0;
}

unsigned long long get_global_data(void *shared_data)
{
    struct record_node *rec_data = NULL;
    u32 key;
    key = hash_ptr(shared_data, POWER_OF_BUCKETS_NUM);

    hash_for_each_possible(record_htable, rec_data, node, key) {
        if (rec_data->addr == (unsigned long long)shared_data ) {
            break;
        }
    }

    return rec_data->global_data;
}

int init_global_record_data(void *kvm_createvm_count, void *kvm_active_vms)
{
    int kvm_create_count_err = 0;
    int kvm_active_vms_err = 0;
    kvm_create_count_err = new_global_data(kvm_createvm_count, *(unsigned long long*)kvm_createvm_count);
    kvm_active_vms_err = new_global_data(kvm_active_vms, *(unsigned long long*)kvm_active_vms);

    if (kvm_create_count_err < 0 || kvm_active_vms_err < 0) {
        return -ENOMEM;
    }
    
    return 0;
}

void restore_record_data_to_global_var(void *kvm_createvm_count, void *kvm_active_vms)
{
    *(unsigned long long*)kvm_createvm_count = get_global_data(kvm_createvm_count);
    *(unsigned long long*)kvm_active_vms = get_global_data(kvm_active_vms);
    printk("create: %d, active %d", *(int *)kvm_createvm_count, *(int *)kvm_active_vms);
}