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
#define FANOUT  10    // number of concurrent forks
#define BURST_MS 500 
#define THRESHOLD 22000000000

#define SLOT 5000000ULL 
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

int i; 
uint64_t s,e,diff,is,ie,idiff;
 uint32_t aux;

      uint64_t count=0;   
//-----------------------setup for pointer chasing-----------------------------------------------

    size_t size_bytes =8*1024* 1024; // 8 MB for LLC
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


//----------------------------Thrash LLC---------------------------------------

//------------------------------Warmup----------------------------------------------    
is =__rdtscp(&aux);
 Node *current = &nodes[indices[0]];  
    s =__rdtscp(&aux);
     for(i=0;i<n_nodes;i++)
    nodes[i].padding[0]++; 
    e=__rdtscp(&aux);
    diff=e-s;
    // printf("Thrash Difference cold:%ld cycles \n",diff);
     // printf("Thrash LLC latency cold: %ld cycles \n", diff/n_nodes);
      _mm_mfence();

//----------------------------Synching---------------------------------

sync_to_slot(SLOT);
//----------------------------------Synching Complete----------------------------------  
printf("-------------STARTING----------------\n");
//-----------------------Starting access--------------------------------------------------------- 

//--------Interrupt->Buffer Access->access time

interrupts(2500);

_mm_mfence();


 current = &nodes[indices[0]];  
    s =__rdtscp(&aux);
   int j=0;
      for(i=0;i<30*n_nodes;i++)
      {
        j=i%n_nodes;
    nodes[j].padding[0]++;
     
      }
    e=__rdtscp(&aux);
  
    diff=e-s;
   
    // printf("Difference:%ld cycles \n",diff);
    //  printf("LLC latency: %ld cycles \n", diff/(30*n_nodes));
     // printf("Count: %ld \n",count);
_mm_mfence();
interrupts(2500);

_mm_mfence();

   s =__rdtscp(&aux);
   j=0;
      for(i=0;i<30*n_nodes;i++)
      {
        j=i%n_nodes;
    nodes[j].padding[0]++;
     
      }
    e=__rdtscp(&aux);
    ie =__rdtscp(&aux);
    diff=e-s;
    idiff=ie-is;
    // printf("Difference:%ld cycles \n",diff);
     // printf("LLC latency: %ld cycles \n", diff/(30*n_nodes));
    //  printf("Total cyles: %ld cycles\n",idiff);
      //printf("Count: %ld \n",count);
      

if(idiff>THRESHOLD)
printf("received 1\n");
else
printf("received 0\n");





  
  return 0;

    
}
