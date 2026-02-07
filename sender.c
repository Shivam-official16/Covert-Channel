#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"
#include<string.h>
#include<sys/mman.h>
#include<stdint.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<x86intrin.h>
#include "./cacheutils.h"
#include <sched.h>
#define FILE_PATH "shared_file.bin"
#define BIT 1
#define SEC_TO_NS(sec) ((uint64_t)(sec) * 1000000000ULL)
//----------------------------------------------------------
#define CACHE_LINE 64   // 64 bytes per cache line


typedef struct Node {
    struct Node *next;
    char padding[CACHE_LINE - sizeof(struct Node *)]; // Ensure each node is in a separate cache line
} Node;


struct timespec ts;
uint64_t getTime()
{
  clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
uint64_t now_ns = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
return now_ns;
}
//-----------------------------------------------------------------

//---------------------------Write your code here-------------------------------------------------

int main(int argc,char *argv[])
{
int i; 
uint64_t s,e,diff;
 uint32_t aux;

//--------------------------------------------------MY CODE------------------------------------------------------
int send=atoi(argv[1]);
printf("sending %d\n",send);


//-----------------------setup for pointer chasing-----------------------------------------------

   size_t size_bytes = 256*1024; // 256KB for L2
    size_t n_nodes = size_bytes / CACHE_LINE;

    Node *nodes = aligned_alloc(CACHE_LINE, n_nodes * sizeof(Node));
    if (!nodes) {
        perror("Allocation failed");
        return 1;
    }

    // Create random permutation for pointer chasing
    size_t *indices = malloc(n_nodes * sizeof(size_t));
    for (size_t i = 0; i < n_nodes; i++) indices[i] = i;

    srand(time(NULL));
    for (size_t i = n_nodes - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        size_t tmp = indices[i];
        indices[i] = indices[j];
        indices[j] = tmp;
    }

    for (size_t i = 0; i < n_nodes - 1; i++) {
        nodes[indices[i]].next = &nodes[indices[i + 1]];
    }
    nodes[indices[n_nodes - 1]].next = &nodes[indices[0]]; // Make it circular  
//---------------------------------------------------------------------------------------------------    

//--------------------------------------Synching---------------------------------------------------

 
// const uint64_t offset = 100;
// const uint64_t interval = 100; 
// uint64_t target_time = __rdtscp(&aux) + offset;
// uint64_t next_boundary =( (target_time / interval) + 1) * interval;
// printf("Synchronizing...Next Boundary is: %ld\n",next_boundary);
// while (__rdtscp(&aux) < next_boundary);

 const uint64_t offset_ns = 1ULL * 1000000000ULL;  // 1 seconds in ns
const uint64_t interval = 10000000000ULL; 
uint64_t target_time = getTime() + offset_ns;
uint64_t next_boundary =( (target_time / interval) + 1) * interval;
printf("Synchronizing...Next Boundary is: %ld\n",next_boundary);
while (getTime() < next_boundary) {
    __asm__ __volatile__("pause");
}


//---------------------synching completed-----------------------------------------------------------
printf("-------------STARTING----------------\n");
//-----------------------Starting access---------------------------------------------------------
 Node *current = &nodes[0];
s=__rdtscp(&aux);
if(send==1)
{
 current = &nodes[0];

    
   current = &nodes[indices[0]];  
      while(__rdtscp(&aux)-s<17400964522)
      {
         current->padding[0]='B';           
        current = current->next;
      }
    e=__rdtscp(&aux);
    diff=e-s;
      printf("L2 access: %ld cycles\n",diff);
      printf("L2 latency: %ld", diff/n_nodes);

  
}
else
{          
      while(__rdtscp(&aux)-s<17426513386);
     printf("Sent 0 \n");

}
   
    return 0;



    // **********************************************
  }
