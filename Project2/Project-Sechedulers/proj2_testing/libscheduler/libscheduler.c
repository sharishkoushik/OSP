/** @file libscheduler.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libscheduler.h"
#include "../libpriqueue/libpriqueue.h"


/**
  Stores information making up a job to be scheduled including any statistics.

  You may need to define some global variables or a struct to store your job queue elements. 
  */
typedef struct _job_t
{
    int job_number;
    int arrival_time;
    int run_time;
    int priority;
	int core_id; /* Core in which job is scheduled; -1 if not assigned */
	int arrived;
	/* Variables for performance calculation */
    int start_time;
    int remaining_time;
    int idle_time;
	int is_qhead; /* Current job is the head of the p_queue */
} job_t;

/* Multiple cores support */
int g_core_usage_array[4];
int g_ncores;

scheme_t 	g_scheduler_scheme;
priqueue_t 	g_jobs_queue;
int 		g_num_jobs;

float 		g_waiting_time = 0; 
float 		g_turnaround_time = 0;
float 		g_response_time = 0;

/* Comparer functions for inserting jobs in the jobs queue */
int cmp_fcfs(const void *a, const void *b) {
    return (((job_t *)a)->arrival_time - ((job_t *)b)->arrival_time);
}

int cmp_sjf(const void *a, const void *b) {
    if ((((job_t *) b)-> is_qhead) == 1) {
		return 1;
	}
    return (((job_t *)a)->run_time - ((job_t *)b)->run_time);
}

int cmp_psjf(const void *a, const void *b) {
    return (((job_t *)a)->remaining_time - ((job_t *)b)->remaining_time);
}

int cmp_pri(const void *a, const void *b) {
    if ((((job_t *) b)-> is_qhead) == 1) {
		return 1;
	}
    return (((job_t *)a)->priority - ((job_t *)b)->priority);
}

int cmp_ppri(const void *a, const void *b) {
    return (((job_t *)a)->priority - ((job_t *)b)->priority);
}

int cmp_rr(const void *a, const void *b) {
	int result = ((job_t *)a)->priority - ((job_t *)b)->priority; /* Satisfy -Werror Flag */
    //return 1;
	return 0;
}

/**
  Initalizes the scheduler.

Assumptions:
- You may assume this will be the first scheduler function called.
- You may assume this function will be called once once.
- You may assume that cores is a positive, non-zero number.
- You may assume that scheme is a valid scheduling scheme.

@param cores the number of cores that is available by the scheduler. These cores will be known as core(id=0), core(id=1), ..., core(id=cores-1).
@param scheme  the scheduling scheme that should be used. This value will be one of the six enum values of scheme_t
*/
void scheduler_start_up(int cores, scheme_t scheme)
{
    int i;

    if (scheme == FCFS){
        g_scheduler_scheme = FCFS; 
        priqueue_init(&g_jobs_queue, cmp_fcfs); 
    }
    else if (scheme == SJF){
        g_scheduler_scheme = SJF; 
        priqueue_init(&g_jobs_queue, cmp_sjf); 
    }
    else if(scheme == PSJF){
        g_scheduler_scheme = PSJF; 
        priqueue_init(&g_jobs_queue, cmp_psjf); 
    }
    else if (scheme == PRI){
        g_scheduler_scheme = PRI; 
        priqueue_init(&g_jobs_queue, cmp_pri);
    }
    else if(scheme == PPRI){
        g_scheduler_scheme = PPRI; 
        priqueue_init(&g_jobs_queue, cmp_ppri);
    }
    else {
        g_scheduler_scheme = RR; 
        priqueue_init(&g_jobs_queue, cmp_rr);
    }

    /* Set global number of cores */
    g_ncores = cores;

    /* Set all cores as free */
    for (i = 0; i < cores; i++)
    {   
        g_core_usage_array[i] = 0;
    }
}


