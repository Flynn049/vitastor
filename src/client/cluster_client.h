// Copyright (c) Vitaliy Filippov, 2019+
// License: VNPL-1.1 or GNU GPL-2.0+ (see README.md for details)

#pragma once

#include "messenger.h"
#include "etcd_state_client.h"

#define DEFAULT_CLIENT_MAX_DIRTY_BYTES 32*1024*1024
#define DEFAULT_CLIENT_MAX_DIRTY_OPS 1024
#define DEFAULT_CLIENT_MAX_BUFFERED_BYTES 32*1024*1024
#define DEFAULT_CLIENT_MAX_BUFFERED_OPS 1024
#define DEFAULT_CLIENT_MAX_WRITEBACK_IODEPTH 256
#define OSD_OP_READ_BITMAP OSD_OP_SEC_READ_BMP
#define OSD_OP_READ_CHAIN_BITMAP 0x102

#define OSD_OP_IGNORE_READONLY 0x08
#define OSD_OP_WAIT_UP_TIMEOUT 0x10

struct cluster_op_t;

struct cluster_op_part_t
{
    cluster_op_t *parent;
    uint64_t offset;
    uint32_t len;
    pg_num_t pg_num;
    osd_num_t osd_num;
    osd_op_buf_list_t iov;
    unsigned flags;
    osd_op_t op;
};

struct __attribute__((visibility("default"))) cluster_op_t
{
    uint64_t opcode; // OSD_OP_READ, OSD_OP_WRITE, OSD_OP_SYNC, OSD_OP_DELETE, OSD_OP_READ_BITMAP, OSD_OP_READ_CHAIN_BITMAP
    uint64_t inode;
    uint64_t offset;
    uint64_t len;
    // for reads and writes within a single object (stripe),
    // reads can return current version and writes can use "CAS" semantics
    uint64_t version = 0;
    // flags: OSD_OP_IGNORE_READONLY - ignore inode readonly flag
    // OSD_OP_WAIT_UP_TIMEOUT - do not retry the operation infinitely if PG is inactive, only for for <wait_up_timeout>
    uint64_t flags = 0;
    // negative retval is an error number
    // write and read return len on success
    // sync and delete return 0 on success
    // read_bitmap and read_chain_bitmap return the length of bitmap in bits(!)
    int retval;
    osd_op_buf_list_t iov;
    // READ, READ_BITMAP, READ_CHAIN_BITMAP return the bitmap here
    void *bitmap_buf = NULL;
    std::function<void(cluster_op_t*)> callback;
    ~cluster_op_t();

    // for deletions, remove after 'atomic delete':
    bool support_left_on_dead();
    std::vector<osd_num_t> get_left_on_dead();
protected:
    int state = 0;
    uint64_t cur_inode; // for snapshot reads
    bool needs_reslice: 1;
    bool deoptimise_snapshot: 1;
    int retry_after = 0;
    int inflight_count = 0, done_count = 0;
    timespec wait_up_until = {};
    std::vector<cluster_op_part_t> parts;
    void *part_bitmaps = NULL;
    unsigned bitmap_buf_size = 0;
    cluster_op_t *prev = NULL, *next = NULL;
    int prev_wait = 0;
    uint64_t flush_id = 0;
    friend class cluster_client_t;
    friend class writeback_cache_t;
};

struct inode_list_t;
struct inode_list_osd_t;
struct inode_list_pg_t;
class writeback_cache_t;

// FIXME: Split into public and private interfaces
class __attribute__((visibility("default"))) cluster_client_t
{
#ifdef __MOCK__
public:
#endif
    timerfd_manager_t *tfd = NULL;
    ring_loop_t *ringloop = NULL;

    std::map<pool_id_t, uint64_t> pg_counts;
    std::map<pool_pg_num_t, osd_num_t> pg_primary;
    // client_max_dirty_* is actually "max unsynced", for the case when immediate_commit is off
    uint64_t client_max_dirty_bytes = 0;
    uint64_t client_max_dirty_ops = 0;
    // writeback improves (1) small consecutive writes and (2) Q1 writes without fsync
    bool enable_writeback = false;
    // client_max_buffered_* is the real "dirty limit" - maximum amount of writes buffered in memory
    uint64_t client_max_buffered_bytes = 0;
    uint64_t client_max_buffered_ops = 0;
    uint64_t client_max_writeback_iodepth = 0;
    std::string conf_hostname;

