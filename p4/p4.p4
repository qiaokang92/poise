#ifndef POLICY_P4
#define POLICY_P4

#define RECIR_TIMES 0

header_type poise_h_t{
   fields {
      h1 : 16;
   }
}

#define m0_TIMEOUT 100000

header_type m0_meta_t {
   fields {
      active: 16;
   }
}

metadata m0_meta_t m0_meta;

register m0_ts_high_32 {
    width: 32;
    instance_count: 1;
}

table monitor_m0_tab {
    reads {
        poise_h.h1: exact;
    }
    actions {nop;}
    default_action: nop;
    size: 1;
}

blackbox stateful_alu read_m0_t {
    reg: m0_ts_high_32;

    condition_lo: meta.tstamp_high_32 - register_lo < m0_TIMEOUT;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: register_lo;

    output_predicate: condition_lo;
    output_dst: m0_meta.active;
    output_value: predicate;
}

action check_m0_active(idx) {
    read_m0_t.execute_stateful_alu(idx);
}

table check_m0_active_tab {
    actions{check_m0_active;}
    default_action: check_m0_active;
    size: 1;
}

blackbox stateful_alu write_m0_t {
    reg: m0_ts_high_32;

    condition_lo: true;

    update_lo_1_predicate: condition_lo;
    update_lo_1_value: meta.tstamp_high_32;

    output_dst: m0_meta.active;
    output_value: predicate;
}

action update_m0(idx) {
    write_m0_t.execute_stateful_alu(idx);
}

table update_m0_tab {
    actions{update_m0;}
    default_action: update_m0;
    size: 1;
}

table eval_m0_tab_rec0_id0 {
    reads {
        m0_meta.active: exact;

    }
    actions {set_ctx_dec; nop;}
    default_action: nop;
    size: 2;
}
control EvaluateContext
{
   if (log.recir_counter == 0) {
      apply(monitor_m0_tab) {
         hit {
            apply(update_m0_tab);
         }
         miss {
            apply(check_m0_active_tab);
         }
      }
      apply(eval_m0_tab_rec0_id0);
   }

   if (log.recir_counter < RECIR_TIMES) {
      apply(do_recirculate_tab);
   } else {
      apply(drop1_tab);
   }
}
#endif
