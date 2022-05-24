import getopt
import socket
import struct
import sys
import time
import subprocess
import os
import uuid
import shutil


def send_msg(sock, msg):
    if type(msg) == bytes:
        packed_msg = struct.pack('>I', len(msg)) + msg
    else:
        packed_msg = struct.pack('>I', len(msg)) + msg.encode()
    sock.send(packed_msg)

def recv_msg(sock):
    packed_msg_len = force_recv_all(sock, 4)
    if not packed_msg_len:
        return None
    msg_len = struct.unpack('>I', packed_msg_len)[0]
    recved_data = force_recv_all(sock, msg_len)
    return recved_data

def force_recv_all(sock, msg_len):
    all_data = bytearray()
    while len(all_data) < msg_len:
        packet = sock.recv(msg_len - len(all_data))
        if not packet:
            return None
        all_data.extend(packet)
    return all_data

def run_command(cmd_str, execution_path):
    if cmd_str.startswith('remote-'):
        cmd_str = cmd_str[7:]

    if verbose: print("\tExecuting", cmd_str)
    proc = subprocess.Popen(cmd_str, cwd=execution_path, shell = True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    global n_active_procs
    n_active_procs+=1
    return proc

def receive_command(received_data, connection):
    index = received_data.split()[0]
    cmd_indexes.append(index)
    n_required_files = int(received_data.split()[1])

    cmd_str = " ".join(received_data.split()[2:])
    cmd_strs.append(cmd_str)

    connections.append(connection)
    addresses.append(addr)
    returned.append(False)
    
    execution_path = f"/tmp/rs-{uuid.uuid4()}"
    paths.append(execution_path)
    try:
        os.mkdir(execution_path)
    except FileExistsError:
        print(f"Directory name collision: {execution_path} already exists.  This may produce unintended results.", file=sys.stderr)
    
    filenames = []
    for i in range(n_required_files):
        filename = recv_msg(connection).decode()
        if verbose: print("\t<-- filename:", filename)
        if filename == "":
            print("Error: received empty filename")
            exit(1)
        filenames.append(filename)
        file = recv_msg(connection)
        if verbose: print(f"\t<-- file (size {len(file)})")
        filepath = os.path.join(execution_path, filename)
        #do we need to check for error here------------------------------------------------ 
        with open(filepath, "wb") as f:
            f.write(file)
            if verbose: print(f"\tSaved file {filepath}")
    
    if len(filenames) > 0:
        last_mod_time = os.path.getmtime(os.path.join(execution_path, filenames[-1]))
    else:
        last_mod_time = 0
    last_mod_times.append(last_mod_time)
    if verbose: print(f"\tSaved {len(filenames)} files to dir: {execution_path}")

    proc = run_command(cmd_str, execution_path)
    processes.append(proc)

verbose = False

optlist, args = getopt.getopt(sys.argv[1:], "v")
for opt in optlist:
    if opt[0] == "-v":
        verbose = True

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
if verbose: print("Opened server")
if verbose: print("Created socket")

addr = "0.0.0.0"
port = 40000

if len(args) > 0:
    port = int(args[0])

s.bind((addr, port))
if verbose: print(f"Socket binded to {port}")

s.listen(5)
if verbose: print("Socket is listening")

nconnections = 0
n_active_procs = 0

processes = []
paths = []
last_mod_times = []

returned = []
connections = []
addresses = []
cmd_strs = []
cmd_indexes = []

# To ensure that a process isn't skipped
s.settimeout(0.1)

try:
    while True:
        try:
            connection, addr = s.accept()
        except socket.timeout:
            connection = None


        if connection is not None:
            if verbose: print(f"Got connection {nconnections} from {addr}")
            nconnections += 1
           
            received_data = recv_msg(connection).decode()
            if verbose: print("\t<--",received_data)
            if received_data == "cost-query":
                msg = "cost " + str(n_active_procs)
                if verbose: print("\t-->",msg)
                send_msg(connection, msg)
                
            elif received_data:
                receive_command(received_data, connection)

        
        for i, proc in enumerate(processes):
            if proc.poll() is not None and returned[i] == False:
                if verbose: print(f"Command {i} complete.  Sending to {connections[i].getpeername()}")
                n_active_procs-=1
                
                output_file = None
                max_time = 0
                max_time_file = None
                for dirname,subdirs,files in os.walk(paths[i]):
                    for file in files:
                        path = os.path.join(dirname, file)
                        mod_time = os.stat(path).st_mtime
                        if mod_time > max_time:
                            max_time = mod_time
                            max_time_file = path

                if max_time > last_mod_times[i]:
                    output_file = max_time_file
                    msg1 = f"{cmd_indexes[i]} {proc.returncode} 1"
                else:
                    msg1 = f"{cmd_indexes[i]} {proc.returncode} 0"

                msg2 = str(proc.stdout.read().decode("utf-8")) 
                msg3 = str(proc.stderr.read().decode("utf-8"))
                
                send_msg(connections[i], msg1)
                send_msg(connections[i], msg2)
                send_msg(connections[i], msg3)

                if verbose: 
                    print("\t--> cmdindex exitcode nfiles", msg1)
                    print(f"\t--> stdout (length: {len(msg2)})")
                    print(f"\t--> stderr (length: {len(msg3)})")

                if output_file is not None:
                    send_msg(connections[i], os.path.basename(output_file))
                    try:
                        with open(output_file, "rb") as f:
                            msg = f.read()
                            send_msg(connections[i], msg) 
                    except OSError:
                        print(f"Error: could not open file {output_file}", file=sys.stderr)

                    if verbose: 
                        print("\t-->",os.path.basename(output_file))
                        print(f"\t--> file (size {len(msg)})")
                connections[i].close()
                returned[i] = True
                
                if verbose: print(f'\tremoved: {paths[i]}')
                shutil.rmtree(paths[i])

except KeyboardInterrupt:
    s.close()
    print("\tClosed server")
