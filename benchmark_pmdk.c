#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "libpmemobj.h"

typedef struct Node Node;

typedef uint64_t Int;

PMEMobjpool *pm_pool;


struct Node {
    Int payload;
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

double benchmark_pmemobj_tx_zalloc(Int N, Int iters) {

    PMEMoid oid;
    double ns;

    Int start = clock();
    for(Int i=0;i<N;i++) {

        TX_BEGIN(pm_pool) {
	    // printf("Running");
            oid = pmemobj_tx_zalloc(sizeof(Node), 0);
        } TX_END
    }
    Int dur = clock() - start;
    ns = 1e9 * dur / CLOCKS_PER_SEC;

    return ns / (N * iters);
}

double benchmark_pmemobj_zalloc(Int N, Int iters) {
    PMEMoid oid;
    double ns;

    Int start = clock();
    for(Int i=0;i<N;i++) {
	// printf("Running");
        pmemobj_zalloc(pm_pool, &oid ,sizeof(Node), 0);
    }
    Int dur = clock() - start;
    ns = 1e9 * dur / CLOCKS_PER_SEC;

    return ns / (N * iters);
}

double benchmark_empty_transaction(Int N, Int iters) {

    PMEMoid oid;
    double ns;

    Int start = clock();
    for(Int i=0;i<N;i++) {

        TX_BEGIN(pm_pool) {
            // No statements
	    // printf("Running");
        } TX_END
    }

    Int dur = clock() - start;
    ns = 1e9 * dur / CLOCKS_PER_SEC;

    return ns / (N * iters);
}

int main() {
    long long int GB = 1024*1024*16;
    long long int pmem_size = GB*1;
    long long int N;
    pm_pool = pmemobj_create("/mnt/mem/a.pm", "store.db", pmem_size+20, 0666);
    N = pmem_size/ sizeof(Node);
    double ans;

    ans = benchmark_empty_transaction(N, 1);

    // ans = benchmark_pmemobj_tx_zalloc(N, 1);

    // ans = benchmark_pmemobj_zalloc(N, 1);

    printf("The average latency for %lu bytes and %lu elements is %f\n", sizeof(Node), N, ans);
}
