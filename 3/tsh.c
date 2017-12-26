/**
 * File: tsh.c
 * -----------
 * Your name: <your name here>
 * Your favorite shell command: <your favorite shell command here> 
 */

#include <stdbool.h>       // for bool type
#include <stdio.h>         // for printf, etc
#include <stdlib.h>        
#include <unistd.h>        // for fork
#include <string.h>        // for strlen, strcasecmp, etc
#include <ctype.h>
#include <signal.h>        // for signal
#include <sys/types.h>
#include <sys/wait.h>      // for wait, waitpid
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include "tsh-state.h"
#include "tsh-constants.h"
#include "tsh-parse.h"
#include "tsh-jobs.h"
#include "tsh-signal.h"
#include "exit-utils.h"    // provides exitIf, exitUnless


static struct termios termOriginal, termNonCanonical;


static void imprimirStatusJob(job_t *job)
{
	int status = job->status ;

	if (WIFSTOPPED(status))
	{
		if (WSTOPSIG(status) == SIGTTIN)
		{
			//processo pausado por tentar read sem permissao no terminal

			printf("[%d] (%d) %s pausado por sinal SIGTTIN \n", job->jid, job->pid, job->commandLine) ;
			return ;
		}

		if (WSTOPSIG(status) == SIGTTOU)
		{
			//processo pausado por tentar write sem permissao no terminal

			printf("[%d] (%d) %s pausado por sinal SIGTTOU \n", job->jid, job->pid, job->commandLine) ;
			return ;
		}

		if (WSTOPSIG(status) == SIGTSTP)
		{
			//processo pausado por ctrl-z

			printf("[%d] (%d) %s pausado por sinal SIGTSTP \n", job->jid, job->pid, job->commandLine) ;
			return ;
		}
	}

	if (WIFEXITED(status))
	{
		if ( WEXITSTATUS(status) != 0 && WEXITSTATUS(status) != 127)
		{
			printf("[%d] (%d) %s terminado por chamada exit com status %d \n", job->jid, job->pid, job->commandLine, WEXITSTATUS(status)) ;
			return ;
		}

		if ( WEXITSTATUS(status) == 127)
		{
			printf("%s : Comando nao encontrado \n", job->commandLine) ;
			return ;
		}
	}

	if (WIFSIGNALED(status))
	{
		if ( WTERMSIG(status) == SIGINT)
		{
			printf("[%d] (%d) %s terminado por sinal SIGINT \n", job->jid, job->pid, job->commandLine) ;
			return ;
		}
		else
		{
			printf("[%d] (%d) %s terminado por sinal %d \n", job->jid, job->pid, job->commandLine, WTERMSIG(status)) ;
			return ;
		}
	}
}

/** 
 * Block until process pid is no longer the foreground process
 */
static void waitfg(pid_t pid)
{
	int ret ;
	job_t *temp ;

	temp = getjobpid(jobs, pid) ;
	temp->state = kForeground ;

	tcsetpgrp(STDIN_FILENO, pid) ;

	kill(pid, SIGCONT) ;

	if ((ret = waitpid(pid, &(temp->status), WUNTRACED)) > 0)
	{
		sigset_t signalSet ;
		sigemptyset(&signalSet) ;
		sigaddset(&signalSet, SIGTTOU) ;
		sigprocmask(SIG_BLOCK, &signalSet, NULL) ;

		tcsetpgrp(STDIN_FILENO, getpid()) ;

		sigprocmask(SIG_UNBLOCK, &signalSet, NULL) ;

		if ( WIFSIGNALED(temp->status) || WIFSTOPPED(temp->status))
			printf("\n") ;

		if (WIFSTOPPED(temp->status))
			temp->state = kStopped ;

		imprimirStatusJob(temp) ;

		if ( WIFEXITED(temp->status) || WIFSIGNALED(temp->status))
		{
			deletejob(jobs, pid) ;
		}
		else
		{
			temp->status = 0 ;
			temp->state = kStopped ;
		}
	}
	else
	{
		//erro no waitpid

		int err = errno ;
		printf("\nwaitpid retornou %d, errno=%d \n", ret, err) ;
	}
}

/**
 * Execute the builtin bg and fg commands
 */
