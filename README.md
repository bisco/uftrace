# ftrace - A function call tracer for user programs

## Requirements
- libglib
- libelf
- libdwarf (should be compiled with -fPIC)
- binutils

## Build
If you use glib 2.0 or higher, you should add ``CPPFLAGS=-DG_DISABLE_DEPRECATED``.
```sh
./configure CPPFLAGS="-I/path/to/glib/include -DG_DISABLE_DEPRECATED -fPIC"
make
make install
```

## Original README
```
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
