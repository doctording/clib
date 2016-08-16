/*
父进程从stdin读取字符串并保存到共享内存中，子进程从共享内存中读出数据并输出到stdout
linux下 ipcs 可以分析message queues,shared memory,semaphore
参考: http://blog.chinaunix.net/uid-27122224-id-3265801.html
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define BUFFER_SIZE 2048

int main() 
{
	pid_t pid;
	int shmid;
	char *shm_addr;
	char flag[]="Parent";
	char buff[BUFFER_SIZE];
	// 创建当前进程的私有共享内存
	if ((shmid=shmget(IPC_PRIVATE,BUFFER_SIZE,0666))<0) {
		perror("shmget");
		exit(1);
	} else 
		printf("Create shared memory: %d.\n",shmid);

	// ipcs 命令往标准输出写入一些关于活动进程间通信设施的信息
	// -m 表示共享内存
	printf("Created shared memory status:\n");
	system("ipcs -m");

	if((pid=fork())<0) {
		perror("fork");
		exit(1);
	}else if (pid==0) {
		// 自动分配共享内存映射地址，为可读可写，映射地址返回给shm_addr
		if ((shm_addr=shmat(shmid,0,0))==(void*)-1) {
			perror("Child:shmat");
			exit(1);
		}else
			printf("Child: Attach shared-memory: %p.\n",shm_addr);

		printf("Child Attach shared memory status:\n");
		system("ipcs -m");
		// 比较shm_addr,flag的长度为strlen(flag)的字符
		// 当其内容相同时，返回0
		// 否则返回（str1[n]-str2[n]）
		while (strncmp(shm_addr,flag,strlen(flag))) {
			printf("Child: Waiting for data...\n");
			sleep(10);
		}

		strcpy(buff,shm_addr+strlen(flag));
		printf("Child: Shared-memory: %s\n",buff);
		// 删除子进程的共享内存映射地址
		if (shmdt(shm_addr)<0) {
			perror("Child:shmdt");
			exit(1);
		}else
			printf("Child: Deattach shared-memory.\n");

		printf("Child Deattach shared memory status:\n");
		system("ipcs -m");

	}else{
		sleep(1);
		// 自动分配共享内存映射地址，为可读可写，映射地址返回给shm_addr
		if ((shm_addr=shmat(shmid,0,0))==(void*)-1) {
			perror("Parent:shmat");
			exit(1);
		}else
			printf("Parent: Attach shared-memory: %p.\n",shm_addr);

		printf("Parent Attach shared memory status:\n");
		system("ipcs -m");
		// shm_addr为flag+stdin
		sleep(1);
		printf("\nInput string:\n");
		fgets(buff,BUFFER_SIZE-strlen(flag),stdin);
		strncpy(shm_addr+strlen(flag),buff,strlen(buff));
		strncpy(shm_addr,flag,strlen(flag));
		// 删除父进程的共享内存映射地址
		if (shmdt(shm_addr)<0) {
			perror("Parent:shmdt");
			exit(1);
		}else
			printf("Parent: Deattach shared-memory.\n");

		printf("Parent Deattach shared memory status:\n");
		system("ipcs -m");
		// 保证父进程在删除共享内存前，子进程能读到共享内存的内容 
		waitpid(pid,NULL,0);
		// 删除共享内存
		if (shmctl(shmid,IPC_RMID,NULL)==-1) {
			perror("shmct:IPC_RMID");
			exit(1);
		}else
			printf("Delete shared-memory.\n");

		printf("Child Delete shared memory status:\n");
		system("ipcs -m");

		printf("Finished!\n");
	}

	exit(0);
}

