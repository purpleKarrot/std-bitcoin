---
title: Adding Bitcoin Vocabulary Types to the C++ Standard Library
date: today
document: DXXXXR0
audience:
  - Library Evolution Working Group
  - SG14 (Low-Latency / Financial)
toc: false
references:
  - id: BIP141
    citation-label: BIP-141
    title: "BIP-141: Segregated Witness (Consensus layer)"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0141.mediawiki
  - id: BIP144
    citation-label: BIP-144
    title: "BIP-144: Segregated Witness (Peer Services)"
    URL: https://github.com/bitcoin/bips/blob/master/bip-0144.mediawiki
  - id: BitcoinCore
    citation-label: Bitcoin Core
    title: "Bitcoin Core source repository"
    URL: https://github.com/bitcoin/bitcoin
---

[For illustrative purposes only. This document is written in the style of a WG21
standardization paper. It has not been submitted to the ISO C++ committee and is
not under active consideration for standardization.]{.draftnote}

# Abstract

This paper proposes adding a set of vocabulary types for the Bitcoin protocol to
the C++ Standard Library under the header `<bitcoin>`. These types live in
`namespace bitcoin`. The proposed facility provides zero-overhead, strongly
typed vocabulary types intended to improve interoperability among independently
developed libraries.

# Motivation and Scope

Bitcoin and the broader ecosystem of protocols built on top of it (Lightning
Network, Liquid, Ark, …) are implemented in many independent C++ libraries.
Such libraries commonly define their own `uint256`, `CTransaction`, `CTxOut`,
or equivalent types. These types are often structurally similar yet
source-incompatible, so code that crosses library boundaries typically requires
adaptation code and conversions.

This is similar to other cases in which the standard library provides a common
vocabulary type for facilities that would otherwise be modeled independently by
each implementation. This paper applies the same approach to core Bitcoin
wire-protocol types.

## In scope

- 32-byte hash types: `hash256`, `txid`, `wtxid`, `block_hash`
- Monetary value: `amount`
- Script byte container: `script`
- Transaction structure: `outpoint`, `tx_input`, `tx_output`, `transaction`
- Block structure: `block_header`, `block`

## Out of scope

- Script execution / interpreter
- Cryptographic primitives (hashing, ECDSA, Schnorr)
- Networking / P2P protocol
- Wallet key derivation (BIP-32, BIP-39)
- Consensus validation logic

# Impact on the Standard

This is a pure library addition. It requires no core language changes. It
depends on the quantities and units library proposed in [@P3045R8] and on:

- `<array>`, `<concepts>`, `<cstddef>`, `<cstdint>`
- `<compare>`, `<format>`, `<ranges>`, `<span>`, `<stdexcept>`

No existing names are modified or deprecated.

# Design considerations

## D1 — Strong types over raw integers and byte arrays

`txid`, `wtxid`, `block_hash`, and `hash256` are all 32-byte digest values, but
they must not be implicitly interconvertible. A function accepting a `txid` must
not silently accept a `block_hash` or a `wtxid`. This rules out aliasing
them to the same underlying type (for example,
`using txid = std::array<std::byte, 32>`{.cpp}).
The proposal models these as distinct specializations of an exposition-only
template `$basic-hash-id$<Tag>`{.cpp}, producing separate,
non-interconvertible types with identical storage representation.

## D2 — Storage and display byte order

Bitcoin's double-SHA256 digests are stored on the wire in little-endian
("natural hash") byte order. Block explorers, wallet software, and similar
tools commonly display them with the bytes reversed. The proposed types store
wire order internally. `std::format` and `std::to_string` produce the customary
display representation.

## D3 — Distinct monetary type

Bitcoin monetary values should not be confused with unrelated integers.
`bitcoin::amount` is a `std::quantity` specialization [@P3045R8] with
`std::int64_t` representation and `bitcoin::units::satoshi` as its reference
unit. The type represents Bitcoin-denominated quantities; protocol-level
constraints such as the valid money range are not imposed by the type itself.

## D4 — Opaque script representation

