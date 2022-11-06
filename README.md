# cMemoryPool

## Overview
Plain C Memory Pool library for use in host devices. It is a lightweight memory pool library, and is suitable for all embedded devices.

## Features highlights
- support multiple memory pools configuration
  - memory ram address configuration
  - memory block size configuration
  - memory pools size configuration
  - memory manager stable size configuration
  - memory align size configuration
- support multiple memory pools debug
  - output memory pools usage percentage
  - output malloc/free total count
  - check memory leaks
  - check double free memory
- blazing fast, non-blocking, robust implementation
- 100% static implementation
- dedicated for embedded systems
- multi-platform and portable
- only two source files(the other two files is optional for debug)

## Licensing
cMemoryPool is licensed under the Apache license. Check the [LICENSE](./LICENSE) file.

## Getting Started
Clone the repo, install dependencies, and serve:
```
git clone git@github.com:Junbo-Zheng/cMemoryPool.git
sudo apt-get install -y cmake gcc
```
### Build
```
$ mkdir -p build
$ cd build
$ cmake .. // or cmake .. -DMEMORY_POOL_DEBUG=ON
$ make -j16
$ ./memorypool
```
Or even better
```
$ cmake -H. -Bbuild -DMEMORY_POOL_DEBUG=ON
$ cd build && make -16
$ ./memorypool
```

If you want to enable memory pool debug, `CONFIG_MEMORY_POOL_DEBUG` Macro need to be defined in advance by `cmake build -DMEMORY_POOL_DEBUG=ON` or `make -DCONFIG_MEMORY_POOL_DEBUG`. Of course, you can also define it directly in your code:
```
#define CONFIG_MEMORY_POOL_DEBUG 1
```

Also, if you enable debug function for memory pools, it is best to periodically call the `memory_pool_debug_trace()` api on idle tasks or background tasks, recalculating all memory usage information each time it is called, but it is not recommended to call it very fast, only as needed.

## Contribute
Anyone is welcome to contribute. Simply fork this repository, make your changes in an own branch and create a pull-request for your change. Please do only one change per pull-request.

You found a bug? Please fill out an issue and include any data to reproduce the bug.

## Contributor
[Junbo Zheng](https://github.com/Junbo-Zheng)
