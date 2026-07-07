# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 4.3)
project(StdBitcoin)

include(FetchContent)

enable_testing()

# BitcoinCore build options are not properly prefixed
set(BUILD_KERNEL_LIB ON)
set(BUILD_BENCH OFF)
set(BUILD_BITCOIN_BIN OFF)
set(BUILD_CLI OFF)
set(BUILD_DAEMON OFF)
set(BUILD_FOR_FUZZING OFF)
set(BUILD_FUZZ_BINARY OFF)
set(BUILD_GUI OFF)
set(BUILD_KERNEL_TEST OFF)
set(BUILD_TESTS OFF)
set(BUILD_TX OFF)
set(BUILD_UTIL OFF)
set(BUILD_UTIL_CHAINSTATE OFF)
set(BUILD_WALLET_TOOL OFF)
set(ENABLE_EXTERNAL_SIGNER OFF)
set(ENABLE_IPC OFF)
set(ENABLE_WALLET OFF)

FetchContent_Declare(BitcoinCore EXCLUDE_FROM_ALL
  GIT_REPOSITORY https://github.com/bitcoin/bitcoin.git
  GIT_TAG 0bc9d354dfd8074d1c36a891a69b6585a8775c65
  )

FetchContent_Declare(beman.any_view SYSTEM EXCLUDE_FROM_ALL
  URL https://github.com/bemanproject/any_view/archive/refs/tags/v1.1.0.tar.gz
  )

set(MP_UNITS_BUILD_CXX_MODULES OFF CACHE BOOL "" FORCE)
set(MP_UNITS_BUILD_INSTALL OFF CACHE BOOL "" FORCE)
set(MP_UNITS_API_STD_FORMAT ON CACHE BOOL "" FORCE)
set(MP_UNITS_API_CONTRACTS NONE CACHE STRING "" FORCE)

FetchContent_Declare(mp_units SYSTEM EXCLUDE_FROM_ALL
  URL https://github.com/mpusz/mp-units/archive/refs/tags/v2.5.0.tar.gz
  SOURCE_SUBDIR src
  )

FetchContent_MakeAvailable(
  BitcoinCore
  beman.any_view
  mp_units
  )

add_library(stdbtc)

target_compile_features(stdbtc PUBLIC cxx_std_20)
target_link_libraries(stdbtc
  PUBLIC
    beman::any_view
    mp-units::mp-units
  PRIVATE
    bitcoinkernel
  )

FetchContent_GetProperties(BitcoinCore SOURCE_DIR BitcoinCore_SOURCE_DIR)
target_include_directories(stdbtc PRIVATE ${BitcoinCore_SOURCE_DIR}/src)

target_sources(stdbtc
  PUBLIC FILE_SET HEADERS BASE_DIRS include FILES
    include/bitcoin/detail/arrow_proxy.hpp
    include/bitcoin/detail/basic_hash_id.hpp
    include/bitcoin/detail/byte_sink_ref.hpp
    include/bitcoin/detail/coin_index_ref.hpp
    include/bitcoin/detail/iterator.hpp
    include/bitcoin/serialization/byte_sink.hpp
    include/bitcoin/amount.hpp
    include/bitcoin/any_chain_view.hpp
    include/bitcoin/block.hpp
    include/bitcoin/block_header.hpp
    include/bitcoin/chain_view.hpp
    include/bitcoin/coin.hpp
    include/bitcoin/coin_index.hpp
    include/bitcoin/consensus_parameters.hpp
    include/bitcoin/hash_id.hpp
    include/bitcoin/predicates.hpp
    include/bitcoin/script.hpp
    include/bitcoin/transaction.hpp
    include/bitcoin/validation.hpp
    include/bitcoin/validation_flags.hpp
    include/bitcoin/validation_result.hpp
    include/bitcoin/validation_status.hpp
    include/bitcoin.hpp
  PRIVATE
    src/amount.cpp
    src/any_chain_view.cpp
    src/basic_hash_id.cpp
    src/block.cpp
    src/block_header.cpp
    src/opcode.cpp
    src/predicates.cpp
    src/script.cpp
    src/transaction.cpp
    src/validation.cpp
    src/validation_flags.cpp
    src/validation_status.cpp
  )

add_subdirectory(tests)