Pattern-matching for `P2PKH`, `P2TR`, opcode enumeration, and script execution
belong to a higher-level facility (not proposed here). At vocabulary level
`script` is an opaque container exposing only a `std::span<const std::byte>`
view.

## D5 — Witness association with inputs

Each `tx_input` exposes its witness data through a `witness()` observer
returning an implementation-defined range of byte strings. An empty range
indicates a non-SegWit input. The segregated-witness wire-format defined in
[@BIP141] places witness data at the end of the transaction, but that is an
encoding artifact and imposes no constraint on the vocabulary-level design.
Implementations may store witness data with each input or in a separate
parallel structure.

## D6 — Opaque class types

All types are `class` with private data members and accessor-only public
interfaces. Collection accessors return an unspecified type satisfying the
`$value-range$<T>`{.cpp} named requirement (see D10) rather than
`const std::vector<T>&`, permitting implementations to choose the backing
representation.

## D7 — Unit namespace

The sub-namespace `bitcoin::units` contains the monetary unit constants
`satoshi` and `btc`. Exposing them as ordinary unit objects permits
construction, conversion, and formatting through the quantity interface.

## D8 — Accessor naming

Bitcoin Core [@BitcoinCore] uses the legacy terms `scriptSig` and
`scriptPubKey` for the input authorization script and output locking script,
respectively. The clearer alternatives are `input_script` and
`output_script`; however, in the proposed interface, the owning classes
`tx_input` and `tx_output` already supply that input/output context. The
accessors are therefore simply named `script()`.

## D9 — ABI considerations

This paper does not mandate any particular ABI versioning scheme. Whether to
use per-type `inline namespace` versioning (e.g. `inline namespace
transaction_v1`), a single top-level `inline namespace bitcoin_v1`, or no
versioning at all is an implementer's choice. The paper requires only that all
types and constants are accessible as `bitcoin::hash256`, `bitcoin::amount`,
etc. — i.e. as direct members of `namespace bitcoin`.

## D10 — Exposition-only range helper

One exposition-only helper constrains return types of collection accessors in
this paper. It is not part of the public API.

`$value-range$<T>`{.cpp} is a **named requirement** for the unspecified return
types of collection accessors. A type `R` satisfies `$value-range$<T>`{.cpp}
if it models `std::ranges::view`, `std::ranges::sized_range`, and
`std::ranges::random_access_range`, and `std::ranges::range_value_t<R>` is
`T`. The reference type is implementation-defined: implementations may return
`const T&` into stored elements or `T` by value for handle-based types. Each
collection class exposes its return type as a named member typedef (e.g.
`transaction::input_view`) whose concrete type is *implementation-defined* but
must satisfy `$value-range$<T>`{.cpp}.

# Proposed wording

The wording in this section is relative to the C++ Working Draft.

[Add `<bitcoin>` to the table of standard library headers in [headers] and
insert a new Clause [bitcoin] after [time.h.syn].]{.ednote}

## [bitcoin.version] Feature test macro

```cpp
#define __cpp_lib_bitcoin 202XXXL    // also in <bitcoin>
```

## [bitcoin.general] General

This Clause describes the header `<bitcoin>`, which provides vocabulary types
for the Bitcoin wire protocol.

## [bitcoin.syn] Header `<bitcoin>` synopsis

```cpp
namespace bitcoin {

  namespace units {
    inline constexpr /* $see [bitcoin.amount]$ */ satoshi;
    inline constexpr /* $see [bitcoin.amount]$ */ btc;
  }

  using amount = std::quantity<units::satoshi, std::int64_t>;

  template<class Tag> class $basic-hash-id$; $// exposition only$
  using hash256 = $basic-hash-id$</* $unspecified$ */>;
  using txid = $basic-hash-id$</* $unspecified$ */>;
  using wtxid = $basic-hash-id$</* $unspecified$ */>;
  using block_hash = $basic-hash-id$</* $unspecified$ */>;
  class parse_error;
  class script;
  class outpoint;
  class tx_input;
  class tx_output;
  class transaction;
  class block_header;
  class block;

} // namespace bitcoin

template<class Tag> struct std::formatter<bitcoin::$basic-hash-id$<Tag>>;

template<class Tag> struct std::hash<bitcoin::$basic-hash-id$<Tag>>;
template<> struct std::hash<bitcoin::outpoint>;
```

