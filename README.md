# uftrace - A simple function call tracer for user programs

## About
ftrace is a program that simply runs the specified command until it
exits. It intercepts and records the user define function calls which are
called by the executed process. 
This command should be compiled with -g and -finstrument-functions
flags.

## Requirements
- libglib
- libelf
- libdwarf (should be compiled with -fPIC)
- binutils

## Build
If you use glib 2.0 or higher, you should add ``CPPFLAGS=-DG_DISABLE_DEPRECATED``.

```sh
./configure CPPFLAGS="$(pkg-config --cflags glib-2.0) -DG_DISABLE_DEPRECATED -fPIC"
make
sudo make install
```

## How to test
You can test uftrace by ``ftrace-test``.

```sh
ftrace ./ftrace-test
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

## Original README

```
                   ftrace - A function call tracer

$Id: README,v 1.6 2007/06/13 04:38:53 hamano Exp $

ftrace is a program that simply runs the specified command until it
exits. It intercepts and records the user define function calls which are
called by the executed process. 
This command should be compiled with -g and -finstrument-functions
flags.

* Requirements
  libglib-1.2
  libelf
  
* Build
  $ ./configure
  $ make
  # make install

* Usage
  ftrace [ command [ arg ...  ] ]
```

## Authors
### Original ftrace authors
- Masanobu Yasui <yasui-m@klab.org>
- Tsukasa Hamano <hamano@klab.org>

### uftrace author
- nbisco <mail@nbis.co>
