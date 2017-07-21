#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define _USE_MATH_DEFINES

const int NUM_OF_POINTS = 5000;
const int NUM_OF_PROCESSES = 2;
int LOOP_COUNT = 0;

// Function Declarations
void readInDoubleCSV(std::string fileName, std::vector<double> &data);
void exportDoubleCSV(std::string outFileName, const std::vector<double> &data);
void printVector(const std::vector<double> &source);
std::vector<double> normalFFT(std::vector<double> input, int s);
std::vector<double> parallelFFT(std::vector<double> &input, int s);
void parallelCode();


int main( int argc, char** argv)
{
	// Read in Data
	auto inputFile = "sinData.csv";
	std::vector<double> signalData;
	readInDoubleCSV(inputFile, signalData);
	//printVector(signalData);
	
	// Transforming Data
	std::vector<double> fftNormalOutput;
	std::vector<double> fftParallelOutput;
	//fftNormalOutput = normalFFT(signalData, 1);
	
	fftParallelOutput = parallelFFT(signalData, 1);
	//printVector(fftNormalOutput);


	// Export Data
	auto outputFileName = "fftNormalOutput.csv";
	exportDoubleCSV(outputFileName, fftNormalOutput);

	
	return 0;	
}


void readInDoubleCSV(std::string fileName, std::vector<double> &data)
{
	std::ifstream file (fileName);
	std::string temp;
	if ( file.is_open() )
	{
		while ( getline(file, temp, ',') )
		{
			data.push_back( std::atof(temp.c_str()) );	
		}
		file.close();
	}
	else
	{
		perror("Error: Input file did not open");
	}
}


void printVector(const std::vector<double> &source)
{
	for (int i = 0; i < source.size(); i++)
	{
		std::cout << "Element [" << i+1 << "]: " << source[i] << std::endl;
	}	
}


void exportDoubleCSV(std::string outFileName, const std::vector<double> &data)
{
	std::ofstream outFile (outFileName);
	if (outFile.is_open())
	{
		for (int i = 0; i < data.size(); i++)
		{
			outFile << data[i] << ",";
		}
		outFile.close();
	}
	else
	{
		perror("Error: Output file did not open");
	}
}

//s is the stride
std::vector<double> normalFFT(std::vector<double> input, int s)
{
	int n = input.size();
	int m = n/2;
	// Real part of g = e^(2*pi*i/N)
	double g = cos(2*M_PI/n);
	
	//printf("IN normalFFT()\n");
	
	// Base case
	if (n == 1)
	{
		return input;
	}
	
	std::vector<double> even;	
	std::vector<double> odd;
	for (int j = 0; j < m; j++)
	{
		even.push_back( input[2*j] );
		odd.push_back( input[ (2*j)+s ] );
		
		//std::cout << "[" << j+1 << "]input: " << input[2*j] << std::endl;	
	}

//	LOOP_COUNT++;
//	std::cout << "Count = " << LOOP_COUNT << std::endl;
//	int x;
//	std::cin >> x;
	
	odd = normalFFT(odd, s);
	even = normalFFT(even, s);

	
	//printf("before result---!!!\n");
	std::vector<double> result;
	double sum;
	for (int k = 0; k < n; k++)
	{
		sum = even[k%m] + (pow(g,-k) * odd[k%m] );
		result.push_back(sum);
	}

	//printf("before the end!!!\n");
	return result;	
}


