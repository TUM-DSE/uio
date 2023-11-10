# How to Utilize UniBPF
## 1. Build UniBPF Terminal

1. Comment out **flake.nix Line 68** `pkgs.llvmPackages_14.clangUseLLVM` with an `#`. This is required since due to potential Nix bugs, including this package will make gcc lose a platform native header file.
2. `nix develop`
3. `cd terminal`
4. `mkdir build && cd build`
5. `cmake .. && make -j` 
6. Alternative to `5.`, to enable BPF verifier with termination check feature: `cmake -DCHECK_TERMINATION=ON .. && make -j`
7. Quit the current session with, e.g., `Ctrl-D`, and then reenable **flake.nix Line 68** `pkgs.llvmPackages_14.clangUseLLVM` by removing the `#` prefix.

## 2. Integrate UniBPF Framework Library to Your Unikraft Application
This can be easily done by declairing the use `$(UK_LIBS)/ubpf`, `$(UK_LIBS)/uk_bpf_helper`, `(UK_LIBS)/uk_bpf_runtime` and `(UK_LIBS)/ubpf_tracer` to the Makefile of your Unikraft application. For more detail please refer to **apps/null**, **apps/nginx** or **apps/redis**.

## 3. Enable/Disable JiT Compiled Mode
1. `cd` to your application directory.
1. `make menuconfig`
1. Select option `Library Configuration`
1. Scroll down and select the option `libukbpfruntime A BPF runtime library...`
1. Select / De-Select the option `Enable JiT compilation before execute BPF program...`
1. Save and exit menuconfig
1. Remove build folder and rebuild application with `make -j` again

## 4. Connect Unikraft Application with UniBPF Terminal
0. Start your Unikraft application with, e.g., `just run`.
1. Under the application folder, e.g., `apps/null`, `apps/nginx`, `apps/redis`, execute `../../terminal/build/ushell_terminal`
2. Happy debugging! Please note, that by debug command `bpf_exec` and `bpf_attach`, the BPF verifier will be triggered to verify and reject invalid BPF programs. 


# Run the Evaluations
## 1. Microbenchmarks
This benchmarks the *time per instrucutre* of arithmetic and memory operation BPF instructions under jitted and interpreterd mode. 

1. cd into `apps/null/fs0/bpf-microbenchmark`.
1. `python3 0_run_tests.py`
1. After evaluations is done, use `python3 1_extract_data.py` to collect data into csv format.

## 2. Verification, JiT compliation Overheads
1. cd into `apps/null`
1. `make -C fs0/bpf-overhead`
1. Use **UniBPF Terminal** to execute `adds.o`, `hash.o` and `nop.o`, collect the results manually (BPF verification time/memory-consumption, JiT complie time-consumption...etc.).

Note: It is recommended to restart the Unikraft application and UniBPF terminal each time before evaluting the above-mentioned BPF programs to get more fair and correct results. 

## 3. Security Evaluation
1. cd into `apps/null`
1. `make -C fs0/bpf-security`
1. Use **UniBPF Terminal** to execute corresponding BPF programs to observe BPF verifier's behaviors.

## 4. Real World App Benchmark - Nginx
1. cd int `apps/nginx/fs1/bpf`
1. `python3 0_run_tests.py`
1. After evaluations is done, use `python3 1_extract_data.py` to collect data into csv format.

Note. The python script mentioned above is missing for now due to server hard-drive defect.

## 4. Real World App Benchmark - Redis
1. cd int `apps/redis/fs1/bpf`
1. `python3 0_run_tests.py`
1. After evaluations is done, use `python3 1_extract_data.py` to collect data into csv format.

Note. The python script mentioned above is missing for now due to server hard-drive defect.