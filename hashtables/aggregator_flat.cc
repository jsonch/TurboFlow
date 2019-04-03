#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <functional>
#include <memory>

// #include <unordered_map>
#include "flat_hash_map.hpp"
#include "common.h"

// Aggregator implementation using flat map hash table. 
// std::unordered_map<std::string, int> flow_map;
ska::flat_hash_map<std::string, int> flow_map;


void initHashTable();
Result runBenchmark();
int main(int argc, char *argv[]) {
  printf("version: flat hash table\n");
  printf("flows: %i records: %i\n", FLOW_COUNT, MESSAGE_COUNT);

  genRandomMicroflows(MESSAGE_COUNT, FLOW_COUNT);
  initHashTable();

  Result r = runBenchmark();
  printResults(r);
  free(inArr);
}

void initHashTable(){
  // reserve at least enough for the expected number of active flows. 
  // Using a prime number gives best performance.
  flow_map.reserve(8296553);
}

// The actual table update function. Just emplace and increment counter.
inline void updateFcn(){
  std::string keyStr = std::string((char *)&inArr[curPos], sizeof(MicroflowBin));
  // emplace.
  flow_map.emplace(keyStr, 0);
  // update.
  flow_map[keyStr]++;

}


Result runBenchmark()
{
  uint matchCount = 0;
  uint missCount = 0;
  long int stime, ftime;

  // do the benchmark.
  stime = cTime();  
  // Run update once for each loaded record.
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
