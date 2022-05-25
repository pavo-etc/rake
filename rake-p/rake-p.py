import struct
import sys
import os
import subprocess
import socket
import select
import random
import time
import getopt

verbose = False
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

def usage():
    print(f'''\
Usage: {sys.argv[0]} [-v] [file]
    -v: Verbose output
    file: file to execute instead of "./Rakefile"''',
        file=sys.stderr)
    exit(1)

# Used for when a missing file is detected
def missing_file(command_data, filename):
    print(f"Error: missing file\n\tcommand: {command_data[0]}\n\tfilename: {filename}", file=sys.stderr)
    exit(1)

# Used for when a command fails
def command_fail(actionset, command_data, exitcode, stderr):
    print(f"Error: command failed\n\tactionset: {actionset}\n\tcommand: {command_data[0]}\n\texitcode: {exitcode}\n\tstderr: {stderr}", file=sys.stderr)
    exit(1)

# Reading and parsing the Rakefile
def read_file(filename):

    if verbose: print(f"Reading from {filename}") 

    try:
        with open(filename, "r") as f:
            rakefile_lines = f.readlines()
    except OSError:
        print(f"Error: could not find {filename}", file=sys.stderr)
        exit(1)
   
    in_actionset = False
    current_actionset = None
    global default_port 

    for line in rakefile_lines:
        line = line.rstrip()
        line = line.partition('#')[0]           # Remove comments
        if line.strip() == "":
            continue                            # Remove empty lines
        elif line.startswith("PORT"):           # Extract port information
            default_port = line.split()[2]
        elif line.startswith("HOSTS"):          # Extract host information
            for host in line.split()[2:]:
                if ":" not in host:
                    host += ":" + default_port
                hosts.append(host)
        elif line.startswith("\t\trequires"):   # Extract required files for a command if needed
            required_files = line.strip().split()[1:]
            action_sets[current_actionset][-1].append(required_files)
        elif line.startswith("\t"):             # Extract command
            action_sets[current_actionset].append([line.strip()])
        else:
            action_sets[line[:-1]] = [] # stores commands and required files
            current_actionset = line[:-1]

# Checks if the file exists
def check_if_file_exists(filename):
    return os.path.exists(filename)

# Sends big endian messages to a given socket
def send_msg(sock, msg):
    if type(msg) == bytes:
        packed_msg = struct.pack('>I', len(msg)) + msg
    else:
        packed_msg = struct.pack('>I', len(msg)) + msg.encode()
    sock.send(packed_msg)

# Receives big endian messages and unpacks them from a socket
def recv_msg(sock):
    packed_msg_len = sock.recv(4)
    if not packed_msg_len:
        return None
    msg_len = struct.unpack('>I', packed_msg_len)[0]
    recved_data = force_recv_all(sock, msg_len)
    return recved_data

# Ensures that all the bytes we want to read on receival are read
def force_recv_all(sock, msg_len):
    all_data = bytearray()
    while len(all_data) < msg_len:
        packet = sock.recv(msg_len - len(all_data))
        if not packet:
            return None
        all_data.extend(packet)
    return all_data

# Finds the host with the lowest cost
def find_host():
    mincost = 1000
    mincost_index = None
    for i, host in enumerate(hosts):
        s = socket.socket()
        addr = host.split(":")[0]
        port = int(host.split(":")[1])
        
        
        msg = "cost-query"
        s.connect((addr,port))
        send_msg(s, msg)
        
        if verbose: 
            print(f"\tQuerying cost from host {i} {host}")
            print(f"\t\t--> {msg}")
        
        data = recv_msg(s)
        decoded_data = data.decode("utf-8")
        if decoded_data.startswith("cost "):
            cost = int(decoded_data.split()[1])
            
            if verbose: print(f"\t\t<-- {decoded_data}")
            
            if cost < mincost:
                mincost_index = i
                mincost = cost
        s.close()
    if verbose: print(f"\t\tSelecting host {mincost_index}")
    
    return mincost_index