## [bitcoin.amount] Type `amount`

### [bitcoin.amount.overview]

`amount` is an alias for `std::quantity<units::satoshi, std::int64_t>`. It
represents a Bitcoin-denominated quantity measured in satoshis. The type does
not impose protocol-level constraints such as the valid money range.

### [bitcoin.amount.syn] Synopsis

```cpp
namespace bitcoin {

  namespace units {
    inline constexpr /* $unit$ */ satoshi;
    inline constexpr /* $unit$ */ btc;
  }

  using amount = std::quantity<units::satoshi, std::int64_t>;

} // namespace bitcoin
```

### [bitcoin.amount.units] Unit constants

`units::satoshi` denotes the reference unit of `amount`.

`units::btc` denotes `100'000'000 * units::satoshi`.

## [bitcoin.hashid] Class template `basic-hash-id`

### [bitcoin.hashid.overview]

`$basic-hash-id$`{.cpp} is an exposition-only class template. `hash256`
([bitcoin.hash256]), `txid` ([bitcoin.txid]), `wtxid` ([bitcoin.wtxid]),
and `block_hash` ([bitcoin.block_hash]) are distinct specializations with
unspecified tag types. Specializations with different tag types are
unrelated types; comparisons between them are ill-formed.

A default-constructed `$basic-hash-id$`{.cpp} holds all-zero bytes.

### [bitcoin.hashid.syn] Synopsis

```cpp
namespace bitcoin {

  template<class Tag>
  class $basic-hash-id$ { $// exposition only$
  public:
    constexpr $basic-hash-id$() noexcept;
    constexpr explicit $basic-hash-id$(
      std::span<const std::byte, 32> bytes) noexcept;

    [[nodiscard]] constexpr explicit operator bool() const noexcept;

    friend constexpr std::span<const std::byte, 32>
      as_bytes(const $basic-hash-id$&) noexcept;
    friend constexpr bool operator==(
      const $basic-hash-id$&,
      const $basic-hash-id$&) noexcept = default;
    friend constexpr std::strong_ordering operator<=>(
      const $basic-hash-id$&,
      const $basic-hash-id$&) noexcept = default;

  private:
    std::array<std::byte, 32> $value$; $// exposition only$
  };

} // namespace bitcoin

template<class Tag> struct std::formatter<bitcoin::$basic-hash-id$<Tag>>;
template<class Tag> struct std::hash<bitcoin::$basic-hash-id$<Tag>>;
```

### [bitcoin.hashid.cons] Constructors

```cpp
constexpr $basic-hash-id$() noexcept;
```

*Postconditions:* `!*this` is `true`.

```cpp
constexpr explicit $basic-hash-id$(
  std::span<const std::byte, 32> bytes) noexcept;
```

*Effects:* Initializes the stored bytes by copying from `bytes`.

### [bitcoin.hashid.obs] Observers

```cpp
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

*Returns:* `false` if all stored bytes are `std::byte{0}`, and `true` otherwise.

```cpp
friend constexpr std::span<const std::byte, 32>
  as_bytes(const $basic-hash-id$& h) noexcept;
```

*Returns:* A read-only view of the 32 wire-order bytes of `h`.

### [bitcoin.hashid.fmt] Formatter

`std::formatter<bitcoin::$basic-hash-id$<Tag>>`{.cpp} formats a value as 64
lowercase hexadecimal digits in display byte order (bytes reversed relative to
wire order), as used by block explorers.

### [bitcoin.hashid.hash] Hash support

`std::hash<bitcoin::$basic-hash-id$<Tag>>`{.cpp} is provided. The hash value
is computed over the 32 wire-order bytes of the value.

## [bitcoin.hash256] Type `hash256`

`hash256` is a general-purpose 32-byte hash value, used where no stronger
domain type applies — for example, the merkle root in a block header
([bitcoin.block_header]).

### [bitcoin.hash256.syn] Synopsis

```cpp
namespace bitcoin {

