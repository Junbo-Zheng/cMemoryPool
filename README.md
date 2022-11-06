# cMemoryPool

## Overview
Plain C Memory Pool library for using in the host devices, it is a lightweight memory pool library, and is suitable for all embedded devices(**RTOS, Linux, etc.**).

## Features highlights
- support multiple memory pools configuration
  - memory ram address section configuration
  - memory blocks size configuration
  - memory pools size configuration
  - memory stables size configuration
  - memory align size configuration
- support multiple memory pools debug
  - output memory pools usage percentage
  - output memory pools malloc/free total count
  - check memory pools memory leaks
  - check memory pools double free
- blazing fast, non-blocking, robust implementation
- 100% static implementation
- dedicated for embedded systems
- multi-platform and portable
- only two source files(the other two files is optional for debug)

## Licensing
[cMemoryPool](https://github.com/Junbo-Zheng/cMemoryPool) is licensed under the Apache license, Check the [LICENSE](./LICENSE) file.

## Getting Started
Clone the repo, install dependencies, and serve:
```shell
git clone git@github.com:Junbo-Zheng/cMemoryPool.git
sudo apt-get install -y cmake gcc
```
### Build on Ubuntu20.04
```shell
$ mkdir -p build
$ cd build
$ cmake .. // or `cmake .. -DMEMORY_POOL_DEBUG=ON` for debug
$ make -j
$ ./memorypool
```
Or even better for build:
```shell
$ cmake -H. -Bbuild -DMEMORY_POOL_DEBUG=ON
$ cd build && make -j
$ ./memorypool
```

If you want to enable memory pool debug, `CONFIG_MEMORY_POOL_DEBUG` Macro need to be defined in advance by `cmake build -DMEMORY_POOL_DEBUG=ON` or `make -DCONFIG_MEMORY_POOL_DEBUG=1`. Of course, you can also define it directly in your source code:
```c
#define CONFIG_MEMORY_POOL_DEBUG 1
or
#define CONFIG_MEMORY_POOL_DEBUG
```

Also, if you enable memory pool debug check for memory pools, you'd better call the `memory_pool_debug_trace()` api on the idle tasks or the background tasks periodically, it will recalcute all memory pools usage information each time, but it is not recommended to call it very quickly, only as needed.

## Contribute
Anyone is welcome to contribute. Simply fork this repository, make your changes in an own branch and create a pull-request for your change. Please do only one change per pull-request.

You found a bug? Please fill out an issue and include any data to reproduce the bug.

## Contributor
- [Junbo Zheng](https://github.com/Junbo-Zheng)
