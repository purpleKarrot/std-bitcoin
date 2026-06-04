---
title: Bitcoin Script Extensions
date: today
document: SCRIPT
audience:
  - Library Evolution Working Group
  - SG14 (Low-Latency / Financial)
author:
  - name: Daniel Pfeifer
    email: <daniel@pfeifer-mail.de>
toc: false
references:
  - id: BIP16
    citation-label: BIP-16
    title: "BIP-16: Pay to Script Hash"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0016.mediawiki
  - id: BIP141
    citation-label: BIP-141
    title: "BIP-141: Segregated Witness (Consensus layer)"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki
  - id: BIP341
    citation-label: BIP-341
    title: "BIP-341: Taproot: SegWit version 1 spending rules"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0341.mediawiki
  - id: VOCABULARY
    citation-label: VOCABULARY
    title: "Adding Bitcoin Vocabulary Types to the C++ Standard Library"
    URL: https://purplekarrot.github.io/std-bitcoin/VOCABULARY.html
---

[For illustrative purposes only. This document is written in the style of a WG21
standardization paper. It has not been submitted to the ISO C++ committee and is
not under active consideration for standardization.]{.draftnote}

# Abstract

This paper proposes script-focused extensions to the existing Bitcoin C++
vocabulary types `bitcoin::script` and `bitcoin::script_ref`:

- `bitcoin::opcode` — a strongly typed opcode enumeration,
- `bitcoin::instruction` — one decoded script instruction,
- `bitcoin::instruction_view` — a lazy instruction range over script bytes,
- `bitcoin::witness_program_ref` — a non-owning reference to a witness program,
- script construction helpers added to the existing `script` class (`append`,
  `append_data`, `append_number`),
- and associated script utility functions and predicates.

The proposal remains intentionally non-executing: it does not specify script
interpreter semantics. It standardizes representation, construction,
inspection, and common recognition helpers for serialized script programs.

# Impact on the Standard

This is a pure library addition. It introduces no core language changes.

This paper is layered on top of [@VOCABULARY]. It adds new script facilities and
amends the existing wording for `bitcoin::script`.

The proposal adds declarations to `<bitcoin>` and requires support from standard
headers used by the synopsis below, notably `<format>`, `<optional>`,
`<ranges>`, and `<span>`.

# Design considerations

## Extensions to existing `script` and `script_ref`

`script` and `script_ref` are already defined by [@VOCABULARY]. This paper
extends `script` with construction helpers and provides new free functions that
operate on `script_ref`.

## `opcode` is strongly typed

A dedicated enum class prevents accidental mixing with unrelated integers and
makes formatting, matching, and overload resolution explicit.

## Decoding and validation are separate operations

`instruction_view` provides lazy decoding. Structural checks are explicit via
`is_well_formed` and related predicates. This allows clients to choose between
best-effort traversal and strict validation.

## Distinct notions of script validity

The API intentionally separates:

- *Well-formedness* (push lengths parse),
- *Opcode-domain validity* (opcode is in the recognized set and push element
  size is bounded),
- *Template recognition* (P2SH/P2WSH/P2TR/witness-program patterns), and
- *Policy-oriented unspendability checks*.

These are not equivalent and are specified independently.

## Limits belong to semantic predicates, not to class invariants

This proposal treats `script` and `script_ref` as vocabulary carriers for script
bytes, including oversized scripts. The constructors and mutators do not impose
a 10'000-byte class invariant.

Protocol and policy limits are still modeled where they are semantically
relevant:

- `has_valid_opcodes` checks pushed-element size against 520 bytes.
- `is_unspendable` checks for leading `OP_RETURN` or script byte length greater
  than 10'000.

Those limits are intentionally specified in predicate semantics rather than as
public constants in the type interface.

# Proposed wording

The wording in this section is relative to the C++ Working Draft and assumes
that the wording of [@VOCABULARY] has been applied.

[The existing subclauses [bitcoin.script.ref] and [bitcoin.script.owning]
from [@VOCABULARY] are amended as indicated. All other subclauses in this paper
are new.]{.ednote}

## [bitcoin.script.version] Feature test macro

```cpp
#define __cpp_lib_bitcoin_script 202XXXL    // also in <bitcoin>
```

## [bitcoin.script] Script vocabulary and utilities

### [bitcoin.script.syn] Header synopsis

[Add the following declarations to the `<bitcoin>` synopsis in [bitcoin.syn].]{.ednote}

