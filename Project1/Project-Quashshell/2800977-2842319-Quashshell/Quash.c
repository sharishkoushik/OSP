
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

#define PATH "set path=" 

//This structure is used to store a particular job
struct jobstructure{
	pid_t pid;
	int jobid;
	int state;
	char *inputcmd;
};
struct jobstructure job_list[20];		//array used to store a list of jobs, maximum number of jobs are limited to 20


void sig_tstp(int signo);
void listjobs(struct jobstructure *job_list,int fd);
void clearjob(struct jobstructure *job);
int maxjobid(struct jobstructure *job_list);
void initjobs(struct jobstructure *job_list);
void addjob(struct jobstructure *job_list,pid_t pid, int state,char *input);
void deletejob(struct jobstructure *job_list,pid_t pid);
struct jobstructure *getjob(struct jobstructure *job_list,pid_t pid);
struct jobstructure *getjobjobid(struct jobstructure *job_list,char *inputargs[]);
void sighndle(int signum);
void handle_sigchild(int signum);
void splitargs(char* cmd, char *inputargs[]);
void splitargspath(char* cmd, char *args[]);
//char* readLine(char* line, size_t len)
void run(char *input,char *inputargs[],int bckgrnd);
void changedir(char *directory);
int chksymbol(char *input,char symbol);
void runpipe(char *inputargs[],char *inputargs2[]);
void execute(char *input, int bckgrnd);
void in_redirection(char *input);
void out_redirection(char *input, int k);
void if_pipe(char *input, int i);
void runmultipipe(char *input,char *inputargs[]);


int nextjob=1;  	//used for job id
extern int errno;
int error=0;
int status;
int fdout,fdin;		//filedescriptors used during redirection
char *cwdtemp= "";


int main(int argc, char *argv[],char *envp[]){
  char *input=NULL;	//stores user input
  
  
  int outredirection,inredirection,pipe, bckgrnd;
  pid_t pid3,pid4;
  char *cwd;
  char *inputargs[100];
  
 // strcpy(buf, input);

  input = (char *) malloc (100);
  while (1) {
 
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTSTP, sig_tstp);
  signal(SIGINT,SIG_IGN);           //capture SIGINT signal and ignore its default function.
  signal(SIGINT,sighndle);	  //capture SIGINT signal and redirect to handle_signal. 
   signal(SIGCHLD,handle_sigchild);
	
  signal(SIGCHLD,handle_sigchild);
    //printf("Quash$");
    //input = readLine(input,100);
	input = readline("Quash$"); // Implemented from the GNU read line library
	
	if(strcmp(input, "")!=0){
	  add_history(input);
	}
	
    bckgrnd = chksymbol(input,'&');
    if(bckgrnd !=0){
      input[bckgrnd] = '\0';
    }
    inredirection=chksymbol(input,'>');		//checking if output redirection is present in command

	if (inredirection != 0)
	{
	in_redirection(input);
	
	}
    outredirection=chksymbol(input,'<');	//checking if output redirection is present in command
    if (outredirection != 0)
	{
		out_redirection(input, outredirection);
	}
	pipe = chksymbol(input, '|');
	 if(pipe != 0)
	{
		if_pipe(input, pipe);
		
	}
	/*if(strcmp(input, "find") == 0)
	{
		multipipe(input, inputargs);
	}*/
    else {
    execute(input,bckgrnd);
	}
  }
  free(input);
}	
/*void runmultipipe(char *input, char *inputargs[])
{
	//printf("find |sort -n |head -n 2\n");
	splitargs(input, inputargs);
	printf("%s", inputargs[1]);
}*/

