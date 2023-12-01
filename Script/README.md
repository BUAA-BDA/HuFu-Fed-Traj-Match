Command of Parameter Choosing:

```shell
python3 ./geoi.py --eps 0.01 --delta 0.00001
```

Command of Query Trajectory Publish:

```shell
python3 ./geoi.py --grid True --eps 0.01 --delta 0.00001 --dataset Dazhong
```

Command of Filter:

```shell
g++ filter.cpp -o filter
./filter dataset_name
```

Command of Partition:

```shell
g++ partition.cpp -o partition
./partition dataset_name
```

Command of Verify for the query user:

```shell
cd Verify && make
./verify address:port 1 dataset_name partition_num
```

Command of Verify for the data owners:

```shell
cd Verify && make
./verify address:port 2 dataset_name partition_num
```

