# uftrace - A simple function call tracer for user programs

## About
uftrace is a program that simply runs the specified command until it
exits. It intercepts and records the user define function calls which are
called by the executed process. 
This command should be compiled with _-g_ and _-finstrument-functions_
flags.

## Requirements
- libglib
- libelf
- libdwarf (should be compiled with -fPIC)
- binutils

## Build
```sh
./configure 
make
sudo make install
```

## How to use
1. build your program with ``-g -finstrument-functions``
1. run your program with uftrace command like this: ``uftrace <your-program> <your program args>``
1. uftrace shows ``HH:MM:SS.SSSSSS [pid] funcname(args)`` such as below

```sh
uftrace ./uftrace-test
16:37:59.084267 [09244] main(0, 0x7fff2f310a38)
16:37:59.084393 [09244] arg_int1(10)
16:37:59.084413 [09244] arg_int2(10, 11)
16:37:59.084431 [09244] arg_int3(10, 11, 12)
16:37:59.084449 [09244] arg_uint1(10)
16:37:59.084466 [09244] arg_uint2(10, 11)
16:37:59.084482 [09244] arg_uint3(10, 11, 12)
16:37:59.084500 [09244] arg_char1(20)
16:37:59.084516 [09244] arg_char2(20, 21)
16:37:59.084532 [09244] arg_char3(20, 21, 22)
16:37:59.084549 [09244] arg_short1(30)
16:37:59.084565 [09244] arg_short2(30, 31)
16:37:59.084582 [09244] arg_short3(30, 31, 32)
16:37:59.084599 [09244] arg_long1(40)
16:37:59.084615 [09244] arg_long2(40, 41)
16:37:59.084632 [09244] arg_long3(40, 41, 42)
16:37:59.084649 [09244] arg_ulong1(50)
16:37:59.084665 [09244] arg_ulong2(50, 51)
16:37:59.084682 [09244] arg_ulong3(50, 51, 52)
16:37:59.084699 [09244] arg_longlong1(60)
16:37:59.084715 [09244] arg_longlong2(60, 61)
16:37:59.084731 [09244] arg_longlong3(60, 61, 62)
16:37:59.084749 [09244] arg_ulonglong1(70)
16:37:59.084764 [09244] arg_ulonglong2(70, 71)
16:37:59.084781 [09244] arg_ulonglong3(70, 71, 72)
16:37:59.084798 [09244] arg_test(100, 0x7fff2f3109f8)
```

You can see simple help by ``uftrace -h``

## Options and Examples
- ``-l`` option adds file name and line no to output

```sh
$ uftrace -l ./uftrace-test
16:45:49.183697 [07612] (uftrace-test.c:  140) main(1, 0xbf872114)
16:45:49.184456 [07612] (uftrace-test.c:   40) arg_int1(10)
16:45:49.184600 [07612] (uftrace-test.c:   36) arg_int2(10, 11)
(snipped)
```

- ``-T`` option adds thread id to output

```sh
$ uftrace -l -T ./uftrace-test
16:47:32.000872 [07666.0xb7232700] (uftrace-test.c:  140) enter main(1, 0xbfe35034)
                       ^^^^^^^^^^thread id
                 ^^^^^pid
16:47:32.001920 [07666.0xb7232700] (uftrace-test.c:   40) enter arg_int1(10)
16:47:32.002073 [07666.0xb7232700] (uftrace-test.c:   36) enter arg_int2(10, 11)
16:47:32.002215 [07666.0xb7232700] (uftrace-test.c:   28) enter arg_int3(10, 11, 12)
(snipped)
```

- ``-e`` option enables uftrace to output when exit function

```sh
$ uftrace -l -T -e ./uftrace-test
16:47:32.000872 [07666.0xb7232700] (uftrace-test.c:  140) enter main(1, 0xbfe35034)
16:47:32.001920 [07666.0xb7232700] (uftrace-test.c:   40) enter arg_int1(10)
16:47:32.002073 [07666.0xb7232700] (uftrace-test.c:   36) enter arg_int2(10, 11)
16:47:32.002215 [07666.0xb7232700] (uftrace-test.c:   28) enter arg_int3(10, 11, 12)
16:47:32.002362 [07666.0xb7232700] (uftrace-test.c:   28) exit  arg_int3(10, 11, 12)
16:47:32.002507 [07666.0xb7232700] (uftrace-test.c:   36) exit  arg_int2(10, 11)
16:47:32.002644 [07666.0xb7232700] (uftrace-test.c:   40) exit  arg_int1(10)
16:47:32.002776 [07666.0xb7232700] (uftrace-test.c:   52) enter arg_uint1(10)
(snipped)
```

- ``-f <regex pattern>`` option filter output matching ``<regex pattern>``. ``<regex pattern>`` matches file name and or function name.

```sh
$ uftrace -l -T -f "_int" ./uftrace-test # filter arg_int* before arg_uint*
17:16:48.067302 [08301.0xb7237700] (uftrace-test.c:  140) main(1, 0xbfb7dc44)
17:16:48.068951 [08301.0xb7237700] (uftrace-test.c:   52) arg_uint1(10)
17:16:48.069196 [08301.0xb7237700] (uftrace-test.c:   48) arg_uint2(10, 11)
17:16:48.069438 [08301.0xb7237700] (uftrace-test.c:   45) arg_uint3(10, 11, 12)
17:16:48.069691 [08301.0xb7237700] (uftrace-test.c:   64) arg_char1(20)
(snipped)
```
- ``-o <file_prefix>`` option output result to file(file name: ``<file_prefix>``.pid)
 - ``-t`` add thread id to file name (valid with ``-o`` option, file name: ``<file_prefix>``.pid.threadid)
- ``-s`` option output result to syslog

## TODO
- add call stack level filter
- add nest-exclude filter
- more stability, more test
- test automation
- support RHEL family (CentOS, etc.)
- glib-free



## Authors
### Original ftrace authors
- Masanobu Yasui <yasui-m@klab.org>
- Tsukasa Hamano <hamano@klab.org>

### uftrace author
- nbisco <mail@nbis.co>