  using hash256 = $basic-hash-id$</* $unspecified$ */>;

} // namespace bitcoin
```

## [bitcoin.txid] Type `txid`

`txid` identifies a transaction by the SHA256d of its witness-stripped
serialization.

### [bitcoin.txid.syn] Synopsis

```cpp
namespace bitcoin {

  using txid = $basic-hash-id$</* $unspecified$ */>;

} // namespace bitcoin
```

## [bitcoin.wtxid] Type `wtxid`

`wtxid` is the SHA256d of the full transaction serialization including witness
data, as defined by [@BIP141]. A `wtxid` and the `txid` of the same transaction
are equal only for transactions that carry no witness data.

### [bitcoin.wtxid.syn] Synopsis

```cpp
namespace bitcoin {

  using wtxid = $basic-hash-id$</* $unspecified$ */>;

} // namespace bitcoin
```

## [bitcoin.block_hash] Type `block_hash`

`block_hash` is the SHA256d of the serialized block header fields.

### [bitcoin.block_hash.syn] Synopsis

```cpp
namespace bitcoin {

  using block_hash = $basic-hash-id$</* $unspecified$ */>;

} // namespace bitcoin
```

## [bitcoin.parse_error] Class `parse_error`

### [bitcoin.parse_error.syn] Synopsis

```cpp
namespace bitcoin {

  class parse_error : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
  };

} // namespace bitcoin
```

`parse_error` is thrown by the deserializing constructors of `transaction`
([bitcoin.transaction.cons]) and `block` ([bitcoin.block.cons]) when `raw`
does not contain a valid, complete wire-format encoding.

## [bitcoin.script] Class `script`

### [bitcoin.script.syn] Synopsis

```cpp
namespace bitcoin {

  class script {
  public:
    script() noexcept;
    explicit script(std::span<const std::byte> b);

    friend std::span<const std::byte> as_bytes(const script& s) noexcept;
    friend bool operator==(const script& lhs, const script& rhs) noexcept;
  };

} // namespace bitcoin
```

### [bitcoin.script.cons] Constructors

```cpp
script() noexcept;
```

*Postconditions:* `as_bytes(*this).empty()` is `true`.

```cpp
explicit script(std::span<const std::byte> b);
```

*Preconditions:* `b.size() <= 10'000`.

*Effects:* Initializes the stored byte sequence by copying from `b`.

*Throws:* `std::length_error` if `b.size() > 10'000`.

### [bitcoin.script.obs] Hidden friends

```cpp
[[nodiscard]] friend std::span<const std::byte>
  as_bytes(const script& s) noexcept;
```

*Returns:* A span over the stored byte sequence of `s`.

## [bitcoin.outpoint] Class `outpoint`

### [bitcoin.outpoint.syn] Synopsis

```cpp
namespace bitcoin {

  class outpoint {
  public:
    [[nodiscard]] bitcoin::txid txid() const noexcept;
    [[nodiscard]] std::size_t index() const noexcept;

    friend bool
      operator==(const outpoint& lhs, const outpoint& rhs) noexcept;
    friend std::strong_ordering
      operator<=>(const outpoint& lhs, const outpoint& rhs) noexcept;
  };

} // namespace bitcoin

template<> struct std::hash<bitcoin::outpoint>;
```

### [bitcoin.outpoint.hash] Hash support

`std::hash<bitcoin::outpoint>` is provided. The hash value is computed by
combining `std::hash<bitcoin::txid>{}(txid())` with `index()` in an
implementation-defined manner consistent with `operator==`.

### [bitcoin.outpoint.obs] Observers

```cpp
[[nodiscard]] bitcoin::txid txid() const noexcept;
[[nodiscard]] std::size_t index() const noexcept;
```

*Returns:* The `txid` of the transaction containing the referenced output, and
the output index within that transaction, respectively.

## [bitcoin.tx_input] Class `tx_input`

### [bitcoin.tx_input.overview]

`witness_view` satisfies the `$value-range$<std::span<const std::byte>>`{.cpp}
named requirement. Construction is implementation-defined.

### [bitcoin.tx_input.syn] Synopsis