/**
  Called when a new job arrives.

  If multiple cores are idle, the job should be assigned to the core with the
  lowest id.
  If the job arriving should be scheduled to run during the next
  time cycle, return the zero-based index of the core the job should be
  scheduled on. If another job is already running on the core specified,
  this will preempt the currently running job.
Assumptions:
- You may assume that every job wil have a unique arrival time.

@param job_number a globally unique identification number of the job arriving.
@param time the current time of the simulator.
@param run_time the total number of time units this job will run before it will be finished.
@param priority the priority of the job. (The lower the value, the higher the priority.)
@return index of core job should be scheduled on
@return -1 if no scheduling changes should be made. 

*/
int scheduler_new_job(int job_number, int time, int run_time, int priority)
{
	int i, ret_core = -1, qindex_offered = -1;
    job_t *new_job, *qhead_job, *qhead_job_new;

    /* Allocate memory for the new job */
    new_job = (job_t *)malloc(sizeof(job_t));

	/* Initialize the default values for new job */
    new_job->job_number = job_number; 
    new_job->arrival_time = time; 
    new_job->run_time = run_time; 
    new_job->priority = priority;
	new_job->core_id = -1;
	new_job->start_time = -1;
    new_job->remaining_time = run_time;
	new_job->idle_time = 0;
    new_job->is_qhead = 0;

    g_num_jobs++;

    /* NB TODO: for premptive we just cannot assign available core without first checking new job's priority.
     * So has to be inserted in queue. Then find the next job to schedule from queue. Assign core to that job.
     For now execute the below code only for non-preemtive cases */

	/* For non pre-emptive schemes assign the new job to the first available core */
    if (g_scheduler_scheme == FCFS || g_scheduler_scheme == SJF || g_scheduler_scheme == PRI) {
        for (i = 0; i < g_ncores; i++) {
            if (g_core_usage_array[i] == 0) {
                new_job->core_id = i;		/* Assign new job to the first avaiable core */
                g_core_usage_array[i] = 1; 	/* Mark the core assigned core as busy */
                break;
            }
        }
    }

    qhead_job = priqueue_peek(&g_jobs_queue); /* check current job which is at the head and running */

	switch (g_scheduler_scheme) {
		/* Non pre-emptive schemes */
		case FCFS:
		case SJF:
		case PRI:
			priqueue_offer(&g_jobs_queue, new_job);

			if (qhead_job == NULL){
				new_job->is_qhead = 1;
				new_job->start_time = time;
				ret_core = new_job->core_id; /* NB */
				goto exit;
				/* return 0; */
			}
			else {
				/* return -1; */
				ret_core = new_job->core_id; /* NB */
				goto exit;
			}

		break;

		/* Pre-emptive schemes */
		case PSJF:
			//printf ("\n"); scheduler_show_queue();/* NB debug */
			// FOR PREEMPTIVE
			/* NB Calculate the statistics for current running job which is at the head of the queue */
			if (qhead_job != NULL){
				qhead_job->remaining_time = (qhead_job->remaining_time) - (time - qhead_job->idle_time); 
			}

			/* Add new job to the queue */
			qindex_offered/*ret_core*/ = priqueue_offer(&g_jobs_queue, new_job); /* NB returns index in queue where it was stored */
			/* New job might have altered the head of the queue, so need to peek again */
			//printf ("\n"); scheduler_show_queue();/* NB debug */
			/* NB qindex_offered will give idea if job is at the head then PREMPTION WILL take place and core reassigned to new job */
			if (qhead_job == NULL) {
				// the core number /* NB case queue is empty and new job arrives. TODO remove below code and add code to make adding to queue more generic */
				new_job->start_time = time; 
				new_job->is_qhead = 1;/* Set as the head */
				new_job->idle_time = time;
				/* Set core, same code as above TODO optimise */
				for (i = 0; i < g_ncores; i++) {
					if (g_core_usage_array[i] == 0 /*&& new_job->core_id < 0*/) {
						new_job->core_id = i;/* NB Assign new job to the first avaiable core */
						g_core_usage_array[i] = 1; /* NB Mark the core as busy */
						break;
					}
				}
				ret_core = new_job->core_id;
				/*ret_core = 0;*/
				goto exit;
			}
			else if (qhead_job != NULL && (qhead_job->remaining_time > run_time)/* && g_ncores == 0*/) {
				qhead_job_new = priqueue_peek(&g_jobs_queue);
				//printf ("\nhead Old: job number %d, core %d", qhead_job->job_number, qhead_job->core_id);
				//printf ("\nhead new: job number %d, core %d", qhead_job_new->job_number, qhead_job_new->core_id);
				if (qhead_job->job_number != qhead_job_new->job_number) {
					//printf ("New job will replace prev running job \n");
					new_job->core_id = qhead_job->core_id;
					/*===== Premption of the currently running job =====*/
					qhead_job->core_id = -1;
					//qhead_job->idle_time = time; /* NB TODO */
				}

				new_job->start_time = time;
				new_job->idle_time = time;
				if (new_job->job_number == qhead_job_new->job_number) /* NB check if new job is is_qhead of queue */
					new_job->is_qhead = 1;
				//new_job->is_qhead = 1;/* NB might be or might not be head. TODO */
				// ODD CASE
				if (qhead_job->start_time == time) {
					qhead_job->start_time = -1;
				}
				ret_core = new_job->core_id;/* NB TODO TEST core_id already set above as new_job->core_id */
				goto exit;
				//return 0;
			} else if (qhead_job != NULL /*&& (qhead_job->remaining_time > run_time)*/ && g_ncores > 0) {
				/* NB For multiple cores : ==== Trying to make it generic for all number of cores ==== */
				/**/
				if (qindex_offered == 0) {
					;/* Head of queue.. prepare for premption */    
				}

			}
			else {
				ret_core = -1;
				goto exit;
			}
		break;

		case PPRI:
			qindex_offered = priqueue_offer(&g_jobs_queue, new_job);

			/* NB TODO use qindex_offered to set head of queue */

			if (qhead_job == NULL){
				// the core number 
				new_job->start_time = time; 
				new_job->idle_time = time;

				/* NB TODO :now hardcoding for single core */
				new_job->is_qhead = 1;
				new_job->core_id = 0;
				ret_core = 0;
				g_core_usage_array[ret_core] = 1;
				goto exit;
			}
			else if (qhead_job != NULL && (qhead_job->priority > priority)) {
				/* If new job's priority is greater than the running job 
				 * then premption will happen. We can check that by qindex_offered
				 * after inserting the new job. Zero value indicates new job has the
				 * the highest priority and will preempt lowest priority job among those
				 * running in all the cores combined? TODO check for mutiple cores */
				new_job->start_time = time;
				new_job->idle_time = time; 
				new_job->is_qhead = 1;

				if (qindex_offered == 0) {
					qhead_job_new = priqueue_peek(&g_jobs_queue);
					//printf ("\nhead Old: job number %d, core %d", qhead_job->job_number, qhead_job->core_id);
					//printf ("\nhead new: job number %d, core %d", qhead_job_new->job_number, qhead_job_new->core_id);
					//printf ("New job will replace prev running job \n");
					new_job->core_id = qhead_job->core_id;
					/*===== Premption of the currently curring job =====*/
					qhead_job->core_id = -1;
				}

				if (qhead_job->start_time == time){
					qhead_job->start_time = -1; 
				}

				/* NB TODO :now hardcoding for single core */
				new_job->is_qhead = 1;
				new_job->core_id = 0;
				/*ret_core = new_job->core_id;   NB TODO Enable for multi-core   */
				ret_core = 0;
				g_core_usage_array[ret_core] = 1;
				/* NB TODO :End */
				goto exit;
			}
			else {
				ret_core = -1;
				goto exit;
			}    
		break;
		
		default:
			qindex_offered = priqueue_offer(&g_jobs_queue, new_job);
			if (qhead_job == NULL) {
				new_job->start_time = time;

				/* NB TODO :now hardcoding for single core */
				new_job->is_qhead = 1;
				new_job->core_id = 0;
				/*ret_core = new_job->core_id;   NB TODO Enable for multi-core   */
				ret_core = 0;
				g_core_usage_array[ret_core] = 1;
				/* NB TODO :End */

				goto exit;
			}
			ret_core = -1;
			goto exit;
		
	}

exit:
    return ret_core;
}


