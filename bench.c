/*
 * bench.c — Standalone C CPU benchmark (same algorithm as cpuspeed.c).
 *
 * Build:  make bench   (or: cc -O2 -o bench bench.c -lpthread)
 * Run:    ./bench
 */

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define BENCH_SECS  1.0
#define INNER_BATCH 10000

static uint64_t run_core(double duration)
{
    struct timespec t0, tn;
    uint64_t iters = 0;
    uint32_t v[16];

    for (int i = 0; i < 16; i++)
        v[i] = (uint32_t)(i * 0x9E3779B9U + 1);

    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (;;) {
        for (int i = 0; i < INNER_BATCH; i++) {
            v[0]  += v[1]  ^ (v[15] >> 5);
            v[2]  -= v[3]  +  v[0];
            v[4]  ^= v[5]  << 3;
            v[6]   = v[7]  *  v[8]  + v[1];
            v[8]  += v[9]  ^  v[0];
            v[10] -= v[11] +  v[4];
            v[12] ^= v[13] >> 2;
            v[14]  = (v[15] + v[0]) ^ v[2];
            v[1]  += v[0]  &  v[3];
            v[3]  ^= v[2]  +  v[5];
            v[5]   = v[4]  *  v[6]  ^ v[7];
            v[7]  -= v[6]  >> 1;
            v[9]  += v[8]  ^  v[10];
            v[11]  = v[10] +  v[12] - v[14];
            v[13] ^= v[12] |  v[0];
            v[15] += v[14] ^  v[1];
        }
        iters += INNER_BATCH;

        clock_gettime(CLOCK_MONOTONIC, &tn);
        double elapsed = (tn.tv_sec  - t0.tv_sec)
                       + (tn.tv_nsec - t0.tv_nsec) * 1e-9;
        if (elapsed >= duration)
            break;
    }

    uint32_t chk = 0;
    for (int i = 0; i < 16; i++) chk ^= v[i];
    if (chk == 0) return iters + 1;

    return iters;
}

typedef struct { double duration; uint64_t result; } WorkerArg;

static void *worker(void *arg)
{
    WorkerArg *w = (WorkerArg *)arg;
    w->result = run_core(w->duration);
    return NULL;
}

int main(void)
{
    long ncpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpus <= 0) ncpus = 1;

    printf("CPU benchmark (Dhrystone-style integer workload, %g s per run)\n",
           BENCH_SECS);
    printf("Logical CPUs: %ld\n\n", ncpus);

    /* Single-core */
    printf("Single-core ... ");
    fflush(stdout);
    uint64_t sc = run_core(BENCH_SECS);
    printf("%llu iterations  (%.1f M/s)\n",
           (unsigned long long)sc, sc / 1e6);

    /* Multi-core */
    printf("Multi-core  (%ld threads) ... ", ncpus);
    fflush(stdout);

    pthread_t *threads = malloc((size_t)ncpus * sizeof(pthread_t));
    WorkerArg *wargs   = calloc((size_t)ncpus, sizeof(WorkerArg));
    if (!threads || !wargs) { perror("malloc"); return 1; }

    for (long i = 0; i < ncpus; i++)
        wargs[i].duration = BENCH_SECS;

    for (long i = 0; i < ncpus; i++)
        pthread_create(&threads[i], NULL, worker, &wargs[i]);

    uint64_t total = 0;
    for (long i = 0; i < ncpus; i++) {
        pthread_join(threads[i], NULL);
        total += wargs[i].result;
    }

    printf("%llu iterations  (%.1f M/s total, %.1f M/s per core)\n",
           (unsigned long long)total,
           total / 1e6,
           total / 1e6 / (double)ncpus);

    free(threads);
    free(wargs);
    return 0;
}
