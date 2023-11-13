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

def do_test(test_name: str, test_filename: str, extentsion_name: str):
    with open(f"{test_filename}-{test_name}-report.txt", mode="w", encoding="utf8") as report_file:
        for count in range(ROUNDS):
            try:
                app = start_process("just test-run")
                print(">>>>> INFO started app")
                
                print(">>>>> DEBUG Sleeping for 3 seconds to make sure the app is ready for test...")
                sleep(3)

                retval = subprocess.call("sudo chmod 777 /tmp/port0", cwd=APP_ROOT, shell=True)
                if retval != 0:
                    print(">>>>> WARN chmod /tmp/port0 failed with", retval)
                print(">>>>> INFO fixed debug interface permission")
                    
                
                if test_filename == "no_hook":
                    print(f">>>>> INFO Measuring <<no hook>> {count+1}/{ROUNDS}")

                    report_file.write(communicate_with_benchmarker("taskset -c 4-7 wrk -t 3 -d30s -c 30 http://172.44.0.2/index.html"))
                    report_file.write("\n")
                else:
                    print(f">>>>> INFO Measuring {test_filename}.{extentsion_name} {count+1}/{ROUNDS}")
                    communicate_with_debug_interface(f"bpf_attach ngx_http_process_request_line /ushell/bpf/{test_filename}.{extentsion_name}")

                    report_file.write(communicate_with_benchmarker("taskset -c 4-7 wrk -t 3 -d30s -c 30 http://172.44.0.2/index.html"))
                    report_file.write("\n")

            finally:
                app.stdin.write(b"\x01\x78")
                sleep(0.5)

                app.kill()
                print(">>>>> INFO Waiting app exit...")
                app.wait()
                print(">>>>> INFO application exited")
                sleep(1)


def start_test(test_name: str):
    compile_application()

    do_test(test_name, "no_hook", None)
    
    for file in ["nop.o", "count.o", "dummy_count.o"]:
        filename = file.split(".")
        print(">>>>> INFO Starting", filename)
        do_test(test_name, filename[0], filename[1])

    
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