/**
  Called when a job has completed execution.

  The core_id, job_number and time parameters are provided for convenience. You may be able to calculate the values with your own data structure.
  If any job should be scheduled to run on the core free'd up by the
  finished job, return the job_number of the job that should be scheduled to
  run on core core_id.

  @param core_id the zero-based index of the core where the job was located.
  @param job_number a globally unique identification number of the job.
  @param time the current time of the simulator.
  @return job_number of the job that should be scheduled to run on core core_id
  @return -1 if core should remain idle.
  */
int scheduler_job_finished(int core_id, int job_number, int time)
{
    int i, q_size;/* NB - i is index_fin_job */
    job_t *temp, *poll, *qhead_job;

    q_size = priqueue_size(&g_jobs_queue);

    /* Free up the core occupied by finished job & find its index in queue */
    for (i = 0; i < q_size; i++) {
        temp = (job_t *)priqueue_at(&g_jobs_queue, i);
        if (temp->core_id == core_id && temp->job_number == job_number) {
            /* Set core as free */
            g_core_usage_array[core_id] = 0;
            break;
        }
    }

    /* Remove the finished job from the queue. Using index to locate it in queue, which was found in previous step */
    if (i == 0) {
        poll = priqueue_poll(&g_jobs_queue);/* NB Remove at head */
    } else
        poll = (job_t *)priqueue_remove_at(&g_jobs_queue, i);

    if (poll != NULL) { /* NB TODO crashing for RR# */
        g_waiting_time += ((time - poll->arrival_time) - poll->run_time); 
        g_turnaround_time += time - poll->arrival_time; 
        g_response_time += (poll->start_time - (poll-> arrival_time));
    }

    /* NB: should return job_number of job to run on core_id  */
    /* NB: Remove this. Set core where it was running as idle */
    if (core_id == poll->core_id) {
        g_core_usage_array[core_id] = 0; /*NB Not need anymore due to condition above */
    }

    free(poll);

    qhead_job = priqueue_peek(&g_jobs_queue); 
    /* Not need peek because for multiple cores the job at the
     * head of the queue will not be the one next to run. priqueue_peek gets the  head of queue */

    /* NB Assign core_id to the next job in the p-queue */
    /* Should this be within some case below ?? */
    /* NB Search the queue by index. Look for firt job with core_id as -1 and assign it to core_id freed above */
    for (i = 0; i < q_size; i++) {
        temp = (job_t *)priqueue_at(&g_jobs_queue, i);/* NB check i, it crashes function for some values when i = 1,and its last element in queue*/

        if (temp != NULL) {
            if (temp->core_id < 0) {
                temp->core_id = core_id;
                /* Set core as busy */
                g_core_usage_array[core_id] = 1;
                break;
            }
        }
    }

    qhead_job = temp;

    if (g_scheduler_scheme == FCFS || g_scheduler_scheme == SJF || g_scheduler_scheme == PRI) {
        if (qhead_job == NULL){
            return -1; 
        }
        else {
            qhead_job->is_qhead = 1; 
            qhead_job->start_time = time;
            return qhead_job->job_number; 
        }
    }
    else if(g_scheduler_scheme == PSJF){
        if (qhead_job == NULL){
            return -1; 
        }
        else{
            qhead_job->idle_time = time; 
            qhead_job->is_qhead = 1;/* NB TODO CHECK Will be head only when its first in queue !!*/ 
            if (qhead_job->start_time == -1){
                qhead_job->start_time = time;
            }
            return qhead_job->job_number; 
        }
    }
    else if (g_scheduler_scheme == PPRI){
        if (qhead_job == NULL){
            return -1; 
        }
        else{
            qhead_job->idle_time = time; 
            qhead_job->is_qhead = 1; 
            if (qhead_job->start_time == -1){
                qhead_job->start_time = time;
            }
            return qhead_job->job_number; 
        }
    }
    else {
        if (qhead_job == NULL){return -1;}
        else {
            if (qhead_job->start_time == -1){qhead_job-> start_time = time;}      
            return qhead_job-> job_number; 
        }
    }
}