```diff
namespace bitcoin {
+ enum class opcode : std::uint8_t;
+
+ class instruction;
+ class witness_program_ref;
+ class instruction_view;
+
+ [[nodiscard]] instruction_view instructions(script_ref value) noexcept;
+
+ [[nodiscard]] bool is_small_integer(opcode code) noexcept;
+ [[nodiscard]] int decode_small_integer(opcode code) noexcept;
+ [[nodiscard]] opcode encode_small_integer(int value) noexcept;
+
+ [[nodiscard]] bool is_well_formed(script_ref value) noexcept;
+ [[nodiscard]] bool has_valid_opcodes(script_ref value) noexcept;
+ [[nodiscard]] bool is_push_only(script_ref value) noexcept;
+ [[nodiscard]] bool is_unspendable(script_ref value) noexcept;
+ [[nodiscard]] std::optional<witness_program_ref>
+   witness_program(script_ref value) noexcept;
+ [[nodiscard]] bool is_pay_to_script_hash(script_ref value) noexcept;
+ [[nodiscard]] bool is_pay_to_witness_script_hash(script_ref value) noexcept;
+ [[nodiscard]] bool is_pay_to_taproot(script_ref value) noexcept;
+}
+
+template<>
+struct std::formatter<bitcoin::opcode> : std::formatter<std::string_view>;
```

