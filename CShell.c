#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>
#include <error.h>
struct process			// Structure storing Background process
{
	char process[20];
	int pid,flag;
}pro[1000];

int k,pro_id;
char *in1[100];

int check(char *wd,char *temp)		// checking current working with initial working directory
{
	int l1=strlen(wd),l2=strlen(temp),i=0;
	if(strcmp(wd,temp)==0)
		return -1;
	if(l2<l1)
		return 0;
	for(i=0;i<l1;i++)
	{
		if(wd[i]!=temp[i])
			return 0;
	}
	return l1;
}
int parse(char line[],char **argv)		// Parsing the input with " "
{
	int i=0;
	while(*line!='\0')
	{
		while(*line==' '||*line=='\t'||*line=='\n')
			*line++ = '\0';
		*argv++ = line;
		i++;
		while(*line!='\0' && *line!=' ' && *line!='\t' && *line != '\n')
			line++;
	}
	*argv = '\0';
	return i;
}

int parse_(char line[] , char **argv,char ch)		// Parsing the input with ch
{
	int i=0;
	while(*line!='\0')
	{
		while(*line==ch || *line==' ' || *line == '\t' || *line == '\n')
		{
			*line++='\0';
		}
		*argv++ = line;
		i++;
		while(*line!=ch && *line!='\n' && *line!='\0')
		{
			line++;
		}
		if(*line==ch)
			*(line-1)='\0';
	}
	*argv = '\0';
	return i;
}

void sig_handler(int signo)		// Signal Handler Function
{
	int i;
	if (signo == SIGUSR1)
		write(1,"received SIGUSR1\n",strlen("received SIGUSR1\n"));
	else if (signo == SIGKILL)
		write(1,"received SIGKILL\n",strlen("received SIGKILL\n"));
	else if (signo == SIGTSTP)
	{
		write(1,"received SIGSTOP\n",strlen("received SIGTSTP\n"));
		kill(pro_id,SIGTSTP);
	}
	else if (signo == SIGINT)
		write(1,"received SIGINT\n",strlen("received SIGINT\n"));
	else if(signo == SIGQUIT)
		printf("recieved SIGQUIT\n");
}

void pipe_redirect(char line[])
{
	char *temp = NULL, *pcom[100],*cmdarg[100],*ncmdarg[100];
	int len=0,l,newpipe[2],oldpipe[2],pcount=0,j,acount=0,i,status,ncount=0;
	FILE *in , *out;
	pid_t pid;
	pcount = parse_(line,pcom,'|');			// Parsing for no of pipes
	int stdin_c,stdout_c;
	stdin_c = dup(STDIN_FILENO);
	stdout_c = dup(STDOUT_FILENO);
	for(i=0;i<pcount;i++)
	{
		memset(cmdarg,0,sizeof(char *)*(100));
		memset(ncmdarg,0,sizeof(char *)*(100));
		if(i>=0 && i!=pcount -1)				// First pipe donot need input to come from anywhere but needs out go to pipe
		{
			acount=parse_(pcom[i],cmdarg,'<');
			ncount = parse(cmdarg[0],ncmdarg);
			if(cmdarg[1]!=NULL)
			{
				in = fopen(cmdarg[1],"r");
				if(in<0)
				{
					perror(cmdarg[1]);
					exit(1);
				}
				dup2(fileno(in),STDIN_FILENO);
			}
		}
		else if(i<=pcount-1 && i!=0)			// Last pipe have to take input from pipe but don't have to put the iutput in pipe
		{	
			acount=parse_(pcom[i],cmdarg,'>');	
			ncount = parse(cmdarg[0],ncmdarg);
			if(cmdarg[1]!=NULL)
			{
				out = fopen(cmdarg[1],"w");
				if(out<0)
				{
					perror(cmdarg[1]);
					exit(1);
				}
				dup2(fileno(out),STDOUT_FILENO);
			}
		}
		if(i<pcount-1)			// initiating a pipe for commands except last command
			pipe(newpipe);
		if(i>0 && i<=pcount-1)		// making dup of file pointer for input except for first command
		{
			dup2(oldpipe[0],0);
			close(oldpipe[1]);
			close(oldpipe[0]);
		}
		pid = fork();
		if(pid<0)
		{
			printf("Problem in implementing %s \n",ncmdarg[0]);
			exit(1);
		}
		else if(pid==0)
		{
			if(i>=0 && i<pcount-1)		// making dup of file pointer for out except for last command
			{
				dup2(newpipe[1],1);
				close(newpipe[0]);
				close(newpipe[1]);
			}
			int res = execvp(cmdarg[0],ncmdarg);
			if(res==-1)
				perror("Command can't be found");
			exit(1);
		}
		else
		{
			pro_id = pid;
			waitpid(pid,&status,0);
			if(i<pcount-1)
			{
				oldpipe[0]=newpipe[0];
				oldpipe[1]=newpipe[1];
			}
		}
	}
	close(newpipe[0]);
	close(newpipe[1]);
	dup2(stdin_c,0);
	dup2(stdout_c,1);
	close(stdin_c);
	close(stdout_c);
}

