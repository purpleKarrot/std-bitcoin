# SPDX-License-Identifier: BSL-1.0

include(FetchContent)
FetchContent_Declare(doctest SYSTEM EXCLUDE_FROM_ALL
  URL https://github.com/doctest/doctest/archive/refs/tags/v2.5.2.tar.gz
  )
FetchContent_MakeAvailable(doctest)

add_executable(stdbtc-tests
  amount.cpp
  block.cpp
  chain_view.cpp
  coin.cpp
  hash_id.cpp
  script.cpp
  transaction.cpp
  validation.cpp
  validation_types.cpp
  )

target_link_libraries(stdbtc-tests PRIVATE
  stdbtc
  doctest::doctest_with_main
  )

add_test(NAME stdbtc-tests COMMAND stdbtc-tests)
