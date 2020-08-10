#
# Use "run_pd_rpc.py -p poise" to run
#

import decimal

FWD1 = 1
FWD2 = 2
FWD3 = 3
FWD4 = 4
APPLY_BLACKLIST = 4
UNSEEN = 0
CONN_TB_SIZE = 100000
class poise():
    def __init__(self):
        self.vlan       = {}
        self.all_ports  = []
        self.poise_age_ttl = 0
        self.free_idxs = []

        for pipe in range(0, 3):
            for port in range(0, 63):
                self.all_ports.append(to_devport(pipe, port))
            if pipe % 2 == 0:
                self.all_ports.append(to_devport(pipe, 64))

    def setup(self):
        clear_all()
        try:
            p4_pd.conn_tab_idle_tmo_disable()

        except:
            pass
        print "Setting up indexes"
        for i in range(CONN_TB_SIZE):
            self.free_idxs.append(i)

        # Enable aging for the connection table. Constant values stolen from lab implementation,
        # can change if needed
        print "Enabling timeout"
        p4_pd.conn_tab_idle_tmo_enable(
            p4_pd.idle_time_params_t(
                mode = p4_pd.idle_time_mode.NOTIFY_MODE,
                ttl_query_interval =  10 * 1000,
                max_ttl            = 300 * 1000,
                min_ttl            =  10 * 1000,
                cookie             = hex_to_i32(0xABCD1234),
            ))

        # evaluate_ctx_tab
        # print "Setting up evaluate_ctx_tab"
        # evaluate_ctx_ms = p4_pd.evaluate_ctx_tab_match_spec_t(FWD1)
        # p4_pd.evaluate_ctx_tab_table_add_with_set_fwd1(evaluate_ctx_ms)

        # enforce_dec_tab
        '''
        print "Setting up enforce_dec_tab"
        for i in range(1, 5):
           enforce_dec_tab_ms = p4_pd.enforce_dec_tab_match_spec_t(i)
           enforce_dec_tab_as = p4_pd.forward_to_port_action_spec_t(i-1)
           p4_pd.enforce_dec_tab_table_add_with_forward_to_port(enforce_dec_tab_ms,
                                                                enforce_dec_tab_as)
        print "Finished adding static entries"
        '''


    def poise_add(self, srcPort, srcAddr, protocol, decision):
        #print "adding"
        a = decimal.Decimal(time.time())
        conn_tab_ms = p4_pd.conn_tab_match_spec_t(srcAddr, srcPort, protocol)
        # find a free idx
        if len(self.free_idxs) == 0:
            print "Error, no indexes available (either conn_table is full or there is a bug)"
            return

        idx = self.free_idxs.pop()
        try:
            conn_tab_as = p4_pd.set_conn_idx_action_spec_t(idx)
            conn_tab_es = p4_pd.conn_tab_table_add_with_set_conn_idx(conn_tab_ms, conn_tab_as, self.poise_age_ttl)
        except Exception as e:
            print "Exception adding entry: ", e
            return

        # Try to set the decision in the register
        try:
            p4_pd.register_write_conn_decision(idx, decision)
        except:
            print "Unable to find register %d"(idx)
        b = decimal.Decimal(time.time())
        decimal.getcontext().prec = 5
        print (b-a)*1000000,
        print ", ",
        #print "returning normally"

    def recycle_idx(self, idx):
        self.free_idxs.append(idx)


