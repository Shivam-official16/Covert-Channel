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

#define SLOT 5000000ULL 
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

static inline uint64_t rdtscp_now(uint32_t *aux) {
    return __rdtscp(aux);
}

void sync_to_slot(uint64_t interval_cycles)
{
    uint32_t aux;

    uint64_t now = rdtscp_now(&aux);

    // Compute next boundary
    uint64_t next =
        ((now / interval_cycles) + 1) * interval_cycles;

    // Spin until boundary
    while (rdtscp_now(&aux) < next)
        asm volatile("pause");

    // Serialize after exit
    _mm_mfence();
}
//-----------------------------------------------------------------

//---------------------------Write your code here-------------------------------------------------

int main(int argc,char * argv[])
{
int i; 
uint64_t s,e,diff;
 uint32_t aux;
int send=atoi(argv[1]);
//--------------------------------------------------MY CODE------------------------------------------------------



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

//--------------------------------------Warmup---------------------------------------------------

s=__rdtscp(&aux);
       for(i=0;i<n_nodes;i++)
    nodes[i].padding[0]++;

    e=__rdtscp(&aux);
    diff=e-s;
     // printf("L2 access cold: %ld cycles\n",diff);
     // printf("L2 latency cold: %ld\n", diff/n_nodes);

//----------------------------Synching---------------------------------
sync_to_slot(SLOT);


_mm_mfence();
//---------------------synching completed-----------------------------------------------------------
printf("-------------STARTING----------------\n");
//-----------------------Starting access---------------------------------------------------------


s=__rdtscp(&aux);
 
 if(send==1)
 { 
    int j=0;
      for(i=0;i<25000*n_nodes;i++)
      {
        j=i%n_nodes;
    nodes[j].padding[0]++;
     
      }

    e=__rdtscp(&aux);
    diff=e-s;
    //  printf("L2 access: %ld cycles\n",diff);
     // printf("L2 latency: %ld\n", diff/(25000*n_nodes));
    }
    else
    {
      sleep(7);
    }  
    return 0;



    // **********************************************
  }
