#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

const int NUM_OF_WORDS = 1;
const int NUM_OF_PROCESSES = 8;
int TEST = 10;

int main( int argc, char** argv)
{	
	//Variables
	size_t len = NUM_OF_WORDS * sizeof(std::string);
	std::string* buf; 			//shared memory buffer
	key_t key = IPC_PRIVATE; 	//assigned by OS
	unsigned int n; 			//fork count	
	unsigned int value; 		//semaphore value
	sem_t *lock; 				// sync semaphore
	pid_t pid;					//fork pid
	int shmid;					//shared memory id
	int status;					//wait
	
	//Initialize shared memory buffer	
	shmid = shmget(key, len, IPC_CREAT | 0664);
	if (shmid == -1)
	{
		perror("Error: shmget");
		exit(1);	
	}

	buf = (std::string *) shmat(shmid, nullptr, 0);
	if (buf == (std::string *)-1)
	{
		perror("Error: shmat");
		exit(1);
	}
	printf("Shared memory buffer initialized...\n");

	// Init semaphore controls
	lock = sem_open("semLock", O_CREAT | O_EXCL, 0644, 1);	
	printf("Semaphore initialized...\n");

	*buf = "hello";
	pid_t parentPid = getpid();
	printf("Parent value: %s  pid: %d\n", buf->c_str(), parentPid);

	// BEGIN Forking
	for (int i = 0; i < NUM_OF_PROCESSES; i++)
	{
		pid = fork();
		if (pid < 0)
		{
			perror("Error: fork");
			sem_unlink("semLock");
			sem_close(lock);
			exit(1);
		}
		if (pid == 0)
		{
			//******************************************
			// Child Process 
			//******************************************
			printf("In the child process...\n");
			sem_wait(lock);
			
			printf("Critical section...\n");
			*buf = "goodbye";
			printf("[%d] Buffer is now: %s with pid: %d\n",i, buf->c_str(), getpid());
			
			sem_post(lock);
			exit(1);	
		}
		sleep(1);
	}	

	//******************************************
	// Parent process
	//******************************************
	printf("Waiting for the child process...\n");
	for (int i = 0; i < NUM_OF_PROCESSES; i++)
	{
		wait(&status);
	}
	
	printf("Buffer is now: %s, with pid: %d\n", buf->c_str(), getpid());

	// Detach shared memory
	shmdt(buf);
	shmctl(shmid, IPC_RMID, 0);
	
	// Cleanup semaphores
	sem_unlink("semLock");
	sem_close(lock);

	exit(1);
	return 0;
}
