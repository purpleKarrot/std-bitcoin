# SPDX-License-Identifier: BSL-1.0

cmake_minimum_required(VERSION 4.3)
project(StdBitcoin)

include(FetchContent)

enable_testing()
add_subdirectory(tests)

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

target_compile_features(stdbtc PUBLIC cxx_std_23)
target_link_libraries(stdbtc
  PUBLIC
    beman::any_view
    mp-units::mp-units
  PRIVATE
    bitcoinkernel
  )

FetchContent_GetProperties(BitcoinCore SOURCE_DIR BitcoinCore_SOURCE_DIR)
target_include_directories(stdbtc PRIVATE
  ${BitcoinCore_SOURCE_DIR}/src
  /opt/homebrew/Cellar/boost/1.90.0_1/include/
  )

target_sources(stdbtc
  PUBLIC FILE_SET CXX_MODULES BASE_DIRS module FILES
    module/bitcoin.cpp
    module/bitcoin.amount.cpp
    module/bitcoin.coin.cpp
    module/bitcoin.consensus_parameters.cpp
    module/bitcoin.hash_id.cpp
    module/bitcoin.predicates.cpp
    module/bitcoin.script.cpp
    module/bitcoin.serdes.cpp
    module/bitcoin.validation.cpp
    module/bitcoin.vocabulary.cpp
    module/copy_on_write.cpp
  PRIVATE FILE_SET internal TYPE CXX_MODULES BASE_DIRS src FILES
    src/legacy.cpp
  PRIVATE
    src/hash_id.cpp
    src/opcode.cpp
    src/script.cpp
    src/serdes_decode.cpp
    src/serdes_encode.cpp
    src/validate_block.cpp
    src/validate_header.cpp
    src/validate_script.cpp
    src/validate_transaction.cpp
    src/validation_flags.cpp
    src/validation_status.cpp
    src/vocabulary_format.cpp
  )

set_target_properties(stdbtc PROPERTIES
  CXX_CLANG_TIDY "clang-tidy"
  )

set_source_files_properties(
    src/validate_block.cpp
    src/validate_header.cpp
    src/validate_script.cpp
    src/validate_transaction.cpp
  PROPERTIES SKIP_LINTING ON
  )

install(TARGETS stdbtc EXPORT stdbtc-targets
  FILE_SET CXX_MODULES DESTINATION share/stdbtc/modules
  )
