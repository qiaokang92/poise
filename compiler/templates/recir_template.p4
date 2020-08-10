/* -*- P4_14 -*- */

#include <tofino/intrinsic_metadata.p4>
#include <tofino/constants.p4>
#include <tofino/stateful_alu_blackbox.p4>
#include <tofino/primitives.p4>

// Decision constants
// decision 2-14 are reserved for different uses (e.g., middleboxes)
// default: never seen this conn
#define DEC_UNSEEN        0
// allow this conn
#define DEC_CLEAN         1
// block this conn
#define DEC_DROP          15
// max number of different decisions is 2^4
#define ENFORCE_TABLE_SIZE 16

// cache hit/miss
#define CACHE_COLL 0
#define CACHE_EMPTY 1
#define CACHE_HIT 2

#define RECIR_PORT 132
#define DROP_PORT  2

#define DEBUG 1

#define MAX_CONN_TABLE_SIZE   100000
#define MAX_CONN_REGS_PER_MAU 57000
#define CONN_REGS_2           43000

// Ethernet header protocol
#define TYPE_IPV4  0x800

// IP header protocol
#define PROTO_TCP        0x06
#define PROTO_UDP        0x11
#define PROTO_POISE_TCP  0x8F
#define PROTO_POISE_UDP  0x90

// Cache constants
#define CACHE_SIZE 65536

#define CACHE_TIMEOUT 5
//#define CACHE_TIMEOUT 100

// BF constants
#define BF_SIZE 86
#define BF_CACHE_SIZE 3
#define BF_KEY1 56
#define BF_KEY2 32
#define BF_KEY3 478

#define egressSpec_t 9
#define macAddr_t 48
#define ip4Addr_t 32
#define ip6Addr_t 128

#include "../build/policy.p4"

// The maximum number of times we will try to recirculate a packet on BF miss
#define MAX_RESUB 3

// =======================================================//
// ===================== metadata ========================//
// =======================================================//

header_type custom_metadata_t {
    fields {
        // higher 32 bits of ingress timestamp
        tstamp_high_32: 16;
        // Boolean whether the timestamp of CACHE indicates it is outdated
        tstamp_high_32_outdated: 16;
        // The src ip found in the cache
        cache_src_ip: ip4Addr_t;
        // The src port found in the cache
        cache_src_port: 16;
        // The protocol found in the cache
        cache_protoc: 8;
        // The register index matched in the connection table
        conn_idx: 20;
        conn_idx_2: 20;
        // The decision made from the context packet.
        ctx_dec: 4;
        // The decision found in cache
        cache_dec: 4;
        // The decision found in conn_tab
        conn_dec: 4;
        // Indicate the 1st/2nd/3rd BF hit
        bf_1_val: 1;
        bf_2_val: 1;
        bf_3_val: 1;
        // Indicates if this is a context packet
        is_context: 1;
        // real IP protocol
        ip_protocol: 8;
        // Indicates if we got a cache hit after missing the connection table
        cache_hit: 2;
        // The number of times we've resubmitted this packet
        resub_num: 2;
        // decision made based on poise headers
        h1_dec: 4;
        conn_idx_offset: 20;
        conn_miss: 1;
    }
}

metadata  custom_metadata_t meta;

// =======================================================//
// ====================== headers ========================//
// =======================================================//

header_type ethernet_t {
    fields {
        dstAddr: macAddr_t;
        srcAddr: macAddr_t;
        etherType: 16;
    }
}

// Minimal IPV4 header: 20 bytes
header_type ipv4_t {
    fields {
        version: 4;
        ihl: 4;
        diffserv: 8;
        totalLen: 16;
        identification: 16;
        flags: 3;
        fragOffset: 13;
        ttl: 8;
        protocol: 8;
        hdrChecksum: 16;
        srcAddr: ip4Addr_t;
        dstAddr: ip4Addr_t;
    }
}

// Minimal TCP header: 20 bytes
header_type tcp_t {
    fields {
        srcPort: 16;
        dstPort: 16;
        seq_no: 32;
        ack_no: 32;
        offset: 4;
        reserved: 4;
        control_b: 8;
        window: 16;
        checksum: 16;
        urgentPtr: 16;
    }
}

