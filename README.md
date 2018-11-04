[![Build Status](https://travis-ci.org/pqrs-org/cpp-osx-cf_run_loop_thread.svg?branch=master)](https://travis-ci.org/pqrs-org/cpp-osx-cf_run_loop_thread)
[![License](https://img.shields.io/badge/license-Boost%20Software%20License-blue.svg)](https://github.com/pqrs-org/cpp-osx-cf_run_loop_thread/blob/master/LICENSE.md)

# cpp-osx-cf_run_loop_thread

`pqrs::cf_run_loop_thread` provides a thread for `CFRunLoop`.

## Requirements

cpp-osx-cf_run_loop_thread depends the following classes.

- [pqrs::cf_ptr](https://github.com/pqrs-org/cpp-osx-cf_ptr).
- [pqrs::thread_wait](https://github.com/pqrs-org/cpp-thread_wait).

## Install

### Manual install

Copy `include/pqrs` directory into your include directory.

### Using package manager

You can also install `include/pqrs` by using [cget](https://github.com/pfultz2/cget).

```shell
cget install pqrs-org/cpp-osx-cf_run_loop_thread@v1.1.0 --cmake header
cget install pqrs-org/cpp-osx-cf_ptr@v1.1.0 --cmake header
cget install pqrs-org/cpp-thread_wait@v1.1.0 --cmake header
```
