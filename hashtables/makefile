all: aggregator_std aggregator_flat aggregator_intkey aggregator_batch

aggregator_std: aggregator_std.cc
	g++ aggregator_std.cc -o aggregator_std -std=c++14 -Ofast -msse4.1 #-march=broadwell

aggregator_flat: aggregator_flat.cc
	g++ aggregator_flat.cc -o aggregator_flat -std=c++14 -Ofast -msse4.1 #-march=broadwell

aggregator_intkey: aggregator_intkey.cc
	g++ aggregator_intkey.cc -o aggregator_intkey -std=c++14 -Ofast -msse4.1 #-march=broadwell

aggregator_batch: aggregator_batch.cc
	g++ aggregator_batch.cc -o aggregator_batch -std=c++14 -Ofast -msse4.1 #-march=broadwell

clean: 
	rm aggregator_std aggregator_flat aggregator_intkey aggregator_batch