/*
 * Name: Alan Cleetus
 * 
 * This is a mini shell program that runs basic unix commands and 
 * also handles simple I/O redirection.  
 * This program has three built in commands: cd, pwd, and exit.
 * Until the user enters exit, the program keeps running.  
 * This program also runs processes in the background and will not exit
 * without closing the bg processes first.`
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#define MAXLINE 80
#define MAXARGS 20

void process_input(int argc, char **argv) {

   if(argc >2)
   {
	int i, posIn = 0, posOut = 0, redir = 0;

	//counts the number of redirctions
	//and also  gets the position of > and < in the input string
	for( i = 0; i<argc;i++)
	{
	   //gets the position of >
	   if(strcmp(argv[i],">")==0)
	   {
		redir++;
	   	posOut = i;
	   }
	   //gets the position of <
	   else if(strcmp(argv[i],"<")==0)
	   {
		redir++;
		posIn = i;
	   }
	}

	//returns of there are too many redirection symbols
	if(redir>1)
	{
	   printf("\tToo many redirections\n");
	   _exit(3);
	   //return;
	}

	//redirects output
	if(posOut !=0)
	{
	   int fileId = open(argv[posOut+1], O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR);
	   if(fileId<0)
	   {
		printf("\tError Creating file\n");
		_exit(3);
	   }
	   
	   dup2(fileId,1);//copy file id to stdout
	   close(fileId);
	   argv[posOut] = NULL;
	}
	
	//redirects input file
	if(posIn !=0)
	{
	   int fileId = open(argv[posIn+1],O_RDONLY,0);
	   
	   if(fileId<0){
		printf("\tError Creating file\n");
		_exit(3); 
	   }
	
	   dup2(fileId,0);
	   close(fileId);
	   argv[posIn] = NULL;
	}
   }
   
   //execute the commands
   if(execvp(argv[0],argv)==-1)
   {
      perror("Shell Program Error");
      _exit(-1);
   }
   else
   	execvp(argv[0],argv);
   
   return;
}

/* ----------------------------------------------------------------- */
/*                  parse input line into argc/argv format           */
/* ----------------------------------------------------------------- */
int parseline(char *cmdline, char **argv)
{
  int count = 0;
  char *separator = " \n\t";
  argv[count] = strtok(cmdline, separator);
  while ((argv[count] != NULL) && (count+1 < MAXARGS)) {
   argv[++count] = strtok((char *) 0, separator);
  }
  return count;
}

//Jobs structure
struct job
{
   int process_id;//process id
   char command[80];// command without the &
   int job_number;
};

struct job jobs_array[20];
int numberOfJobs = 0;

void childDone(int signal)
{ 
   int status;
   pid_t pid;
   pid = waitpid(-1,&status,WNOHANG);

   int x,flag=-1;
   for(x = 0;x<numberOfJobs;x++)
   {
	if(flag==0)
	{
	   jobs_array[x-1]=jobs_array[x];
        }

	if(jobs_array[x].process_id==pid)
	{
	   flag=0;
	   printf("\n\t[%d]Done %s\n",jobs_array[x].job_number,
			jobs_array[x].command);
	   printf("\tChild returned with status:%d\n",status);
	   fflush(stdout);
	   numberOfJobs--;
	}
   }
   if(numberOfJobs == 0)strncpy(jobs_array[0].command,"",1);
};

