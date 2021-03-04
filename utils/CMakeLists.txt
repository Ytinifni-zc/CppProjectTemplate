add_library(PODArray STATIC PODArray.cpp PODArray.h)
target_link_libraries(PODArray numa jemalloc)