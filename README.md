# C-Shell

This is a shell written in C that supports the following:

- Constant loop
- Simple command (e.g. ls)
- More complex commands (e.g. (sudo) apt-get update or pkgin update) Pipe (e.g. echo 123 — grep 1)
- File I/O redirection (e.g. echo 123 > 1.txt)
- File I/O redirection (e.g. echo 465 >> 1.txt)
- File I/O redirection (e.g. cat < 1.txt)
- More commands (e.g. vi 2.txt)
- More complex commands with pipes (e.g. ls — xargs grep .txt)
- Clean exit, no memory leak (e.g. Ctrl+d or exit)
- Display the working directory (pwd command)
- Wait for command to be completed when encountering > or | (e.g. echo abc >
>
> 3.txt)
- Handle quotes, regarding their content as a whole (e.g. echo ”def ghi jkl” | grep hi)
- Catch errors (e.g. cat <)
- Change working directory (e.g. cd /home) 
- Ctrl+c is properly handled

## Future improve:

- Pipe composed with file I/O redirection (e.g. cat < 1.txt — sort -R > 2.txt) 
- Change working directory using ../ ~/ ./ etc.
