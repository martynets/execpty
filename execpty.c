/*
 * execpty version 1.0 - Execute a command in pseudo terminal redirecting pty I/O to standard I/O.
 *
 * Copyright (C) 2015 Andriy Martynets <martynets@volia.ua>
 *--------------------------------------------------------------------------------------------------------------
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *--------------------------------------------------------------------------------------------------------------
 */


/*
 *	Feature Test Macro Requirements for glibc:
 *			_GNU_SOURCE || _XOPEN_SOURCE >= 500
 *					(the last one is implicitly defined by the first one)
 */
#define _XOPEN_SOURCE 700

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>

#define PROJECT_NAME "execpty"
#define VERSION "1.0"

/*	Exit status:
 *		- 0 success (--help or --version only)
 *		- 127 system call failure (with corresponding error message)
 *		- if the command is terminated by a signal - the signal number plus 128
 *		- on success - exit status of the command executed
*/
#define E_SUCCESS	0
#define E_ERROR		127

#define BUFFER_SIZE 4096
#define EOF_TIMEOUT 100	// number of milliseconds that stdin is not polled once EOF is detected

static char* myname;
static char* buffer;
static pid_t cpid=0;
static int ret_value=E_ERROR, quiet=0, noecho=0, cr_output=0, command=0;
static int max_pollable_fds=2;
static int pollable_fds=1;
static int poll_timeout=-1;


static struct termios* tio=NULL;

static void data_transfer(int fromfd, int tofd);
static void error(const char*);
static void terminal_restore();
static void signal_handler(int);
static void help();
static void version();


int main(int argc, char** argv)
{
	char buf[BUFFER_SIZE];
	struct termios term;
	int mfd;
	int i;

	buffer=buf;
	myname=argv[0];
	for(i=0; argv[0][i]; i++) if(argv[0][i] == '/') myname=argv[0]+i+1;

	for(i=1; i<argc; i++)
		{
			if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help"))
				{
					help();
					return(E_SUCCESS);
				}
			if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version"))
				{
					version();
					return(E_SUCCESS);
				}
			if(!strcmp(argv[i], "-q") || !strcmp(argv[i], "--quiet"))
				{
					quiet=1;
					continue;
				}
			if(!strcmp(argv[i], "-e") || !strcmp(argv[i], "--no-echo"))
				{
					noecho=1;
					continue;
				}
			if(!strcmp(argv[i], "-2") || !strcmp(argv[i], "--crnl"))
				{
					cr_output=1;
					continue;
				}
			if(!strcmp(argv[i], "-b") || !strcmp(argv[i], "--blind"))
				{
					pollable_fds=2;
					continue;
				}
			command=i;
			break;
		}

	if(!command)
		{
			help();
			return(E_SUCCESS);
		}

	if((mfd=posix_openpt(O_RDWR | O_NOCTTY)) == -1 || grantpt(mfd) || unlockpt(mfd)) error("posix_openpt(), grantpt() or unlockpt()");

	if((cpid=fork()) == -1) error("fork()");

	if(!cpid)	// child
		{
			int sfd;

			if(setsid() == (pid_t) -1) error("setsid()");

			/*
			 *	The next call to open sets the pts as the controlling terminal
			 * 	as this is session leader without controlling terminal.
			 * 	This is true for Linux but POSIX leaves this up to the implementation.
			 */
			if((sfd=open(ptsname(mfd), O_RDWR)) == -1) error("open()");

			// configure pts
			if(tcgetattr(sfd, &term)) error("tcgetattr()");
			term.c_lflag &= ~ECHO;	// disable ECHO
			if(!cr_output)
				{	// Unix like - LN (0x0A) only at end of line (default)
					term.c_oflag &= ~ONLCR;		// disable NL -> CR/NL translation on output
					term.c_iflag &= ~ICRNL;		// disable CR -> NL translation on input
				}
			else
				{	// Windows like - CR/NL (0x0D 0x0A) at end of line
					term.c_iflag |= ICRNL;		// enable CR -> NL translation on input
					term.c_iflag &= ~INLCR;		// disable NL -> CR translation on input
					term.c_iflag &= ~IGNCR;		// disable ignoring CR on input
					term.c_oflag |= ONLCR;		// enable NL -> CR/NL translation on output
					term.c_oflag &= ~OCRNL;		// disable CR -> NL translation on output
					term.c_oflag &= ~ONLRET;	// disable suppression of CR output
				}
			if(tcsetattr(sfd, TCSANOW, &term)) error("tcsetattr()");

			if(dup2(sfd, STDIN_FILENO) == -1) error("dup2()");
			if(dup2(sfd, STDOUT_FILENO) == -1) error("dup2()");
			if(dup2(sfd, STDERR_FILENO) == -1) error("dup2()");

			if(close(mfd)) error("close()");
			if(close(sfd)) error("close()");

			execvp(argv[command], argv+command);
			error("execvp()");
		}

	// parent

	struct sigaction sa;

	sa.sa_handler=signal_handler;
	sa.sa_flags = SA_RESETHAND;
	sigemptyset(&sa.sa_mask);
	if(sigaction(SIGHUP, &sa, NULL) == -1) error("sigaction()");
	if(sigaction(SIGINT, &sa, NULL) == -1) error("sigaction()");
	if(sigaction(SIGQUIT, &sa, NULL) == -1) error("sigaction()");
	if(sigaction(SIGTERM, &sa, NULL) == -1) error("sigaction()");

	if(noecho && isatty(STDIN_FILENO))
		{
			if(tcgetattr(STDIN_FILENO, &term)) error("tcgetattr()");
			if(term.c_lflag & ECHO)
				{
					sigset_t set, sset;

					sigemptyset(&set);
					sigaddset(&set, SIGHUP);
					sigaddset(&set, SIGINT);
					sigaddset(&set, SIGQUIT);
					sigaddset(&set, SIGTERM);
					sigprocmask(SIG_BLOCK, &set, &sset);

					tio=&term;
					tio->c_lflag^=ECHO;
					if(tcsetattr(STDIN_FILENO, TCSANOW, tio)) error("tcsetattr()");
					tio->c_lflag^=ECHO;

					sigprocmask(SIG_SETMASK, &sset, NULL);
					atexit(terminal_restore);
				}
		}

	struct pollfd fds[2];
	fds[0].fd=mfd;
	fds[0].events=POLLIN;
	fds[1].fd=STDIN_FILENO;
	fds[1].events=POLLIN;

	while(1)
		{
			if(poll(fds, pollable_fds, poll_timeout) == -1) error("poll()");
			pollable_fds=max_pollable_fds;
			poll_timeout=-1;

			if(fds[0].revents & POLLIN) data_transfer(mfd, STDOUT_FILENO);
			if(fds[0].revents & POLLHUP) break;	// child terminated

			/* We have to stay in the loop in case of STDIN either EOF or pipe write end close
			 * to continue output transfer.
			 * POLLIN @ EOF temporary disables poll on STDIN when POLLHUP disables it permanently.
			 */
			if(fds[1].revents & POLLIN) data_transfer(STDIN_FILENO, mfd);
			if(fds[1].revents & POLLHUP)	// pipe write end closed
				pollable_fds=max_pollable_fds=1;	// We will not poll the pipe any more
			fds[1].revents=0; // in case we detected EOF and postponed next poll
		}

	int wstatus;

	if(waitpid(cpid, &wstatus, 0) == -1) error("waitpid()");

	if(WIFEXITED(wstatus)) ret_value=WEXITSTATUS(wstatus);
	if(WIFSIGNALED(wstatus)) ret_value=WTERMSIG(wstatus)+128;

	close(mfd);	// close pty only AFTER child exited, otherwise it is killed by SIGHUP
	terminal_restore();	// prevent tio access after main's return
	return(ret_value);
}

