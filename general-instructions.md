## General Instructions ##

* [LLVM documentation](http://llvm.org/docs/)
* [LLVM Installation](http://llvm.org/docs/GettingStarted.html)
* [LLVM Programmer's Manual](http://llvm.org/docs/ProgrammersManual.html))
* [LLVM doxygen docs](http://llvm.org/doxygen/)
* [LLVM command guide](http://llvm.org/docs/CommandGuide/index.html)

For quick installation in linux systems, run:  
`sudo apt install llvm`

To run a pass:  
1. Run `cmake <path-where-CMakelists.txt-exists>`.
2. Run `make` in the folder where Makefile is generated.
3. Load and run the pass using `opt` command in the terminal.
