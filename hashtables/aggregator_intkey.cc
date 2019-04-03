#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <functional>
#include <memory>

#include "common.h"

// Aggregator implementation using flat map hash table with 128b int key. 
#include "optimizations.h"



void initHashTable();
Result runBenchmark();
int main(int argc, char *argv[]) {
  printf("version: integer keys\n");
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

// The actual table update function. Just emplace and increment counter.
inline void updateFcn(){
  // emplace.
  __m128i *k = (__m128i *)&inArr[curPos];
  flow_map.emplace(*k, 0);
  // update.
  flow_map[*k]++;
}

Result runBenchmark()
{
  uint matchCount = 0;
  uint missCount = 0;
  long int stime, ftime;

  stime = cTime();  
  for (curPos=0; curPos<loadedMfCt; curPos++){
    updateFcn();
  }
  ftime = cTime();

  Result r;
  r.processedMfCt = loadedMfCt;
  r.flowCount = flow_map.size();
  r.executionTime = ftime - stime;
  return r;
}