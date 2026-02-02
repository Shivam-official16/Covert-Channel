// receiver_fixed.c
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "./utils.h"
#include "./cacheutils.h"
#include <unistd.h>
#include <sched.h>
#include <sched.h>
#include <sys/mman.h>
#include <x86intrin.h>
#define _GNU_SOURCE
#include <sched.h>
#include <sys/wait.h>
#define SEC_TO_NS(sec) ((uint64_t)(sec) * 1000000000ULL)
#define f
#define CPU     0
#define FANOUT  16    // number of concurrent forks
#define BURST_MS 500 
#define THRESHOLD 1000000
static long now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void interrupts(long runtime_ms)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(CPU, &cpuset);

    pid_t child_pgid = -1;
    long end_time = now_ms() + runtime_ms;

    while (now_ms() < end_time) {
        long end = now_ms() + BURST_MS;

        while (now_ms() < end && now_ms() < end_time) {
            for (int i = 0; i < FANOUT; i++) {
                pid_t pid = fork();
                if (pid == 0) {
                    /* first child becomes group leader */
                    if (child_pgid == -1)
                        setpgid(0, 0);
                    else
                        setpgid(0, child_pgid);

                    sched_setaffinity(0, sizeof(cpuset), &cpuset);
                    execl("/bin/true", "true", NULL);
                    _exit(1);
                } else if (pid > 0 && child_pgid == -1) {
                    /* record PGID from first child */
                    child_pgid = pid;
                    setpgid(pid, child_pgid);
                }
            }
        }
    }

    /* kill only children */
    if (child_pgid != -1)
        killpg(child_pgid, SIGKILL);

    while (wait(NULL) > 0)
        ;
}

//---------------------------------Write your code--------------------------
#define CACHE_LINE 64UL   // 64 bytes per cache line

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

//-------------------------------Code here---------------------------------------
int main()
{ 

      uint64_t count=0;
      uint64_t start =rdtsc();

      uint64_t y= 25472580;
  printf("start: %lu \n",start);

   
//-----------------------setup for pointer chasing-----------------------------------------------

   size_t size_bytes = 8*1024* 1024; // 8 MB for LLC
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

//------------------------------Synching ----------------------------------------------    
//uint64_t now;
//now =rdtsc();

// while(now-start<6800000000){
 //printf("difference: %lu \n",now-start);
//  now=rdtsc();
// }


//Thrash LLC
 uint64_t trhash_start=rdtsc();     
    Node *current = &nodes[0];
    for (size_t i = 0; i < n_nodes; i++) {
        current = current->next;
    }
  _mm_mfence();
  uint64_t thrash_end= rdtsc();
  printf("thashing LLC took (aka y) : %lu\n", thrash_end-trhash_start);
  
  
  
  
 const uint64_t offset_ns = 1ULL * 1000000000ULL;  // 1 seconds in ns
const uint64_t interval = 10000000000ULL; 
uint64_t target_time = getTime() + offset_ns;
uint64_t next_boundary =( (target_time / interval) + 1) * interval;
printf("Synchronizing...Next Boundary is: %ld\n",next_boundary);
while (getTime() < next_boundary) {
    __asm__ __volatile__("pause");
}

//----------------------------------Synching Complete----------------------------------  
printf("-------------STARTING----------------\n");
//-----------------------Starting access--------------------------------------------------------- 
 
//--------Interrupt->Buffer Access->access time
interrupts(5000);
 current = &nodes[indices[0]];  
    uint64_t s =getTime();
      for (int i = 0; i < n_nodes; i++) {             
       current = current->next;
      } 
    uint64_t e=getTime();
    uint64_t diff=e-s;
     printf("Time taken for complete LLC access:%ld ns \n",diff);
     printf("Time taken for complete LLC access:%ld ms\n",diff/1000000);
      

if(diff>THRESHOLD)
printf("received 0\n");
else
printf("received 1\n");





  
  return 0;

    
}
