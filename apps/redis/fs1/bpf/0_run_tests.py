import logging
import os
from pathlib import Path
from signal import SIGINT
import socket
import subprocess
from threading import Timer
import threading
from time import sleep

ROUNDS = 10
APP_ROOT = Path(os.getcwd()).parent.parent.absolute()
print("DEBUG APP_ROOT = ", APP_ROOT)

def compile_application():
    retval = subprocess.call("rm -r build/", cwd=APP_ROOT, shell=True)
    if retval != 0:
        raise RuntimeError(f"Failed to rm build folder, code: {retval}")
    
    retval = subprocess.call(["make", "-j"], cwd=APP_ROOT, shell=True)
    if retval != 0:
        raise RuntimeError(f"Failed to compile Unikernel application, code: {retval}")
    print(">>>>> INFO Compiled application.")

def compile_bpf_program():
    retval = subprocess.call(["make", "clean", "all"])
    if retval != 0:
        raise RuntimeError("Failed to compile BPF programs, code: {retval}")
    print(">>>>> INFO Compiled all test BPF programs.")

def enable_jit():
    updated_configs = []
    with open("../../.config", mode="r", encoding="utf8") as configFile:
        lines = configFile.readlines()
        
        for line in lines:
            if line.strip() == "# CONFIG_LIB_UNIBPF_JIT_COMPILE is not set":
                updated_configs.append("CONFIG_LIB_UNIBPF_JIT_COMPILE=y\n")
            else:
                updated_configs.append(line)

    
    with open("../../.config", mode="w", encoding="utf8") as configFile:
        for line in updated_configs:
            configFile.write(line)

def disable_jit():
    updated_configs = []
    with open("../../.config", mode="r", encoding="utf8") as configFile:
        lines = configFile.readlines()
        
        for line in lines:
            if line.strip() == "CONFIG_LIB_UNIBPF_JIT_COMPILE=y":
                updated_configs.append("# CONFIG_LIB_UNIBPF_JIT_COMPILE is not set\n")
            else:
                updated_configs.append(line)

    
    with open("../../.config", mode="w", encoding="utf8") as configFile:
        for line in updated_configs:
            configFile.write(line)

def start_process(command):
    
    process = subprocess.Popen(command, 
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE, 
                               cwd=APP_ROOT, shell=True)
    
    os.set_blocking(process.stdout.fileno(), False)
    
    return process

def communicate_with_debug_interface(command: str):
    result = b""
    
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as debug_interface:
        debug_interface.connect("/tmp/port0")
        debug_interface.send((command + "\n").encode())
        
        output = debug_interface.recv(1)
        result += output
        while output != b">":
            output = debug_interface.recv(1)
            result += output

    return result.decode("utf8")

def communicate_with_benchmarker(start_benchmark_command: str):
    process = start_process(command=start_benchmark_command)
    process.wait()

    return process.stdout.read().decode("utf8")

def do_test(runtime_type: str, test_filename: str, extentsion_name: str, instruction: str):
    print(f"{test_filename}-{runtime_type}-{instruction}")
    with open(f"{test_filename}-{runtime_type}-{instruction}-report.txt", mode="w", encoding="utf8") as report_file:
        for count in range(ROUNDS):
            try:
                app = start_process("just test-run") # just test-run
                print(">>>>> INFO started app")
                
                print(">>>>> DEBUG Sleeping for 3 seconds to make sure the app is ready for test...")
                sleep(3)

                retval = subprocess.call("sudo chmod 777 /tmp/port0", cwd=APP_ROOT, shell=True)
                if retval != 0:
                    print(">>>>> WARN chmod /tmp/port0 failed with", retval)
                print(">>>>> INFO fixed debug interface permission")
                    
                
                if test_filename == "no_hook":
                    print(f">>>>> INFO Measuring <<no hook>> {count+1}/{ROUNDS}")

                    # 
                    report_file.write(communicate_with_benchmarker(f"taskset -c 4-7 redis-benchmark -h 172.44.0.2 -p 6379 -c 30 -n 2000000 -t {instruction} -q"))
                    report_file.write("\n")
                else:
                    print(f">>>>> INFO Measuring {test_filename}.{extentsion_name} {count+1}/{ROUNDS}")
                    print(communicate_with_debug_interface(f"bpf_attach processCommand /ushell/bpf/{test_filename}.{extentsion_name}"))

                    # taskset -c 4-7
                    report_file.write(communicate_with_benchmarker(f"taskset -c 4-7 redis-benchmark -h 172.44.0.2 -p 6379 -c 30 -n 2000000 -t {instruction} -q"))
                    report_file.write("\n")

                    print("TEST", communicate_with_debug_interface(f"bpf_exec /ushell/bpf/get_count.o get_count processCommand"))

            finally:
                app.stdin.write(b"\x01\x78")
                sleep(0.5)

                app.kill()
                print(">>>>> INFO Waiting app exit...")
                app.wait()
                print(">>>>> INFO application exited")
                sleep(1)


def start_test(runtime_type: str):
    compile_application()

    do_test(runtime_type, "no_hook", None, "SET")
    do_test(runtime_type, "no_hook", None, "GET")
    do_test(runtime_type, "no_hook", None, "INCR")
    
    for file in ["count.o", "dummy_count.o"]:
        filename = file.split(".")
        do_test(runtime_type, filename[0], filename[1], "SET")
        do_test(runtime_type, filename[0], filename[1], "GET")
        do_test(runtime_type, filename[0], filename[1], "INCR")

    
if __name__ == "__main__":
    compile_bpf_program()

    print(">>>>> INFO Starting JiT tests")
    enable_jit()
    start_test("jit")
    print(">>>>> INFO JiT tests completed")

    print(">>>>> INFO Starting Interpretation tests")
    disable_jit()
    start_test("interpreter")
    print(">>>>> INFO Interpretation tests completed")