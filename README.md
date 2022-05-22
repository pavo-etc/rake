# cits3002-project

Due date: 5PM Friday 20th May (end of week 11)
Weight: 


Resources:
 + [Project Spec](https://teaching.csse.uwa.edu.au/units/CITS3002/project2022/index.php)
 + [Getting Started Explanation](https://teaching.csse.uwa.edu.au/units/CITS3002/project2022/gettingstarted.php)
 + [Clarifications](https://teaching.csse.uwa.edu.au/units/CITS3002/project2022/clarifications.php)

## Executing

```sh
python3 ./rakeserver/rakeserver.py
```

```sh
python3 ./path/to/rake-p.py [-v] [filename]
```

```sh
./rake-c/rake-c [filename]
```

## Todo

 + [ ] Python Client (rakeclient-p)
   + [ ] Rakefile processing
   + [ ] Figure our whats required for rakefile parsing, add here
 + [ ] Rakeserver (Python)
   + [ ] Multiprocessing
 + [ ] C Client (rakeclient-c)
 + [ ] Report (PDF, 3 A4 pages long) (ideally with diagrams and examples)
   + [ ] the protocol you have designed and developed for all communication between your client and server programs
   + [ ] a 'walk-through' of the execution sequence employed to compile and link an multi-file program
   + [ ] the conditions under which remote compilation and linking appears to perform better (faster) than just using your local machine