// UDP header: 8 bytes
header_type udp_t {
    fields {
        srcPort: 16;
        dstPort: 16;
        len: 16;
        checksum: 16;
    }
}

// Debugging log headers, see update_log action
header_type log_t {
   fields {
      recir_counter: 8;
      cache_status: 16;
      cache_src_ip: 32;
      cache_port: 16;
      cache_protoc: 8;
      cache_dec: 4;
      ctx_dec: 4;
      outdated: 16;
      ts_high_32: 32;
      ts_ingress_48: 48;
      ts_egress_48: 48;
   }
}

header    ethernet_t   ethernet;
header    ipv4_t       ipv4;
header    tcp_t        tcp;
header    udp_t        udp;
header    poise_h_t    poise_h;
header    log_t        log;

// =======================================================//
// ====================== parser =========================//
// =======================================================//

parser start {
    set_metadata(meta.conn_idx_offset, MAX_CONN_REGS_PER_MAU);
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(ethernet.etherType) {
        TYPE_IPV4: parse_ipv4;
        default: MyIngress;
    }
}

parser parse_ipv4 {
    extract(ipv4);
    return select(ipv4.protocol) {
       PROTO_TCP:       parse_tcp;
       PROTO_UDP:       parse_udp;
       PROTO_POISE_TCP: parse_poise_tcp;
       PROTO_POISE_UDP: parse_poise_udp;
       default: MyIngress;
    }
}

parser parse_tcp {
    extract(tcp);
    return parse_data;
}

parser parse_udp {
    extract(udp);
    return parse_data;
}

parser parse_poise_tcp {
    set_metadata(meta.is_context, 1);
    set_metadata(meta.ip_protocol, PROTO_TCP);
    extract(log);
    extract(poise_h);
    return MyIngress;
}

parser parse_poise_udp {
    set_metadata(meta.is_context, 1);
    set_metadata(meta.ip_protocol, PROTO_UDP);
    extract(log);
    extract(poise_h);
    return MyIngress;
}

parser parse_data {
    return MyIngress;
}

register conn_decision_1 {
    width: 8;
    instance_count: MAX_CONN_REGS_PER_MAU;
}

register conn_decision_2 {
    width: 8;
    instance_count: CONN_REGS_2;
}

// Registers to hold the Cache
register cache_dec {    // Decision of cache entry
    width: 8;
    instance_count : CACHE_SIZE;
}

// High 32 bits of timestamp
// Note: for optimization, it was changed to 16 bits.
// TODO: enable 32-bit timestamp.
register cache_tstamp_high_32 {
    width: 16;
    instance_count : CACHE_SIZE;
}
register cache_src_ip {     // Src ip of cache entry
    width: ip4Addr_t;
    instance_count : CACHE_SIZE;
}
register cache_src_port {   // Src port of cache entry
    width: 16;
    instance_count: CACHE_SIZE;
}
register cache_protoc {     // Protocol of cache entry
    width: 8;
    instance_count: CACHE_SIZE;
}

// Registers for BF
register bf_1 {
    width: 8;
    instance_count: BF_SIZE;
}
register bf_2 {
    width: 8;
    instance_count: BF_SIZE;
}
register bf_3 {
    width: 8;
    instance_count: BF_SIZE;
}

action forward_to_port(port){
   modify_field(ig_intr_md_for_tm.ucast_egress_port, port);
}

action nop() {}

action my_drop() {
#if DEBUG
    forward_to_port(DROP_PORT);
#else
    mark_for_drop();
#endif
}

// Read value from register-indexed connection tables
action set_conn_idx(conn_idx) {
    modify_field(meta.conn_idx, conn_idx);
    subtract(meta.conn_idx_2, conn_idx, meta.conn_idx_offset);
    modify_field(meta.conn_miss, 0);
}
action set_conn_miss() {
    modify_field(meta.conn_miss, 1);
}

