Copyright (C) 2015 Andriy Martynets [martynets@volia.ua](mailto:martynets@volia.ua)<br>
See the end of the file for license conditions.

-------------------------------------------------------------------------------

#####Introduction
The `execpty` (EXECute in PTY) is a tiny utility to run a command in a pseudo terminal with redirection of the pty I/O to standard I/O. It is developed to run an interactive command line utility (a utility which reads/outputs from/to its controlling terminal) within a script and to communicate with it via standard input/output streams. In other words it helps to make interactive programs non-interactive.
There are plenty projects with the similar goal - to script interactive applications: `expect`, `empty`, `pty4`, etc. This one is probably the simplest. The `execpty` utility does the only thing - redirects pty I/O to standard streams and the rest is up to the author of the final sript...

#####Software Requirements
The source code of the `execpty` utility contains calls to standard C library functions and POSIX compliant system calls. Thus it most likely can be compiled for any POSIX compliant system but the author tested it in GNU/Linux environment only.
To compile the utility from the source code the following packages must be installed on a GNU/Linux system:
- gcc
- make
- libc-dev

#####Downloading
The latest version of the `execpty` utility can be downloaded from the link below:

https://github.com/martynets/execpty/releases/latest

The `execpty` project is also available via Git access from the GitHub server:

https://github.com/martynets/execpty.git

#####Installation
To compile and install the utility issue the following commands from the directory containing the source:
```
make
make install
```
> Note: the utility is installed in `/usr/bin` directory and thus the last one command requires root privileges

To uninstall the utility issue the following command from the same directory (the same note is applicable here):
```
make uninstall
```
#####Usage
The utility recognizes several command line options. The command line syntax is the following:
```
execpty  [OPTIONS] [COMMAND-WITH-ARGUMENTS]
```
The below options are recognized. The first unrecognized option starts the command to execute and its arguments.

|Option|Action|
|------|------|
|-e, --no-echo|turn off echoing on the terminal connected to standard input of the `execpty` process|
|-b, --blind|blind input for the command (don't wait for a prompt)|
|-2, --crnl|pts to use CR/NL as end of line (NL only by default)|
|-q, --quiet|suppress error messages|
|-h, --help|display brief usage information and exit|
|-v, --version|display version information and exit|

Turning off echoing on the terminal connected to standard input makes sense only if the input data are typed from a console which is not the case the `execpty` utility is designed for. This option can be found useful for test cases only and when sensitive data are to be typed.

There is no standard way to know when the system completed loading the image of the command as well as there is no way to guess when the command has completed its own initialization, reset buffers and started to read the data. To avoid the case when the data are written to pts before the command starts read them the `execpty` utility, by default, writes data to pts only after anything was read from it. This means the data are provided in response to a prompt from the command. This is very common scenario for interactive commands. But if the command does not fall in this scope and silently expects some data the blind input mode can be activated by --blind option. To avoid the issue described above some delay must be made. For example:
```
execpty --blind silent_command < <(sleep 1; echo $OUTPUT_DATA)
```
The `execpty` utility makes some configuration of the pts. It turns off echoing and sets it, by default, to use single byte NL (new line or LF - line feed, 0x0A) as the end of line. Option --crnl changes to use 2 bytes CR/NL (carriage return/new line, 0x0D 0x0A) instead.

######Values of exit status mean the following:
- 0 - success (`--help` or `--version` only).
- 127 - system call failure. Corresponding error message is displayed unless `--quiet` option is specified.
- Values greater than 128 - the command is terminated by a signal. The value is the signal number plus 128
- None of the above cases (successful command execution) - exit status of the command executed

#####Examples
The following command runs the `./maintenance.sh` script with root privileges providing root's password in the `PASSWORD` variable
```
echo $PASSWORD | execpty su -c ./maintenance.sh
```
> Note: here-document and here-string make the shell to create a temporary file within the file system with the output data. It is very ill-advised to use this technic for sensitive data. Using pipes (including named pipes and process substitution) is much safer. On systems which doesn't support `/dev/fd/nn` process substitution might also use temporary files. To be sure check: `readlink  <(true)`

The `demo` directory contains two scripts which demonstrate usage of the `execpty` utility to automate an interactive program. The `interactive-command` script emulates an interactive program. It supports commands `name`, `tty`, `echo` and `quit`. The `demo` script automates it. See their sources for more details.

> Note: if you need examples on how to script unidirectional or bidirectional communication with a background process see the Invocations section in `README.md` file of the [dialogbox project](https://github.com/martynets/dialogbox/).

#####Bug Reporting
You can send `execpty` bug reports and/or any compatibility issues directly to the author [martynets@volia.ua](mailto:martynets@volia.ua).

You can also use the online bug tracking system in the GitHub `execpty` project to submit new problem reports or search for existing ones:

  https://github.com/martynets/execpty/issues

#####Change Log
1.0    Initial release

#####License
Copyright (C) 2015 Andriy Martynets [martynets@volia.ua](mailto:martynets@volia.ua)

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