    int log_level = 0;
    int client_retry_interval = 50; // ms
    int client_eio_retry_interval = 1000; // ms
    bool client_retry_enospc = true;
    int client_wait_up_timeout = 16; // sec (for listings)

    std::string client_hostname;
    std::map<std::string, int> self_tree_metrics;
    std::map<osd_num_t, int> osd_tree_metrics;

    int retry_timeout_id = -1;
    int retry_timeout_duration = 0;
    std::vector<cluster_op_t*> offline_ops;
    cluster_op_t *op_queue_head = NULL, *op_queue_tail = NULL;
    writeback_cache_t *wb = NULL;
    std::set<osd_num_t> dirty_osds;
    uint64_t dirty_bytes = 0, dirty_ops = 0;

    void *scrap_buffer = NULL;
    unsigned scrap_buffer_size = 0;

    bool pgs_loaded = false;
    ring_consumer_t consumer;
    std::vector<std::function<void(void)>> on_ready_hooks;
    int list_retry_timeout_id = -1;
    timespec list_retry_time = {};
    std::vector<inode_list_t*> lists;
    std::multimap<osd_num_t, osd_op_t*> raw_ops;
    int continuing_ops = 0;
    bool msgr_initialized = false;

public:
    etcd_state_client_t st_cli;

    osd_messenger_t msgr;
    void init_msgr();

    json11::Json::object cli_config, file_config, etcd_global_config;
    json11::Json::object config;

    cluster_client_t(ring_loop_t *ringloop, timerfd_manager_t *tfd, json11::Json config);
    ~cluster_client_t();
    void execute(cluster_op_t *op);
    void execute_raw(osd_num_t osd_num, osd_op_t *op);
    bool is_ready();
    void on_ready(std::function<void(void)> fn);
    bool flush();

    bool get_immediate_commit(uint64_t inode);

    void list_inode(inode_t inode, uint64_t min_offset, uint64_t max_offset, int max_parallel_pgs, std::function<void(
        int status, int pgs_left, pg_num_t pg_num, std::set<object_id>&& objects)> pg_callback);

    //inline uint32_t get_bs_bitmap_granularity() { return st_cli.global_bitmap_granularity; }
    //inline uint64_t get_bs_block_size() { return st_cli.global_block_size; }

#ifndef __MOCK__
protected:
#endif
    void continue_ops(int time_passed = 0);

protected:
    bool affects_osd(uint64_t inode, uint64_t offset, uint64_t len, osd_num_t osd);
    bool affects_pg(uint64_t inode, uint64_t offset, uint64_t len, pool_id_t pool_id, pg_num_t pg_num);

    void on_load_config_hook(json11::Json::object & config);
    void on_load_pgs_hook(bool success);
    void on_change_pool_config_hook();
    void on_change_pg_state_hook(pool_id_t pool_id, pg_num_t pg_num, osd_num_t prev_primary);
    void on_change_osd_state_hook(uint64_t peer_osd);
    void on_change_node_placement_hook();

    void execute_internal(cluster_op_t *op);
    void unshift_op(cluster_op_t *op);
    int continue_rw(cluster_op_t *op);
    bool check_rw(cluster_op_t *op);
    void slice_rw(cluster_op_t *op);
    void reset_retry_timer(int new_duration);
    int try_send(cluster_op_t *op, int i);
    int continue_sync(cluster_op_t *op);
    void send_sync(cluster_op_t *op, cluster_op_part_t *part);
    void handle_op_part(cluster_op_part_t *part);
    void copy_part_bitmap(cluster_op_t *op, cluster_op_part_t *part);
    void erase_op(cluster_op_t *op);
    void calc_wait(cluster_op_t *op);
    void inc_wait(uint64_t opcode, uint64_t flags, cluster_op_t *next, int inc);
    void continue_lists();
    bool continue_listing(inode_list_t *lst);
    bool restart_listing(inode_list_t* lst);
    void retry_start_pg_listing(inode_list_pg_t *pg);
    int start_pg_listing(inode_list_pg_t *pg);
    void send_list(inode_list_osd_t *cur_list);
    bool set_list_retry_timeout(int ms, timespec new_time);
    void finish_list_pg(inode_list_pg_t *pg, bool retry_epipe);
    bool check_finish_listing(inode_list_t *lst);
    void continue_raw_ops(osd_num_t peer_osd);

    osd_num_t select_random_osd(const std::vector<osd_num_t> & osds);
    osd_num_t select_nearest_osd(const std::vector<osd_num_t> & osds);

    friend class writeback_cache_t;
};