/**
  When the scheme is set to RR, called when the quantum timer has expired
  on a core.

  If any job should be scheduled to run on the core free'd up by
  the quantum expiration, return the job_number of the job that should be
  scheduled to run on core core_id.

  @param core_id the zero-based index of the core where the quantum has expired.
  @param time the current time of the simulator. 
  @return job_number of the job that should be scheduled on core cord_id
  @return -1 if core should remain idle
  */
int scheduler_quantum_expired(int core_id, int time)
{
    job_t * qhead_job = priqueue_poll(&g_jobs_queue); /* NB Revome the job which quantume expired from head of queue. Next job to run will be returned */

    if (qhead_job == NULL) {
        return -1;
    }
    else {

        /*===== Premption of the currently running job =====*/
        qhead_job->core_id = -1;
        /* NB Insert the quantum expired job at the end of the queue */
        priqueue_offer(&g_jobs_queue, qhead_job);/* NB Store the removed job at the end of the queue */
        qhead_job = priqueue_peek(&g_jobs_queue);/* Get the job at head of queue to schedule to run */ 
        if (qhead_job == NULL) {
            return -1;
        }
        else {
            /* NB Assing the freed up core to the new job */
            qhead_job->core_id = core_id;
            if (qhead_job->start_time == -1) {
                qhead_job->start_time = time;
            }
            return qhead_job->job_number;
        }
    }

    return -1;
}


