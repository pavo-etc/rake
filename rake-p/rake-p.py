import sys

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




read_file()
print(hosts)
print("\n".join("{}\n\t{}".format(k, v) for k, v in action_sets.items()))