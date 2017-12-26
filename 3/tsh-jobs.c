/**
 * File: tsh-jobs.c
 * ---------------- 
 * Presents the implementation of all of the 
 * jobs manipulation functions defined and 
 * documented in tsh-jobs.h
 */

#include "tsh-state.h"
#include "tsh-jobs.h"
#include <stdio.h>    // for printf
#include <string.h>   // for strcpy
#include <stdlib.h>

// standalone module state
static int nextJobID = 1;

void listjobs(job_t jobs[]) {
  for (size_t i = 0; i < kMaxJobs; i++) {
    if (jobs[i].pid != 0) {
      printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid );
      switch (jobs[i].state) {
      case kBackground:
      	printf("Running ");
				break;
      case kForeground: 
				printf("Foreground ");
				break;
			case kStopped:
				printf("Stopped ");
				break ;
      case kTerminado:
      	printf("Terminado ");
      	break;
      default:
      	printf("listjobs: Internal error: job[%zu].state=%d ",
	       i, jobs[i].state);
      }
      printf("%s\n", jobs[i].commandLine);
    }
  }
}

/* clearjob - Clear the entries in a job struct */
void clearjob(job_t *job) {
  job->pid = 0;
  job->jid = 0;
  job->state = kUndefined;
  job->commandLine[0] = '\0';
  job->status = 0 ;
}

/* initjobs - Initialize the job list */
void initjobs(job_t jobs[]) {
  for (size_t i = 0; i < kMaxJobs; i++)
    clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(job_t jobs[]) {
  int max = jobs[0].jid;
  for (size_t i = 1; i < kMaxJobs; i++) {
    if (jobs[i].jid > max) {
      max = jobs[i].jid;
    }
  }
  
  return max;
}

/* addjob - Add a job to the job list */
job_t* addjob(job_t jobs[], pid_t pid, int state, const char *commandLine) {
  if (pid < 1) return false;
  for (size_t i = 0; i < kMaxJobs; i++) {
    if (jobs[i].pid == 0) {
      jobs[i].pid = pid;
      jobs[i].state = state;
      jobs[i].jid = nextJobID++;
      if (nextJobID > kMaxJobID)
      	nextJobID = 1;
      strcpy(jobs[i].commandLine, commandLine);
      if (verbose) {
      	printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].commandLine);
      }
      return jobs+i;
    }
  }

  printf("Tried to create too many jobs\n");
  return NULL;
}

/* deletejob - Delete a job whose PID=pid from the job list */
bool deletejob(job_t jobs[], pid_t pid)  {
  if (pid < 1) return false;
  for (size_t i = 0; i < kMaxJobs; i++) {
    if (jobs[i].pid == pid) {
      clearjob(&jobs[i]);
      nextJobID = maxjid(jobs)+1;
      return true;
    }
  }
  return false;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
  for (size_t i = 0; i < kMaxJobs; i++) {
    if (jobs[i].state == kForeground) {
      return jobs[i].pid;
    }
  }
  return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
job_t *getjobpid(job_t jobs[], pid_t pid) {
  if (pid < 1) return NULL;
  for (size_t i = 0; i < kMaxJobs; i++) {
    if (jobs[i].pid == pid) {
      return &jobs[i];
    }
  }
  return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
job_t *getjobjid(struct job_t jobs[], int jid) {
  if (jid < 1) return NULL;
  for (int i = 0; i < kMaxJobs; i++) {
    if (jobs[i].jid == jid) {
      return &jobs[i];
    }
  }
  return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) {
  if (pid < 1) return 0;
  for (size_t i = 0; i < kMaxJobs; i++) {
    if (jobs[i].pid == pid) {
      return jobs[i].jid;
    }
  }
  return 0;
}


void atualizarAtributosJob(job_t *job) {

	if ( WIFEXITED(job->status) || WIFSIGNALED(job->status) ) {

		deletejob(jobs, job->pid);

	} else {

		job->status = 0 ;
		job->state = kStopped ;

	}

}




