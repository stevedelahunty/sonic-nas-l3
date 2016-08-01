bin_SCRIPTS = scripts/bin/*

pyutilsdir=$(libdir)/sonic
pyutils_SCRIPTS = scripts/lib/python/*.py

nobase_include_HEADERS=sonic/hal_rt_extn.h

lib_LTLIBRARIES=libsonic_hal_routing.la

libsonic_hal_routing_la_SOURCES=src/hal_rt_arp.c src/hal_rt_debug.c \
         src/hal_rt_dr.c src/hal_rt_main.c src/hal_rt_mem.c \
         src/hal_rt_nh.c src/hal_rt_util.cpp src/hal_rt_host.c \
         src/hal_rt_route.c src/hal_rt_mpath.c src/hal_rt_mpath_grp.c \
         src/hal_rt_mpath_util.c src/nas_rt_api.c src/nas_rt_cps.c

libsonic_hal_routing_la_CPPFLAGS= -D_FILE_OFFSET_BITS=64 -I$(top_srcdir)/sonic -I$(includedir)/sonic

libsonic_hal_routing_la_LDFLAGS=-shared -lsonic_nas_linux -lsonic_common -lsonic_nas_ndi -lsonic_object_library -lsonic_cps_class_map -lsonic_logging -lcrypto -version-info 1:1:0

libsonic_hal_routing_la_CXXFLAGS=-std=c++11

