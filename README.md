# canosh
Shell written in C

## Quick Start

```
make
./main
```

> [!TIP]
> To see other available target, use `make help`


The goal is to make a POSIX compliant shell

## Features

- [x] shell repl in ncurses
- [x] execute binaries on system
### Shortcuts
- [x] CTRL+A (Move to beginning of line)
- [x] CTRL+E (Move to end of line)
- [x] CTRL+K (Cut everything after cursor)
- [x] CTRL+Y (Paste cutted content)
- [ ] CTRL+R (Reverse search command history)
### Builtins
- [x] exit
- [x] cd
- [x] history
- [ ] :
- [ ] echo
- [ ] env
- [ ] eval
- [ ] kill
- [ ] setenv
- [ ] unsetenv
- [ ] setpath
- [ ] printenv
- [ ] repeat
- [ ] termname
- [ ] time
- [ ] where
- [ ] which
- [ ] pwd
- [ ] alias

