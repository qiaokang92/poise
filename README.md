# Programmable In-Network Security

Poise, or Programmable In-Network Security, is a novel approach that leverages
P4-programmable switches to enforce network access control policies directly
on the data plane.

Our paper, Programmable In-Network Security for Context-aware BYOD Policies,
has been published at USENIX Security Symposium (SEC '20).
Please find our paper at [USENIX website](https://www.usenix.org/conference/usenixsecurity20/presentation/kang).

This repo contains a prototype implementation of the Poise compiler, Poise
Android client module and other tools.

## Preperations

This Poise compiler has been tested on Ubuntu 18.04.1 LTS.
It is based on flex and bison. G++ is required to build Poise.

```
sudo apt install bison flex g++ make
```

To run compiled P4 programs, you need to set up a Tofino switch or
its simulator. These P4 programs have been tested using Tofino SDE 8.4
and the Tofino Wedge 100BF-32X switch
made by Barefoot networks.
More information about the Tofino switch
can be found at [Barefoot website](https://www.barefootnetworks.com/products/brief-tofino/).

## Compiling Poise policies to P4 programs

`policies/*.poise` are examples Poise policies that demonstrate the usage of the
Poise language/compiler.

To compile a Poise program, run the following command:

```
./poise-tofino -f <policy_file> -o <P4_file> -s <sw_command_file>
```

For instance, running `./poise-tofino -f ../policies/p1.poise -o ../p4/p1.p4 -s ../p4/p1.cmd`
will process the `p1.poise` policy and generate switch programs in the `p4` folder.

The key function in `p1.p4` is the `EvaluateContext` routine, which implements
the Poise policy.
This `p1.p4` is the policy-dependent part of the final P4 program.
`poise.p4` implements the P4-independent part, which is shared across all policies.

## Running P4 programs in Tofino switch

Next, we will show to run the generated `p1.p4` program on Tofino switch.
We will need to modify the `poise.p4` program in the `p4` folder to include
the generated `p1.p4` program:

```
#include "./p1.p4"
```

Next, copy these two p4 files to the Tofino SDE folder and invoke `p4_build.sh`
to compile `poise.p4`.
The `p1.p4` will be compiled together with `poise.p4`.
```
./p4_build.sh -p poise.p4
```

After the compilation finishes, we can load the P4 program into switch:
```
./run_switchd.sh -p poise
```

Next, invoke the `run_pd_rpc.py` script (provided by Tofino) to start the control plane
the Poise program. This program is used for installing new incoming flows
to the flow table.

```
./run_pd_rpc.py --target asic-model -p <p4-program> <absolute_path_to_ctrpln_progs>/setup.py
```

Add table entries in the `p1.cmd` file to the switch:
```
pd eval_h1_tab_rec0_id0 add_entry set_ctx_dec poise_h_h1_start 0 poise_h_h1_end 9 priority 0 action_x 15 entry_hdl 1
pd eval_h1_tab_rec0_id1 add_entry set_ctx_dec poise_h_h1_start 17 poise_h_h1_end 10000 priority 0 action_x 15 entry_hdl 1
```

Add forwarding rules to the switch. port 0 is the port of the server and 129 is the drop port. Modify port number based on your topology.
```
pd enforce_cache_dec_tab add_entry forward_to_port meta_cache_dec 0 action_port 0
pd enforce_conn_dec_tab set_default_action forward_to_port action_port 129

pd enforce_conn_dec_tab add_entry forward_to_port meta_conn_dec 0 action_port 129
pd enforce_conn_dec_tab set_default_action forward_to_port action_port 129
```

## Sending/receiving traffic

Once the P4 program and control plane start, we can use the python scripts in
the `tools` folder to synthesize some traffic for testing.

For instance, the following command sends one context packet with customized
header=1 to the switch. Note that `<iface>` is the NIC interface that's connected
to the Tofino switch.
```
/send_to_iface.py 1 1 context <iface>
```
Then we can capture packets from the receiving interfaces and see if Poise enforces
the policies correctly.
```
#see if packets go to the allow iface
./receive_from_iface.py <allow_iface>
#see if packets go to the drop iface
./receive_from_iface.py <drop iface>
```

## Contact

For more information, please feel free to send an email to: qiaokang at rice dot edu
