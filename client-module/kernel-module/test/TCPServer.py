#!/usr/bin/python           # This is server.py file

import socket               # Import socket module
import time

bind_ip = "10.42.0.1"
bind_port = 12345

sk = socket.socket()        # Create a socket object
sk.bind((bind_ip, bind_port))       # Bind to the port
sk.listen(5)                # Now wait for client connection.

while True:
    client, client_ip = sk.accept()     # Establish connection with client.
    print 'Got connection from', client_ip
    if True:
        try:
            rcvData = client.recv(4096)
            if len(rcvData) > 0: 
                print time.time()
                print rcvData
                print ""
                client.send("Thanks!")
                #if rcvData == "Close":
                #    break
        except:
            print "Connection colsed and continue.."
            break
    client.close()                # Close the connection
