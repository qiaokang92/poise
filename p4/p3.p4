#ifndef POLICY_P4
#define POLICY_P4

#define RECIR_TIMES 0

header_type poise_h_t{
   fields {
      h1 : 16;
      h2 : 16;
   }
}

table eval_h1_tab_rec0_id0 {
   reads {
      poise_h.h1 : range;
   }
   actions {set_ctx_dec; nop;}
   default_action: nop;
   size: 2;
}

table eval_h1_tab_rec0_id1 {
   reads {
      poise_h.h1 : range;
   }
   actions {set_ctx_dec; nop;}
   default_action: nop;
   size: 2;
}

table eval_h2_tab_rec0_id2 {
   reads {
      poise_h.h2 : range;
   }
   actions {set_ctx_dec; nop;}
   default_action: nop;
   size: 2;
}

table eval_h2_tab_rec0_id3 {
   reads {
      poise_h.h2 : range;
   }
   actions {set_ctx_dec; nop;}
   default_action: nop;
   size: 2;
}

control EvaluateContext
{
   if (log.recir_counter == 0) {
      apply(eval_h1_tab_rec0_id0);
      apply(eval_h1_tab_rec0_id1);
      apply(eval_h2_tab_rec0_id2);
      apply(eval_h2_tab_rec0_id3);
   }

   if (log.recir_counter < RECIR_TIMES) {
      apply(do_recirculate_tab);
   } else {
      apply(drop1_tab);
   }
}
#endif
