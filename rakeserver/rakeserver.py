import socket
import sys
import time
import subprocess

def run_command(command_data):
    if len(command_data) == 2:
        for file in command_data[1]:
            does_file_exist = check_if_file_exists(file)
            if not does_file_exist:
                missing_file(command_data, file)

    proc = subprocess.Popen(command_data[0][7:], shell = True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
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

try:
    while True:
        connection, addr = s.accept()
        print("Got connection from", addr)

        #connection.send(f"Succesful connection ({i}) to {addr}:{port}".encode())
        i+=1

        received_data = connection.recv(1024).decode()
        if received_data:
            proc = run_command([received_data,])
            processes.append(proc)
            returned.append(False)

        print(f"\t{received_data=}")

        '''for i, proc in enumerate(processes):
            if proc.poll() is not None and returned[i] == False:
                connection.send(str(proc.returncode).encode())
                print(f"Returning {str(proc.returncode)} to {addr})
                returned[i] = True'''

        connection.send(received_data.encode())
        print("\tEchoing received_data back to client")
        connection.close()
except KeyboardInterrupt:
    s.close()
    print("Closed server")