@pragma immediate 1
@pragma idletime_precision 2
table conn_tab {
    reads {
        ipv4.srcAddr: exact;
        tcp.srcPort: exact;
        meta.ip_protocol: exact;
    }
    actions {
        set_conn_idx;
    }
    default_action: set_conn_miss;
    size: MAX_CONN_TABLE_SIZE;
    support_timeout: true;
}

// Read and/or write the connection table registers
blackbox stateful_alu read_conn_1 {
    reg : conn_decision_1;

    // If it is a context packet, update the entry
    condition_lo: meta.is_context != 0;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: log.ctx_dec;

    // If it is a data packet, read the decision into metadata
    output_dst: meta.conn_dec;
    output_value: register_lo;
    output_predicate: not condition_lo;
}

// Read and/or write the connection table registers
blackbox stateful_alu read_conn_2 {
    reg : conn_decision_2;

    // If it is a context packet, update the entry
    condition_lo: meta.is_context != 0;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: log.ctx_dec;

    // If it is a data packet, read the decision into metadata
    output_dst: meta.conn_dec;
    output_value: register_lo;
    output_predicate: not condition_lo;
}

action read_conn_dec_1() {
    read_conn_1.execute_stateful_alu(meta.conn_idx);
}

@pragma ternary 1
table read_conn_dec_1_tab {
    reads{meta.conn_idx: range;}
    actions{read_conn_dec_1;}
    default_action: nop;
    size: 1;
}

action read_conn_dec_2() {
    read_conn_2.execute_stateful_alu(meta.conn_idx_2);
}

@pragma ternary 1
table read_conn_dec_2_tab {
    actions{read_conn_dec_2;}
    default_action: read_conn_dec_2;
    size: 1;
}

// Populate two enforcement tables like this:
// key                     action
//---------------------------------
// DEC_UNSEEN              my_drop()
// DEC_CLEAN               forward_to_port(0)
// DEC_DROP                my_drop()
// 2~14                    forward_to_port(mb_n)
@pragma ternary 1
table enforce_cache_dec_tab {
    reads {
        meta.cache_dec: exact;
    }
    actions {
        forward_to_port;
        my_drop;
    }
    default_action: my_drop;
    size: ENFORCE_TABLE_SIZE;
}

@pragma ternary 1
table enforce_conn_dec_tab {
    reads {
        meta.conn_dec: exact;
    }
    actions {
        forward_to_port;
        my_drop;
    }
    default_action: my_drop;
    size: ENFORCE_TABLE_SIZE;
}

// Read and/or write the cache
// hash calculation for cache index
field_list tup_3_fields {
    tcp.srcPort;
    ipv4.srcAddr;
    //ipv4.protocol;
    meta.ip_protocol;
}
field_list_calculation cache_hash_calc {
    input {tup_3_fields;}
    algorithm : crc_16;
    output_width: 16;
}

// For context packet:
//    update the cache tstamp_high_32
// For all packets:
//    calculate whether the entry is outdated and store the result to metadata
// NOTE: output value in meta.tstamp_high_32_outdated may not be 1 when outdated,
//       it could be 0x41 or 0x81.
//       so alway use == 0 or != 0 to judge this value.
blackbox stateful_alu handle_cache_tstamp_high_32_stfu{
    reg : cache_tstamp_high_32;

    condition_lo: meta.is_context != 0;
    condition_hi: meta.tstamp_high_32 - register_lo > CACHE_TIMEOUT;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: meta.tstamp_high_32;

    output_predicate: condition_hi;
    output_dst: meta.tstamp_high_32_outdated;
    output_value: predicate;
}
action handle_cache_tstamp_high_32() {
    handle_cache_tstamp_high_32_stfu.execute_stateful_alu_from_hash(cache_hash_calc);
}
@pragma ternary 1
@pragma stage 8
table apply_cache_tstamp_high_32_tab {
    actions {handle_cache_tstamp_high_32;}
    default_action: handle_cache_tstamp_high_32;
    size: 1;
}

