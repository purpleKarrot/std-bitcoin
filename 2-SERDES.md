---
title: "Bitcoin Wire Formats: Parsing and Serialization"
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
---

[For illustrative purposes only. This document is written in the style of a WG21
standardization paper. It has not been submitted to the ISO C++ committee and is
not under active consideration for standardization.]{.draftnote}

# Abstract

This paper is a companion to *Adding Bitcoin Vocabulary Types to the C++
Standard Library*. That paper standardizes vocabulary types for Bitcoin data
structures but deliberately leaves wire-format codecs out of scope. This paper
proposes free functions for parsing and serializing `bitcoin::transaction`,
`bitcoin::block_header`, and `bitcoin::block`.

Parsing failure is specified as an ordinary negative result rather than as an
exceptional condition: the supplied bytes may simply fail to contain a complete,
valid encoding of the requested object. The proposed parsing functions therefore
return `std::optional<T>` rather than throwing a dedicated parse exception or
returning `std::expected<T, E>`.

# Motivation and Scope

The vocabulary-types paper standardizes the shape and observation of Bitcoin
protocol objects such as `transaction`, `block_header`, and `block`. That is
useful even in code that never touches raw bytes. In practice, however, Bitcoin
software regularly crosses the boundary between those in-memory objects and the
Bitcoin wire format.

That codec boundary raises questions distinct from the vocabulary layer:

- constructor versus free-function parsing,
- whether malformed input is an error or merely an unsuccessful match,
- exact-consumption versus prefix parsing,
- serialization target abstractions,
- and the relationship between serialization, hashing, and size computation.

Separating codecs into a companion paper keeps the vocabulary paper narrowly
focused while still allowing a standard, interoperable wire-format API.

## In scope

- Parsing a complete `bitcoin::transaction` from raw bytes.
- Parsing a complete `bitcoin::block_header` from raw bytes.
- Parsing a complete `bitcoin::block` from raw bytes.
- Serializing `bitcoin::transaction`, `bitcoin::block_header`, and
  `bitcoin::block` to a byte sink.
- Reporting serialization size for those types.
- Accepting both the legacy transaction encoding and the segregated-witness
  transaction encoding defined by [@BIP141] and [@BIP144].

## Out of scope

- Diagnostic categories explaining *why* parsing failed.
- Partial or cursor-based parsing APIs that report how many bytes were consumed.
- Hex or text encoding and decoding.
- Consensus validation.
- Script decoding or execution.
- Additional free hashing APIs beyond the existing hash observers provided by
  the vocabulary types paper.

# Impact on the Standard

This is a pure library addition. It depends on the Bitcoin vocabulary-types
paper and introduces no core language changes.

# Design considerations

## D1 — Free functions, not constructors

Parsing is an algorithm over raw bytes, not an intrinsic construction policy of
`transaction`, `block_header`, or `block`. A constructor has only two natural
outcomes: it either constructs an object or reports failure by throwing. That
makes it a poor fit for parsing when failure to match the wire format is an
ordinary outcome.

Free functions avoid that problem. They also scale better to future extensions
such as partial parsing, alternate sources, or overloads for additional object
types.

## D2 — Parsing failure is not an error in the P0709 sense

As [@P0709R4]{.title} observes, an error is "a function couldn't do what it
advertised" — its preconditions were met, but it could not achieve its
successful-return postconditions, and the calling code can recover.

The parsing functions proposed here do not advertise that they can successfully
parse an arbitrary sequence of bytes as a transaction, block header, or block.
Malformed, truncated, or trailing-junk input therefore does not represent an
error in that sense. It is simply an unsuccessful parse.

This paper models that result as `std::nullopt`.

This does not imply that the parsing functions are `noexcept`. Implementations
may still propagate exceptions arising from ordinary execution, such as memory
allocation failure. The important distinction is that failure to match the wire
format is reported by the return value, not by a dedicated parse exception type.

## D3 — Why `expected<T, parse_error>` is rejected

