/**
 * File: tsh-jobs.h
 * ----------------
 * Defines the struct job type and a collection 
 * of utility functions that can be used to 
 * access and manipulate them.
 */

#ifndef _tsh_jobs_
#define _tsh_jobs_

#include "tsh-constants.h"
#include <stdbool.h>
#include <sys/types.h>

/**
 * Defines the four different states a job can be in.
 */

enum {
  kUndefined = 0, kBackground, kForeground, kStopped, kTerminado
};

/**
 * Type: struct job_t
 * ------------------
 * Manages all of the information about a currently executing job.
 *
 *   pid: the job's process id
 *   jid: the process's job id [1, 2, 3, 4, etc.]
 *   state: kUndefined, kBackground, kForeground, or kStopped 
 *   commandLine: The original command line used to create the job
 */

typedef struct job_t {
  pid_t pid;
  int jid;
  int state;
  char commandLine[kMaxLine];
  int status ; // adicionado para imprimir informaçoes de status
} job_t;

void clearjob(job_t *job);
void initjobs(job_t jobs[]);
int maxjid(job_t jobs[]);

/*
 * pequena modificação para retornar o endereco do ultimo job adicionado, para imprimir a string de informação após
 * adição de um background job
 */
job_t* addjob(job_t jobs[], pid_t pid, int state, const char *commandLine);

bool deletejob(job_t jobs[], pid_t pid); 
pid_t fgpid(job_t jobs[]);
job_t *getjobpid(job_t jobs[], pid_t pid);
job_t *getjobjid(job_t jobs[], int jid); 
int pid2jid(pid_t pid); 
void listjobs(job_t jobs[]);

void atualizarAtributosJob( job_t *job ) ;

#endif
