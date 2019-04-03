#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <functional>
#include <memory>

#include "common.h"

// Aggregator implementation using flat map hash table with 128b int key and batching.
#define BATCHING TRUE
#include "optimizations.h"


void initHashTable();
Result runBenchmark();
int main(int argc, char *argv[]) {
  printf("version: batching map -- batch size: %i\n",PREFETCHCT);
  printf("flows: %i records: %i\n", FLOW_COUNT, MESSAGE_COUNT);

  genRandomMicroflows(MESSAGE_COUNT, FLOW_COUNT);
  initHashTable();

  Result r = runBenchmark();
  printResults(r);
  free(inArr);
}

void initHashTable(){
  // reserve at least enough for the expected number of active flows. 
  // A prime number gives better performance.
  flow_map.reserve(8296553);
}


inline void parseFcn(int parsej){
  // cast the keys in the batch to 128 bit ints.
  v[parsej] = (__m128i*)(inArr+curPos+parsej);
  // calculate the addresses where the records would be stored in the hash table.
  ptrs[parsej] = flow_map.getPtr(*v[parsej]);

  // ptrs[parsej] = flow_map.getPtr(((__m128i*)(inArr+curPos+parsej))[0]);
  // fetch records for all keysin the batch. 
  __builtin_prefetch((const void*)(ptrs[parsej]),0,0);
}


inline void updateFcn(int updatej){
  // update the records in the batch.
  // emplace.
  __m128i *k = v[updatej];
  auto v = flow_map.emplace(*k, 0);
  // update.
  flow_map[*k]++;
}

Result runBenchmark()
{
  Result r;
  uint matchCount = 0;
  uint missCount = 0;
  long int stime, ftime;

  stime = cTime();  
  for (curPos=0; curPos<loadedMfCt; curPos+=PREFETCHCT) {
    unroll_f(_int<PREFETCHCT-1>(),parseFcn); // parse the batch in an unrolled loop.
    unroll_f(_int<PREFETCHCT-1>(),updateFcn); // update the batch in an unrolled loop.
  }
  ftime = cTime();

  r.processedMfCt = loadedMfCt;
  r.flowCount = flow_map.size();
  r.executionTime = ftime - stime;
  return r;
}
