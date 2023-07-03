### crisp

A lisp implementation (with a repl) built on top of Daniel Holden's ([theorangeduck](https://theorangeduck.com)) mpc parsing library. Works on Windows and Unix machines.

#### Build
```bash
gcc mpc.c repl.c -lm -ledit -o repl
```
>-lm and -ledit are packages required to run the language. You may need to install these.

#### Run
```bash
./repl
```

#### Add. Specs
- `def` to declare variables.
- `(@ (x y) {x + y})`