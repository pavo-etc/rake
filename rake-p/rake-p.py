from email.policy import default
import struct
import sys
import os
import subprocess
import socket
import select
import random
import time

hosts = []
action_sets = {}

'''
Format of action_sets:
"actionset1" : [
    ['echo starting actionset1'],
    ['remote-cc [optional-flags] -c program.c', ['program.c', 'program.h', 'allfunctions.h', 'balls.h']],
    ['remote-cc [optional-flags] -c square.c', ['square.c', 'allfunctions.h']],
    ['remote-cc [optional-flags] -c cube.c', ['cube.c', 'allfunctions.h']]
],
"actionset2" : [
    ['echo starting actionset2'], 
    ['remote-cc [optional-flags] -o program program.o square.o cube.o', ['program.o', 'square.o', 'cube.o']]
]
'''


def read_file():
    filename = "./Rakefile"
    if len(sys.argv) > 1:
        filename = sys.argv[1]

    print(f"Reading from {filename}") 
    with open(filename, "r") as f:
        rakefile_lines = f.readlines()
   
    in_actionset = False
    current_actionset = None
    global default_port 

    for line in rakefile_lines:
        line = line.rstrip()
        line = line.partition('#')[0] # Remove comments
        if line.strip() == "":
            continue # Remove empty lines
        elif line.startswith("PORT"):
            default_port = line.split()[2]
        elif line.startswith("HOSTS"):
            for host in line.split()[2:]:
                if ":" not in host:
                    host += ":" + default_port
                hosts.append(host)
        elif line.startswith("\t\trequires"):
            required_files = line.strip().split()[1:]
            action_sets[current_actionset][-1].append(required_files)
        elif line.startswith("\t"):
            action_sets[current_actionset].append([line.strip()])
        else:
            action_sets[line[:-1]] = [] # stores commands and required files
            current_actionset = line[:-1]

def check_if_file_exists(filename):
    return os.path.exists(filename)

def missing_file(command_data, filename):
    print(f"Error: missing file\n\tcommand: {command_data[0]}\n\tfilename: {filename}", file=sys.stderr)
    exit(1)

def command_fail(actionset, command_data, exitcode, stderr):
    print(f"Error: command failed\n\tactionset: {actionset}\n\tcommand: {command_data[0]}\n\texitcode: {exitcode}\n\tstderr: {stderr}")
    exit(1)

def send_msg(sock, msg):
    print(f"Sending to {sock.getpeername()}: {msg}")
    msg = struct.pack('>I', len(msg)) + msg.encode()
    sock.send(msg)

def recv_msg(sock):
    packed_msg_len= sock.recv(4)
    if not packed_msg_len:
        return None
    msg_len = struct.unpack('>I', packed_msg_len)[0]
    return sock.recv(msg_len)

def find_host():
    mincost = 1000
    mincost_index = None
    for i, host in enumerate(hosts):
        s = socket.socket()
        addr = host.split(":")[0]
        port = int(host.split(":")[1])
        print(f"\t\tQuerying cost from host {i} {host}")
        s.connect((addr,port))
        send_msg(s, 'cost-query')
        data = recv_msg(s)
        decoded_data = data.decode("utf-8")
        if decoded_data.startswith("cost "):
            cost = int(decoded_data.split()[1])
            print(f"\t\t\tReceived: {decoded_data}")
            if cost < mincost:
                mincost_index = i
                mincost = cost
    
    print(f"\t\tSelecting host {mincost_index}")
    return mincost_index

def send_command(command_data, i, n_required_files, is_local=False):
    '''
    Function that takes a remote command.  Choses a server to send it to,
    sends required files (if necessary), gets command feedback.
    '''
    if is_local:
        addr = "localhost"
        port = int(default_port)
    else:
        host = hosts[find_host()] 
        addr = host.split(":")[0]
        port = int(host.split(":")[1])

    s = socket.socket()
    print(f"\tAttempting conection to {addr}:{port}")
    s.connect((addr,port))
    msg = f'{i} {n_required_files} {command_data[0]}'
    print(f"\tSending: {msg}")
    send_msg(s, msg)

    for j in range(n_required_files):
        print(f'Sending filename: { command_data[1][j]=}')
        send_msg(s, command_data[1][j])


    return s


read_file()
print(f"Hosts: {hosts}")

for actionsetname, commandlist in action_sets.items():
    print(f"------------Starting {actionsetname}------------")
    sockets = []
    stdouts = [None] * len(commandlist)
    stderrs = [None] * len(commandlist)
    exitcodes = [None] * len(commandlist)
    for i, command_data in enumerate(commandlist):
        print(command_data[0])
        if len(command_data) > 1:
            n_required_files = len(command_data[1])
        else:
            n_required_files = 0
            
        if command_data[0].startswith("remote-"):
            sock = send_command(command_data, i, n_required_files)
        else:
            sock = send_command(command_data, i, n_required_files, is_local=True)

        sockets.append(sock)
    
    while True:
        rsocks, wsocks, esocks = select.select(sockets,[],[])
    
        for sock in rsocks:
            data1 = recv_msg(sock)
            if data1 is None:
                continue
            print(f"{data1=}")
            data2 = recv_msg(sock)
            print(f"{data2=}")

            data3 = recv_msg(sock)
            print(f"{data3=}")
            
            command_index = int(data1.split()[0])
            exitcodes[command_index] = int(data1.split()[1])
            stdouts[command_index] = data2.decode()
            stderrs[command_index] = data3.decode()

        if len(rsocks) == len(sockets):
            break

    print(f"{stdouts=}")
    print(f"{stderrs=}")
    print(f"{exitcodes=}")
    print("\n\n")

    for i,exitcode in enumerate(exitcodes):
        if exitcode != 0:
            command_fail(actionsetname, commandlist[i], exitcode, stderrs[i])

print("Completed Rake successfully!")

                