```cpp
namespace bitcoin {

  enum class opcode : std::uint8_t {
    op_0 = 0x00,
    op_false = op_0,
    op_pushdata1 = 0x4c,
    op_pushdata2 = 0x4d,
    op_pushdata4 = 0x4e,
    op_1negate = 0x4f,
    op_reserved = 0x50,
    op_1 = 0x51,
    op_true = op_1,
    op_2 = 0x52,
    op_3 = 0x53,
    op_4 = 0x54,
    op_5 = 0x55,
    op_6 = 0x56,
    op_7 = 0x57,
    op_8 = 0x58,
    op_9 = 0x59,
    op_10 = 0x5a,
    op_11 = 0x5b,
    op_12 = 0x5c,
    op_13 = 0x5d,
    op_14 = 0x5e,
    op_15 = 0x5f,
    op_16 = 0x60,

    op_nop = 0x61,
    op_ver = 0x62,
    op_if = 0x63,
    op_notif = 0x64,
    op_verif = 0x65,
    op_vernotif = 0x66,
    op_else = 0x67,
    op_endif = 0x68,
    op_verify = 0x69,
    op_return = 0x6a,

    op_toaltstack = 0x6b,
    op_fromaltstack = 0x6c,
    op_2drop = 0x6d,
    op_2dup = 0x6e,
    op_3dup = 0x6f,
    op_2over = 0x70,
    op_2rot = 0x71,
    op_2swap = 0x72,
    op_ifdup = 0x73,
    op_depth = 0x74,
    op_drop = 0x75,
    op_dup = 0x76,
    op_nip = 0x77,
    op_over = 0x78,
    op_pick = 0x79,
    op_roll = 0x7a,
    op_rot = 0x7b,
    op_swap = 0x7c,
    op_tuck = 0x7d,

    op_cat = 0x7e,
    op_substr = 0x7f,
    op_left = 0x80,
    op_right = 0x81,
    op_size = 0x82,

    op_invert = 0x83,
    op_and = 0x84,
    op_or = 0x85,
    op_xor = 0x86,
    op_equal = 0x87,
    op_equalverify = 0x88,
    op_reserved1 = 0x89,
    op_reserved2 = 0x8a,

    op_1add = 0x8b,
    op_1sub = 0x8c,
    op_2mul = 0x8d,
    op_2div = 0x8e,
    op_negate = 0x8f,
    op_abs = 0x90,
    op_not = 0x91,
    op_0notequal = 0x92,
    op_add = 0x93,
    op_sub = 0x94,
    op_mul = 0x95,
    op_div = 0x96,
    op_mod = 0x97,
    op_lshift = 0x98,
    op_rshift = 0x99,
    op_booland = 0x9a,
    op_boolor = 0x9b,
    op_numequal = 0x9c,
    op_numequalverify = 0x9d,
    op_numnotequal = 0x9e,
    op_lessthan = 0x9f,
    op_greaterthan = 0xa0,
    op_lessthanorequal = 0xa1,
    op_greaterthanorequal = 0xa2,
    op_min = 0xa3,
    op_max = 0xa4,
    op_within = 0xa5,

    op_ripemd160 = 0xa6,
    op_sha1 = 0xa7,
    op_sha256 = 0xa8,
    op_hash160 = 0xa9,
    op_hash256 = 0xaa,
    op_codeseparator = 0xab,
    op_checksig = 0xac,
    op_checksigverify = 0xad,
    op_checkmultisig = 0xae,
    op_checkmultisigverify = 0xaf,

    op_nop1 = 0xb0,
    op_checklocktimeverify = 0xb1,
    op_nop2 = op_checklocktimeverify,
    op_checksequenceverify = 0xb2,
    op_nop3 = op_checksequenceverify,
    op_nop4 = 0xb3,
    op_nop5 = 0xb4,
    op_nop6 = 0xb5,
    op_nop7 = 0xb6,
    op_nop8 = 0xb7,
    op_nop9 = 0xb8,
    op_nop10 = 0xb9,

    op_checksigadd = 0xba,

    op_invalidopcode = 0xff,
  };

  class instruction {
  public:
    constexpr instruction() noexcept = default;
    constexpr instruction(opcode code,
                          std::span<const std::byte> immediate = {}) noexcept;

    [[nodiscard]] constexpr opcode code() const noexcept;
    [[nodiscard]] constexpr std::span<const std::byte> immediate() const noexcept;
    [[nodiscard]] constexpr bool pushes_data() const noexcept;
  };

  class witness_program_ref {
  public:
    constexpr witness_program_ref(std::uint8_t version,
                                  std::span<const std::byte> program) noexcept;

    [[nodiscard]] constexpr std::uint8_t version() const noexcept;
    [[nodiscard]] constexpr std::span<const std::byte> program() const noexcept;
  };

  class script;
  class script_ref;

  class instruction_view : public std::ranges::view_interface<instruction_view> {
  public:
    class iterator {
    public:
      using iterator_concept = std::input_iterator_tag;
      using difference_type = std::ptrdiff_t;
      using value_type = bitcoin::instruction;

      iterator() noexcept = default;

      [[nodiscard]] value_type operator*() const noexcept;
      iterator& operator++() noexcept;
      iterator operator++(int) noexcept;

      friend bool operator==(const iterator&, std::default_sentinel_t) noexcept;
    };

    instruction_view() noexcept = default;

    [[nodiscard]] iterator begin() const noexcept;
    [[nodiscard]] std::default_sentinel_t end() const noexcept;
  };

  class script_ref {
  public:
    script_ref() noexcept;
    explicit script_ref(std::span<const std::byte> b) noexcept;
    script_ref(const script& s) noexcept;
    script_ref(script&&) = delete;
    script_ref(const script&&) = delete;

    [[nodiscard]] bool empty() const noexcept;

    friend std::span<const std::byte> as_bytes(script_ref s) noexcept;
    friend bool operator==(script_ref lhs, script_ref rhs) noexcept;
  };

  class script {
  public:
    script() noexcept;
    explicit script(std::span<const std::byte> b);
    explicit script(script_ref s);

    [[nodiscard]] bool empty() const noexcept;
    void clear() noexcept;

    script& append(opcode code);
    script& append_data(std::span<const std::byte> data);
    script& append_number(std::int64_t value);
    script& append(script_ref suffix);

    friend std::span<const std::byte> as_bytes(const script& s) noexcept;
    friend bool operator==(const script&, const script&) = default;
  };

  [[nodiscard]] instruction_view instructions(script_ref value) noexcept;

  [[nodiscard]] bool is_small_integer(opcode code) noexcept;
  [[nodiscard]] int decode_small_integer(opcode code) noexcept;
  [[nodiscard]] opcode encode_small_integer(int value) noexcept;

  [[nodiscard]] bool is_well_formed(script_ref value) noexcept;
  [[nodiscard]] bool has_valid_opcodes(script_ref value) noexcept;
  [[nodiscard]] bool is_push_only(script_ref value) noexcept;
  [[nodiscard]] bool is_unspendable(script_ref value) noexcept;
  [[nodiscard]] std::optional<witness_program_ref>
    witness_program(script_ref value) noexcept;
  [[nodiscard]] bool is_pay_to_script_hash(script_ref value) noexcept;
  [[nodiscard]] bool is_pay_to_witness_script_hash(script_ref value) noexcept;
  [[nodiscard]] bool is_pay_to_taproot(script_ref value) noexcept;

} // namespace bitcoin

template<>
struct std::formatter<bitcoin::opcode> : std::formatter<std::string_view> {
  auto format(bitcoin::opcode code, std::format_context& ctx) const
    -> std::format_context::iterator;
};
```

### [bitcoin.script.opcode] Enum class `opcode`

`opcode` is a distinct type whose underlying type is `std::uint8_t`.

- The enumerators and values are those shown in [bitcoin.script.syn].
- Alias enumerators denote the same value (`op_false == op_0`,
  `op_true == op_1`, `op_nop2 == op_checklocktimeverify`,
  `op_nop3 == op_checksequenceverify`).