// Read and/or write the cache decision
blackbox stateful_alu handle_cache_dec_stfu {
    reg: cache_dec;

    condition_lo: meta.is_context != 0;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: log.ctx_dec;

    output_predicate: true;
    output_dst: meta.cache_dec;
    output_value: register_lo;
}
action handle_cache_dec() {
    handle_cache_dec_stfu.execute_stateful_alu_from_hash(cache_hash_calc);
}
@pragma ternary 1
table apply_cache_dec_tab {
    actions {handle_cache_dec;}
    default_action: handle_cache_dec;
    size: 1;
}

// Read and/or write the cache key (src ip, src port, protocol)
blackbox stateful_alu handle_cache_src_ip_stfu {
    reg: cache_src_ip;

    condition_lo: meta.is_context != 0;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: ipv4.srcAddr;

    output_dst: meta.cache_src_ip;
    output_predicate: true;
    output_value: register_lo;
}

blackbox stateful_alu handle_cache_src_port_stfu {
    reg: cache_src_port;

    condition_lo: meta.is_context != 0;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: tcp.srcPort;

    output_dst: meta.cache_src_port;
    output_predicate: true;
    output_value: register_lo;
}

blackbox stateful_alu handle_cache_protoc_stfu {
    reg: cache_protoc;

    condition_lo: meta.is_context != 0;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: meta.ip_protocol;

    output_dst: meta.cache_protoc;
    output_predicate: true;
    output_value: register_lo;
}
action handle_cache_ip() {
    handle_cache_src_ip_stfu.execute_stateful_alu_from_hash(cache_hash_calc);
}
@pragma ternary 1
@pragma stage 8
table apply_cache_ip_tab {
    actions {handle_cache_ip;}
    default_action: handle_cache_ip;
    size: 1;
}
action handle_cache_port() {
    handle_cache_src_port_stfu.execute_stateful_alu_from_hash(cache_hash_calc);
}
@pragma ternary 1
@pragma stage 8
table apply_cache_port_tab {
    actions {handle_cache_port;}
    default_action: handle_cache_port;
    size: 1;
}
action handle_cache_protoc() {
    handle_cache_protoc_stfu.execute_stateful_alu_from_hash(cache_hash_calc);
}
@pragma ternary 1
@pragma stage 8
table apply_cache_protoc_tab {
    actions {handle_cache_protoc;}
    default_action: handle_cache_protoc;
    size: 1;
}

// For writing the BF based on the values found in the cache
// (for context packet evictions)
field_list cache_bfk1_fields {
    meta.cache_src_port;
    meta.cache_src_ip;
    meta.cache_protoc;
    BF_KEY1;
}
field_list_calculation cache_bf1_hash_calc {
    input {cache_bfk1_fields;}
    algorithm : crc_16;
    output_width: 16;
}
field_list cache_bfk2_fields {
    meta.cache_src_port;
    meta.cache_src_ip;
    meta.cache_protoc;
    BF_KEY2;
}
field_list_calculation cache_bf2_hash_calc {
    input {cache_bfk2_fields;}
    algorithm : crc_16;
    output_width: 16;
}
field_list cache_bfk3_fields {
    meta.cache_src_port;
    meta.cache_src_ip;
    meta.cache_protoc;
    BF_KEY3;
}
field_list_calculation cache_bf3_hash_calc {
    input {cache_bfk3_fields;}
    algorithm : crc_16;
    output_width: 16;
}

blackbox stateful_alu handle_bf1_stfu {
    reg: bf_1;

    condition_lo: meta.cache_dec - DEC_DROP == 0;
    condition_hi: meta.is_context != 0;

    update_lo_1_predicate: condition_hi and condition_lo;
    update_lo_1_value: 0x1;

    output_dst: meta.bf_1_val;
    output_predicate: not condition_hi;
    output_value: register_lo;
}

blackbox stateful_alu handle_bf2_stfu {
    reg: bf_2;

    condition_lo: meta.cache_dec - DEC_DROP == 0;
    condition_hi: meta.is_context != 0;

    update_lo_1_predicate: condition_hi and condition_lo;
    update_lo_1_value: 0x1;

    output_dst: meta.bf_2_val;
    output_predicate: not condition_hi;
    output_value: register_lo;
}

