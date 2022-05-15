import sys
import os
import subprocess
import socket
import select
import random

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
   
    default_port = []
    in_actionset = False
    current_actionset = None
    
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

def send_remote_command(command_data):
    '''
    Function that takes a remote command.  Choses a server to send it to,
    sends required files (if necessary), gets command feedback.
    '''
    s = socket.socket()
    #s.setblocking(0)
    host = hosts[host_index] # TODO replace with function for finding host to use
    #host_index = (host_index + 1) % len(hosts)
    addr = host.split(":")[0]
    port = int(host.split(":")[1])

    print(f"Attempting conection to {addr}:{port}")
    s.connect((addr,port))
    s.send(command_data[0].encode())

    return s

# This potentially will be replaced by just executing these on localhost
def run_local_command(command_data):
    ''' 
    Runs the command and if successful returns that output and a 0 
    to represent sucessful execution, if there's an error we catch 
    the error and communicate what the error was. At the moment we 
    also print for debug purposes.
    '''
    # if it has required files
    if len(command_data) == 2:
        for file in command_data[1]:
            does_file_exist = check_if_file_exists(file)
            if not does_file_exist:
                missing_file(command_data, file)

    proc = subprocess.Popen(command_data[0], shell = True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return proc


    
   

read_file()
print(f"Hosts: {hosts}")

for actionsetname, commandlist in action_sets.items():
    print(f"------------Starting {actionsetname}------------")
    processes = []
    sockets = []
    stdouts = []
    stderrs = []
    exitcodes = []
    for command_data in commandlist:
        print(command_data[0])
        if command_data[0].startswith("remote-"):
            sock = send_remote_command(command_data)
            sockets.append(sock)
            host_index = (host_index + 1) % len(hosts)
        else:
            proc = run_local_command(command_data)
            processes.append(proc)

    for proc in processes:
        proc.wait()
        stdouts.append(str(proc.stdout.read().decode("utf-8")))
        stderrs.append(str(proc.stderr.read().decode("utf-8")))
        exitcodes.append(proc.returncode)
    
    while True:
        rsocks, wsocks, esocks = select.select(sockets,[],[])
        for sock in rsocks:
            data, addr = sock.recvfrom(1024)
            if data != b"":
                print(f"From: {addr}\nRecieved: {data}")
        if len(rsocks) == len(sockets):
            break

    print(stdouts)
    print(stderrs)
    print(exitcodes)
    for i,exitcode in enumerate(exitcodes):
        if exitcode != 0:
            command_fail(actionsetname, commandlist[i], exitcode)


                
