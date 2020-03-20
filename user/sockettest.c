#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define P_PORT 4096
#define C_PORT 4097
#define LOCAL_HOST_IP 2130706433

void socktest()
{
	int pid, fd;
	
	char ibuf[2048];
	
	pid = fork();
    if(pid < 0)
		return;
    if(pid == 0)
	{
		pid = getpid(); //Get pid.  Not sure this works on xv6
		printf("Child PID = %d\n", pid);  
		
		if((fd = connect(LOCAL_HOST_IP, C_PORT, P_PORT)) < 0)
		{
			printf("PID %d : Connect failed!\n", pid);
			exit(1);
		}
		else
		{
			printf("PID %d : Bound to port %d\n", pid, C_PORT);
		}
		// goodbye wait(0); //USED TO BE 1
		
		printf("PID %d : Testing send\n", pid);
		
		char obuf[26] = "Hello from child process!\n";
		
		if(write(fd, obuf, sizeof(obuf)) < 0)
		{
			fprintf(2, "send() failed\n");
			exit(1);
		}

		else {
			printf(obuf);
		}

		int cc = read(fd, ibuf, sizeof(ibuf));
		printf("read\n");
		if(cc < 0){
			fprintf(2, "recv() failed\n");
			exit(1);
		}
		else
		{
			printf("recv success\n");
			printf("Got \"%s\" from child proc!\n", ibuf);
		}

		exit(0);
	}
	else if (pid > 0)
	{
		pid = getpid(); //Get pid.  Not sure this works on xv6
		printf("Parent PID = %d\n", pid);
		
		if((fd = connect(LOCAL_HOST_IP, P_PORT, C_PORT)) < 0)
		{
			printf("PID %d : Connect failed!\n", pid);
			exit(1);
		}
		else
		{
			printf("PID %d : Bound to port %d\n", pid, P_PORT);
		}
		
		printf("PID %d : Testing recv\n", pid);
		
		int cc = read(fd, ibuf, sizeof(ibuf));
		printf("read\n");
		if(cc < 0){
			fprintf(2, "recv() failed\n");
			exit(1);
		}
		else
		{
			printf("recv success\n");
			printf("Got \"%s\" from child proc!\n", ibuf);
		}

		exit(0);
	}
	exit(0);
	return;
}