This module working in the kernel layer with the following functionalities:
1) It monitors the packet sent by the specified apps  (i.e., uid == 10022);
2) It registers a virtual file (i.e., /dev/montest) to communicate with the user module;
3) It periodically sends the collected context information to the server.


mon_nf.c: it is the main file of this moudle.
          1) It initializes the kernel module;
					2) It monitor the outgoing packets.

mon_fs.c: it is used to register the virtual file and communicate with user module.

mon_nl.c: it is used to commnuicate with the user moudle based on netlink (it's not used in the current version).

mon_nc.c: it is used to build a TCP connection with the server, and send message to the server
