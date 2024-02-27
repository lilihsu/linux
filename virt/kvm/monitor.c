#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/hashtable.h>
#include <linux/hash.h>
#include <asm-generic/errno.h>
#include "monitor.h"

DEFINE_HASHTABLE(record_htable, POWER_OF_BUCKETS_NUM);


bool is_compart_activated = false;

u64 return_val_in_mem(void *addr)
{
    printk("[return_val_in_mem] addr : %llu", (unsigned long long)addr);
    return *(unsigned long long *)addr;

}

void set_origin_mem(void *addr, u64 val)
{
    printk("[set_origin_mem] addr : %llu", (unsigned long long)addr);
    *(unsigned long long *)addr = val;
}

void set_ull_node(void *var_addr, u64 val,struct record_node *rec_data) 
{
    rec_data->rec.global_data.addr = (u64) var_addr;
    rec_data->rec.global_data.data = val;
}

int new_global_data(void *shared_data, u64 val)
{
    struct record_node *rec_data;

    if (!is_compart_activated) {
        set_origin_mem(shared_data, val);
        return 0;
    }


    u32 key = hash_ptr(shared_data, POWER_OF_BUCKETS_NUM);

    rec_data = kzalloc(sizeof(struct record_node), GFP_KERNEL);
    if (!rec_data)
	return -ENOMEM;
    rec_data->type = GLOBAL;
    set_ull_node(shared_data, val, rec_data);
    
    hash_add(record_htable, &rec_data->node, key);
    printk("[new global data node] addr: %llu, val: %d", (unsigned long long) shared_data, (int)val);
    return 0;
}

int set_global_data(void *shared_data, u64 val) 
{
    printk("[set_global_data]");
    struct record_node *rec_data = NULL;
    u32 key;

    if (!is_compart_activated) {
        printk("[compart deactivated]");
        set_origin_mem(shared_data, val);
        return 0;
    }

    key = hash_ptr(shared_data, POWER_OF_BUCKETS_NUM);

    hash_for_each_possible(record_htable, rec_data, node, key) {
        if (rec_data->rec.global_data.addr == (u64)shared_data ) {
            break;
        }
    }

    if (rec_data == NULL && hash_empty(record_htable)) {
        printk("[can't set global node]");
        return new_global_data(shared_data, val);
        
    }

    set_ull_node(shared_data, val, rec_data);

    printk("[set global data node] addr: %llu, val: %d", (unsigned long long) shared_data, (int)val);
    return 0;
}

u64 get_global_data(void *shared_data)
{
    struct record_node *rec_data = NULL;
    u32 key;
    if (!is_compart_activated) {
        return return_val_in_mem(shared_data);
    }
    key = hash_ptr(shared_data, POWER_OF_BUCKETS_NUM);

    hash_for_each_possible(record_htable, rec_data, node, key) {
        if (rec_data->rec.global_data.addr == (u64)shared_data ) {
            break;
        }
    }

    if (rec_data == NULL){
        printk("[monitor error]: unable to fetch data");
        return -1; 
    }
    return rec_data->rec.global_data.data;
}

int init_global_record_data(void *kvm_createvm_count, void *kvm_active_vms)
{
    is_compart_activated = true;
    int kvm_create_count_err = 0;
    int kvm_active_vms_err = 0;
    kvm_create_count_err = new_global_data(kvm_createvm_count, *(unsigned long long*)kvm_createvm_count);
    kvm_active_vms_err = new_global_data(kvm_active_vms, *(unsigned long long*)kvm_active_vms);

    if (kvm_create_count_err < 0 || kvm_active_vms_err < 0) {
        return -ENOMEM;
    }
    
    printk("[Initailization] record all global data");
    return 0;
}

void restore_record_data_to_global_var(void *kvm_createvm_count, void *kvm_active_vms)
{
    *(unsigned long long*)kvm_createvm_count = get_global_data(kvm_createvm_count);
    *(unsigned long long*)kvm_active_vms = get_global_data(kvm_active_vms);
    printk("create: %d, active %d", *(int *)kvm_createvm_count, *(int *)kvm_active_vms);
}

int set_mem_obj(void *obj_addr, bool is_alloc_outside)
{
    struct record_node *rec_data = NULL;
    u32 key;
    if (!is_compart_activated) 
        return 0;
    key = hash_ptr(obj_addr, POWER_OF_BUCKETS_NUM);

    hash_for_each_possible(record_htable, rec_data, node, key) {
        if (rec_data->rec.mem_obj.addr == (u64)obj_addr && rec_data->type == MEMOBJ) {
            break;
        }
    }

    if (rec_data == NULL ) {
        rec_data = kzalloc(sizeof(struct record_node), GFP_KERNEL);
        if (!rec_data)
			    return -ENOMEM;
        
        rec_data->type = MEMOBJ;
        rec_data->rec.mem_obj.addr = (u64) obj_addr;
        rec_data->rec.mem_obj.is_valid = true;
        rec_data->rec.mem_obj.is_alloc_outside = is_alloc_outside;

        hash_add(record_htable, &rec_data->node, key);
    }

    return 0;
}

int invalid_mem_obj(void *obj_addr)
{
    struct record_node *rec_data = NULL;
    u32 key;
    if (!is_compart_activated)
        return 0;
    key = hash_ptr(obj_addr, POWER_OF_BUCKETS_NUM);

    hash_for_each_possible(record_htable, rec_data, node, key) {
        if (rec_data->rec.mem_obj.addr == (u64)obj_addr && rec_data->type == MEMOBJ) {
            break;
        }
    }

    if (rec_data != NULL) {
        rec_data->rec.mem_obj.is_valid = false;
    }
    printk("[invalid_mem_obj] addr:%llu", (unsigned long long) obj_addr);
    return 0;
}