# Sends remote commands to a chosen server and receives results of command
def send_command(command_data, i, n_required_files):
    if command_data[0].startswith("remote-"):
        host = hosts[find_host()] 
        addr = host.split(":")[0]
        port = int(host.split(":")[1])
    else:
        addr = "localhost"
        port = int(default_port)

    s = socket.socket()
    if verbose: print(f"\tAttempting conection to {addr}:{port}")
    s.connect((addr,port))

    if command_data[0].startswith("remote-"):
        msg = f'{i} {n_required_files} {command_data[0][7:]}'
    else:
        msg = f'{i} {n_required_files} {command_data[0]}'
        
    if verbose: print(f"\t\t--> {msg}")
    send_msg(s, msg)

    for j in range(n_required_files):
        if verbose: print(f'\t\t--> filename: {command_data[1][j]}')
        
        send_msg(s, command_data[1][j])
        
        with open(command_data[1][j], "rb") as f:
            msg = f.read()
            send_msg(s, msg)
            if verbose: print(f'\t\t--> file (size {len(msg)})')
        
    return s

# Executes all the actionsets (if no failures) from the read Rakefile
def execute_actionsets():
    for actionsetname, commandlist in action_sets.items():
        if verbose: print(f"------------Starting {actionsetname}------------")
        sockets = []
        stdouts = [None] * len(commandlist)
        stderrs = [None] * len(commandlist)
        exitcodes = [None] * len(commandlist)
        for i, command_data in enumerate(commandlist):
            if verbose: print(command_data[0])
            if len(command_data) > 1:
                n_required_files = len(command_data[1])
            else:
                n_required_files = 0
                
            sock = send_command(command_data, i, n_required_files)
            sockets.append(sock)
        
        while True:
            active_socks = [sock for sock in sockets if sock.fileno() >= 0]
            rsocks, wsocks, esocks = select.select(active_socks,[],[])
        
            for sock in rsocks:
                # Receives cmd index, stdout, stderr and exitcode
                cmdindex_exitcode_nfile = recv_msg(sock)
                if cmdindex_exitcode_nfile is None:
                    continue
                
                cmd_index = cmdindex_exitcode_nfile.split()[0]
                exitcode = cmdindex_exitcode_nfile.split()[1]
                n_return_files = cmdindex_exitcode_nfile.split()[2]
                
                stdout = recv_msg(sock)
                stderr = recv_msg(sock)
                
                if verbose: 
                    print(f"\t<-- cmdindex exitcode nfile {cmdindex_exitcode_nfile.decode()}")
                    print(f"\t<-- stdout (length {len(stdout)})")
                    print(f"\t<-- stderr (length {len(stderr)})")

                if int(n_return_files):
                    filename = recv_msg(sock).decode()
                    file = recv_msg(sock)
                    if verbose: print("\t<-- filename:", filename)
                    try:
                        with open(filename, "wb") as f:
                            f.write(file)
                            if verbose: 
                                print(f"\t<-- file (size {len(file)})")
                                print(f"\tSaved file {filename=}")
                    except OSError:
                        print(f"Error: writing {filename} failed", file=sys.stderr)
                        exit(1)

                sock.close()

                command_index = int(cmd_index)
                exitcodes[command_index] = int(exitcode)
                stdouts[command_index] = stdout.decode()
                stderrs[command_index] = stderr.decode()

            if len(rsocks) == len(active_socks):
                break
            
        if verbose:
            # Format to display our results received
            print(f"\n{actionsetname} execution results:")
            for i in range(len(stdouts)):
                print(f'--------------------\ncommand: {i}\nOutput:\n{stdouts[i]}Error:\n{stderrs[i]}Exit Code:{exitcodes[i]}\n--------------------')
            print("\n\n")

        for i,exitcode in enumerate(exitcodes):
            if exitcode != 0:
                command_fail(actionsetname, commandlist[i], exitcode, stderrs[i])

    if verbose: print("Executed Rakefile successfully!")

if __name__ == "__main__":
    try:
        optlist, args = getopt.getopt(sys.argv[1:], "v")
    except getopt.GetoptError as err:
        usage()

    # Checks for usage of the "-v" flag
    for opt, arg in optlist:
        if opt == "-v":
            verbose = True

    # Set the file to be read if one is provided else use a default option
    if len(args) > 0:
        filename = args[0]
    else:
        filename = "Rakefile"
    
    # Parse the Rakefile
    read_file(filename)
    
    if verbose: print("Hosts: ", hosts)

    # Execute the actionsets
    execute_actionsets()

    