`std::expected<T, E>` is appropriate when the alternative state represents an
error result. That is not the model in this paper. A type such as
`expected<transaction, parse_error>` would classify routine parse failure as an
error, contrary to D2.

It would also force the specification to invent and standardize an error type
whose sole purpose would be to describe negative parse outcomes that callers are
already expected to treat as ordinary control flow. The additional machinery
would not buy stronger semantics, because the essential information conveyed by
this interface is simply whether the input contains exactly one complete
encoding of the requested object.

For the same reason, this paper also rejects throwing a dedicated `parse_error`
exception on malformed input.

## D4 — Exact-input semantics

The parsing functions in this paper accept a `std::span<const std::byte>` and
return only the parsed object, not a cursor or a count of consumed bytes. For
such an interface, exact-consumption semantics are the least surprising choice:
a parse succeeds if and only if the input contains the complete encoding of
exactly one object and no trailing bytes remain.

If a future facility needs prefix parsing, stream-oriented parsing, or
incremental decoding, those can be added as separate APIs.

## D5 — `block_header` belongs in the codec surface

`block_header` has a distinct and widely used wire format: exactly 80 bytes.
That encoding is used independently in many contexts, including hashing,
transport, storage, and header-first synchronization. Omitting standalone
header codecs would force users to route through larger enclosing objects or to
maintain a separate ad hoc interface for a central protocol type.

This paper therefore includes standalone parsing and serialization of
`block_header`.

## D6 — Sink-based serialization and non-materializing computations

Returning a freshly allocated byte buffer from every serialization operation
would impose allocation even when the caller already has an output buffer, a
stream, a hasher, or a counting sink. This paper therefore specifies
serialization in terms of a minimal byte-sink abstraction.

The sink model is intentionally simple: a sink is any object with a
`write(std::span<const std::byte>)` operation. This allows serialization to be
used not only for producing bytes, but also for other computations over the
serialized form. A hasher may be implemented as a `byte_sink`, allowing an
implementation to compute `transaction::id()`, `transaction::witness_id()`,
`block_header::hash()`, or `block::hash()` without first materializing a
contiguous temporary buffer. Likewise, `serialized_size(x)` can be implemented
with a counting sink.

This paper does not add separate free hashing functions. The vocabulary-types
paper already provides the relevant hash observers. The purpose of the
sink-based design is to permit efficient implementation of those observers and
of `serialized_size` without requiring an intermediate byte container.

## D7 — Canonical serialization

For transactions, two related serializations exist: the legacy witness-stripped
encoding and the segregated-witness extended encoding. The vocabulary types
already expose both resulting identifiers through `id()` and `witness_id()`.

This paper specifies `serialize(tx, sink)` in terms of the full wire encoding of
`tx`: witness data is included whenever present and omitted otherwise. This
matches the ordinary notion of serializing the transaction as transmitted on the
wire and aligns with `witness_id()`.

For `block_header` and `block`, the serialization functions likewise write the
ordinary Bitcoin wire format of the object.

# Proposed wording

The wording in this section is relative to the C++ Working Draft and assumes
that the wording of *Adding Bitcoin Vocabulary Types to the C++ Standard
Library* has been applied.

## [bitcoin.parse.syn] Header `<bitcoin>` synopsis

```cpp
namespace bitcoin {

  namespace serialization {
    template<class Sink>
      concept byte_sink = requires(Sink& sink, std::span<const std::byte> bytes) {
        sink.write(bytes);
      };

    class byte_sink_ref {
    public:
      template<byte_sink Sink>
        byte_sink_ref(Sink& sink) noexcept;

      void write(std::span<const std::byte> bytes);
    };
  }

  [[nodiscard]] std::optional<transaction>
    parse_transaction(std::span<const std::byte> raw);

  [[nodiscard]] std::optional<block_header>
    parse_block_header(std::span<const std::byte> raw);

  [[nodiscard]] std::optional<block>
    parse_block(std::span<const std::byte> raw);

  void serialize(const transaction& tx, serialization::byte_sink_ref sink);
  [[nodiscard]] std::size_t serialized_size(const transaction& tx);

  void serialize(const block_header& h, serialization::byte_sink_ref sink);
  [[nodiscard]] std::size_t serialized_size(const block_header& h);

  void serialize(const block& b, serialization::byte_sink_ref sink);
  [[nodiscard]] std::size_t serialized_size(const block& b);

} // namespace bitcoin
```

