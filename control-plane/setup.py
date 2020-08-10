import time
import atexit
import pdb
import threading
execscript("poise.py")

def do_age():
	print "Aging thread started"
	while True:
	    try:
	        if poll_mode:
	            aged_entries = aging_poll()
	        else:
	            aged_entries = aging_notify()
	        age_out(aged_entries)
	    except Exception as e:
	    	#print "Aging broken: ", e
	      break

	    time.sleep(aging_interval)

def do_learn():
	p4_pd.context_fields_register()
	#print "Learning receiver registered, ready to start receiving connections"
	while True:
	    try:
	        msg = learning_notifications_get()
	        learning_notifications_process(msg)
	    except Exception as e:
	    	print "Learning broken: ", e
	        break

	    time.sleep(1/1000.0)

@atexit.register
def learn_unregister():
    global digest
    try:
        p4_pd.context_fields_digest_notify_ack(digest.msg_ptr)
        p4_pd.context_fields_deregister()
    except:
        pass

def learning_notifications_get():
    global digest
    try:
        digest = p4_pd.context_fields_get_digest()
    except Exception as e:
        print "Got Exception ", e
        return []
    if digest.msg != []:
        #print "Found digest message"
        # This prevents a crash in learn_unregister, by ensuring that it
        # will not attempt to ack the same msg_ptr twice (DRV-1108)
        msg_ptr = digest.msg_ptr
        digest.msg_ptr = 0
        p4_pd.context_fields_digest_notify_ack(msg_ptr)
    return digest.msg

def learning_notifications_process(msg):
    global spoise
    ttl = 10*1000 # Change if needed
    conn_mgr.begin_txn(isAtomic=True)
    for m in msg:
        #print "Processing entry"
        if not m.meta_is_context:
            print "Error, can't learn on a non-context packet"
            continue
        spoise.poise_add(m.tcp_srcPort, m.ipv4_srcAddr, m.ipv4_protocol, m.meta_digest_dec);
    conn_mgr.commit_txn(hwSynchronous = True)

UNSEEN = 0

def aging_poll():
    print "Sweeping conn_tab Table"
    p4_pd.conn_tab_update_hit_state()
    entries=get_entries("conn_tab")
    aged_entries = []

    for e in entries:
        hit = p4_pd.conn_tab_get_hit_state(e)
        if hit == 0:
            aged_entries.append(e)
    return aged_entries

def aging_notify():
    print "Checking for expired entries"
    aged_entries = []
    expired = p4_pd.conn_tab_idle_tmo_get_expired()
    for exp in expired:
        if exp.dev_id == dev:
            aged_entries.append(exp.entry)
    return aged_entries

def age_out(entries):
    # DRV-1116
    #conn_mgr.begin_batch()
    global spoise
    for e in entries:
    	# print "In age_out"
        (conn_tab_ms, conn_tab_as) = p4_pd_pose.conn_tab_get_entry(e, from_sw)
        p4_pd.conn_tab_table_delete(e)

        try:
            idx = conn_tab_as.p4_pd_poise_set_conn_idx.action_conn_idx;
            spoise.recycle_idx(idx);
            p4_pd.register_write_conn_decision(idx, UNSEEN);


        except:
            # print "Unable to find Register ", idx
    conn_mgr.complete_operations()
#
# MAIN
#

#pdb.set_trace()
# stolen from lab
spoise = poise()
spoise.setup()

aging_interval = 10 # Seconds
poll_mode = False   # How can we determine it automatically?

aging_t = threading.Thread(target=do_age)
learning_t = threading.Thread(target=do_learn)
#Sleep to make sure the switch is fully set up before trying to register the digest
print "Sleeping before starting aging and learning threads..."
time.sleep(10)
aging_t.start()
learning_t.start()

learning_t.join()
aging_t.join()
