## Build the compiler

Use `make tofino` to build a tofino specific Poise compiler

## Compile Poise program

Run `./poise-<target>` for help information.

To test sequential policy compilation, run
```
./poise-tofino -f policies/test.poise -o build/policy.p4 -s build/test.cmd
```

Then include `policy.p4` in `recir_template.p4` program.
