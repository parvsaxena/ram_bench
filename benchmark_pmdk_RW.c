//  Created by Emil Ernerfeldt on 2014-04-17.

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "libpmemobj.h"

typedef uint64_t Int;

typedef struct Node Node;

PMEMobjpool *pm_pool;


struct Node {
    Int payload; // ignored; just for plausability.
    Int payload1;
    Int payload2;
    Int payload3;
    Int payload4;
    Int payload5;
    Int payload6;
    Node* next;
};

void random_shuffle(Node** list, Int N)
{
    for (Int i=0; i<N-1; ++i) {
        Int swap_ix = i + rand() % (N-i);
        Node* tmp = list[swap_ix];
        list[swap_ix] = list[i];
        list[i] = tmp;
    }
}

// Returns nanoseconds per element.
double bench(Int N, Int iters) {
    // Allocate all the memory continuously so we aren't affected by the particulars of the system allocator:

    PMEMoid oid;
    double ns;
    TX_BEGIN(pm_pool) {
        oid = pmemobj_tx_zalloc(N * sizeof(Node), 0);
    } TX_END

        Node* memory = (Node*)pmemobj_direct(oid);

        Node** nodes = (Node**)malloc(N * sizeof(Node*));
        for (Int i=0; i<N; ++i) {
            nodes[i] = &memory[i];
        }
        // Randomize so emulate a list that has been shuffled around a bit.
        // This is so that each memory acces is a *random* memory access.
        // This is the worst case scenario for a linked list, which is exactly what we want to measure.
        // Without the random_shuffle we get O(N) because it enables the pre-fetcher to do its job.
        // Without a prefetcher, a random memory access in N bytes of RAM costs O(N^0.5) due to the Bekenstein bound.
        // This means we get O(N^1.5) complexity of a linked list traversal.

        random_shuffle(nodes, N);

        for (Int i = 0; i < N - 1; ++i) {
            nodes[i]->next = nodes[i + 1];
        }

        nodes[N-1]->next = NULL;

        Node* start_node = nodes[0];
        int arr[7];
        free(nodes); // Free up unused memory before meassuring:
        // Do the actual measurements:
        Int start = clock();
//        TX_BEGIN(pm_pool) {

//            pmemobj_tx_add_range_direct(memory, sizeof(N* sizeof(Node)));
            for (Int it = 0; it < iters; ++it) {
                // Run through all the nodes:
                Node *node = start_node;
                while (node) {
                    arr[0] = node->payload;
                    arr[1] = node->payload1;
                    arr[2] = node->payload2;
                    arr[3] = node->payload3;
                    arr[4] = node->payload4;
                    arr[5] = node->payload5;
                    arr[6] = node->payload6;
//                    node->payload = 300;
//                    node->payload1 = 300;
//                    node->payload2 = 300;
//                    node->payload3 = 300;
//                    node->payload4 = 300;
//                    node->payload5 = 300;
//                    node->payload6 = 300;
                    node = node->next;
                }
            }
//        } TX_END

        Int dur = clock() - start;
        ns = 1e9 * dur / CLOCKS_PER_SEC;

        TX_BEGIN(pm_pool) {
            pmemobj_tx_free(oid);
        } TX_END

    return ns / (N * iters);
}


int main(int argc, const char * argv[])
{
    // Outputs data in gnuplot friendly .data format
    printf("#bytes    ns/elem\n");
    long long int GB = 1024*1024*1024;
    long long int pmem_size = GB*4;
    Int stopsPerFactor = 4; // For every power of 2, how many measurements do we do?
    Int minElemensFactor = 6;  // First measurement is 2^this number of elements.
    Int maxElemsFactor = 30; // Last measurement is 2^this number of elements. 30 == 16GB of memory
    //Int elemsPerMeasure = Int(1) << 28; // measure enough times to process this many elements (to get a good average)

    Int min = stopsPerFactor * minElemensFactor;
    Int max = stopsPerFactor * maxElemsFactor;

    pm_pool = pmemobj_create("/mnt/mem/a.pm", "store.db", pmem_size+20, 0666);

    if (pm_pool == NULL) {
        printf("Something went wrong\n");
        exit(-1);
    }

    for (Int ei=min; ei<=max; ++ei) {
        Int N = (Int)floor(pow(2.0, (double)ei / stopsPerFactor) + 0.5);
        //Int reps = elemsPerMeasure / N;
        Int reps = (Int)floor(1e9 / pow(N, 1.5) + 0.5);
        if (reps<1) reps = 1;
        // We create the file in Pmem of 1024^3 = 1 GB + some additional setting.
        // The struct node is 8 bytes, so we want to cap N at 1024*1024*128
        if (N >= pmem_size/ sizeof(Node)) {
            continue;
        }
        double ans = bench(N, reps);
        printf("%llu   %f   # (N=%llu, reps=%llu) %llu/%llu\n", N*sizeof(Node), ans, N, reps, ei-min+1, max-min+1);

        /////// Remove
        // return 1;
        ///////
    }
}