//function used to splitargs the user command, check for pipe and execute the command
void execute(char *input, int bckgrnd){		//function to execute a command
  char *input2=NULL;
  char *inputtemp=NULL;
  char *inputargs[100];  		//array to save the user input after splitargsing by spaces
  char *inputargs2[50]; 
  char *usrcmdsargspath[50];  
  int k =1 ;
  char *s;
  char *j;	
 
  pid_t pid6;
  struct jobstructure *tempjob_list;
  inputtemp=(char *) malloc (100 + 1);
  char *tok;

    strcpy(inputtemp,input);
    splitargs(input, inputargs);
    if(inputargs[0]){		//checking for \n condition, when user is pressing enter.
      if(strcmp(inputargs[0],"cd") == 0){
        changedir(inputargs[1]);
      }
	 
	else if((strcmp(inputargs[0],"echo"))  == 0)
	  {
		while(inputargs[k]!= NULL)
		{
			if(strchr(inputargs[k],'$'))
			{
		      s = getenv(++inputargs[k]);
		      printf("%s ",(s!=NULL)? s : "getenv returned NULL");
			}
			else
			{
			  printf("%s ", inputargs[k]);
			}
		    k++;
		}			
	  printf("\n");
	  }
	 
	 /*else if(strcmp(inputargs[0],"kill") == 0){
		tempjob_list=getjobjobid(job_list,inputargs);
		kill(tempjob_list->pid,9);	
	  }*/
	  
      else if((strcmp(inputargs[0], "exit") == 0) || (strcmp(inputargs[0], "quit") == 0)) { 
		printf("bye\n");
        exit(0);
      }
	  
	  	else if((strncmp(inputargs[0],"HOME=",5))==0)
	{
	char *path;
	path = strpbrk(inputargs[0],"=");
	++path;
	setenv("HOME",path,1);
	}
	  
	else if((strncmp(inputargs[0],"PATH=",5))==0)
	{
	char *path;
	path = strpbrk(inputargs[0],"=");
	++path;
	setenv("PATH",path,1);
	} 
	  
	 else if(strcmp(inputargs[0], "set") == 0) { 
		splitargspath(inputtemp,usrcmdsargspath);
	
		 if((strncmp(inputargs[1],"HOME=",5))==0){
			 char *path;
			 path = strpbrk(inputargs[1],"=");
	         ++path;
	         setenv("HOME",path,1);
			if((setenv("HOME",path,1))==-1){
				printf("could not set the given path to HOME");
			}
		}
		else if((strncmp(inputargs[1],"PATH=",5))==0){
			char *path;
	        path = strpbrk(inputargs[1],"=");
	        ++path;
	        setenv("PATH",path,1);
			if((setenv("PATH",path,1))==-1){
				printf("could not set the given path to PATH");
			}
		}
      }
	  else if(strcmp(inputargs[0], "jobs") == 0) {   
	  listjobs(job_list,STDOUT_FILENO);
		
      }
      else{
        run(inputtemp,inputargs, bckgrnd);
      }
    }

}

void in_redirection(char *input){
	pid_t t;
       t=fork();
	
       if(t==0)
		   execlp("/bin/sh","/bin/sh","-c",input,(char*)0);
	else
        waitpid(t,&status,0);
free(input);	
}	

