import socket
import struct
import sys
import time
import subprocess

def send_msg(sock, msg):
    print(f"Sending to {sock.getpeername()}: {msg}")
    if type(msg) == bytes:
        packed_msg = struct.pack('>I', len(msg)) + msg
    else:
        packed_msg = struct.pack('>I', len(msg)) + msg.encode()
    sock.send(packed_msg)

def recv_msg(sock):
    print(f"Starting msg receive")
    packed_msg_len = sock.recv(4)
    if not packed_msg_len:
        return None
    msg_len = struct.unpack('>I', packed_msg_len)[0]
    return sock.recv(msg_len)

def run_command(command_data):
    if command_data[0].startswith('remote-'):
        command_string = command_data[0][7:]
    else:
        command_string = command_data[0]

    proc = subprocess.Popen(command_string, shell = True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    global n_active_procs
    n_active_procs+=1
    return proc

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
print("Created socket")

addr = "localhost"
port = 40000

if len(sys.argv) > 1:
    port = int(sys.argv[1])

s.bind((addr, port))
print(f"Socket binded to {port}")

s.listen(5)
print("Socket is listening")
i = 0

n_active_procs = 0

processes = []
returned = []
connections = []
addresses = []
command_strs = []
command_indexes = []

#to ensure that a process isn't skipped
s.settimeout(0.01)

try:
    while True:
        try:
            connection, addr = s.accept()
        except socket.timeout:
            connection = None


        if connection is not None:
            print(f"Got connection {i} from {addr}")
            i+=1
           
            received_data = recv_msg(connection).decode()
            print(f"\t{received_data=}")
            
            if received_data == "cost-query":
                msg = "cost " + str(n_active_procs)
                print(f"Received cost-query from: {addr}.  Replying: {msg}")
                send_msg(connection, msg)
                
            
            elif received_data:
                index = received_data.split()[0]
                command_indexes.append(index)
                n_required_files = int(received_data.split()[1])
                print(f'{n_required_files=}')
                command_str = " ".join(received_data.split()[2:])
                command_strs.append(command_str)

                connections.append(connection)
                addresses.append(addr)
                returned.append(False)
                
                print(f"\tRunning command {index}: {command_str}")
                filenames = []
                for i in range(n_required_files):
                    filename = recv_msg(connection)
                    filenames.append(filename.decode())
                    file = recv_msg(connection)
                    with open(filename, "wb") as f:
                        f.write(file)
                    print("Received file", file)
                print(f'{filenames=}')


                proc = run_command([command_str,])
                processes.append(proc)
                

        
        for i, proc in enumerate(processes):
            if proc.poll() is not None and returned[i] == False:
                n_active_procs-=1

                msg1 = f"{command_indexes[i]} {proc.returncode}"
                msg2 = str(proc.stdout.read().decode("utf-8")) 
                msg3 = str(proc.stderr.read().decode("utf-8"))
                
            
                send_msg(connections[i], msg1)
                send_msg(connections[i], msg2)
                send_msg(connections[i], msg3)

        
                connections[i].close()
                returned[i] = True

except KeyboardInterrupt:
    s.close()
    print("\tClosed server")