## [bitcoin.serialization.byte.sink] Byte sink adapter

### [bitcoin.serialization.byte.sink.overview]

`serialization::byte_sink` is satisfied by a type `Sink` if the expression
`sink.write(bytes)` is well-formed for an lvalue `sink` of type `Sink` and a
value `bytes` of type `std::span<const std::byte>`.

`serialization::byte_sink_ref` is a non-owning wrapper referring to an object
that satisfies `serialization::byte_sink`.

### [bitcoin.serialization.byte.sink.syn] Synopsis

```cpp
namespace bitcoin::serialization {

  template<class Sink>
    concept byte_sink = requires(Sink& sink, std::span<const std::byte> bytes) {
      sink.write(bytes);
    };

  class byte_sink_ref {
  public:
    template<byte_sink Sink>
      byte_sink_ref(Sink& sink) noexcept;

    void write(std::span<const std::byte> bytes);
  };

} // namespace bitcoin::serialization
```

### [bitcoin.serialization.byte.sink.cons] Constructors

```cpp
template<byte_sink Sink>
  byte_sink_ref(Sink& sink) noexcept;
```

*Effects:* Constructs a `byte_sink_ref` that refers to `sink`.

*Remarks:* `byte_sink_ref` does not own `sink`. The program is responsible for
ensuring that the referred-to sink outlives the `byte_sink_ref`.

### [bitcoin.serialization.byte.sink.ops] Operations

```cpp
void write(std::span<const std::byte> bytes);
```

*Effects:* Equivalent to invoking `write(bytes)` on the referred-to sink.

## [bitcoin.parse.transaction] Parsing transactions

### [bitcoin.parse.transaction.syn] Synopsis

```cpp
namespace bitcoin {

  [[nodiscard]] std::optional<transaction>
    parse_transaction(std::span<const std::byte> raw);

} // namespace bitcoin
```

### [bitcoin.parse.transaction.func] Function `parse_transaction`

```cpp
[[nodiscard]] std::optional<transaction>
  parse_transaction(std::span<const std::byte> raw);
```

*Returns:* An engaged `std::optional` containing a `transaction` if `raw`
contains the valid, complete wire-format encoding of exactly one transaction;
otherwise `std::nullopt`.

*Remarks:* Both the legacy transaction format and the segregated-witness
extended transaction format defined by [@BIP141] and [@BIP144] are accepted.
A return value of `std::nullopt` indicates any unsuccessful parse, including a
malformed encoding, truncated input, or trailing bytes remaining after one
complete transaction has been parsed.

## [bitcoin.serialize.transaction] Serializing transactions

### [bitcoin.serialize.transaction.syn] Synopsis

```cpp
namespace bitcoin {

  void serialize(const transaction& tx, serialization::byte_sink_ref sink);
  [[nodiscard]] std::size_t serialized_size(const transaction& tx);

} // namespace bitcoin
```

### [bitcoin.serialize.transaction.func] Function `serialize`

```cpp
void serialize(const transaction& tx, serialization::byte_sink_ref sink);
```

*Effects:* Writes the wire-format encoding of `tx` to `sink`.

*Remarks:* If `tx` contains no witness data, the encoding is the legacy
transaction format. Otherwise, the encoding is the segregated-witness extended
transaction format defined by [@BIP141] and [@BIP144]. The SHA256d of the bytes
written by this function is equal to `tx.witness_id()`. For a transaction with
no witness data, this is also equal to `tx.id()` when both are compared as
`bitcoin::hash256` values.

### [bitcoin.serialize.transaction.size] Function `serialized_size`

