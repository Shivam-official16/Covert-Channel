#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"
#include<string.h>
#include<sys/mman.h>
#include<stdint.h>
#include<fcntl.h>
#include<unistd.h>
#include<math.h>
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

    int* generate_secret_key(int length) {
    static int seeded = 0;
    if (!seeded) {
        srand(time(NULL));
        seeded = 1;
    }

    int* key1 = (int*)malloc(length * sizeof(int));
    if (key1 == NULL) return NULL;

    for (int i = 0; i < length; i++) {
        // Box-Muller transform
        double u1 = ((double)rand() + 1.0) / ((double)RAND_MAX + 1.0);
        double u2 = ((double)rand() + 1.0) / ((double)RAND_MAX + 1.0);
        double z = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);

        // Convert normal value to binary
        key1[i] = (z >= 0) ? 1 : 0;
    }

    return key1;
}

//-----------------------------------------------------------------

//---------------------------Write your code here-------------------------------------------------

int main(int argc,char * argv[])
{
if(argc <1)
{printf("USAGE: <KEY SIZE><CASE>\nCASE 1 : All 1\n CASE 2: All 2\n CASE 3: Alternating 1\n CASE 4: Random\n");
return 0;
}
int i; 
uint64_t s,e,diff,count;
 uint32_t aux;
int key_size=atoi(argv[1]);
int CASE=atoi(argv[2]);
int* key = (int*)malloc(key_size * sizeof(int));
if (key == NULL) return 0;
    if(CASE==1)
    for(int q=0;q<key_size;q++)
    {
        key[q]=0;
    }
else if(CASE==2)
    for(int q=0;q<key_size;q++)
    {
        key[q]=1;
    }
else if(CASE==3)
{
    for(int q=0;q<key_size;q++)
    {
    if(q%2==0)
        key[q]=0;
    else
        key[q]=1;
    }
} 
else if(CASE==4)
{   free(key);
    key = generate_secret_key(key_size);
}
for(int q=0;q<key_size;q++)
{
printf("%d",key[q]);}
printf("\n");
//--------------------------------------------------MY CODE------------------------------------------------------



//-----------------------setup for pointer chasing-----------------------------------------------

   size_t size_bytes = 700*1024; // 1280KB for L2
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

const uint64_t offset_ns = 1ULL * 1000000000ULL;  // 1 seconds in ns
const uint64_t interval = 10000000000ULL; 
uint64_t target_time = getTime() + offset_ns;
uint64_t next_boundary =( (target_time / interval) + 1) * interval;
printf("Synchronizing...Next Boundary is: %ld\n",next_boundary);
while (getTime() < next_boundary) {
   __asm__ __volatile__("pause");
}

// sync_to_slot(SLOT);


_mm_mfence();
//---------------------synching completed-----------------------------------------------------------
printf("-------------STARTING----------------\n");
//-----------------------Starting access---------------------------------------------------------



 for(int q=0;q<key_size;q++)
 {

if(key[q]==1)
{   count=0;
    s=__rdtscp(&aux);
    int j=0;
    //   for(i=0;i<25000*n_nodes;i++)

    //   {
        //       j=i%n_nodes;
        //      nodes[j].padding[0]++;
        //   }
    i=0;
    while(__rdtscp(&aux)-s<18217760734)
     {
        j=i%n_nodes;
        nodes[j].padding[0]++;
        i++;
        count++;
     }
      e=__rdtscp(&aux);
      diff=e-s;
    //   printf("L2 access: %ld cycles\n",diff);
    //printf("L2 latency: %ld\n", diff/(25000*n_nodes));
    // printf("L2 latency: %lf\n", diff/(float)(count));
    //   printf("1\n");
    // printf("Count: %ld \n",count);
 }
    else
    {  s=__rdtscp(&aux);
      usleep(5047029);
    //   while(__rdtscp(&aux)-s<65601974657);
     // printf("0\n");
    }  

} 
// for(int q=0;q<key_size;q++)
// {
// printf("%d",key[q]);}
// printf("\n");
    return 0;



    // **********************************************
  }