/* ----------------------------------------------------------------- */
/*                  The main program starts here                     */
/* ----------------------------------------------------------------- */
int main(void)
{
   char cmdline[MAXLINE];
   char *argv[MAXARGS];
   int argc;
   int status;
   pid_t pid;

   int isBackgroundJob;//true = 0, false = 1

   struct sigaction act;
   struct sigaction oact;
   sigaction(SIGTERM,NULL,&oact);//save old action

   act.sa_handler = SIG_IGN;	 //new action
   sigemptyset(&act.sa_mask);
   act.sa_flags = 0;
   sigaction(SIGINT, &act, 0);   //set new action;

   //child process done or exited
   if(signal(SIGCHLD, childDone)==SIG_ERR)
	_exit(0);

   while(1) 
   {
	//print prompt
        printf("csc60mshell> ");
	fgets(cmdline, MAXLINE, stdin);

	//parese input
    	argc = parseline(cmdline, argv); 
	
	//restart while loop if user simply pressed enter
  	if(argc == 0)
	{
	   continue;
   	}
	else
	{
	   //check if it is a background proc
	   if(strcmp(argv[argc-1],"&")==0)
	   {
		isBackgroundJob = 0;
		argv[ argc-1]=NULL;//removes &
		argc--;
		//strncpy(jobs_array[numberOfJobs].command,cmdline,strlen(cmdline));
		
		int m;
		for(m = 0; m<argc;m++){
		   strcat(jobs_array[numberOfJobs].command,argv[m]);
		   strcat(jobs_array[numberOfJobs].command," ");
		}//does not work?

		jobs_array[numberOfJobs].job_number = numberOfJobs+1;
	   }
	   else 
		isBackgroundJob = 1;

	   //checks and runs built in commands
	   if(strcmp(argv[0],"cd")== 0)
	   {
		// changes dir to home if user entered "cd"
		if(argc == 1)
		   chdir(getenv("HOME"));
		//changes directory to the path inputted
		else if(argc == 2)
		   chdir(argv[1]);
		else
		{
		   printf("\tError: Too many arguments.\n");
		   continue;
		}
	
		//update env
		char tempbuf[256];
		getcwd(tempbuf, 256);
		setenv("PWD", tempbuf, 1);
	   }
	   else if(strcmp(argv[0],"pwd") == 0)
	   {
		if(argc == 1)
		   printf("\t%s\n", getenv("PWD"));
		else
		   printf("\tError: Too many arguments.\n");
	   }
	   else if(strcmp(argv[0],"exit") == 0)
	   {
		if(numberOfJobs==0)exit(0);
		else 
		   printf("Close all background jobs before exiting\n");
		 
	   }
	   else if(strcmp(argv[0],"jobs")==0)
	   {
		if(numberOfJobs == 0)
		   printf("No jobs currently running.\n");

		else
		{
		   int i;
		   for(i = 0; i<numberOfJobs;i++)
		      printf("[%d]Running   %s\n",
				jobs_array[i].job_number,
					jobs_array[i].command);
		} 
	  }
	   // non built in commands
	  else
	  {
		//input validation
		if((strcmp(argv[0],">")==0))
		{
		   printf("\tError - No command.\n");
		   continue;
		}
		else if(strcmp(argv[0],"<")==0)
		{
		   printf("\tError - No command.\n");	
		   continue;
		}
		else if (strcmp(argv[(argc-1)],"<") ==0) 
	   	{
		   printf("\tError - No input file specified.\n");
		   continue;
		}
		else if(strcmp(argv[(argc-1)],">")==0)
		{
		   printf("\tError - No output file specified.\n");
		   continue;
		}//end input validation
		
		/*------------------------------------------------*/
		//if no errors input
		pid  = fork();
		if(pid == -1)
		   perror("\tShell Program: Fork Error\n");
		else if(pid == 0)//child
		{
		   if(isBackgroundJob==0) setpgid(0,0);
		   else sigaction(SIGINT,&oact,NULL);
		   process_input(argc, argv);
		   _exit(EXIT_SUCCESS);
		}
		else//parent waits if not background process;
		{
		   if(isBackgroundJob==0)
		   {
			jobs_array[numberOfJobs].process_id = pid;
		   	numberOfJobs++;
		   }
	    	   else
	           {
		      if(waitpid(-1,&status,0) == -1)
		         perror("\tShell Program Error\n");
		      else if(WIFSIGNALED(status))
		      {
		         printf("\tChild returned status: %d\n", 
				WTERMSIG(status));
		      }
		      else if(WIFSTOPPED(status))
		      {
			 printf("\tChild returned status:%d\n",
				WIFSTOPPED(status));
		      }
		      else if(WIFEXITED(status))
		      {
		         printf("\tChild returned status:%d\n",
				WEXITSTATUS(status));
		      }
		   }
						
		}//end for loop
	   }//end running non built in commands
   	   continue;
	}
   }//end while
}
