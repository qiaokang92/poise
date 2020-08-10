#ifndef POLICY_P4
#define POLICY_P4

#define RECIR_TIMES 0

header_type poise_h_t{
   fields {
      h1 : 16;
   }
}

table eval_h1_tab_rec0_id0 {
   reads {
      poise_h.h1 : exact;
   }
   actions {set_ctx_dec; nop;}
   default_action: nop;
   size: 2;
}

control EvaluateContext
{
   if (log.recir_counter == 0) {
      apply(eval_h1_tab_rec0_id0);
   }

   if (log.recir_counter < RECIR_TIMES) {
      apply(do_recirculate_tab);
   } else {
      apply(drop1_tab);
   }
}
#endif
