# The RUM Master
This directory contains the code for the master library of the RUM system. 

This code is responsible for capturing user memory accesses/modifications, and communicating with the RUM slaves to enact such memory accesses/modifications. 

This code build on the HPC system built by Rick Lan. 

## Requirements

* cmake
* C++11
* Boost

## Build

```
./build_lib.sh && ./build_benchmark.sh
```

## Run

```
./build_benchmark/benchmark -P 4096 -M 536870912 -T 4096 -S 920 -C 1 -A 64
```

Will need startup slave nodes FIRST.


Command line options:

```
./build_benchmark/benchmark --help
Allowed options:
  -H [ --help ]            show help message
  -P [ --pagesize ] arg    Page size in bytes
  -M [ --memorysize ] arg  Memory size in bytes
  -T [ --testsize ] arg    Number of random access to test
  -S [ --seed ] arg        Random seed, integer. Optional
  -C [ --threadcount ] arg Number of threads to use. Must be >= 2.
  -A [ --cachesize ] arg   Number of pages to use for cache.
```

