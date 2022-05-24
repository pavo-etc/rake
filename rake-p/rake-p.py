from email.policy import default
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

def missing_file(command_data, filename):
    print(f"Error: missing file\n\tcommand: {command_data[0]}\n\tfilename: {filename}", file=sys.stderr)
    exit(1)

def command_fail(actionset, command_data, exitcode, stderr):
    print(f"Error: command failed\n\tactionset: {actionset}\n\tcommand: {command_data[0]}\n\texitcode: {exitcode}\n\tstderr: {stderr}", file=sys.stderr)
    exit(1)

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

def send_msg(sock, msg):
    if type(msg) == bytes:
        packed_msg = struct.pack('>I', len(msg)) + msg
    else:
        packed_msg = struct.pack('>I', len(msg)) + msg.encode()
    sock.send(packed_msg)

def recv_msg(sock):
    packed_msg_len = sock.recv(4)
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
    
    if verbose: print(f"\t\tSelecting host {mincost_index}")
    
    return mincost_index

def send_command(command_data, i, n_required_files):
    '''
    Function that takes a remote command.  Choses a server to send it to,
    sends required files (if necessary), gets command feedback.
    '''
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
            rsocks, wsocks, esocks = select.select(sockets,[],[])
        
            for sock in rsocks:
                # Receives cmd index and exitcode
                
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
                


                command_index = int(cmd_index)
                exitcodes[command_index] = int(exitcode)
                stdouts[command_index] = stdout.decode()
                stderrs[command_index] = stderr.decode()

            if len(rsocks) == len(sockets):
                break
            
        if verbose:
            print(f"\n{actionsetname} execution results:")
            print(f"{stdouts=}")
            print(f"{stderrs=}")
            print(f"{exitcodes=}")
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
    for opt, arg in optlist:
        if opt == "-v":
            verbose = True

    if len(args) > 0:
        filename = args[0]
    else:
        filename = "Rakefile"
        
    read_file(filename)
    
    if verbose: print("Hosts: ", hosts)

    execute_actionsets()

    