```cpp
namespace bitcoin {

  class tx_input {
  public:
    using witness_view = /* $see [bitcoin.tx_input.overview]$ */;

    [[nodiscard]] const bitcoin::outpoint& previous_output() const noexcept;
    [[nodiscard]] const bitcoin::script& script() const noexcept;
    [[nodiscard]] std::uint32_t sequence() const noexcept;
    [[nodiscard]] witness_view witness() const noexcept;

    friend bool operator==(const tx_input& lhs, const tx_input& rhs) noexcept;
  };

} // namespace bitcoin
```

### [bitcoin.tx_input.obs] Observers

```cpp
[[nodiscard]] const bitcoin::outpoint& previous_output() const noexcept;
[[nodiscard]] const bitcoin::script& script() const noexcept;
[[nodiscard]] std::uint32_t sequence() const noexcept;
```

*Returns:* The previous output reference, input script, and sequence
number, respectively.

```cpp
[[nodiscard]] witness_view witness() const noexcept;
```

*Returns:* A view of the witness items for this input. Each element is a
`std::span<const std::byte>` over one witness item. An empty view indicates
a non-SegWit input.

## [bitcoin.tx_output] Class `tx_output`

### [bitcoin.tx_output.syn] Synopsis

```cpp
namespace bitcoin {

  class tx_output {
  public:
    [[nodiscard]] bitcoin::amount value() const noexcept;
    [[nodiscard]] const bitcoin::script& script() const noexcept;

    friend bool operator==(const tx_output& lhs, const tx_output& rhs)
      noexcept;
  };

} // namespace bitcoin
```

### [bitcoin.tx_output.obs] Observers

```cpp
[[nodiscard]] bitcoin::amount value() const noexcept;
[[nodiscard]] const bitcoin::script& script() const noexcept;
```

*Returns:* The stored output value and output script, respectively.

## [bitcoin.transaction] Class `transaction`

### [bitcoin.transaction.overview]

`input_view` and `output_view` each satisfy the `$value-range$<T>`{.cpp}
named requirement for their respective element types. Objects of type
`transaction` may be default-constructed or deserialized from raw bytes
([bitcoin.transaction.cons]). All other construction is implementation-defined.

### [bitcoin.transaction.syn] Synopsis

```cpp
namespace bitcoin {

  class transaction {
  public:
    using input_view = /* $see [bitcoin.transaction.overview]$ */;
    using output_view = /* $see [bitcoin.transaction.overview]$ */;

    transaction() noexcept;
    explicit transaction(std::span<const std::byte> raw);

    [[nodiscard]] bitcoin::txid id() const noexcept;
    [[nodiscard]] bitcoin::wtxid witness_id() const noexcept;
    [[nodiscard]] std::int32_t version() const noexcept;
    [[nodiscard]] std::uint32_t locktime() const noexcept;
    [[nodiscard]] input_view inputs() const noexcept;
    [[nodiscard]] output_view outputs() const noexcept;

    friend bool operator==(const transaction& lhs, const transaction& rhs)
      noexcept;
  };

} // namespace bitcoin
```

### [bitcoin.transaction.cons] Constructors

```cpp
transaction() noexcept;
```

*Postconditions:* `inputs().empty()` and `outputs().empty()`.

```cpp
explicit transaction(std::span<const std::byte> raw);
```

*Effects:* Initializes `*this` by deserializing the Bitcoin wire-format
encoding in `raw`. Both the legacy format and the segregated-witness extended
format defined by [@BIP141] and [@BIP144] are accepted.

*Throws:* `bitcoin::parse_error` if `raw` does not contain a valid, complete
wire-format encoding of exactly one transaction. In particular, `parse_error`
is thrown if `raw` is truncated or if `raw` contains trailing bytes beyond the
encoding.

### [bitcoin.transaction.obs] Observers

```cpp
[[nodiscard]] bitcoin::txid id() const noexcept;
[[nodiscard]] bitcoin::wtxid witness_id() const noexcept;
```

*Returns:* The SHA256d hash of the witness-stripped serialization and of the
full serialization including witness data, respectively.