static void data_transfer(int fromfd, int tofd)
{
	int rc, wc, c;
	struct pollfd fds[2];

	fds[0].fd=fromfd;
	fds[0].events=POLLIN;
	fds[1].fd=tofd;
	fds[1].events=POLLOUT;

	do
		{
			if((rc=read(fromfd, buffer, BUFFER_SIZE)) == -1) error("read()");
			if(!rc)
				{
					/* POLLIN @ EOF
					 * This can happen on stdin when redirected to a regular file
					 * (including here-document and here-string cases) or with
					 * pipes on some non-Linux systems.
					 * See more: http://www.greenend.org.uk/rjk/tech/poll.html
					 */
					pollable_fds=1;
					poll_timeout=EOF_TIMEOUT;
					return;
				}
			wc=0;
			do
				{
					if(poll(fds+1, 1, -1) == -1) error("poll()");
					if(fds[1].revents & POLLOUT)
						{
							if((c=write(tofd, buffer+wc, rc-wc)) == -1) error("write()");
							wc+=c;
						}
				}
			while(wc<rc);
			if(poll(fds, 1, 0) == -1) error("poll()");
		}
	while(fds[0].revents & POLLIN);
}

static void error(const char* comment)
{
	if(!quiet)
		{
			sprintf(buffer, "%s: %s error", myname, comment);
			perror(buffer);
		}
	if(cpid && cpid != -1) kill(cpid, SIGTERM);
	exit(ret_value);
}

static void terminal_restore()
{
	if(tio)
		{
			tcsetattr(STDIN_FILENO, TCSANOW, tio);
			tio=NULL;
		}
}

static void signal_handler(int sig)
{
	kill(cpid, sig);
	terminal_restore();
	raise (sig);
}

static void help()
{
	char* usage=
"Usage:	"PROJECT_NAME" [OPTIONS] [COMMAND-WITH-ARGUMENTS]\n\
Execute a command in pseudo terminal redirecting pty I/O to standard I/O.\n\
\n\
Options are:\n\
	-e, --no-echo	suppress echo for current controlling terminal if any\n\
	-b, --blind	blind input for the command (don't wait for a prompt)\n\
	-2, --crnl	pts to use CR/NL as end of line (NL only by default)\n\
	-q, --quiet	suppress error messages\n\
	-h, --help	display this help and exit\n\
	-v, --version	output version information and exit\n\
\n\
Exit status:\n\
	- 0 success (--help or --version only)\n\
	- 127 system call failure (with corresponding error message)\n\
	- if the command is terminated by a signal - the signal number plus 128\n\
	- on success - exit status of the command executed\n\
\n\
Full documentation at: <https://github.com/martynets/execpty/>\n";

	puts(usage);
}

static void version()
{
	char* ver=
PROJECT_NAME" v"VERSION"\n\
Copyright (C) 2015 Andriy Martynets <martynets@volia.ua>\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n\
This program comes with ABSOLUTELY NO WARRANTY.\n\
This is free software, and you are welcome to redistribute it\n\
under certain conditions. See the GNU GPL for details.";

	puts(ver);
}