void out_redirection(char *input, int k){
  char *filename=NULL;
  pid_t pid3,pid4;
 char *inputargs[100];
  int bckgrnd;		//used to determine a background process
  FILE *fp;
  char *s;

	      filename=&input[k+1];
      input[k]='\0';			//execute the input redirection block	
      if(strcmp(input,"quash") == 0){	//block used to read commands from a file and execute them
        fp = fopen(filename, "r");
        while(fgets(input, 1024, fp) != NULL){
		  s=strchr(input,'\n');
		  if(s != NULL){			// if \n is present in the command, then remove it and replacing by \0
          input[strlen(input)-1] = '\0';
		  }
	  execute(input,bckgrnd);
        }
	fclose(fp);
      }
     else{				//block used to perform input redirection operation
        pid4=fork();	
        if(pid4 == 0){	
          fdin = open(filename,O_RDONLY);
          dup2(fdin, STDIN_FILENO);
          close(fdin);
          splitargs(input, inputargs);
          if(inputargs[0]){		//checking for \n condition, when user is pressing enter.
            if(strcmp(inputargs[0],"cd") == 0){
              changedir(inputargs[1]);
            }
            else if((strcmp(inputargs[0], "exit") == 0) || (strcmp(inputargs[0], "quit") == 0)) {  
              exit(0);
            }
            else{
              error=execvp(inputargs[0],inputargs);
              printf("errno is %d\n", errno);
			  if(error == -1){
				printf("command do not exist\n");
				//return EXIT_FAILURE;
			  }
            }
          }
		  exit(0);
        }
        else if(bckgrnd !=0){
          printf("[1] %d\n",pid4);
        }
        else{
          waitpid(pid4,&status, 0);
        }
		free(input); // Should verify.
      }
	
}
void if_pipe(char *input, int i){
	
  char *input2=NULL;
  char *input3=NULL;
  char *inputtemp=NULL;
  char *inputargs[100];  		//array to save the user input after splitargs by spaces
  char *inputargs2[50]; 
  char *inputargs3[50];
  char *usrcmdsargspath[50];  

  pid_t pid6;
  
  input2 = &input[i+1];
    input[i]='\0';
    splitargs(input, inputargs);
    splitargs(input2, inputargs2);
	//splitargs(input3, inputargs3);
	pid6=fork();
    if(pid6 == 0){             //creates a child process
      runpipe(inputargs, inputargs2);
    }
    else{
	  waitpid(pid6,&status,0);         //wait till the child process terminates	  
    }
	
}

//function used to display listjobstructure  of jobs to the user
void listjobs(struct jobstructure *job_list,int fd){
	int i;
	char buf[1024];
	for(i=0;i<20;i++){
		memset(buf,'\0',1024);
		if(job_list[i].pid != 0){
			sprintf(buf,"[%d] %d ",job_list[i].jobid,job_list[i].pid);
			write(fd,buf,strlen(buf));
			memset(buf,'\0',1024);
			sprintf(buf,"%s\n",job_list[i].inputcmd);
			write(fd,buf,strlen(buf));
		}
	}
}

//function used to clear the jobs list
void clearjob(struct jobstructure *job){
	job->pid=0;
	job->jobid=0;
	job->state=0;
	job->inputcmd='\0';
}

//function returns maxjobid from the job list
int maxjobid(struct jobstructure *job_list){
	int i,max=0;
	for(i=0;i<20;i++){
		if(job_list[i].jobid>max)
			max=job_list[i].jobid;
	}
	return max;
}

//function used to initialize the job list
void initjobs(struct jobstructure *job_list){
	int i;
	for(i=0;i<20;i++){
		clearjob(&job_list[i]);
	}
}

// function used to add job to job list
void addjob(struct jobstructure *job_list,pid_t pid, int state,char *input){
int i;
for(i=0;i<20;i++){
		if(job_list[i].pid==0){
			job_list[i].pid=pid;
			job_list[i].state=state;
			job_list[i].jobid=nextjob++;
			job_list[i].inputcmd=input;
			if(nextjob>20){
			nextjob=1;
			}
		break;
		}
	}
}

// function used to delete job from job list
void deletejob(struct jobstructure *job_list,pid_t pid){
int i;
for(i=0;i<20;i++){
		if(job_list[i].pid==pid){
			clearjob(&job_list[i]);
			nextjob=maxjobid(job_list)+1;
			break;
			}
		}
}

//function used to find job based on pid
struct jobstructure *getjob(struct jobstructure *job_list,pid_t pid){
	int i;
	for(i=0;i<20;i++){
		if(job_list[i].pid== pid)
			return &job_list[i];
	}
	return &job_list[i];
}

//function used to find job based on jobid
struct jobstructure *getjobjobid(struct jobstructure *job_list,char *inputargs[]){
	int i;
	int j=atoi(inputargs[2]);
	for(i=0;i<20;i++){
		if(job_list[i].jobid== j)
			return &job_list[i];
	}
	return &job_list[i];
}

/*function used to handle the signal SIGINT, when user presses ctrl+c,
  we must still display quash>, not bash prompt*/