*Remarks:* For a transaction with no witness data, `witness_id() == id()`
when both are compared as `bitcoin::hash256` values. The return types remain
distinct regardless.

```cpp
[[nodiscard]] std::int32_t version() const noexcept;
[[nodiscard]] std::uint32_t locktime() const noexcept;
```

*Returns:* The transaction version and locktime fields, respectively.

```cpp
[[nodiscard]] input_view inputs() const noexcept;
[[nodiscard]] output_view outputs() const noexcept;
```

*Returns:* Views of the inputs and outputs, respectively.

## [bitcoin.block_header] Class `block_header`

### [bitcoin.block_header.syn] Synopsis

```cpp
namespace bitcoin {

  class block_header {
  public:
    [[nodiscard]] bitcoin::block_hash hash() const noexcept;
    [[nodiscard]] std::int32_t version() const noexcept;
    [[nodiscard]] bitcoin::block_hash prev_block_hash() const noexcept;
    [[nodiscard]] bitcoin::hash256 merkle_root() const noexcept;
    [[nodiscard]] std::chrono::sys_seconds time() const noexcept;
    [[nodiscard]] std::uint32_t bits() const noexcept;
    [[nodiscard]] std::uint32_t nonce() const noexcept;

    friend bool operator==(const block_header& lhs, const block_header& rhs)
      noexcept;
  };

} // namespace bitcoin
```

### [bitcoin.block_header.obs] Observers

```cpp
[[nodiscard]] bitcoin::block_hash hash() const noexcept;
[[nodiscard]] std::int32_t version() const noexcept;
[[nodiscard]] bitcoin::block_hash prev_block_hash() const noexcept;
[[nodiscard]] bitcoin::hash256 merkle_root() const noexcept;
[[nodiscard]] std::chrono::sys_seconds time() const noexcept;
[[nodiscard]] std::uint32_t bits() const noexcept;
[[nodiscard]] std::uint32_t nonce() const noexcept;
```

*Returns:* `hash()` returns the SHA256d hash of the 80-byte serialized block
header. The remaining observers return the values of their respective header
fields.

## [bitcoin.block] Class `block`

### [bitcoin.block.overview]

`transaction_view` satisfies the `$value-range$<bitcoin::transaction>`{.cpp}
named requirement. Objects of type `block` may be default-constructed or
deserialized from raw bytes ([bitcoin.block.cons]). All other construction is
implementation-defined.

### [bitcoin.block.syn] Synopsis

```cpp
namespace bitcoin {

  class block {
  public:
    using transaction_view = /* $see [bitcoin.block.overview]$ */;

    block() noexcept;
    explicit block(std::span<const std::byte> raw);

    [[nodiscard]] bitcoin::block_hash hash() const noexcept;
    [[nodiscard]] const bitcoin::block_header& header() const noexcept;
    [[nodiscard]] transaction_view transactions() const noexcept;

    friend bool operator==(const block& lhs, const block& rhs) noexcept;
  };

} // namespace bitcoin
```

### [bitcoin.block.cons] Constructors

```cpp
block() noexcept;
```

*Postconditions:* `transactions().empty()`.

```cpp
explicit block(std::span<const std::byte> raw);
```

*Effects:* Initializes `*this` by deserializing the Bitcoin wire-format
encoding in `raw`. The format is an 80-byte block header followed by a
variable-length integer encoding the transaction count, followed by the
serialized transactions in order.

*Throws:* `bitcoin::parse_error` if `raw` does not contain a valid, complete
wire-format encoding of exactly one block. In particular, `parse_error` is
thrown if `raw` is truncated or if `raw` contains trailing bytes beyond the
encoding.

### [bitcoin.block.obs] Observers

```cpp
[[nodiscard]] bitcoin::block_hash hash() const noexcept;
```

*Returns:* `header().hash()`.

```cpp
[[nodiscard]] const bitcoin::block_header& header() const noexcept;
```

*Returns:* The block header.

```cpp
[[nodiscard]] transaction_view transactions() const noexcept;
```

*Returns:* A view of the transactions. By convention,
`transactions().front()` is the coinbase transaction when the block is
non-empty.
