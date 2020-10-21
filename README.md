# UnixShell
This program, as its name suggest behaves like a shell in a Unix environment<br>
It can perform all the function all the functionalities of the regular terminal<br>
In addition, the program counts how many execution have been done (ls, mkdir, cat, chown ..etc) during it's run time and display however many upon termination <br>
The program can, in theory run itself, anamount of time limited by the amount of memory in the system it is running. For example one can run shell.c, run shell.c within shell.c and keep going<br>
The program uses multiple processing and piplinin to perform the tasks. It goes to the unix bin to grab the prgram to run such as ssh, vim, cd, ls and so on, then use the piplining in C to show them to hide and show them  
