import sys

def main():
    rakefile_lines = []

    if len(sys.argv) > 1:
        with open(sys.argv[1], "r") as file:
            next(file)
            for line in file:
                if not line.isspace():
                    rakefile_lines.append(line)
    else:
        print("No file specified, will execute default Rakefile\n") #for testing/know what's happening
        with open("../Rakefile", "r") as file:
            next(file)
            for line in file:
                if not line.isspace():
                    rakefile_lines.append(line)
   
    for i in rakefile_lines:
        print(i)

print(main())
