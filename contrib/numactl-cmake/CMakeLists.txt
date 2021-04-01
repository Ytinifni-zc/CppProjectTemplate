set(NUMA_SOURCE_DIR ${CMAKE_SOURCE_DIR}/contrib/numactl)

set(SRCS
        ${NUMA_SOURCE_DIR}/affinity.c
        ${NUMA_SOURCE_DIR}/affinity.h
        ${NUMA_SOURCE_DIR}/clearcache.c
        ${NUMA_SOURCE_DIR}/clearcache.h
        ${CMAKE_CURRENT_SOURCE_DIR}/config.h
        ${NUMA_SOURCE_DIR}/distance.c
        ${NUMA_SOURCE_DIR}/libnuma.c
        ${NUMA_SOURCE_DIR}/memhog.c
        ${NUMA_SOURCE_DIR}/migratepages.c
        ${NUMA_SOURCE_DIR}/migspeed.c
        ${NUMA_SOURCE_DIR}/mt.c
        ${NUMA_SOURCE_DIR}/mt.h
        ${NUMA_SOURCE_DIR}/numa.h
        ${NUMA_SOURCE_DIR}/numactl.c
        ${NUMA_SOURCE_DIR}/numademo.c
        ${NUMA_SOURCE_DIR}/numaif.h
        ${NUMA_SOURCE_DIR}/numaint.h
        ${NUMA_SOURCE_DIR}/numamon.c
        ${NUMA_SOURCE_DIR}/numastat.c
        ${NUMA_SOURCE_DIR}/rtnetlink.c
        ${NUMA_SOURCE_DIR}/rtnetlink.h
        ${NUMA_SOURCE_DIR}/shm.c
        ${NUMA_SOURCE_DIR}/shm.h
        ${NUMA_SOURCE_DIR}/stream_lib.c
        ${NUMA_SOURCE_DIR}/stream_lib.h
        ${NUMA_SOURCE_DIR}/stream_main.c
        ${NUMA_SOURCE_DIR}/syscall.c
        ${NUMA_SOURCE_DIR}/sysfs.c
        ${NUMA_SOURCE_DIR}/sysfs.h
        ${NUMA_SOURCE_DIR}/util.c
        ${NUMA_SOURCE_DIR}/util.h
        )

add_library(numa STATIC ${SRCS})

target_include_directories(numa PUBLIC ${NUMA_SOURCE_DIR})