/**
  Returns the average waiting time of all jobs scheduled by your scheduler.

Assumptions:
- This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
@return the average waiting time of all jobs scheduled.
*/
float scheduler_average_waiting_time()
{
    return ((float)(g_waiting_time))/g_num_jobs;
}


/**
  Returns the average turnaround time of all jobs scheduled by your scheduler.

Assumptions:
- This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
@return the average turnaround time of all jobs scheduled.
*/
float scheduler_average_turnaround_time()
{
    return ((float)(g_turnaround_time))/g_num_jobs;
}


/**
  Returns the average response time of all jobs scheduled by your scheduler.

Assumptions:
- This function will only be called after all scheduling is complete (all jobs that have arrived will have finished and no new jobs will arrive).
@return the average response time of all jobs scheduled.
*/
float scheduler_average_response_time()
{
    return ((float)(g_response_time))/g_num_jobs;
}


/**
  Free any memory associated with your scheduler.

Assumptions:
- This function will be the last function called in your library.
*/
void scheduler_clean_up()
{

}


/**
  This function may print out any debugging information you choose. This
  function will be called by the simulator after every call the simulator
  makes to your scheduler.
  In our provided output, we have implemented this function to list the jobs in the order they are to be scheduled. Furthermore, we have also listed the current state of the job (either running on a given core or idle). For example, if we have a non-preemptive algorithm and job(id=4) has began running, job(id=2) arrives with a higher priority, and job(id=1) arrives with a lower priority, the output in our sample output will be:

  2(-1) 4(0) 1(-1)  

  This function is not required and will not be graded. You may leave it
  blank if you do not find it useful.
  */
void scheduler_show_queue()
{
    int i, q_size;
    job_t * temp;
    q_size = priqueue_size(&g_jobs_queue);

    for (i = 0; i < q_size; i++) {
        temp = (job_t *)priqueue_at(&g_jobs_queue, i); 
        printf("%d(%d) ",temp->job_number, temp->core_id);/* NB */
    }
}