static void do_bg(char *argv[])
{
	job_t *temp ;

	if (argv[1][0] == '%')
	{
		int jobId = strtol(argv[1] + 1, NULL, 10) ;
		temp = getjobjid(jobs, jobId) ;
	}
	else
	{
		int pId = strtol(argv[1], NULL, 10) ;
		temp = getjobpid(jobs, pId) ;
	}

	if (temp != NULL)
	{
		kill(temp->pid, SIGCONT) ;
		temp->state = kBackground ;
	}
	else
	{
		printf("Job %s nao encontrado\n", argv[1]) ;
	}
}

static void do_fg(char *argv[])
{
	job_t *temp ;

	if (argv[1][0] == '%')
	{
		int jobId = strtol(argv[1] + 1, NULL, 10) ;
		temp = getjobjid(jobs, jobId) ;
	}
	else
	{
		int pId = strtol(argv[1], NULL, 10) ;
		temp = getjobpid(jobs, pId) ;
	}

	if (temp != NULL)
	{
		waitfg(temp->pid) ;
	}
	else
	{
		printf("Job %s nao encontrado\n", argv[1]) ;
	}
}

/**
 * If the user has typed a built-in command then execute 
 * it immediately.  Return true if and only if the command 
 * was a builtin and executed inline.
 */
static bool handleBuiltin(char *argv[])
{
	if (strcmp(argv[0], "quit") == 0)
		exit(1) ;

	if (strcmp(argv[0], "jobs") == 0)
	{
		listjobs(jobs) ;
		return true ;
	}

	if (strcmp(argv[0], "bg") == 0 && argv[1] != NULL)
	{
		do_bg(argv) ;
		return true ;
	}

	if (strcmp(argv[0], "fg") == 0 && argv[1] != NULL)
	{
		do_fg(argv) ;
		return true ;
	}

	return false ;
}

/**
 * The kernel sends a SIGCHLD to the shell whenever a child job terminates 
 * (becomes a zombie), or stops because it receives a SIGSTOP or SIGTSTP signal.  
 * The handler reaps all available zombie children, but doesn't wait for any other
 * currently running children to terminate.  
 */
static void handleSIGCHLD(int unused)
{
	int status ;
	pid_t idFilhoTerminado ;

	while ((idFilhoTerminado = waitpid(-1, &status, WUNTRACED | WNOHANG)) > 0)
	{
		if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		{
			deletejob(jobs, idFilhoTerminado) ;
			return ;
		}

		job_t *temp = getjobpid(jobs, idFilhoTerminado) ;
		temp->status = status ;

		if (WIFEXITED(status) || WIFSIGNALED(status))
		{
			temp->state = kTerminado ;
		}
		else if (WIFSTOPPED(status))
		{
			temp->state = kStopped ;
		}
	}
}

/**
 * The kernel sends a SIGTSTP to the shell whenever
 * the user types ctrl-z at the keyboard.  Catch it and suspend the
 * foreground job by sending it a SIGTSTP.
 */
static void handleSIGTSTP(int sig)
{
	pid_t processoForeground = fgpid(jobs) ;

	if (processoForeground != 0)
	{
		kill(-processoForeground, sig) ;
	}
}

/**
 * The kernel sends a SIGINT to the shell whenver the
 * user types ctrl-c at the keyboard.  Catch it and send it along
 * to the foreground job.  
 */
static void handleSIGINT(int sig)
{
	pid_t processoForeground = fgpid(jobs) ;

	if (processoForeground != 0)
	{
		kill(-processoForeground, sig) ;
	}
}

/**
 * The driver program can gracefully terminate the
 * child shell by sending it a SIGQUIT signal.
 */
static void handleSIGQUIT(int sig) {
  printf("Terminating after receipt of SIGQUIT signal\n");
  exit(1);
}

/**
 * Prints a nice little usage message to make it clear how the
 * user should be invoking tsh.
 */
static void usage() {
  printf("Usage: ./tsh [-hvp]\n");
  printf("   -h   print this message\n");
  printf("   -v   print additional diagnostic information\n");
  printf("   -p   do not emit a command prompt\n");
  exit(1);
}

/**
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately.  Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
 */
