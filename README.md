# jfind

Find utility for java classes.  Currently works for files compiled with Java 6 compatibility.
I would like to add compatibility up through 8 (and some other features), but haven't had time, 
and I figured this might be useful, so here it is.

## Usage

usage: jfind path resource-name

The resource-name can be a full package/class name, such as org.sl4j.Logger, or can include
wildcards, such as 'org.sl4j.*' or '*.Logger'.

When jfind encounters a jar, war, or ear, it will decompress it (and compressed contents)
to a temporary location and continue the search.

No guarantees are made about this utility.  It's something I created for troubleshooting
with the hope that it would be faster than the shell scripts I used previously.

### Examples: 

To find if/where SLF4J was distributed as part of a popular application server, potentially 
leading to classpath and versioning problems:

$ jfind Middleware/wlserver org.slf4j.Logger

To determine if a war contains any Apache dependencies:
$ jfind mywar.war 'org.apache.*'

To find a class that I've forgotten the package name of:
$  jfind . '*.MyClass'

## Required Libraries

### RHEL/CentOS based
The following packages should be installed on the build system: gcc, make, zlib-devel, libzip-devel
Systems that run jfind will require zlib and libzip.

### Debian based
The following packages should be installed on the build system: build-essentials, libzip, libzip-dev, 
Systems that run jfind will require libzip and zlib1g.

## Build instructions

This is a simple application that uses dynamic linking by default.  Some environments will not have
libzip, and it may be easier to compile with static linking than it is to get libzip installed.
Instructions for both are provided.

### Dynamic linking:

make clean; make

### Static linking on Debian based systems:

make clean; make static-deb

Static linking on other systems:

It seems most system have libz.so by default, so there isn't much need to statically link that.
libzip is another story, and as previously mentioned, non-technical restrictions can impose a
problem for getting libzip loaded.  If this is your story, I empathize, and encourage you to
do the following.

Grab the source for libzip from http://www.nih.at/libzip/ and extract it somewhere.
Build libzip from its source root (./configure && make)
Copy libz.a to the jfind build directory (cp lib/.libs/libzip.a ~/path/to/jfind/)
Build jfind from its source root: make clean; make semi-static

