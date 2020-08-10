from scapy.all import *
import argparse
import subprocess
import sys
import socket
import random
import struct
import readline

user_conn = {"srcmac": "00:00:00:00:00:01", "dstmac": "00:00:00:00:00:05",
   "srcip":"10.10.0.12", "dstip":"158.132.255.113",
   "sport":1001,"dport":1001}

class Log(Packet):
   fields_desc = [
                   BitField("recir_counter", 0, 8)
                  ]

class Poise1(Packet):
   fields_desc = [ BitField("v", 0, 16)]

class Poise2(Packet):
   fields_desc = [ BitField("v", 0, 16)]

class Poise3(Packet):
   fields_desc = [ BitField("v", 0, 16)]

bind_layers(TCP, Log)
bind_layers(Log, Poise1)
bind_layers(Poise1, Poise2)
bind_layers(Poise2, Poise3)

def gen_pkt_from_conn(conn, size, ctx = False,
                      hdr1 = 0):
  src_mac = conn["srcmac"]
  dst_mac = conn["dstmac"]
  src_ip = conn["srcip"]
  dst_ip = conn["dstip"]
  src_port = conn["sport"]
  dst_port = conn["dport"]

  # 14 bytes Ethernet header
  ether = Ether(src=src_mac, dst=dst_mac)

  # 20 bytes IP header
  ip = IP(src=src_ip, dst=dst_ip)

  # 20 bytes TCP header
  tcp = TCP(sport=src_port, dport=dst_port)

  # Log headers (for testing only)
  log = Log()

  # packet layout for context packet
  '''
  header    ethernet_t   ethernet;
  header    ipv4_t       ipv4;
  header    tcp_t        tcp;
  header    log_t        log;
  header    poise_h1_t   poise_h1;
  header    poise_h2_t   poise_h2;
  header    poise_h3_t   poise_h3;
  '''

  # packet layout for data packet
  '''
  header    ethernet_t   ethernet;
  header    ipv4_t       ipv4;
  header    tcp_t        tcp;
  header    log_t        log;
  payload
  '''

  if (ctx):
     p1 = Poise1(v=hdr1)
     p2 = Poise2(v=hdr1)
     p3 = Poise3(v=hdr1)
     ip.proto = 143;
     #payload = 'x' * size
     pkt = ether/ip/tcp/log/p1/p2/p3
  else:
     payload = 'x' * size
     pkt = ether/ip/tcp/log/payload

  return pkt
