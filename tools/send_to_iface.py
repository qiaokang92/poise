#!/usr/bin/env python
import argparse
import sys
import socket
import random
import struct
from scapy.all import sendp, send, get_if_list, get_if_hwaddr, bind_layers
from scapy.all import Packet
from scapy.all import Ether, IP, TCP
from scapy.fields import *
import readline
from gen_trace import *

conn = {"srcmac": "d8:c4:97:72:48:70", "dstmac": "ff:ff:ff:ff:ff:ff",
   "srcip":"192.168.101.11", "dstip":"192.168.101.12",
   "sport":123,"dport":4320}

def handle_pkt(pkt):
    print "got a packet"
    pkt.show2()
    hexdump(pkt)
    sys.stdout.flush()

def main():
   if (len(sys.argv) != 5):
      print "Usgae: %s <num> <hdr> <send_what> <iface>" \
            % sys.argv[0]
      print "Example: %s 10 1 context+data ens3f0" % sys.argv[0]
      exit(1)

   num = int(sys.argv[1])
   hdr1 = int(sys.argv[2])
   send_what = sys.argv[3]
   iface = sys.argv[4]

   s = conf.L2socket(iface=iface)

   for i in range(0, num):
      # we don't really need hdr2 and hdr3, they will be 0 by default
      ctx_pkt1 = gen_pkt_from_conn(conn, 0, True, hdr1)
      data_pkt = gen_pkt_from_conn(conn, 150, False)

      conn["sport"] += 1

      if "context" in send_what:
         s.send(ctx_pkt1)
         ctx_pkt1.show()
         #hexdump(ctx_pkt1)
         print "context packet %d sent" % i

      if "data" in send_what:
         s.send(data_pkt)
         data_pkt.show()
         #hexdump(data_pkt)
         print "data packet sent"

if __name__ == '__main__':
    main()
