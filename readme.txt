Use ./shell command to start the shell.
$<space>path/executable to run the executable(please specify the path without beginning it with "/").
Eg. type $ ayuj/run to run the "run" executable present inside ayuj directory.
$<space>path/executable > out.txt to store the output in file named out.txt
Ctrl + C doesn't work on the default behaviour of the shell. But during the execution of the cmd_run command it works in the child process since we have kept the default behaviour of signals there.