### [bitcoin.script.instruction] Class `instruction`

`instruction` represents one decoded script instruction.

```cpp
constexpr instruction(opcode code, std::span<const std::byte> immediate = {}) noexcept;
```

*Preconditions:* Let `raw` be `static_cast<std::uint8_t>(code)`.

- If `raw < static_cast<std::uint8_t>(opcode::op_pushdata1)`,
  `immediate.size() == raw`.
- Else if `code == opcode::op_pushdata1`,
  `immediate.size() <= std::numeric_limits<std::uint8_t>::max()`.
- Else if `code == opcode::op_pushdata2`,
  `immediate.size() <= std::numeric_limits<std::uint16_t>::max()`.
- Else if `code == opcode::op_pushdata4`,
  `immediate.size() <= std::numeric_limits<std::uint32_t>::max()`.
- Otherwise, `immediate.empty()` is `true`.

```cpp
[[nodiscard]] constexpr opcode code() const noexcept;
```

*Returns:* The opcode value of the instruction.

```cpp
[[nodiscard]] constexpr std::span<const std::byte> immediate() const noexcept;
```

*Returns:* The immediate byte sequence associated with `code()`.

```cpp
[[nodiscard]] constexpr bool pushes_data() const noexcept;
```

*Returns:* `static_cast<std::uint8_t>(code()) <=
static_cast<std::uint8_t>(opcode::op_pushdata4)`.

### [bitcoin.script.witness.program.ref] Class `witness_program_ref`

`witness_program_ref` is a non-owning reference to a decoded witness program.

```cpp
constexpr witness_program_ref(std::uint8_t version,
                              std::span<const std::byte> program) noexcept;
```

*Preconditions:* `version <= 16`, `program.size() >= 2`, and
`program.size() <= 40`.

```cpp
[[nodiscard]] constexpr std::uint8_t version() const noexcept;
[[nodiscard]] constexpr std::span<const std::byte> program() const noexcept;
```

*Returns:* The witness version and witness program bytes, respectively.

### [bitcoin.script.instruction.view] Class `instruction_view`

`instruction_view` is a non-owning view over serialized script bytes that yields
`instruction` values.

- `instruction_view` models `std::ranges::view` and
  `std::ranges::input_range`.
- Iteration decodes one instruction at a time from left to right.
- If decoding fails at some byte position, iteration terminates at that
  position.

```cpp
[[nodiscard]] instruction_view::iterator begin() const noexcept;
[[nodiscard]] std::default_sentinel_t end() const noexcept;
```

*Returns:* Input-range iterator/sentinel over decoded instructions.

### [bitcoin.script.ref] Amendments to class `script_ref`

[The existing wording for `script_ref` in [bitcoin.script.ref] applies
without modification. No new members are added.]{.ednote}

### [bitcoin.script.owning] Amendments to class `script`

[The existing wording for `script()`, `script(std::span<const std::byte>)`,
`script(script_ref)`, `empty()`, and `as_bytes()` in [bitcoin.script.cons] and
[bitcoin.script.obs] applies without modification.]{.ednote}

The class synopsis in [bitcoin.script.syn] is changed as follows:

```diff
  class script {
  public:
    script() noexcept;
    explicit script(std::span<const std::byte> b);
    explicit script(script_ref s);

    [[nodiscard]] bool empty() const noexcept;
+   void clear() noexcept;
+
+   script& append(opcode code);
+   script& append_data(std::span<const std::byte> data);
+   script& append_number(std::int64_t value);
+   script& append(script_ref suffix);

    friend std::span<const std::byte> as_bytes(const script& s) noexcept;
    friend bool operator==(const script& lhs, const script& rhs) noexcept;
  };
```

```cpp
void clear() noexcept;
```

*Effects:* Erases all stored bytes.

*Postconditions:* `empty()` is `true`.

```cpp
script& append(opcode code);
```

*Effects:* Appends `static_cast<std::byte>(static_cast<std::uint8_t>(code))`.

*Returns:* `*this`.

```cpp
script& append_data(std::span<const std::byte> data);
```

*Effects:* Appends canonical push-length prefix for `data.size()` using:

- single-byte length if `size < 0x4c`,
- `OP_PUSHDATA1` + one-byte little-endian length if `size <= 0xff`,
- `OP_PUSHDATA2` + two-byte little-endian length if `size <= 0xffff`,
- otherwise `OP_PUSHDATA4` + four-byte little-endian length.

Then appends `data`.

*Returns:* `*this`.

```cpp
script& append_number(std::int64_t value);
```

*Effects:*

