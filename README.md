# CppProjectTemplate

## Usage
Change project name at the directory and the CMakeLists's `project(CppProjectTemplate)`.

```shell script
mkdir -p build && cd build
cmake .. [-GNinja]
make -j or ninja
```

## Example

```shell script
cd build
./test/GenRandArray # A random array generator to generator 100,000,000 random integers
./test/PSort # Test __gnu_parallel::sort and ips4o::parallel::sort

