# Project 3: TCP Socket Programming
Tests were ran on Duke virtual machine with Ubuntu 18.

please see [project3.pdf](https://github.com/ys270/HOT_POTATO/blob/master/Project%203.pdf) for detailed requirement.
after complilation (make),

#### To run ringmaster:
Ringmaster process can be invoked with the format ./ringmaster <port_num> <num_players> <num_hops>

For example, the following command will create a master listening on port 1234, allowing 50 players connected and 50 hops for potato.

```
./ringmaster 1234 50 500
```

#### To run Player:
Player program can be invoked with the format ./player <machine_name> <port_num>

For example, the following command will create a player connecting to master vcm-1234.vm.duke.edu at port 1234.
```
./player vcm-1234.vm.duke.edu 1234
```