- If `value == -1`, appends `opcode::op_1negate`.
- Else if `0 <= value && value <= 16`, appends `encode_small_integer(value)`.
- Otherwise appends minimally encoded ScriptNum bytes via `append_data`.

*Returns:* `*this`.

```cpp
script& append(script_ref suffix);
```

*Effects:* Appends `as_bytes(suffix)`.

*Returns:* `*this`.

### [bitcoin.script.ops] Free functions

```cpp
[[nodiscard]] instruction_view instructions(script_ref value) noexcept;
```

*Returns:* `instruction_view` over `as_bytes(value)`.

```cpp
[[nodiscard]] bool is_small_integer(opcode code) noexcept;
```

*Returns:* `true` if `code == opcode::op_0` or
`opcode::op_1 <= code <= opcode::op_16`; otherwise `false`.

```cpp
[[nodiscard]] int decode_small_integer(opcode code) noexcept;
```

*Preconditions:* `is_small_integer(code)` is `true`.

*Returns:* `0` for `opcode::op_0`; otherwise the integer in `[1,16]`
represented by `code`.

```cpp
[[nodiscard]] opcode encode_small_integer(int value) noexcept;
```

*Preconditions:* `0 <= value && value <= 16`.

*Returns:* `opcode::op_0` for `value == 0`; otherwise the corresponding opcode
from `opcode::op_1` through `opcode::op_16`.

```cpp
[[nodiscard]] bool is_well_formed(script_ref value) noexcept;
```

*Returns:* `true` if `as_bytes(value)` can be partitioned into a complete
sequence of encoded instructions using Bitcoin push-length rules; otherwise
`false`.

```cpp
[[nodiscard]] bool has_valid_opcodes(script_ref value) noexcept;
```

*Returns:* `true` if `value` is well-formed, every decoded opcode has numeric
value `<= static_cast<std::uint8_t>(opcode::op_checksigadd)`, and every
instruction immediate size is `<= 520`; otherwise `false`.

```cpp
[[nodiscard]] bool is_push_only(script_ref value) noexcept;
```

*Returns:* `true` if `value` is well-formed and every decoded opcode has numeric
value `<= static_cast<std::uint8_t>(opcode::op_16)`; otherwise `false`.

```cpp
[[nodiscard]] bool is_unspendable(script_ref value) noexcept;
```

*Returns:* `true` if `as_bytes(value)` is non-empty and its first byte is
`opcode::op_return`, or if `as_bytes(value).size() > 10'000`; otherwise
`false`.

```cpp
[[nodiscard]] std::optional<witness_program_ref>
  witness_program(script_ref value) noexcept;
```

*Returns:* If `as_bytes(value)` has length `L` and all of the following hold:

- `4 <= L && L <= 42`,
- byte `0` encodes a small-integer opcode (witness version),
- byte `1` is `N` with `2 <= N && N <= 40`,
- `L == N + 2`,

returns `witness_program_ref{version, program}` where `version` is the decoded
small integer from byte `0`, and `program` is bytes `[2, 2 + N)`.
Otherwise returns `std::nullopt`.

```cpp
[[nodiscard]] bool is_pay_to_script_hash(script_ref value) noexcept;
```

*Returns:* `true` if `as_bytes(value)` has size 23 and matches
`OP_HASH160 0x14 <20 bytes> OP_EQUAL`; otherwise `false` [@BIP16].

```cpp
[[nodiscard]] bool is_pay_to_witness_script_hash(script_ref value) noexcept;
```

*Returns:* `true` if `as_bytes(value)` has size 34 and matches
`OP_0 0x20 <32 bytes>`; otherwise `false` [@BIP141].

```cpp
[[nodiscard]] bool is_pay_to_taproot(script_ref value) noexcept;
```

*Returns:* `true` if `as_bytes(value)` has size 34 and matches
`OP_1 0x20 <32 bytes>`; otherwise `false` [@BIP341].

### [bitcoin.script.fmt] Formatter for `opcode`

```cpp
template<>
struct std::formatter<bitcoin::opcode> : std::formatter<std::string_view> {
  auto format(bitcoin::opcode code, std::format_context& ctx) const
    -> std::format_context::iterator;
};
```

*Effects:* Formats `code` as its canonical uppercase opcode mnemonic (e.g.
`"OP_0"`, `"OP_RETURN"`, `"OP_CHECKSIGADD"`). Unknown values format as
`"OP_INVALIDOPCODE"`.

# Implementation experience

The API in this paper is implemented in this repository as
`include/bitcoin/script.hpp` and `src/script.cpp`, with tests in
`tests/script.cpp`.
