/*
**Name: Mark Ruoff
**Assignment: smallsh
**Date Due: 11/15/17
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
//prototypes
void shellGetInput();//Used to get in inputs from user
void inputInter(char* command);
//Some Gloal Vraibles
int endProg=0;//a "flag" to keep track if the pgoram should be ended"i
int foregroundonly=0;
int currentid=-5;
int statuscheck;
int bgarray[20];
int bgnum=0;
//signal handling
void catchSIGINT(int signo)
{
char message[35];
memset(message,'\0',35);
sprintf(message, "Process Terminated by signal %d\n", signo);
write(1,message,35);
if(currentid>0)
{
  kill(currentid, SIGKILL);
}
}

void catchSIGSTP(int signo)
{
if(foregroundonly)
{
  char* outmessage="Exiting foreground only mode\n";
  write(1, outmessage,28);
  foregroundonly=0;//switch flag
}
else
{
  char* entermessage="Entering foreground only mode(ignoring $)\n";
  write(1, entermessage,42);
  foregroundonly=1;
}

}
int main()
{
//printf("%d\n",getpid()); used for initial debugging
//signal set ups
struct sigaction SIGSTP_action={0}, SIGINT_action={0};
SIGSTP_action.sa_handler=catchSIGSTP;
sigfillset(&SIGSTP_action.sa_mask);
SIGSTP_action.sa_flags=0;
SIGINT_action.sa_handler=catchSIGINT;
sigfillset(&SIGINT_action.sa_mask);
SIGINT_action.sa_flags=0;
sigaction(SIGTSTP, &SIGSTP_action, NULL);
sigaction(SIGINT, &SIGINT_action,NULL);
//starting the shell
while(!endProg)
{
shellGetInput();
//check background processes before next loop
int bgexit;
int bgid;
bgid=waitpid(-1,&bgexit,WNOHANG);//checkts on child background processes if finished returns status
if(bgid!=0 && bgid !=-1)
{
   if(WIFEXITED(bgexit))
	{
	  int exitstatus=WEXITSTATUS(bgexit);
	  printf("Process: %d exit status was %d\n",bgid,bgexit);
	}
   else if(WIFSIGNALED(bgexit))
	{
		printf("Process: %d terminated by a signal ", bgid);
		int termSignal=WTERMSIG(bgexit);
		printf("Signal was %d\n",termSignal);
	}
}
}
return 0;
}

//implementation

void shellGetInput()
{
char *input=NULL;//taken from lectures in block 2, to automatically allocate proper space
size_t bufferSize=0;
int numEntered=-5;
fflush(stdin);
fflush(stdout);
printf(":");
numEntered=getline(&input,&bufferSize,stdin);//gets length of line andputs stdin into input
if(numEntered==-1)
{
  clearerr(stdin);//if signal call doesn't cause error with rest of program and cause seg fault
  free(input);
}
else
{
input[numEntered-1]='\0';//clear out that pesky new line character
inputInter(input);//send string to be interpreted
free(input);//frees memoryfrom getline call
}
}

void inputInter(char* command)
{
//Need to break command into seperate arguments
if(command[0]=='#' || strlen(command)==0)
{
   return;
}
else
{
char*parameter=strtok(command," ");
//printf("%s\n",parameter); line was used for debugging
//Built in functions
if(strcmp(parameter, "exit")==0)//if exit is in the command list
{
  printf("Exiting\n");
  endProg=1;
  int index;
  for(index=0;index<bgnum;index++)
	{
		kill(bgarray[index],SIGKILL);//Kill any background processes
	}
  return;  
}
else if(strcmp(parameter, "cd")==0)//change directories
{
//if no other parameters go to HOME
char*direc=strtok(NULL, " ");
if(direc==NULL)
{
  char *home=getenv("HOME");
  chdir(home);  
}

// parameters go to proper directory
else
{
//modified from http://www.cplusplus.com/reference/cstring/strstr/
 char*pch;
 pch=strstr(direc,"$$");
 if(pch!=NULL)
 {
   sprintf(pch, "%d",getpid());//repalces $$ with pid
 } 
 if(chdir(direc)==-1)
  {
	perror("ERROR CHANGING DIRECTORIES");
  }

}
   return;
}
else if(strcmp(parameter, "status")==0)//checks exit status and term signal taken setps taken from lecture slides
{
   if(WIFEXITED(statuscheck))
	{
	   int exitStatus=WEXITSTATUS(statuscheck);
	   printf("Exit status was %d \n", exitStatus);
	}
   else if(WIFSIGNALED(statuscheck))
	{
		printf("Child terminated by a signal");
		int termSignal=WTERMSIG(statuscheck);
		printf("Signal was %d\n",termSignal);
	}

   return;
}
else//functions not built in
{
  char *arguments[512];
   int index=0;
   //flags for if backgound or if there are in or outfiles
   int background=0;
   int infile=0;
   int outfile=0;
   int filenumin;
   int filenumout;
   int redirect;
   int childexitmethod;
   char *inputF;
   char *outputF;
   pid_t forkid;
  while(parameter!=NULL)
  {
      //steps taken from  http://www.cplusplus.com/reference/cstring/strstr/
      if(strcmp(parameter,"<")==0)
	{
	  inputF=strtok(NULL," ");//set up input file if it exits
          parameter=strtok(NULL, " ");
          infile=1;
	}
     else if(strcmp(parameter,">")==0)
	{
	  outputF=strtok(NULL, " ");//set output file
          parameter=strtok(NULL, " ");
          outfile=1;
	}
     else
	{
	 char*pch2=strstr(parameter,"$$");
	 if(pch2!=NULL)
	 {
	   char filler[100]; //create a seperate string so we don't overtie the orgiansl when replaceing $$
	    memset(filler,'\0',100);
	   strcpy(filler, parameter);
	   char*pch3=strstr(filler,"$$");
	   sprintf(pch3,"%d",getpid());
 	   arguments[index]=filler;
 	    index++;
           parameter=strtok(NULL, " ");
	 }
	 else{
     	 arguments[index]=parameter; //fill parameter arrway if only words
     	 index++;
     	 parameter=strtok(NULL," ");
         }
	}
  }
  int numberof=index;
  if(strcmp(arguments[numberof-1],"&")==0) //iof final word of arguments is & then set as background
	{
 	   if(!foregroundonly){
	   background=1;
	   arguments[numberof-1]=NULL;
	   numberof--;
	   }
	  else
	  {
		arguments[numberof-1]=NULL;
		numberof--;
	  }
	}
  else
 	{
	  arguments[numberof]=NULL;
	}
//at this point arguments is full of the  necessary commands, and proper flags are set, forking can now occur and call proper commands
  forkid=fork();
  //test if erros
  switch(forkid){
	  //from slides in lecture
	   case-1:perror("ERROR ERROR\n"); exit(1); break; //something went really wrong get out
 	  //if child
	   case 0: 
		 currentid=getpid();
		if(infile)
		{//file opening from lecture slides
		  filenumin=open(inputF,O_RDONLY);
		  if(filenumin==-1)
			{
				perror("COULD NOT OPENFILE");
				exit(1);
			}
		else
			{
				redirect=dup2(filenumin,0);//redirects input
				if(redirect==-1)
					{
						perror("ERROR REDIRECTING STDIN");
						exit(1);
					}
			}
		}
		if(outfile)
		{
			filenumout=open(outputF, O_WRONLY | O_CREAT,0744);
			if (filenumout==-1)
			   {
				perror("COULD NOT OPEN/CREATE OUTFILE");
				exit(1);
			   }
		        else
			   {
				redirect=dup2(filenumout,1);//redirects output
				if(redirect==-1)
					{
					      perror("ERROR REDIRECTING STDOUT");
						exit(1);
					}
			   }
		}
		if(background)
		 {//if no output or input files redirects to dev/null for background process
		   if(!infile)
		   {
		      filenumin=open("/dev/null",O_RDONLY);
		      dup2(filenumin,0);
	           }
		   if(!outfile)
		   {
		      filenumout=open("/dev/null",O_WRONLY);
		      dup2(filenumout,1);
		   }
		 }
		if(execvp(arguments[0],arguments)<0)
			{
			  perror("EXEC FAILURE!");
			  exit(1);
			}
		 
		
		break;
	default:
		//if process is parent check if should be background
		if(!background)
			{
				int waitingid;
				waitingid=waitpid(forkid,&statuscheck,0);
				
			}
		else
			{
				printf("Bacgkground Process %d working \n",forkid);
				bgarray[bgnum]=forkid;
				bgnum++;//fortracking background processes
			}
	}
}
}
}
