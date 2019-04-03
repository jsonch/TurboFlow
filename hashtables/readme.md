### Hash table implementations from TurboFlow ###


#### Overview ####
This directory contains simple implementations of the core hash table data-structures evaluated with TurboFlow. 

Each program contains a variant of the data-structure: 
- ```aggregator_std```: uses c++ unordered_map as the hash table.
- ```aggregator_flat```: uses a custom flat hash table implementation.
- ```aggregator_intkey```: uses 128 bit integer keys with custom key comparison and hashing operations.
- ```aggregator_batch```: batches updates to the hash table.

Each program also contains a simple benchmarking wrapper that: 
1) generates random microflow-like messages, randomly distributed across a number of flows.
2) uses the hash table to count the number of microflows in each flow. 
3) measures hash table throughput in messages per second.

#### Usage ####
```
jsonch@johnshack:~/gits/turboflow/hashtables$ ./benchmarkAll.sh 
--------- building all ---------
rm aggregator_std aggregator_flat aggregator_intkey aggregator_batch
g++ aggregator_std.cc -o aggregator_std -std=c++14 -Ofast -msse4.1 #-march=broadwell
g++ aggregator_flat.cc -o aggregator_flat -std=c++14 -Ofast -msse4.1 #-march=broadwell
g++ aggregator_intkey.cc -o aggregator_intkey -std=c++14 -Ofast -msse4.1 #-march=broadwell
g++ aggregator_batch.cc -o aggregator_batch -std=c++14 -Ofast -msse4.1 #-march=broadwell
--------------------------------
---- running aggregator_std ----
version: std unordered map
flows: 100000 records: 10000000
microflows per second: 1848770
flow records collected: 100000
--------------------------------
---- running aggregator_flat ----
version: flat hash table
flows: 100000 records: 10000000
microflows per second: 2541942
flow records collected: 100000
--------------------------------
---- running aggregator_intkey ----
version: integer keys
flows: 100000 records: 10000000
microflows per second: 5479452
flow records collected: 100000
--------------------------------
---- running aggregator_batch ----
version: batching map -- batch size: 10
flows: 100000 records: 10000000
microflows per second: 7800312
flow records collected: 100000
--------------------------------
```

#### Notes ####
- The 128 bit integer operations require gcc >= 5.5.
- On many systems, the -march=<CHIPSET> flag can improve performance. 
- Optimal batch size depends on the system. Change PREFETCHCT (default 10) in optimizations.h.
