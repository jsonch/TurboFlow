#include <pcap.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <math.h>
#include <stdlib.h>

#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream> // for ostringstream
using namespace std;

// Generate 
// Dump the microflow records the TurboFlow microflow generator 
// would send to the switch CPU. 
// parameters: 
// 1 -- input pcap trace (expects IP trace, change traceType for eth)
// 2 -- height of the hash table (number of flow slots)

// g++ turboflow.cpp -o turboflow -lpcap -std=c++11
// ./turboflow ~/datasets/caida2015/caida2015_02_dirA.pcap 13


uint64_t dbg_packetCt;
uint64_t dbg_evictCt;

uint64_t dbg_collisionCt;
uint64_t dbg_removeFlowCt;
uint64_t dbg_addFlowCt;


// Static options.
#define traceType 1 // 0 = ethernet pcap, 1 = ip4v pcap (i.e., caida datasets)
#define KEYLEN 12 // key length in bytes.
#define STOP_CT 10000000 // stop execution after STOP_CT packets.
#define LOG_CT 1000000 // print info every LOG_CT packets.

char nullKey[KEYLEN] = { 0 };

// Internal structures. 
struct MicroflowRecord {
  char key[KEYLEN];
  uint32_t byteCount;  
  uint16_t packetCount;  
};

struct Metadata {
  std::string key;
  unsigned hash;
  uint32_t byteCount;  
  uint64_t ts;
};

// Global state. 
uint32_t TABLELEN;
uint32_t packetSinceEvict = 0;
uint32_t packetsPerEvict;
// Table of hash -> keys.
char ** keyTable;
// Table of hash -> byte counts.
uint32_t * byteCountTable;
// Table of hash -> packet counts.
uint16_t * packetCountTable;

uint64_t startTs = 0;
uint64_t dur = 0;

// init global state for turboflow.
void stateInit(int tableSize);

// Handle packets.
void packetHandler(u_char *userData, const struct pcap_pkthdr* pkthdr, const u_char* packet);

// update microflow tables. 
void updateTables(Metadata md);

// Write a micro flow record to a file or std out.
void dumpMicroflowRecord(MicroflowRecord mfr, bool collision);

// Helper functions.
void printHeader();
unsigned simpleHash(const char* s, int len, int maxHashVal);
std::string getKey(const struct ip* ipHeader, const struct tcphdr* tcpHeader);
uint64_t getMicrosecondTs(uint32_t seconds, uint32_t microSeconds);
void printStats();


int main(int argc, char *argv[]) {
  if (argc != 3){
    cout << "incorrect number of arguments. Need 2. (filename, hash size)." << endl;
    return 0;
  }
  char * inputFile = argv[1];
  cout << "reading from file: " << inputFile << endl;
  // Setup state. 
  int tableSize = atoi(argv[2]);
  stateInit(tableSize);

  pcap_t *descr;
  char errbuf[PCAP_ERRBUF_SIZE];
  // open capture file for offline processing
  descr = pcap_open_offline(inputFile, errbuf);
  printHeader();
  if (descr == NULL) {
      cerr << "pcap_open_live() failed: " << errbuf << endl;
      return 1;
  }
  // start packet processing loop, just like live capture
  if (pcap_loop(descr, 0, packetHandler, NULL) < 0) {
      cerr << "pcap_loop() failed: " << pcap_geterr(descr);
      return 1;
  }
  cout << "FINAL STATS:" << endl;
  printStats();

  return 0;
}

void stateInit(int tableSize){
  TABLELEN = tableSize;
  cout << "initializing hash tables to size: " << TABLELEN << endl;
  // Keys. 
  keyTable = new char*[tableSize];
  for (int i = 0; i< tableSize; i++){
    keyTable[i] = new char[KEYLEN];
  }
  // Counters. 
  byteCountTable = new uint32_t[tableSize];
  packetCountTable = new uint16_t[tableSize];

  // cout << "setting to attempt eviction once every " << pktsPerEvict << " packets " << endl;
  // packetsPerEvict = pktsPerEvict;
  return;  
}


