import socket
import sys
import time
import subprocess

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

            received_data = connection.recv(1024).decode()
            print(f"\t{received_data=}")
            
            if received_data == "cost-query":
                msg = "cost " + str(n_active_procs)
                print(f"Received cost-query from: {addr}.  Replying: {msg}")
                connection.send(msg.encode())
            
            elif received_data:
                index = received_data[:received_data.find(" ")]
                command_indexes.append(index)
                command_str = received_data[received_data.find(" ")+1:]
                command_strs.append(command_str)

                connections.append(connection)
                addresses.append(addr)
                returned.append(False)
                
                print(f"\tRunning command {index}: {command_str}")

                proc = run_command([command_str,])
                processes.append(proc)
                

        
        for i, proc in enumerate(processes):
            if proc.poll() is not None and returned[i] == False:
                n_active_procs-=1
                msg = f"{command_indexes[i]} {proc.returncode}"
                connections[i].send(msg.encode())
                print(f"Sending to: {addresses[i]}\n\t{msg}")
                connections[i].close()
                returned[i] = True

except KeyboardInterrupt:
    s.close()
    print("\tClosed server")
