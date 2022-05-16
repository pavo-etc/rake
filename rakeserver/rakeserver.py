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

processes = []
returned = []
connections = []
addresses = []
txt_command = []

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
            if received_data:
                connections.append(connection)
                txt_command.append(received_data)
                addresses.append(addr)
                proc = run_command([received_data,])
                processes.append(proc)
                returned.append(False)

            print(f"\t{received_data=}")
        
        for i, proc in enumerate(processes):
            if proc.poll() is not None and returned[i] == False:
                msg =(txt_command[i] + '\t' + str(proc.returncode))
                connections[i].send(msg.encode())
                print(f"Sending to: {addresses[i]}\n\t{msg}")
                connections[i].close()
                returned[i] = True


        '''
        for i, proc in enumerate(processes):
            if proc.poll() is not None and returned[i] == False:
                connection.send((received_data + '\t' + str(proc.returncode)).encode())
                print(f"Returning {str(proc.returncode)} to {addr}")
                returned[i] = True
        '''

        #connection.send(received_data.encode())
        #print("\tEchoing received_data back to client")
        #connection.close()
except KeyboardInterrupt:
    s.close()
    print("\tClosed server")
