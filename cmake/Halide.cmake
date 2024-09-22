# TODO: Detect and pull correct platform
FetchContent_Declare(
  halide
  URL https://github.com/halide/Halide/releases/download/v18.0.0/Halide-18.0.0-x86-64-linux-8c651b459a4e3744b413c23a29b5c5d968702bb7.tar.gz
  URL_HASH MD5=1902be198404231490d415cb852827a8
)
FetchContent_MakeAvailable(halide)

list(APPEND CMAKE_PREFIX_PATH "${halide_SOURCE_DIR}")

find_package(Halide REQUIRED PATHS "${halide_SOURCE_DIR}/lib")