blackbox stateful_alu handle_bf3_stfu {
    reg: bf_3;

    condition_lo: meta.cache_dec - DEC_DROP == 0;
    condition_hi: meta.is_context != 0;

    update_lo_1_predicate: condition_hi and condition_lo;
    update_lo_1_value: 0x1;

    output_dst: meta.bf_3_val;
    output_predicate: not condition_hi;
    output_value: register_lo;
}

action handle_bf1() {
    handle_bf1_stfu.execute_stateful_alu_from_hash(cache_bf1_hash_calc);
}

action handle_bf2() {
    handle_bf2_stfu.execute_stateful_alu_from_hash(cache_bf2_hash_calc);
}

action handle_bf3() {
    handle_bf3_stfu.execute_stateful_alu_from_hash(cache_bf3_hash_calc);
}

@pragma ternary 1
table apply_bf1_tab {
    actions{handle_bf1;}
    default_action: handle_bf1;
    size: 1;
}

@pragma ternary 1
table apply_bf2_tab {
    actions{handle_bf2;}
    default_action: handle_bf2;
    size: 1;
}

@pragma ternary 1
table apply_bf3_tab {
    actions{handle_bf3;}
    default_action: handle_bf3;
    size: 1;
}

// Generate a learning digest for new connections
field_list context_fields {
    tcp.srcPort;
    ipv4.srcAddr;
    //ipv4.protocol;
    meta.ip_protocol;
    log.ctx_dec;
    meta.is_context;
}
action _generate_digest() {
    generate_digest(0, context_fields);
}
@pragma ternary 1
table generate_digest_tab {
    actions{_generate_digest;}
    default_action: _generate_digest;
    size: 1;
}

action set_cache_hit() {
    modify_field(meta.cache_hit, CACHE_HIT);
}
@pragma ternary 1
table set_cache_hit_tab {
    actions{set_cache_hit;}
    default_action: set_cache_hit;
    size: 1;
}

action set_cache_empty() {
    modify_field(meta.cache_hit, CACHE_EMPTY);
}
@pragma ternary 1
table set_cache_empty_tab {
    actions{set_cache_empty;}
    default_action: set_cache_empty;
    size: 1;
}

action set_ctx_dec(x) {
    modify_field(log.ctx_dec, x);
}

@pragma ternary 1
table drop1_tab {
    actions{my_drop;}
    default_action: my_drop;
    size: 1;
}

@pragma ternary 1
table drop3_tab {
    actions{my_drop;}
    default_action: my_drop;
    size: 1;
}

@pragma ternary 1
table drop4_tab {
    actions{my_drop;}
    default_action: my_drop;
    size: 1;
}

// Use high 32-bit tstamp of the full 48 bits
field_list tstamp {
    _ingress_global_tstamp_;
}
field_list_calculation tstamp_shift {
    input {tstamp;}
    algorithm : identity_msb;
    // TODO: 16 bit width is for resource optimization.
    output_width: 16;
}
action split_tstamp() {
    modify_field_with_hash_based_offset(meta.tstamp_high_32, 0, tstamp_shift, 65536);
}
@pragma ternary 1
table split_tstamp_tab {
    actions{split_tstamp;}
    default_action: split_tstamp;
    size: 1;
}

// Send the packet to the cpu, used when we receive a miss on the BF
// (do not know real destination)
field_list resub_list{
    meta.resub_num;
}

action resub() {
    add_to_field(meta.resub_num, 1);
    resubmit(resub_list);
}
@pragma ternary 1

table resub_tab {
    actions{resub;}
    default_action: resub;
    size: 1;
}

action update_log() {
   modify_field(log.cache_status, meta.cache_hit);
   modify_field(log.cache_src_ip, meta.cache_src_ip);
   modify_field(log.cache_port, meta.cache_src_port);
   modify_field(log.cache_protoc, meta.cache_protoc);
   modify_field(log.cache_dec, meta.cache_dec);
   modify_field(log.outdated, meta.tstamp_high_32_outdated);
   modify_field(log.ts_high_32, meta.tstamp_high_32);
}
@pragma ternary 1
table update_log_tab {
   actions {update_log;}
   default_action: update_log;
   size: 1;
}