void sighndle(int signum){
  printf("\n%s", cwdtemp);
  printf("Quash$");
  fflush(stdout);
}
void sig_tstp(int signo){
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGTSTP);
  sigprocmask(SIG_UNBLOCK, &mask, NULL);
  signal(SIGTSTP, SIG_DFL);
  kill(getpid(), SIGTSTP);
  signal(SIGTSTP, sig_tstp);
  return;
}

//function used to handle SIGCHILD and delete the job from job list
void handle_sigchild(int signum){
  int status;
  pid_t pid;
  struct jobstructure *tempjob_list;
  while((pid = waitpid(-1,&status,WNOHANG|WUNTRACED)) > 0){
	if(WIFEXITED(status)){		//checks if child process is terminated normally
		tempjob_list=getjob(job_list,pid);
		printf("[%d] %d finished %s\n",tempjob_list->jobid,tempjob_list->pid,tempjob_list->inputcmd);
		deletejob(job_list,pid);	//if child terminates normally, delete it from the jobs list
	}
	else if(WIFSIGNALED(status)){
		deletejob(job_list,pid);
	}
	else if(WIFSTOPPED(status)){
		tempjob_list=getjob(job_list,pid);
		tempjob_list->state=3;
	}
  }
}

/*function used to splitargs the command cmd by space(" ") and store them in an array 
  so that we can pass array to execvp command to execute the command*/
void splitargs(char* cmd, char *inputargs[]){
  char *p;
  int j=0;
  p=strtok(cmd," ");  		//tokanize the user input command by using space(" ") as delimiter and save it in p.
  while(p != NULL)
  {
    inputargs[j]=p;
    j++;
    p=strtok(NULL," ");		
  }
  inputargs[j]='\0';
}

void splitargspath(char* cmd, char *args[]){
  char *p;
  int j=0;
  p=strtok(cmd,"=");  		//tokanize the user input command by using space(" ") as delimiter and save it in p.
  while(p != NULL)
  {
    args[j]=p;
    j++;
    p=strtok(NULL," ");		
  }
  args[j]='\0';
}
/*function used to read a line */

/*char* readLine(char* line, size_t len){ 
  getline(&line, &len, stdin);            
  *(line+strlen(line)-1)='\0';
  return line;
}*/

/*function used to run user command using execvp*/
void run(char *input,char *inputargs[],int bckgrnd){
  int pid1;
  pid1=fork();
  struct jobstructure *tempjob_list;
  if(pid1 == 0){
    close(fdin);
    close(fdout);
    error=execvp(inputargs[0],inputargs);	
    printf("errno is %d\n", errno);
    if(error == -1){
      printf("command do not exist\n");	
		exit(EXIT_FAILURE);
    }
    exit(0);
  }
  else if(bckgrnd != 0){
	addjob(job_list,pid1,2,input);
	tempjob_list=getjob(job_list,pid1);
    printf("[%d] %d running in background\n",tempjob_list->jobid,pid1);
  }
  
  else{
    waitpid(pid1,&status, 0);
  }
}

//function used to changedirectory
void changedir(char *directory){
 // printf("Directory is %s", directory);
  if(!(directory)){
    chdir(getenv("HOME"));
  }
  else{
    if(chdir(directory) == -1){
      printf("Invalid Directory path\n");
    }
  }
}

//function used to check if the command consists of the second parameter passed to function
int chksymbol(char *input,char symbol){
  int i=0;
  int returnvalue=0;
  for(;i<strlen(input);i++){
    if(input[i] == symbol){
      returnvalue=i;
      break;
    }
  }
  return returnvalue;
}

void runpipe(char *inputargs[],char *inputargs2[]){
  int fd[2];	
  pipe(fd);
  int pid2;
  pid2=fork();
  if(pid2 == 0){
    dup2(fd[1],STDOUT_FILENO);
    close(fd[0]);
    close(fd[1]);
    execvp(inputargs[0],inputargs);
    exit(0);
  }
  else{
    dup2(fd[0],STDIN_FILENO);
    close(fd[1]);
    close(fd[0]);
    waitpid(pid2,&status, 0);
    execvp(inputargs2[0],inputargs2);
  }
}




