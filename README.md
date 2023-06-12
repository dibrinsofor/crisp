### crisp

Works on Windows and Unix machines.

#### Build the REPL
```bash
gcc mpc.c repl.c -lm -ledit -o repl
```
>-lm and -ledit are packages required to run the language. You may need to install these.

#### Run
```bash
./repl
```

#### Specs