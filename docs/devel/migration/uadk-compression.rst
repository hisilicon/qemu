=========================================================
User Space Accelerator Development Kit (UADK) Compression
=========================================================
UADK is a framework for user application to access hardware accelerator in a
unified, secure, and efficient way. UADK is comprised of UACCE, libwd and many
other algorithm libraries for different applications.

https://github.com/Linaro/uadk/tree/master/docs

UADK Framework
==============

::

          +----------------------------------+
          |                apps              |
          +----+------------------------+----+
               |                        |
               |                        |
       +-------+--------+       +-------+-------+
       |   scheduler    |       | alg libraries |
       +-------+--------+       +-------+-------+
               |                         |
               |                         |
               |                         |
               |                +--------+------+
               |                | vendor drivers|
               |                +-+-------------+
               |                  |
               |                  |
            +--+------------------+--+
            |         libwd          |
    User    +----+-------------+-----+
    --------------------------------------------------
    Kernel    +--+-----+   +------+
              | uacce  |   | smmu |
              +---+----+   +------+
                  |
              +---+------------------+
              | vendor kernel driver |
              +----------------------+
    --------------------------------------------------
             +----------------------+
             |   HW Accelerators    |
             +----------------------+

UADK Installation
================================================================
#. Build UADK

   * git clone https://github.com/Linaro/uadk.git
   * cd uadk
   * mkdir build
   * ./autogen.sh
   * ./configure --prefix=$PWD/build
   * make
   * make install

   Without --prefix, UADK will be installed to /usr/local/lib by default.
   If get error:"cannot find -lnuma", please install the libnuma-dev

#. Run pkg-config libwd to ensure env is setup correctly

   * export PKG_CONFIG_PATH=$PWD/build/lib/pkgconfig
   * pkg-config libwd --cflags --libs
     -I/usr/local/include -L/usr/local/lib -lwd

   * export PKG_CONFIG_PATH is required on demand,
     not needed if UADK is installed to /usr/local/lib

How To Use UADK Compression In Migration
========================================
   * Make sure UADK is installed as above
   * Build ``Qemu`` with ``--enable-uadk`` parameter

     E.g. configure --target-list=aarch64-softmmu --enable-kvm ``--enable-uadk``

   * Enable ``UADK`` compression during migration

     Set ``migrate_set_parameter multifd-compression uadk``
