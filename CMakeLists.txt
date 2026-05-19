# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 4.3)
project(StdBitcoin)

enable_testing()

add_library(stdbtc)

target_compile_features(stdbtc PUBLIC cxx_std_20)

target_sources(stdbtc
  PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
    include/bitcoin/detail/basic_hash_id.hpp
    include/bitcoin/hash_id.hpp
  PRIVATE
    src/basic_hash_id.cpp
  )

add_subdirectory(tests)
