
#include <stdlib.h>
/*======================================
=            Configuration.            =
======================================*/

#define MESSAGE_COUNT 10000000
#define FLOW_COUNT 100000

/*=====  End of Configuration.  ======*/


struct Result{
  long int processedMfCt; 
  long int executionTime;
  long int flowCount;
};

struct MicroflowBin {
  uint64_t addrs;
  uint64_t ports;
};


MicroflowBin * inArr;
uint64_t loadedMfCt;
uint64_t curPos;


long int cTime(){
  struct timeval tp;
  gettimeofday(&tp, NULL);
  long int ms = tp.tv_sec * 1000 + tp.tv_usec / 1000;
  return ms;
}

// Generate maxMessages microflows from numFlows distinct flows.
void genRandomMicroflows(long int maxMessages, long int numFlows){
  // Generate random flow keys.
  MicroflowBin * flowKeys = (MicroflowBin *)malloc(numFlows * sizeof(MicroflowBin));
  for (int i = 0; i < numFlows; i++) {
    flowKeys[i].addrs = rand();
    flowKeys[i].ports = rand();
  }
  // Generate messages from flows uniformly selected from the keys.
  inArr = (MicroflowBin *)malloc(maxMessages * sizeof(MicroflowBin));
  for (int i = 0; i < maxMessages; i++) {
    int selectedFid = rand()%numFlows;
    // printf("(%i)\n",selectedFid);
    inArr[i].addrs = flowKeys[selectedFid].addrs;
    inArr[i].ports = flowKeys[selectedFid].ports;
  }
  // Clean up flow keys.
  free(flowKeys);
  loadedMfCt = maxMessages;
}

void printResults(Result r){
  int mf_ps = int(float(r.processedMfCt) / float(r.executionTime/1000.0));
  printf("microflows per second: %i\n",mf_ps);
  printf("flow records collected: %lu\n",r.flowCount);  
}