std::vector<double> parallelFFT(std::vector<double> &input, int s)
{
	//Variables
	size_t len = 3 * sizeof(input);
	std::vector<double>* data;				//shared memory buffer
	key_t key = IPC_PRIVATE; 				//assigned by OS
	sem_t *lock; 							// sync semaphore
	pid_t pid;								//fork pid
	int shmid;								//shared memory id
	int status;								//wait	
	int n = input.size();
	int m = n/2;	
	double g = cos(2*M_PI/n);				// Real part of g = e^(2*pi*i/N)
	
	//Initialize shared memory datafer	
	shmid = shmget(key, len, IPC_CREAT | 0664);
	if (shmid == -1)
	{
		perror("Error: shmget");
		exit(1);	
	}

	data = (std::vector<double> *) shmat(shmid, nullptr, 0);
	if (data == (std::vector<double> *)-1)
	{
		perror("Error: shmat");
		exit(1);
	}
	printf("Shared memory buffer initialized...\n");

	// Init semaphore controls
	lock = sem_open("semLock", O_CREAT | O_EXCL, 0644, 1);	
	printf("Semaphore initialized...\n");
	
	
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
			// Child Process 1
			//******************************************
			printf("In the child process...\n");
			sem_wait(lock);
				
			printf("Critical section...\n");
			
			if (i == 0)
			{	
				// Separate even values from input
				for (int j = 0; j < m; j++)
				{
					//std::cout << "[" << j+1 << "]child 1" << std::endl;
					data[1].push_back(data[0][ 2*j ]);
				}	
			} 
			//******************************************
			// Child Process 2
			//******************************************
			if (i == 1)
			{
				// Separate odd values from input
				for (int j = 0; j < m; j++)
				{
					//std::cout << "[" << j+1 << "]child 2" << std::endl;
					data[2].push_back(data[0][ (2*j)+s ]);	
				}

				//*data = "goodbye";
			}

			sem_post(lock);
			exit(1);	
			
		}
		//sleep(1);
	}	

	//******************************************
	// Parent process
	//******************************************
	sem_wait(lock);
	printf("Parent critical region");

	for (int i = 0; i < input.size(); i++)
	{
		data[0][i].push_back(input[i]);
	}

	sem_post(lock);

	printf("Waiting for the child process...\n");
	for (int i = 0; i < NUM_OF_PROCESSES; i++)
	{
		wait(&status);
	}
	
	// Detach shared memory
	shmdt(data);
	shmctl(shmid, IPC_RMID, 0);
	
	// Cleanup semaphores
	sem_unlink("semLock");
	sem_close(lock);

	exit(1);
}


void parallelCode()
{
	//Variables
	size_t len = 2 * sizeof(std::string);
	std::string* data; 			//shared memory datafer
	key_t key = IPC_PRIVATE; 	//assigned by OS
	unsigned int n; 			//fork count	
	unsigned int value; 		//semaphore value
	sem_t *lock; 				// sync semaphore
	pid_t pid;					//fork pid
	int shmid;					//shared memory id
	int status;					//wait
	
	//Initialize shared memory datafer	
	shmid = shmget(key, len, IPC_CREAT | 0664);
	if (shmid == -1)
	{
		perror("Error: shmget");
		exit(1);	
	}

	data = (std::string *) shmat(shmid, nullptr, 0);
	if (data == (std::string *)-1)
	{
		perror("Error: shmat");
		exit(1);
	}
	printf("Shared memory datafer initialized...\n");

	// Init semaphore controls
	lock = sem_open("semLock", O_CREAT | O_EXCL, 0644, 1);	
	printf("Semaphore initialized...\n");

	*data = "hello";
	pid_t parentPid = getpid();
	printf("Parent value: %s  pid: %d\n", data->c_str(), parentPid);

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
			*data = "goodbye";
			printf("[%d] Buffer is now: %s with pid: %d\n",i, data->c_str(), getpid());
			
			sem_post(lock);
			exit(1);	
		}
		//sleep(1);
	}	

	//******************************************
	// Parent process
	//******************************************
	printf("Waiting for the child process...\n");
	for (int i = 0; i < NUM_OF_PROCESSES; i++)
	{
		wait(&status);
	}
	
	printf("Buffer is now: %s, with pid: %d\n", data->c_str(), getpid());

	// Detach shared memory
	shmdt(data);
	shmctl(shmid, IPC_RMID, 0);
	
	// Cleanup semaphores
	sem_unlink("semLock");
	sem_close(lock);

	exit(1);

}










