
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include "webserver.h"

#define MAX_REQUEST 100

int port, numThread, i, j, in = 0, out = 0;

int request[MAX_REQUEST]; // shared buffer

// semaphores and a mutex lock
sem_t sem_full;
sem_t sem_empty;
pthread_mutex_t mutex;


void* req_handler()
{
		int r;
		struct sockaddr_in sin;
		struct sockaddr_in peer;
		int peer_len = sizeof(peer);
		int sock;

		sock = socket(AF_INET, SOCK_STREAM, 0);

		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = INADDR_ANY;
		sin.sin_port = htons(port);
		r = bind(sock, (struct sockaddr *) &sin, sizeof(sin));
		if(r < 0) {
				perror("Error binding socket:");
				exit(0);
		}

		r = listen(sock, 5);
		if(r < 0) {
				perror("Error listening socket:");
				exit(0);
		}

		printf("HTTP server listening on port %d\n", port);

		while (1)
		{
				int s;
				s = accept(sock, NULL, NULL);
				if (s < 0) break;

				//process(s);
				
				sem_wait(&sem_empty);
				pthread_mutex_lock(&mutex);

				request[in] = s;
				in = (in + 1) % MAX_REQUEST;

				pthread_mutex_unlock(&mutex);
				sem_post(&sem_full);
		}

		close(sock);
}

void* req_process(){
  while(1){
    int t;
    sem_wait(&sem_full);
    pthread_mutex_lock(&mutex);

    t = request[out];
    out = (out + 1) % MAX_REQUEST;
    
    pthread_mutex_unlock(&mutex);
    sem_post(&sem_empty);

    process(t);
  }
}

int main(int argc, char *argv[])
{
		if(argc < 2 || atoi(argv[1]) < 2000 || atoi(argv[1]) > 50000)
		{
				fprintf(stderr, "./webserver PORT(2001 ~ 49999) (#_of_threads) (crash_rate(%))\n");
				return 0;
		}

		
		// port number
		port = atoi(argv[1]);
		
		// # of worker thread
		if(argc > 2) 
				numThread = atoi(argv[2]);
		else numThread = 1;

		// crash rate
		if(argc > 3) 
				CRASH = atoi(argv[3]);
		if(CRASH > 50) CRASH = 50;
		printf("[pid %d] CRASH RATE = %d\%\n", getpid(), CRASH);


		sem_init(&sem_empty, 0, MAX_REQUEST);
		sem_init(&sem_full, 0, 0);
		pthread_t threadPool[numThread];
		pthread_t listener;
		pthread_mutex_init(&mutex, NULL);
		int error;

		pthread_create(&listener, NULL, req_handler, NULL);
		
		for(i = 0; i < numThread; i++){
		  pthread_create(&threadPool[i], NULL, req_process, NULL);
		  printf("Worker thread created. pid %d, tid %d\n", getpid(), i + 2);
		}

		while(1){
		  for(j = 0; j < numThread; j++){
		    // pthread_join(threadPool[j], NULL);
		    error = pthread_tryjoin_np(threadPool[j], NULL);
		    if(error == 0){
		      printf("Worker thread %d killed. Created another worker.\n", j + 2);
		      pthread_create(&threadPool[j], NULL, req_process, NULL);
		      printf("Worker thread created. pid %d, tid %d\n", getpid(), j + 2);
		    }
		  }
		}
	

		
		pthread_join(listener, NULL);
		//req_handler();

		return 0;
}