// The packet handler that implements the flow record generator. 
void packetHandler(u_char *userData, const struct pcap_pkthdr* pkthdr, const u_char* packet) {
  const struct ether_header* ethernetHeader;
  const struct ip* ipHeader;
  const struct tcphdr* tcpHeader;

  if (traceType == 0){
    ethernetHeader = (struct ether_header*)packet;
    if (ntohs(ethernetHeader->ether_type) == ETHERTYPE_IP) {
        ipHeader = (struct ip*)(packet + sizeof(struct ether_header));
    }
  }
  else if (traceType == 1) {
    ipHeader = (struct ip*)(packet);

  }
  tcpHeader = (tcphdr*)((u_char*)ipHeader + sizeof(struct ip));

  // Build metadata.
  Metadata md;  
  md.key = getKey(ipHeader, tcpHeader);
  md.ts = getMicrosecondTs(pkthdr->ts.tv_sec, pkthdr->ts.tv_usec);
  md.byteCount = pkthdr->len;
  md.hash = simpleHash(md.key.c_str(), KEYLEN, TABLELEN);
  if (startTs == 0){
    startTs = md.ts;
  }
  dur = md.ts - startTs;  

  // Update microflow tables.
  updateTables(md);

  // break after STOP_CT packets.
  dbg_packetCt++;
  #ifdef STOP_CT
    if (dbg_packetCt > STOP_CT){
      printStats();
      exit(1);
    }
  #endif
  #ifdef LOG_CT
    if (dbg_packetCt % LOG_CT == 0){
      printStats();
    }
  #endif
}

void updateTables(Metadata md){
  // increment packet counter. 
  packetSinceEvict++;
  // update key table.
  // read key at hash.
  char curKey[KEYLEN];
  memcpy(curKey, keyTable[md.hash], KEYLEN);
  bool evictedFlow = false;
  MicroflowRecord evictedMfr;
  // cout << "hash: " << md.hash << endl;
  // if the key is null, insert new entry. 
  if (memcmp(curKey, nullKey, KEYLEN) == 0){
    dbg_addFlowCt++;
    // cout << "inserting new. " << endl;
    memcpy(keyTable[md.hash], md.key.c_str(), KEYLEN);
    packetCountTable[md.hash] = 1;
    byteCountTable[md.hash] = md.byteCount;
  }
  else {
    // if key matches packet's key, update. 
    if (memcmp(curKey, md.key.c_str(), KEYLEN) == 0){
      packetCountTable[md.hash]++;
      byteCountTable[md.hash]+= md.byteCount;
    }
    // otherwise, it is a collision. Evict and then replace. 
    else {
      // Evict.
      evictedFlow = true;
      memcpy(evictedMfr.key, curKey, KEYLEN);
      evictedMfr.packetCount = packetCountTable[md.hash];
      evictedMfr.byteCount = byteCountTable[md.hash];
      // Replace.
      memcpy(keyTable[md.hash], md.key.c_str(), KEYLEN);
      packetCountTable[md.hash] = 1;
      byteCountTable[md.hash] = md.byteCount;

    }
  }
  // write microflow record if anything was evicted.
  if (evictedFlow){
    dumpMicroflowRecord(evictedMfr, true);    
  }
  return;
}


void printHeader(){
  cout << "packet counts, trace time (ms), packets per microflow" << endl;
}

void printStats(){
    float packetsPerMicroflow = float(dbg_packetCt) / float(dbg_evictCt);
  cout << dbg_packetCt << "," << dur/1000 << "," << packetsPerMicroflow << endl;
  // fwrite(&mfr, 1, sizeof(mfr), stdout);
  return;
}


void dumpMicroflowRecord(MicroflowRecord mfr, bool collision){
  if (collision) dbg_collisionCt++;
  else dbg_removeFlowCt++;

  dbg_evictCt++;
  // Just write the microflow record to stdout. 
  // fwrite(&mfr, 1, sizeof(mfr), stdout);
  return;
}


std::string getKey(const struct ip* ipHeader, const struct tcphdr* tcpHeader){
  char keyBuf[KEYLEN];
  memcpy(&(keyBuf[0]), &ipHeader->ip_src, 4);
  memcpy(&(keyBuf[4]), &ipHeader->ip_dst, 4);
  memcpy(&(keyBuf[8]), &tcpHeader->source, 2);
  memcpy(&(keyBuf[10]), &tcpHeader->dest, 2);
  std::string key = string(keyBuf, KEYLEN);
  return key;
}

// Get 64 bit timestamp.
uint64_t getMicrosecondTs(uint32_t seconds, uint32_t microSeconds){
  uint64_t sec64, ms64;
  sec64 = (uint64_t) seconds;
  ms64 = (uint64_t) microSeconds;
  uint64_t ts = sec64 * 1000000 + ms64;
  return ts;
}
// A simple hashing function.
unsigned simpleHash(const char* s, int len, int maxHashVal)
{
    unsigned h = 0;
    for (int i=0; i<len; i++){
      h = h * 101 + (unsigned)s[i];
    }
    return h % maxHashVal;
}
