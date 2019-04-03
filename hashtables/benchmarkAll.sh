echo "--------- building all ---------"
make clean; make
echo "--------------------------------"

echo "---- running aggregator_std ----"
taskset -c 1 ./aggregator_std
echo "--------------------------------"

echo "---- running aggregator_flat ----"
taskset -c 1 ./aggregator_flat
echo "--------------------------------"

echo "---- running aggregator_intkey ----"
taskset -c 1 ./aggregator_intkey
echo "--------------------------------"

echo "---- running aggregator_batch ----"
taskset -c 1 ./aggregator_batch
echo "--------------------------------"