int new_field(void *field_addr, void *base_addr, u64 val)
{
    struct record_node *rec_data;
    if (!is_compart_activated) {
        set_origin_mem(field_addr, val);
        return 0;
    } 
    u32 key = hash_ptr(field_addr, POWER_OF_BUCKETS_NUM);
    rec_data = kzalloc(sizeof(struct record_node), GFP_KERNEL);
    if (!rec_data)
			return -ENOMEM;
    rec_data->type = FIELD;

    rec_data->rec.field.base_addr = (u64) base_addr;
    rec_data->rec.field.addr = (u64) field_addr;
    rec_data->rec.field.data = val;

    hash_add(record_htable, &rec_data->node, key);

    printk("[new field] new_field_addr: %llu, val: %d", (unsigned long long) field_addr, (int) val);
    return 0;
}

int set_field(void *field_addr, void *base_addr, u64 val)
{
    struct record_node *rec_data = NULL;
    u32 key;
    if (!is_compart_activated) {
        set_origin_mem(field_addr, val);
        return 0;
    }
    key = hash_ptr(field_addr, POWER_OF_BUCKETS_NUM);

    hash_for_each_possible(record_htable, rec_data, node, key) {
        if (rec_data->rec.field.addr == (u64)field_addr && rec_data->type == FIELD) {
            break;
        }
    }

    if (rec_data == NULL ) {
        return new_field(field_addr, base_addr, val);
    }

    rec_data->rec.field.data = val;

    printk("[set field] set_addr: %llu, set_val: %d", (unsigned long long) field_addr, (int) val);
    return 0;
}

u64 get_field(void *field_addr)
{
    struct record_node *rec_data = NULL;
    u32 key;
    if (!is_compart_activated) {
        return return_val_in_mem(field_addr);
    }
    key = hash_ptr(field_addr, POWER_OF_BUCKETS_NUM);

    hash_for_each_possible(record_htable, rec_data, node, key) {
        if (rec_data->rec.field.addr == (u64)field_addr && rec_data->type == FIELD) {
            break;
        }
    }

    return rec_data->rec.field.data;

}

void restore_modified_val_to_global(struct record_node *rec_data) 
{
    u64 addr = rec_data->rec.global_data.addr;
    u64 val = rec_data->rec.global_data.data;
    *(u64 *) addr = val;
    printk("[global data] restored_addr: %llu, restored_val: %d", (unsigned long long) addr, (int) val);
}

bool is_mem_valid(u64 mem_addr)
{
    struct record_node *rec_data = NULL;
    u32 key = hash_ptr((void *)mem_addr, POWER_OF_BUCKETS_NUM);
    hash_for_each_possible(record_htable, rec_data, node, key) {
        if (rec_data->rec.mem_obj.addr == mem_addr && rec_data->type == MEMOBJ) {
            break;
        }
    }    

    if (rec_data == NULL) 
        return false;
    return rec_data->rec.mem_obj.is_valid;
}

void restore_modified_val_to_mem(struct record_node *rec_data) 
{
    u64 base_addr = rec_data->rec.field.base_addr;
    u64 addr = rec_data->rec.field.addr;
    u64 val = rec_data->rec.field.data;
    if (!is_mem_valid(base_addr))
        return;
    *(u64 *) addr = val; 
    printk("[restored val back to field] restored_addr: %llu, restored_val%d", (unsigned long long) addr, (int) val);
}

void free_htable(void)
{
    struct record_node *rec_data = NULL;
    struct hlist_node *tmp = NULL;
    int bkt;
    printk("[Free hash table]");
    hash_for_each_safe(record_htable, bkt, tmp, rec_data , node) {
        hash_del(&rec_data->node);
        kfree(rec_data); 
    }
}

void restore(void)
{
    struct record_node *rec_data = NULL;
    int bkt;

    hash_for_each(record_htable, bkt, rec_data, node) {
        enum Node_type rec_type = rec_data->type;

        switch (rec_type) {
            case MEMOBJ:
                break;
            case GLOBAL:
                restore_modified_val_to_global(rec_data);
                break;
            case FIELD:
                restore_modified_val_to_mem(rec_data);
                break;
            default:
                break;
        }
    }
    
    free_htable();
    is_compart_activated = false;
    printk("[Restore] restore record value back to memory");
    return;
}

void free_mem_obj(struct record_node *rec_data) 
{
    void *addr = (void *)rec_data->rec.mem_obj.addr;
    bool is_valid = rec_data->rec.mem_obj.is_valid;
    bool is_alloc_outside = rec_data->rec.mem_obj.is_alloc_outside;

    if (!is_valid || is_alloc_outside) 
        return;
    printk("[Free memobj] addr: %llu", (unsigned long long) addr);
    kvfree(addr);    
}

void recover(void)
{
    struct record_node *rec_data = NULL;
    struct hlist_node *tmp = NULL;
    int bkt;
    
    hash_for_each_safe(record_htable, bkt, tmp, rec_data, node) {
        enum Node_type rec_type = rec_data->type;

        if (rec_type == MEMOBJ) 
            free_mem_obj(rec_data);

    } 
    
    free_htable();
}