int main(void)
{
	signal(SIGUSR1, sig_handler);		// initializing signal handler
	signal(SIGKILL, sig_handler);		// initializing signal handler
	signal(SIGTSTP, sig_handler);		// initializing signal handler
	signal(SIGINT, sig_handler);		// initializing signal handler
	signal(SIGCHLD,sig_handler);		// initializing signal handler
	signal(SIGQUIT,sig_handler);		// initializing signal handler
	char *token,temp2[100],in2[100],temp[100],host[100],host1[100],in[100],host2[100],temp3[100],t1[128],t2[20],*out=NULL,*inp=NULL;
	sigset_t intmask;
	memset(in1,0, sizeof(char*) * (100));
	struct passwd *p = getpwuid(getuid());		// for getting Username of currently logged User
	int fl=0,i=0,j=0,len,temp1,pid,status,er,proid,old_stdin,old_stdout;k=0;
	gethostname(host1,sizeof(host1));		// Getting hostname of Computer
	sprintf(host,"<%s@%s:",p->pw_name,host1);
	strcpy(host1,"~");
	write(1,host,strlen(host));
	write(1,host1,strlen(host1));
	write(1,">",1);
	strcpy(temp2,"");
	getcwd(host2,100);
	gets(in);
	FILE *fp,*fp1,*fp2;
	while(strcmp(in,"quit")!=0)
	{
		strcpy(in2,in);
		int pid_=waitpid(-1,&status,WNOHANG);		// Checking for any background process finish
		while(pid_>1)
		{
			for(i=0;i<k;i++)
			{
				if(pro[i].pid == pid_)
				{
					pro[i].flag=0;
					printf("Process %s with PID[%d] exits\n",pro[i].process,pid_);
				}
			}
			pid_=waitpid(-1,&status,WNOHANG);
		}
		if(strlen(in)!=0)
		{
			i=0;len=0;j=0;er=0;fl=0;int pflag=0;out=NULL;inp=NULL;
			j = parse(in,in1);
			for(i=0;i<j;i++)
			{
				if(strcmp(in1[i],">")==0 || strcmp(in1[i],"<")==0)
					fl=1;
				if(strcmp(in1[i],"|")==0)
					pflag=1;
			}
			len = j ;
			if(strcmp(in1[0],"cd")==0)		// changing current directory
			{
				if(in1[1]!=NULL)
				{
					er = chdir(in1[1]);
					if(er==-1)
						perror("Error");
					else
					{
						getcwd(temp2,100);
						temp1 = check(host2,temp2);
						if(temp1 != 0 )
						{
							if(temp1==-1)
								strcpy(host1,"~");
							else
							{
								strcpy(host1,"~/");
								strcat(host1,temp2+temp1+1);
							}
						}
						else
							strcpy(host1,temp2);
					}
				}
				else
				{
					chdir(host2);
					strcpy(host1,"~");
				}
			}
			else if(strcmp(in1[len-1],"&")==0)	// checking for Background Process initiation
			{
				pid=fork();
				if(pid<0)
				{
					perror("Error");
					_exit(-1);
				}
				else if(pid==0)
				{
					in1[len-1]=NULL;
					er = execvp(in1[0],in1);
					if(er == -1)
					{
						perror("Error");
						_exit(-1);
					}
					else
						_exit(0);
				}
				else
				{
					if(er !=-1)
					{
						pro[k].pid = pid;
						pro[k].flag = 1;
						strcpy(pro[k].process,in1[0]);
						k++;
					}
				}
			}
			else if(strcmp(in1[0],"jobs")==0)	//Implementing Jobs command
			{
				j=1;
				for(i=0;i<k;i++)
				{
					if(pro[i].flag==1)
					{
						printf("[%d] %s [%d]\n",j,pro[i].process,pro[i].pid);
						j++;
					}
				}
			}
			else if(strcmp(in1[0],"kjob")==0)	// Implementing kjob command
			{
				int a=atoi(in1[1]),b=atoi(in1[2]),c;
				j=1;
				for(i=0;i<k;i++)
				{
					if(pro[i].flag==1)
					{
						if(j==a)
						{
							c=pro[i].pid;
						}
						j++;
					}
				}
				er = kill(c,b);
				if(er==-1)
					perror("Error");
				else
				{
					if(b==9)
						pro[i].flag=0;
				}
			}
			else if(strcmp(in1[0],"overkill")==0)		// Implementing overkill command
			{
				int e;
				for(i=0;i<k;i++)
				{
					if(pro[i].flag==1)
					{
						e = pro[i].pid;
						er = kill(e,9);
						if(er == -1)
							perror("Error");
						else
						{
							pro[i].flag=0;
						}
					}
				}
			}
			else if(strcmp(in1[0],"fg")==0)		// Implementing fg command
			{
				int f=atoi(in1[1]);j=1;
				for(i=0;i<k;i++)
				{
					if(pro[i].flag==1)
					{
						if(j==f)
						{
							waitpid(pro[i].pid,&status,0);
							pro[i].flag=0;
							break;
						}
						j++;
					}
				}
			}
			else if(strcmp(in1[0],"pinfo")==0 && len ==1)	// Implementing pinfo for shell
			{
				int count=0;
				char str[100],temp4[100];
				sprintf(temp3,"/proc/%d/status",getpid());
				sprintf(temp4,"/proc/%d/exe",getpid());
				fp = fopen(temp3,"r");
				if(fp!=NULL)
				{
					while ( fgets ( t1, sizeof(t1), fp ) != NULL )
					{
						if(count==12)
							break;
						if(count==1 || count==3 || count==11)
							fputs ( t1, stdout );
						count++;
					}
					fclose ( fp );
					readlink(temp4,str,sizeof(str));
					printf("Executable path -- %s\n",str);
				}
				else
					perror ("Can't open it" );
			}
			else if(strcmp(in1[0],"pinfo")==0 && len ==2)		// Implementing pinfo for a pid
			{
				int count=0;
				char str[100],temp4[100];
				sprintf(temp3,"/proc/%s/status",in1[1]);
				sprintf(temp4,"/proc/%d/exe",getpid());
				fp = fopen(temp3,"r");
				if(fp!=NULL)
				{
					while ( fgets ( t1, sizeof(t1), fp ) != NULL )
					{
						if(count==12)
							break;
						if(count==1 || count==3 || count==11)
							fputs ( t1, stdout );
						count++;
					}
					fclose ( fp );
					readlink(temp4,str,sizeof(str));
					printf("Executable path -- %s\n",str);
				}
				else
					perror ("Can't open it" );
			}
			else if(pflag==1 && fl==1)		//function call for pitpe + > / <
				pipe_redirect(in2);
			else if(pflag==1)			// function call for pipe only
				pipe_redirect(in2);
			else if(fl==1)				// for > / < in input
			{
				old_stdout = dup(1);		// copying file desc. of input and output to deafult them afterwards
				old_stdin = dup(0);
				for(i=0;i<len;i++)
				{
					if(strcmp(in1[i],">")==0)
					{
						in1[i]=NULL;
						out = in1[i+1];
						in1[i+1]=NULL;
						len = i;
						break;
					}
				}
				for(i=0;i<len;i++)
				{
					if(strcmp(in1[i],"<")==0)
					{
						in1[i]=NULL;
						inp = in1[i+1];
						in1[i+1]=NULL;
						break;
					}
				}
				pid = fork();
				if(pid<0)
				{
					printf("Error : Child process can't be initiated\n");
					exit(-1);
				}
				else if(pid ==0 )		//	child process
				{
					if((inp!=NULL))
					{
						if(access(inp,F_OK|R_OK)==0)
						{
							fp1=fopen(inp,"r");
							dup2(fileno(fp1),0);
						}
						else if (access(inp,F_OK|R_OK)==-1)
						{
							printf("Error in reading from file\n");
							exit(-1);
						}
					}
					if((out!=NULL))
					{
						if(access(out,F_OK)==-1)
						{
							fp2=fopen(out,"w+");
							dup2(fileno(fp2),1);
						}
						else if(access(out,F_OK)==0)
						{
							printf("Output file already exists\n");
							exit(-1);
						}
					}
					er = execvp(in1[0],in1);
					if(er<0)
					{
						printf("Error in executing command\n");
						exit(-1);
					}
					dup2(old_stdin,0);
					dup2(old_stdout,1);
					close(old_stdin);
					close(old_stdout);
				}
				else if(pid>0)
				{
					pro_id = pid;
					waitpid(pid,&status,0);
				}
			}
			else if(strcmp(in1[len-1],"&")!=0)	// Checking for Foreground process
			{
				pid=fork();
				if(pid<0)
				{

					perror("Error");
					_exit(-1);
				}
				else if(pid==0)
				{
					if(execvp(in1[0],in1)<0)
					{
						perror("Error");
						_exit(1);
					}
				}
				else
				{
					pro_id = pid;
					waitpid(pid,&status,WUNTRACED);
					if(WIFSTOPPED(status))
					{
						pro[k].pid = pid;
						pro[k].flag = 1;
						strcpy(pro[k].process,in1[0]);
						k++;
					}
				}
			}
			for(i=0;i<100;i++)
				in1[i]=NULL;
		}
		write(1,host,strlen(host));
		write(1,host1,strlen(host1));
		write(1,">",1);
		gets(in);
	}
	return 0;
}
