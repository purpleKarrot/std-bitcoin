// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/script.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>

#include <doctest/doctest.h>

namespace {

inline constexpr auto script_size_limit = std::size_t{10'000};
inline constexpr auto script_element_size_limit = std::size_t{520};

constexpr auto byte(unsigned value) noexcept -> std::byte
{
  return std::byte{static_cast<std::uint8_t>(value)};
}

constexpr auto byte(bitcoin::opcode code) noexcept -> std::byte
{
  return std::byte{static_cast<std::uint8_t>(code)};
}

} // namespace

static_assert(std::ranges::input_range<bitcoin::instruction_view>);
static_assert(std::ranges::view<bitcoin::instruction_view>);
static_assert(
  std::is_constructible_v<bitcoin::script_ref, bitcoin::script const&>);
static_assert(!std::is_constructible_v<bitcoin::script_ref, bitcoin::script&&>);
static_assert(
  !std::is_constructible_v<bitcoin::script_ref, bitcoin::script const&&>);

TEST_CASE("script defaults to an empty byte sequence")
{
  auto const script = bitcoin::script{};

  CHECK(script.empty());
  CHECK(as_bytes(script).empty());
  CHECK(bitcoin::is_well_formed(script));
  CHECK(bitcoin::has_valid_opcodes(script));
}

TEST_CASE("script copies bytes and can represent oversized scripts")
{
  auto const bytes = std::array{
    byte(bitcoin::opcode::op_dup),
    byte(bitcoin::opcode::op_checksig),
  };

  auto const script = bitcoin::script{std::span{bytes}};

  CHECK_FALSE(script.empty());
  CHECK(as_bytes(script).size() == bytes.size());
  CHECK(std::ranges::equal(as_bytes(script), std::span{bytes}));

  auto const ref = bitcoin::script_ref{script};
  CHECK_FALSE(ref.empty());
  CHECK(as_bytes(ref).size() == bytes.size());
  CHECK(std::ranges::equal(as_bytes(ref), std::span{bytes}));
  CHECK(ref == script);

  auto const copied = bitcoin::script{ref};
  CHECK(copied == script);

  auto const oversized =
    std::vector<std::byte>(script_size_limit + 1, byte(0x00));
  auto const oversized_script = bitcoin::script{std::span{oversized}};
  CHECK(as_bytes(oversized_script).size() == oversized.size());
  CHECK(bitcoin::is_well_formed(oversized_script));
  CHECK(bitcoin::has_valid_opcodes(oversized_script));
  CHECK(bitcoin::is_unspendable(oversized_script));
}

TEST_CASE("small integer opcodes encode and decode")
{
  CHECK(bitcoin::is_small_integer(bitcoin::opcode::op_0));
  CHECK(bitcoin::is_small_integer(bitcoin::opcode::op_9));
  CHECK(bitcoin::is_small_integer(bitcoin::opcode::op_16));
  CHECK_FALSE(bitcoin::is_small_integer(bitcoin::opcode::op_1negate));
  CHECK_FALSE(bitcoin::is_small_integer(bitcoin::opcode::op_checksig));

  CHECK(bitcoin::encode_small_integer(0) == bitcoin::opcode::op_0);
  CHECK(bitcoin::encode_small_integer(16) == bitcoin::opcode::op_16);

  CHECK(bitcoin::decode_small_integer(bitcoin::opcode::op_0) == 0);
  CHECK(bitcoin::decode_small_integer(bitcoin::opcode::op_9) == 9);
  CHECK(bitcoin::decode_small_integer(bitcoin::encode_small_integer(16)) == 16);
}

TEST_CASE("instruction views decode opcodes and pushed bytes")
{
  auto const bytes = std::array{
    byte(bitcoin::opcode::op_dup),
    byte(0x02),
    byte(0x12),
    byte(0x34),
    byte(bitcoin::opcode::op_equalverify),
    byte(bitcoin::opcode::op_checksig),
  };
  auto const script = bitcoin::script{std::span{bytes}};
  auto const ref = bitcoin::script_ref{script};

  auto instructions_vector = std::vector<bitcoin::instruction>{};
  for (auto const instruction : bitcoin::instructions(ref)) {
    instructions_vector.push_back(instruction);
  }

  REQUIRE(instructions_vector.size() == 4);

  CHECK(instructions_vector[0].code() == bitcoin::opcode::op_dup);
  CHECK_FALSE(instructions_vector[0].pushes_data());
  CHECK(instructions_vector[0].immediate().empty());

  CHECK(instructions_vector[1].code() == static_cast<bitcoin::opcode>(0x02));
  CHECK(instructions_vector[1].pushes_data());
  CHECK(std::ranges::equal(instructions_vector[1].immediate(),
                           std::array{byte(0x12), byte(0x34)}));

  CHECK(instructions_vector[2].code() == bitcoin::opcode::op_equalverify);
  CHECK(instructions_vector[3].code() == bitcoin::opcode::op_checksig);
}

TEST_CASE("well-formedness and opcode validity are distinct")
{
  auto const truncated_push_bytes = std::array{byte(0x02), byte(0xaa)};
  auto const truncated_push = bitcoin::script{std::span{truncated_push_bytes}};
  CHECK_FALSE(bitcoin::is_well_formed(truncated_push));
  CHECK_FALSE(bitcoin::has_valid_opcodes(truncated_push));
  CHECK_FALSE(bitcoin::is_push_only(truncated_push));

  auto const undefined_opcode_bytes = std::array{byte(0xbb)};
  auto const undefined_opcode =
    bitcoin::script{std::span{undefined_opcode_bytes}};
  CHECK(bitcoin::is_well_formed(undefined_opcode));
  CHECK_FALSE(bitcoin::has_valid_opcodes(undefined_opcode));

  auto large_push_payload =
    std::vector<std::byte>(script_element_size_limit + 1, byte(0x42));
  auto large_push = bitcoin::script{};
  large_push.append_data(large_push_payload);

  CHECK(bitcoin::is_well_formed(large_push));
  CHECK_FALSE(bitcoin::has_valid_opcodes(large_push));
}

TEST_CASE("script predicates recognize push-only and unspendable scripts")
{
  auto push_only = bitcoin::script{};
  push_only.append_number(0).append_number(2).append_data(
    std::array{byte(0xde), byte(0xad)});

  CHECK(bitcoin::is_push_only(push_only));
  CHECK_FALSE(bitcoin::is_unspendable(push_only));

  auto not_push_only = bitcoin::script{};
  not_push_only.append_number(1).append(bitcoin::opcode::op_dup);
  CHECK_FALSE(bitcoin::is_push_only(not_push_only));

  auto op_return = bitcoin::script{};
  op_return.append(bitcoin::opcode::op_return)
    .append_data(std::array{byte(0x01), byte(0x02)});
  CHECK(bitcoin::is_unspendable(op_return));
}

TEST_CASE("witness-program and common output templates are recognized")
{
  auto p2sh_bytes = std::array<std::byte, 23>{};
  p2sh_bytes[0] = byte(bitcoin::opcode::op_hash160);
  p2sh_bytes[1] = byte(0x14);
  p2sh_bytes[22] = byte(bitcoin::opcode::op_equal);
  auto const p2sh = bitcoin::script{std::span{p2sh_bytes}};
  CHECK(bitcoin::is_pay_to_script_hash(p2sh));
  CHECK_FALSE(bitcoin::witness_program(p2sh).has_value());

  auto p2wsh_bytes = std::array<std::byte, 34>{};
  p2wsh_bytes[0] = byte(bitcoin::opcode::op_0);
  p2wsh_bytes[1] = byte(0x20);
  p2wsh_bytes[33] = byte(0x7f);
  auto const p2wsh = bitcoin::script{std::span{p2wsh_bytes}};

  auto const program = bitcoin::witness_program(p2wsh);
  REQUIRE(program.has_value());
  CHECK(program->version() == 0);
  CHECK(program->program().size() == 32);
  CHECK(program->program().back() == byte(0x7f));
  CHECK(bitcoin::is_pay_to_witness_script_hash(p2wsh));
  CHECK_FALSE(bitcoin::is_pay_to_taproot(p2wsh));

  auto p2tr_bytes = std::array<std::byte, 34>{};
  p2tr_bytes[0] = byte(bitcoin::opcode::op_1);
  p2tr_bytes[1] = byte(0x20);
  p2tr_bytes[2] = byte(0x55);
  auto const p2tr = bitcoin::script{std::span{p2tr_bytes}};

  auto const taproot_program = bitcoin::witness_program(p2tr);
  REQUIRE(taproot_program.has_value());
  CHECK(taproot_program->version() == 1);
  CHECK(bitcoin::is_pay_to_taproot(p2tr));
}

TEST_CASE("script mutators assemble scripts with explicit named operations")
{
  auto const suffix_bytes = std::array{byte(bitcoin::opcode::op_equal)};
  auto suffix = bitcoin::script{std::span{suffix_bytes}};
  auto script = bitcoin::script{};
  script.append(bitcoin::opcode::op_dup)
    .append(bitcoin::opcode::op_hash160)
    .append_data(std::array{byte(0xde), byte(0xad), byte(0xbe), byte(0xef)})
    .append(suffix);
  auto const expected = std::array{
    byte(bitcoin::opcode::op_dup),
    byte(bitcoin::opcode::op_hash160),
    byte(0x04),
    byte(0xde),
    byte(0xad),
    byte(0xbe),
    byte(0xef),
    byte(bitcoin::opcode::op_equal),
  };

  CHECK(std::ranges::equal(as_bytes(script), std::span{expected}));

  auto numbers = bitcoin::script{};
  numbers.append_number(-1).append_number(0).append_number(16).append_number(
    17);
  auto const number_bytes = std::array{
    byte(bitcoin::opcode::op_1negate),
    byte(bitcoin::opcode::op_0),
    byte(bitcoin::opcode::op_16),
    byte(0x01),
    byte(0x11),
  };
  CHECK(std::ranges::equal(as_bytes(numbers), std::span{number_bytes}));

  auto oversized = bitcoin::script{};
  auto const payload = std::vector<std::byte>(script_size_limit, byte(0x99));
  oversized.append_data(payload);
  CHECK(as_bytes(oversized).size() > script_size_limit);
  CHECK(bitcoin::is_unspendable(oversized));

  script.clear();
  CHECK(script.empty());
}

TEST_CASE("opcode stringification")
{
  CHECK(std::format("{}", bitcoin::opcode::op_false) == "OP_0");
  CHECK(std::format("{}", bitcoin::opcode::op_true) == "OP_1");
  CHECK(std::format("{}", bitcoin::opcode::op_nop) == "OP_NOP");
}
