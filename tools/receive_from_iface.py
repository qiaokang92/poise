#!/usr/bin/env python
import sys
import struct
import os

from scapy.all import sniff, sendp, hexdump, get_if_list, get_if_hwaddr
from scapy.all import Packet, IPOption
from scapy.all import ShortField, IntField, LongField, BitField, FieldListField, FieldLenField
from scapy.all import IP, UDP, Raw
from scapy.layers.inet import _IPOption_HDR
from gen_trace import *

bind_layers(TCP, Log)
bind_layers(Log, Poise1)

delay_list = []

class IPOption_MRI(IPOption):
    name = "MRI"
    option = 31
    fields_desc = [ _IPOption_HDR,
                    FieldLenField("length", None, fmt="B",
                                  length_of="swids",
                                  adjust=lambda pkt,l:l+4),
                    ShortField("count", 0),
                    FieldListField("swids",
                                   [],
                                   IntField("", 0),
                                   length_from=lambda pkt:pkt.count*4) ]
def handle_pkt(pkt):
    # print "got a packet"
    if (pkt.haslayer(Log)):
       delay = pkt[Log].ts_egress_48 - pkt[Log].ts_ingress_48
       delay_list.append(delay)

    pkt.show()
    #print pkt[Log].resub_counter
    if (len(delay_list) == int(sys.argv[2])):
       print delay_list
       print sum(delay_list) / len(delay_list)

    sys.stdout.flush()

def main():
    if (len(sys.argv) != 3):
       print "Usage: ./program <iface> <num>"
       exit(1)
    iface = sys.argv[1]
    print "sniffing on %s" % iface
    sys.stdout.flush()
    sniff(iface = iface,
          prn = lambda x: handle_pkt(x))

if __name__ == '__main__':
    main()
