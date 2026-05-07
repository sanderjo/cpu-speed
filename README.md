# cpu-speed
CPU Speed - both from python and command line

## On an Intel i3:

```
$ ./bench 
CPU benchmark (Dhrystone-style integer workload, 1 s per run)
Logical CPUs: 4

Single-core ... 113710000 iterations  (113.7 M/s)
Multi-core  (4 threads) ... 205650000 iterations  (205.7 M/s total, 51.4 M/s per core)
```

```
$ python3
Python 3.12.3 (main, Mar 23 2026, 19:04:32) [GCC 13.3.0] on linux
Type "help", "copyright", "credits" or "license" for more information.
>>> import cpuspeed
>>> cpuspeed.singlecore()/1_000_000
113.87
>>> cpuspeed.multicore()/1_000_000
275.23
>>> 
```

## On a K1 RISC-V

```
$ ./bench 
CPU benchmark (Dhrystone-style integer workload, 1 s per run)
Logical CPUs: 8

Single-core ... 61130000 iterations  (61.1 M/s)
Multi-core  (8 threads) ... 475780000 iterations  (475.8 M/s total, 59.5 M/s per core)
```

## Details

```
  Files                                                                                                                                                  
                                                                                                                                                         
  ┌────────────┬──────────────────────────────────────────────────┐                                                                                      
  │    File    │                     Purpose                      │                                                                                      
  ├────────────┼──────────────────────────────────────────────────┤                                                                                      
  │ cpuspeed.c │ Python C extension — the module itself           │                                                                                      
  ├────────────┼──────────────────────────────────────────────────┤                                                                                      
  │ bench.c    │ Standalone C program — same algorithm, no Python │                                                                                      
  ├────────────┼──────────────────────────────────────────────────┤                                                                                      
  │ setup.py   │ Alternative build via setuptools                 │                                                                                      
  ├────────────┼──────────────────────────────────────────────────┤                                                                                      
  │ Makefile   │ make all builds both targets                     │                                                                                      
  └────────────┴──────────────────────────────────────────────────┘                                                                                      
                                                                                                                                                         
  Build                                                                                                                                                  
                                                                                                                                                         
  make all          # build both                                                                                                                         
  make module       # Python extension only                                                                                                            
  make bench        # standalone binary only
                                                                                                                                                         
  Usage (Python)
                                                                                                                                                         
  import cpuspeed                                                                                                                                      
  sc = cpuspeed.singlecore()   # ~1 s, one core
  mc = cpuspeed.multicore()    # ~1 s, all cores in parallel                                                                                             
                                                                                                                                                         
  Returns an int — the number of inner loop iterations completed. Higher = faster CPU.                                                                   
                                                                                                                                                         
  Benchmark core (run_core in both files)                                                                                                                
                                                                                                                                                       
  Each iteration executes 16 integer operations: add, sub, xor, shift, multiply, and array accesses across a 16-element uint32_t array. Cross-iteration  
  data dependencies prevent the compiler from vectorising or unrolling the work away. A checksum over v[] is "used" at the end to prevent dead-code    
  elimination. The time check happens every 10 000 inner iterations so clock_gettime overhead is negligible.                                             

                                                                                                                                                   
  For multicore, pthreads are spawned (one per logical CPU via _SC_NPROCESSORS_ONLN), the GIL is released with Py_BEGIN_ALLOW_THREADS, and results are   
  summed after all threads join.
```
