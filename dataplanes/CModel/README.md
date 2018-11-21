This is a reference C implementation of the TurboFlow dataplane (i.e., microflow generator).

Usage: 
./turboflow <input pcap> <cache size>

- modify dumpMicroflowRecord if you want to save evicted microflows to a file. 
- see example.sh for an example of running it.
- In the paper, this implementation was used to evaluate eviction rates with CAIDA header traces: https://www.caida.org/data/passive/passive_dataset.xml
