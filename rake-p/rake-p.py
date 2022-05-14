import sys
import os
import subprocess

hosts = []
action_sets = {}


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

def run_remote_commands(command):
    pass

def send_remote_commands(command):
    '''
    Function checks to see if command leads with remote, if so
    it takes a slice ignoring the word remote and stores it 
    ready to send off.
    '''
    remote_command = ''
    if command[0][0].startswith('remote'):
        remote_command = command[7:]
    else:
        print("comman is not a remote command")
    return remote_command

def run_local_command(command):
    ''' 
    Runs the command and if successful returns that output and a 0 
    to represent sucessful execution, if there's an error we catch 
    the error and communicate what the error was. At the moment we 
    also print for debug purposes.
    '''
    result = subprocess.Popen(command, shell = True,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    output,err=result.communicate()
    if output:
        print(os.system(command))
        return output
    else:
        print(err)
        return err
    
   

read_file()
print(hosts)
print("\n".join("{}\n\t{}".format(k, v) for k, v in action_sets.items()))

for actionset in action_sets.items():
    for command_data in actionset:
        print(command_data[0][0])
        if command_data[0][0].startswith("a"): 
            continue
        print("-------" + command_data[0][0] + "-------------")
        run_local_command(str(command_data[0][0]))
