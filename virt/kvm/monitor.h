/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KVM_MONITOR_H__
#define __KVM_MONITOR_H__

#define POWER_OF_BUCKETS_NUM 7

#include <linux/kernel.h>


enum Node_type {
    MEMOBJ,
    GLOBAL,
    FIELD,
};

struct global_data_node {
    u64 addr;
    u64 data;
};

struct mem_obj_node {
    bool is_valid;
    bool is_alloc_outside;
    u64 addr;
};

struct field_node {
    u64 base_addr;
    u64 addr;
    u64 data;
};

struct record_node {
    enum Node_type type;
    union {
        struct global_data_node global_data;
        struct mem_obj_node mem_obj;
        struct field_node field;
    } rec;
    // void *heap_data;
    struct hlist_node node;
};
//common utility function
void free_htable(void);
u64 return_val_in_mem(void *addr);
void set_origin_mem(void *addr, u64 val);

// global node
int new_global_data(void *var_addr, u64 val);
int set_global_data(void *shared_data, u64 val);
void set_ull_node(void *shared_data, u64 val, struct record_node *rec_data);
u64 get_global_data(void *shared_data);
int init_global_record_data(void *kvm_createvm_count, void *kvm_active_vms);
void restore_record_data_to_global_var(void *kvm_createvm_count, void *kvm_active_vms);

// memory object node
int set_mem_obj(void *obj_addr, bool is_alloc_outside);
int invalid_mem_obj(void *obj_addr);

// field
int new_field(void *field_addr, void *base_addr, u64 val);
int set_field(void *field_addr, void *base_addr, u64 val);
u64 get_field(void *field_addr);

//restore 
void restore_modified_val_to_global(struct record_node *rec_data);
bool is_mem_valid(u64 mem_addr);
void restore_modified_val_to_mem(struct record_node *rec_data);
void free_htable(void);
void restore(void);

//recover
void free_mem_obj(struct record_node *rec_data);
void recover(void);

#endif