action update_log_ingress_tstamp() {
   modify_field(log.ts_ingress_48, _ingress_global_tstamp_);
}
@pragma ternary 1
table update_log_ingress_tstamp_tab {
   actions {update_log_ingress_tstamp;}
   default_action: update_log_ingress_tstamp;
   size :1;
}

action update_log_egress_tstamp() {
   modify_field(log.ts_egress_48,
                eg_intr_md_from_parser_aux.egress_global_tstamp);
}
@pragma ternary 1
table update_log_egress_tstamp_tab {
   actions {update_log_egress_tstamp;}
   default_action: update_log_egress_tstamp;
   size :1;
}

action do_recirculate() {
   add_to_field(log.recir_counter, 1);
   forward_to_port(RECIR_PORT);
}

table do_recirculate_tab {
   actions {do_recirculate;}
   default_action: do_recirculate;
   size :1;
}

// =======================================================//
// ====================== ingress ========================//
// =======================================================//

// TODO: add LOG header
control MyIngress {

#if DEBUG
   if (log.recir_counter == 0) {
       apply(update_log_ingress_tstamp_tab);
   }
#endif

   if (meta.is_context != 0) {
      EvaluateContext();
   }

   // check the conn table first
   apply(split_tstamp_tab);
   apply(conn_tab);

   // data packet: read the cache values to metadata
   // ctx packet : update cache entry
   apply(apply_cache_tstamp_high_32_tab);
   apply(apply_cache_ip_tab);
   apply(apply_cache_port_tab);
   apply(apply_cache_protoc_tab);
   apply(apply_cache_dec_tab);

   // hit conn_tab
   // - data packets:    read decision from conn tab to meta.conn_dec
   // - context packets: update decision in conn_tab
   if (meta.conn_miss == 0) {
       apply(read_conn_dec_1_tab) {
           nop {
               apply(read_conn_dec_2_tab);
           }
       }
   }

   // check if the packet hits the cache
   // TODO: add port and protocol check
   if (meta.cache_src_ip == ipv4.srcAddr) {
      //and meta.cache_src_port == tcp.srcPort) {
      apply(set_cache_hit_tab);
   } else if (meta.cache_dec == DEC_UNSEEN) {
      // unseen connection
      apply(set_cache_empty_tab);
   } else {
      // collision happens: read/write BF
      apply(apply_bf1_tab);
      apply(apply_bf2_tab);
      apply(apply_bf3_tab);
   }

   // miss conn_tab
   if (meta.conn_miss == 1) {
      if (meta.is_context != 0) {
         // context packet: generate digest and send to control plane
         if (meta.cache_hit != CACHE_HIT) {
            apply(generate_digest_tab);
         }
      } else {
         // data packet: enforce decision found in cache
         // if (meta.cache_hit == CACHE_HIT and meta.tstamp_high_32_outdated == 0) {
         // TODO: not checking timeout
         if (meta.cache_hit == CACHE_HIT) {
            apply(enforce_cache_dec_tab);
         } else if (meta.cache_hit == CACHE_COLL) {
            if (meta.bf_1_val == 1 and meta.bf_2_val == 1 and meta.bf_3_val == 1) {
               apply(drop3_tab);
            } else if(meta.resub_num < MAX_RESUB) {
               apply(resub_tab);
            } else {
               apply(drop3_tab);
            }
         } else {
            apply(drop4_tab);
         }
      }
   // hit conn_tab
   } else {
      if (meta.is_context == 0) {
         apply(enforce_conn_dec_tab);
      }
   }
#if DEBUG
   apply(update_log_tab);
#endif
}

// =======================================================//
// ======================= egress ========================//
// =======================================================//
control egress {
#if DEBUG
   apply(update_log_egress_tstamp_tab);
#endif
}
