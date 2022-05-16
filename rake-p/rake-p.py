from email.policy import default
import sys
import os
import subprocess
import socket
import select
import random
import time

hosts = []
host_index = 0
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

def command_fail(actionset, command_data, exitcode):
    print(f"Error: command failed\n\tactionset: {actionset}\n\tcommand: {command_data[0]}\n\texitcode: {exitcode}")
    exit(1)

def send_command(command_data, is_local=False):
    '''
    Function that takes a remote command.  Choses a server to send it to,
    sends required files (if necessary), gets command feedback.
    '''
    s = socket.socket()
  
    host = hosts[host_index] # TODO replace with function for finding host to use
    if is_local:
        addr = "localhost"
        #print(default_port)
        port = int(default_port)
    else:
        addr = host.split(":")[0]
        port = int(host.split(":")[1])

    print(f"Attempting conection to {addr}:{port}")
    s.connect((addr,port))
    s.send(command_data[0].encode())

    return s


read_file()
print(f"Hosts: {hosts}")

for actionsetname, commandlist in action_sets.items():
    print(f"------------Starting {actionsetname}------------")
    sockets = []
    stdouts = []
    stderrs = []
    exitcodes = []
    for command_data in commandlist:
        print(command_data[0])
        if command_data[0].startswith("remote-"):
            sock = send_command(command_data)
            host_index = (host_index + 1) % len(hosts)
        else:
            sock = send_command(command_data, is_local=True)

        sockets.append(sock)
    
    
    while True:
        rsocks, wsocks, esocks = select.select(sockets,[],[])
    
        for sock in rsocks:
            data, addr = sock.recvfrom(1024)
            if data != b"":
                print(f"Recieved: {data.decode('utf-8')}")
        if len(rsocks) == len(sockets):
            break

    print(stdouts)
    print(stderrs)
    print(exitcodes)
    for i,exitcode in enumerate(exitcodes):
        if exitcode != 0:
            command_fail(actionsetname, commandlist[i], exitcode)


                
