# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 4.3)
project(StdBitcoin)

include(FetchContent)

enable_testing()

set(MP_UNITS_BUILD_CXX_MODULES OFF CACHE BOOL "" FORCE)
set(MP_UNITS_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
set(MP_UNITS_API_STD_FORMAT ON CACHE BOOL "" FORCE)
set(MP_UNITS_API_CONTRACTS NONE CACHE STRING "" FORCE)

FetchContent_Declare(mp_units SYSTEM EXCLUDE_FROM_ALL
  URL https://github.com/mpusz/mp-units/archive/refs/tags/v2.5.0.tar.gz
  SOURCE_SUBDIR src
  )
FetchContent_MakeAvailable(mp_units)

add_library(stdbtc)

target_compile_features(stdbtc PUBLIC cxx_std_20)
target_link_libraries(stdbtc PUBLIC mp-units::mp-units)

target_sources(stdbtc
  PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
    include/bitcoin/detail/basic_hash_id.hpp
    include/bitcoin/amount.hpp
    include/bitcoin/hash_id.hpp
    include/bitcoin/script.hpp
  PRIVATE
    src/amount.cpp
    src/basic_hash_id.cpp
    src/script.cpp
  )

add_subdirectory(tests)