static void eval(char commandLine[])
{
	char *args[kMaxArgs] ;
	int backgroundJob = parseline(commandLine, args, kMaxArgs) ;

	if (args[0] == NULL)
		return ;

	if (handleBuiltin(args))
		return ;

	sigset_t signalSet ;
	sigemptyset(&signalSet) ;
	sigaddset(&signalSet, SIGCHLD) ;
	sigprocmask(SIG_BLOCK, &signalSet, NULL) ;

	for (int i = 0; args[i] != NULL; i++)
	{
		//checando parametros do commandline por possiveis erros

		if (strcmp(args[i], ">") == 0)
		{
			if (args[i + 1] == NULL)
			{
				printf("%s : Erro de sintaxe apos operador >\n", args[i + 1]) ;
				return ;
			}
		}

		if (strcmp(args[i], "<") == 0)
		{
			if (args[i + 1] == NULL)
			{
				printf("%s : Erro de sintaxe apos operador <\n", args[i + 1]) ;
				return ;
			}

			if (access(args[i + 1], R_OK) != 0)
			{
				printf("%s : Arquivo nao encontrado\n", args[i + 1]) ;
				return ;
			}
		}
	}

	pid_t retornoFork = fork() ;

	if (retornoFork == 0)
	{
		setpgid(0, 0) ;

		sigprocmask(SIG_UNBLOCK, &signalSet, NULL) ;

		for (int i = 0; args[i] != NULL; i++)
		{
			if (strcmp(args[i], ">") == 0)
			{
				int output = open(args[i + 1], O_WRONLY | O_CREAT, 0666) ;
				dup2(output, STDOUT_FILENO) ;
				close(output) ;
				args[i] = NULL ;
				i += 1 ;
			}

			if (strcmp(args[i], "<") == 0)
			{
				int input = open(args[i + 1], O_RDONLY) ;
				dup2(input, STDIN_FILENO) ;
				close(input) ;
				args[i] = NULL ;
				i += 1 ;
			}
		}

		if (execvp(args[0], args) == -1)
		{
			exit(127) ;
		}
	}

	if (retornoFork > 0)
	{
		if (backgroundJob)
		{
			job_t *ret = addjob(jobs, retornoFork, kBackground, commandLine) ;

			sigprocmask(SIG_UNBLOCK, &signalSet, NULL) ;

			listjobs(ret) ;
		}
		else
		{
			//foreground job

			addjob(jobs, retornoFork, kUndefined, commandLine) ;

			sigprocmask(SIG_UNBLOCK, &signalSet, NULL) ;

			waitfg(retornoFork) ;
		}
	}
}



/**
 * Redirect stderr to stdout (so that driver will get all output
 * on the pipe connected to stdout) 
 */
static void mergeFileDescriptors() {
  dup2(STDOUT_FILENO, STDERR_FILENO);
}

/**
 * Defines the main read-eval-print loop
 * for your simplesh.
 */
int main(int argc, char *argv[]) {
  mergeFileDescriptors();
  bool showPrompt = true;
  while (true) {
    int option = getopt(argc, argv, "hvp");
    if (option == EOF) break;
    switch (option) {
    case 'h':
      usage();
      break;
    case 'v': // emit additional diagnostic info
      verbose = true;
      break;
    case 'p':           
      showPrompt = false;
      break;
    default:
      usage();
    }
  }
  
  installSignalHandler(SIGQUIT, handleSIGQUIT); 
  installSignalHandler(SIGINT,  handleSIGINT);   // ctrl-c
  installSignalHandler(SIGTSTP, handleSIGTSTP);  // ctrl-z
  installSignalHandler(SIGCHLD, handleSIGCHLD);  // terminated or stopped child
  initjobs(jobs);  

  tcgetattr(STDIN_FILENO, &termOriginal);
  termNonCanonical = termOriginal ;
  termNonCanonical.c_lflag &= ~ICANON ;
  termNonCanonical.c_cc[VTIME] = 0 ;
  termNonCanonical.c_cc[VMIN] = 0 ;

  while (true) {

  	int flagPrint = 1 ;

	for (int i = 0; i < kMaxJobs; i++)
	{
		if (jobs[i].status != 0)
		{
			if (flagPrint)
			{
				printf("=Imprimindo atualizações de processos filhos=\n") ;
				flagPrint = 0 ;
			}

			imprimirStatusJob(jobs + i) ;
			atualizarAtributosJob(jobs + i) ;
		}
	}

	if (showPrompt)
	{
		printf("%s", kPrompt) ;
		fflush(stdout) ;
	}

    char command[kMaxLine];
    exitIf((fgets(command, kMaxLine, stdin) == NULL) && ferror(stdin),
	   1, stdout, "fgets failed.\n");
    if (feof(stdin)) break;

    command[strlen(command) - 1] = '\0'; // overwrite fgets's \n
    eval(command);
    fflush(stdout);
  }
  
  fflush(stdout);  
  return 0;
}
