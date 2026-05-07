/*
 * cpuspeed.c — Python C extension: Dhrystone-style integer CPU benchmark.
 *
 * Exports:
 *   cpuspeed.singlecore() -> int   iterations in ~1 s on one core
 *   cpuspeed.multicore()  -> int   total iterations in ~1 s across all cores
 *
 * Each "iteration" executes the 16-operation inner body once.
 * Higher return values = faster CPU.
 *
 * Build:  python3 setup.py build_ext --inplace
 *   or:   make module
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define BENCH_SECS   1.0
#define INNER_BATCH  10000   /* iterations between clock checks */

/* ------------------------------------------------------------------ */
/* Benchmark core                                                       */
/* ------------------------------------------------------------------ */

/*
 * Integer/arithmetic workload similar in spirit to Dhrystone:
 * a mix of add, sub, xor, shift, multiply, and array accesses that
 * resist auto-vectorisation and dead-code elimination.
 *
 * v[] lives in registers on x86-64 (16 × uint32_t = 64 bytes).
 * The multiply keeps the FU busy; shifts + branches stress the
 * branch predictor a little.
 */
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

    /* Force use of v[] so the compiler cannot eliminate the loop. */
    uint32_t chk = 0;
    for (int i = 0; i < 16; i++) chk ^= v[i];
    if (chk == 0) return iters + 1;   /* branch never taken */

    return iters;
}

/* ------------------------------------------------------------------ */
/* Multi-core support                                                   */
/* ------------------------------------------------------------------ */

typedef struct {
    double   duration;
    uint64_t result;
} WorkerArg;

static void *worker(void *arg)
{
    WorkerArg *w = (WorkerArg *)arg;
    w->result = run_core(w->duration);
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Python-callable functions                                            */
/* ------------------------------------------------------------------ */

static PyObject *py_singlecore(PyObject *self, PyObject *args)
{
    uint64_t iters;
    Py_BEGIN_ALLOW_THREADS
    iters = run_core(BENCH_SECS);
    Py_END_ALLOW_THREADS
    return PyLong_FromUnsignedLongLong(iters);
}

static PyObject *py_multicore(PyObject *self, PyObject *args)
{
    long ncpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpus <= 0) ncpus = 1;

    pthread_t *threads = malloc((size_t)ncpus * sizeof(pthread_t));
    WorkerArg *wargs   = calloc((size_t)ncpus, sizeof(WorkerArg));
    if (!threads || !wargs) {
        free(threads);
        free(wargs);
        PyErr_NoMemory();
        return NULL;
    }

    for (long i = 0; i < ncpus; i++)
        wargs[i].duration = BENCH_SECS;

    Py_BEGIN_ALLOW_THREADS
    for (long i = 0; i < ncpus; i++)
        pthread_create(&threads[i], NULL, worker, &wargs[i]);
    for (long i = 0; i < ncpus; i++)
        pthread_join(threads[i], NULL);
    Py_END_ALLOW_THREADS

    uint64_t total = 0;
    for (long i = 0; i < ncpus; i++)
        total += wargs[i].result;

    free(threads);
    free(wargs);
    return PyLong_FromUnsignedLongLong(total);
}

/* ------------------------------------------------------------------ */
/* Module definition                                                    */
/* ------------------------------------------------------------------ */

static PyMethodDef methods[] = {
    {"singlecore", py_singlecore, METH_NOARGS,
     "singlecore() -> int\n\n"
     "Run a ~1-second Dhrystone-style integer benchmark on one core.\n"
     "Returns total iteration count; higher means faster CPU."},
    {"multicore",  py_multicore,  METH_NOARGS,
     "multicore() -> int\n\n"
     "Run a ~1-second benchmark simultaneously on all logical cores.\n"
     "Returns total iteration count across all cores."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef moddef = {
    PyModuleDef_HEAD_INIT,
    "cpuspeed",
    "Dhrystone-style integer CPU speed benchmark.",
    -1,
    methods
};

PyMODINIT_FUNC PyInit_cpuspeed(void)
{
    return PyModule_Create(&moddef);
}