```cpp
[[nodiscard]] std::size_t serialized_size(const transaction& tx);
```

*Returns:* The number of bytes that would be written by
`serialize(tx, sink)`.

## [bitcoin.parse.block.header] Parsing block headers

### [bitcoin.parse.block.header.syn] Synopsis

```cpp
namespace bitcoin {

  [[nodiscard]] std::optional<block_header>
    parse_block_header(std::span<const std::byte> raw);

} // namespace bitcoin
```

### [bitcoin.parse.block.header.func] Function `parse_block_header`

```cpp
[[nodiscard]] std::optional<block_header>
  parse_block_header(std::span<const std::byte> raw);
```

*Returns:* An engaged `std::optional` containing a `block_header` if `raw`
contains the valid, complete wire-format encoding of exactly one block header;
otherwise `std::nullopt`.

*Remarks:* The wire-format encoding of a block header is exactly 80 bytes,
consisting of the serialized version, previous block hash, merkle root, time,
bits, and nonce fields, in that order. A return value of `std::nullopt`
indicates any unsuccessful parse, including truncated input or trailing bytes
remaining after one complete block header has been parsed.

## [bitcoin.serialize.block.header] Serializing block headers

### [bitcoin.serialize.block.header.syn] Synopsis

```cpp
namespace bitcoin {

  void serialize(const block_header& h, serialization::byte_sink_ref sink);
  [[nodiscard]] std::size_t serialized_size(const block_header& h);

} // namespace bitcoin
```

### [bitcoin.serialize.block.header.func] Function `serialize`

```cpp
void serialize(const block_header& h, serialization::byte_sink_ref sink);
```

*Effects:* Writes the wire-format encoding of `h` to `sink`.

*Remarks:* The bytes written by this function consist of the serialized
version, previous block hash, merkle root, time, bits, and nonce fields, in
that order. The SHA256d of those 80 bytes is equal to `h.hash()`.

### [bitcoin.serialize.block.header.size] Function `serialized_size`

```cpp
[[nodiscard]] std::size_t serialized_size(const block_header& h);
```

*Returns:* `80`.

## [bitcoin.parse.block] Parsing blocks

### [bitcoin.parse.block.syn] Synopsis

```cpp
namespace bitcoin {

  [[nodiscard]] std::optional<block>
    parse_block(std::span<const std::byte> raw);

} // namespace bitcoin
```

### [bitcoin.parse.block.func] Function `parse_block`

```cpp
[[nodiscard]] std::optional<block>
  parse_block(std::span<const std::byte> raw);
```

*Returns:* An engaged `std::optional` containing a `block` if `raw` contains
the valid, complete wire-format encoding of exactly one block; otherwise
`std::nullopt`.

*Remarks:* The wire-format encoding consists of a serialized block header,
followed by a variable-length integer transaction count, followed by the
serialized transactions in order. A return value of `std::nullopt` indicates
any unsuccessful parse, including a malformed encoding, truncated input, or
trailing bytes remaining after one complete block has been parsed.

## [bitcoin.serialize.block] Serializing blocks

### [bitcoin.serialize.block.syn] Synopsis

```cpp
namespace bitcoin {

  void serialize(const block& b, serialization::byte_sink_ref sink);
  [[nodiscard]] std::size_t serialized_size(const block& b);

} // namespace bitcoin
```

### [bitcoin.serialize.block.func] Function `serialize`

```cpp
void serialize(const block& b, serialization::byte_sink_ref sink);
```

*Effects:* Writes the wire-format encoding of `b` to `sink`.

*Remarks:* The bytes written by this function consist of the serialized block
header, followed by a variable-length integer transaction count, followed by
the serialized form of each transaction in order. The header is serialized as
if by `serialize(b.header(), sink)`. Each transaction is serialized as if by
`serialize(tx, sink)`.

### [bitcoin.serialize.block.size] Function `serialized_size`

```cpp
[[nodiscard]] std::size_t serialized_size(const block& b);
```

*Returns:* The number of bytes that would be written by `serialize(b, sink)`.
