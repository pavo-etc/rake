# rake

Client and server applications for rake, a protocol for distributed compilation and remote command execution.

*r(emote) + (m)ake = rake*

Created by Zach Manson and Dempsey Thompson

Completed for CITS3002

## Usage

rakeserver is written in Python:
```sh
python3 ./rakeserver/rakeserver.py [port]
```

rakeclient has 2 versions, one in Python and one in C:

```sh
python3 ./path/to/rake-p.py [-v] [filename]
```

```sh
./rake-c/rake-c [-v] [filename]
```

Supported on Unix-like systems. Tested on Ubuntu 20.04 and macOS Mojave.

Final mark: 95%

Due date: 5PM Friday 20th May (end of week 11)

Resources:
 + [Project Spec](https://teaching.csse.uwa.edu.au/units/CITS3002/project2022/index.php)
 + [Getting Started Explanation](https://teaching.csse.uwa.edu.au/units/CITS3002/project2022/gettingstarted.php)
 + [Clarifications](https://teaching.csse.uwa.edu.au/units/CITS3002/project2022/clarifications.php)

## Todo

 + [x] Python Client (rakeclient-p)
   + [x] Rakefile processing
   + [x] Figure our whats required for rakefile parsing
 + [x] Rakeserver (Python)
   + [x] Multiprocessing
 + [x] C Client (rakeclient-c)
 + [x] Report (PDF, 3 A4 pages long) (ideally with diagrams and examples)
   + [x] the protocol you have designed and developed for all communication between your client and server programs
   + [x] a 'walk-through' of the execution sequence employed to compile and link an multi-file program
   + [x] the conditions under which remote compilation and linking appears to perform better (faster) than just using your local machine
