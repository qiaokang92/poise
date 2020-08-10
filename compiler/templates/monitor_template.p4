#define MONNAME_TIMEOUT TIMEOUTVAL

header_type MONNAME_meta_t {
   fields {
      active: 16;
   }
}

metadata MONNAME_meta_t MONNAME_meta;

register MONNAME_ts_high_32 {
    width: 32;
    instance_count: 1;
}

table monitor_MONNAME_tab {
    reads {
        poise_h.HNAME: exact;
    }
    actions {nop;}
    default_action: nop;
    size: 1;
}

blackbox stateful_alu read_MONNAME_t {
    reg: MONNAME_ts_high_32;

    condition_lo: meta.tstamp_high_32 - register_lo < MONNAME_TIMEOUT;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: register_lo;

    output_predicate: condition_lo;
    output_dst: MONNAME_meta.active;
    output_value: predicate;
}

action check_MONNAME_active(idx) {
    read_MONNAME_t.execute_stateful_alu(idx);
}

table check_MONNAME_active_tab {
    actions{check_MONNAME_active;}
    default_action: check_MONNAME_active;
    size: 1;
}

blackbox stateful_alu write_MONNAME_t {
    reg: MONNAME_ts_high_32;

    condition_lo: true;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: meta.tstamp_high_32;

    output_dst: MONNAME_meta.active;
    output_value: predicate;
}

action update_MONNAME(idx) {
    write_MONNAME_t.execute_stateful_alu(idx);
}

table update_MONNAME_tab {
    actions{update_MONNAME;}
    default_action: update_MONNAME;
    size: 1;
}

table eval_MONNAME_tab_recRECNUM_idIDNUM {
    reads {
        MONNAME_meta.active: exact;
EXTRA_CHECK
    }
    actions {set_ctx_dec; nop;}
    default_action: nop;
    size: 2;
}
