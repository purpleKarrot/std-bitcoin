// SPDX-License-Identifier: BSL-1.0

import bitcoin;

#include <array>
#include <cstddef>
#include <span>

#include <doctest/doctest.h>
#include <mp-units/framework.h>

#include "hex.hpp"

using namespace hex_literal;
using flags = bitcoin::validation_flags;
using bitcoin::verify;

namespace {

[[nodiscard]] constexpr flags operator&(flags l, flags r) noexcept
{
  return flags(static_cast<int>(l) & static_cast<int>(r));
}

[[nodiscard]] constexpr flags operator~(flags x) noexcept
{
  return flags(~static_cast<int>(x));
}

} // namespace

TEST_CASE("p2pkh verification")
{
  // Spending a P2PKH output using a mainnet tx with id
  // aca326a724eda9a461c10a876534ecd5ae7b27f10f26c3862fb996f80ea2d45d
  auto const spk =
    bitcoin::script{"76a9144bfbaf6afb76cc5771bc6404810d1cc041a6933988ac"_hex};
  auto const tx_valid = *bitcoin::parse_transaction(
    "02000000013f7cebd65c27431a90bba7f796914fe8cc2ddfc3f2cbd6f7e5f2fc854534da95"
    "000000006b483045022100de1ac3bcdfb0332207c4a91f3832bd2c2915840165f876ab47c5"
    "f8996b971c3602201c6c053d750fadde599e6f5c4e1963df0f01fc0d97815e8157e3d59fe0"
    "9ca30d012103699b464d1d8bc9e47d4fb1cdaa89a1c5783d68363c4dbc4b524ed3d8571486"
    "17feffffff02836d3c01000000001976a914fc25d6d5c94003bf5b0c7b640a248e2c637fcf"
    "b088ac7ada8202000000001976a914fbed3d9b11183209a57999d54d59f67c019e756c88ac"
    "6acb0700"_hex);
  auto const amount = 0 * bitcoin::units::satoshi;

  CHECK(verify(spk, amount, tx_valid, 0, flags::none));
  CHECK(verify(spk, amount, tx_valid, 0, flags::all & ~flags::taproot));

  // same tx but with corrupted signature
  auto const tx_corrupted_sig = *bitcoin::parse_transaction(
    "02000000013f7cebd65c27431a90bba7f796914fe8cc2ddfc3f2cbd6f7e5f2fc854534da95"
    "000000006b483045022100de1ac3bcdfb0332207c4a91f3832bd2c2915840165f876ab47c6"
    "f8996b971c3602201c6c053d750fadde599e6f5c4e1963df0f01fc0d97815e8157e3d59fe0"
    "9ca30d012103699b464d1d8bc9e47d4fb1cdaa89a1c5783d68363c4dbc4b524ed3d8571486"
    "17feffffff02836d3c01000000001976a914fc25d6d5c94003bf5b0c7b640a248e2c637fcf"
    "b088ac7ada8202000000001976a914fbed3d9b11183209a57999d54d59f67c019e756c88ac"
    "6acb0700"_hex);
  CHECK(!verify(spk, amount, tx_corrupted_sig, 0, flags::none));

  // same tx but with a non-DER signature
  auto const tx_non_der_sig = *bitcoin::parse_transaction(
    "02000000013f7cebd65c27431a90bba7f796914fe8cc2ddfc3f2cbd6f7e5f2fc854534da95"
    "000000006b483046022100de1ac3bcdfb0332207c4a91f3832bd2c2915840165f876ab47c5"
    "f8996b971c3602201c6c053d750fadde599e6f5c4e1963df0f01fc0d97815e8157e3d59fe0"
    "9ca30d012103699b464d1d8bc9e47d4fb1cdaa89a1c5783d68363c4dbc4b524ed3d8571486"
    "17feffffff02836d3c01000000001976a914fc25d6d5c94003bf5b0c7b640a248e2c637fcf"
    "b088ac7ada8202000000001976a914fbed3d9b11183209a57999d54d59f67c019e756c88ac"
    "6acb0700"_hex);
  CHECK(verify(spk, amount, tx_non_der_sig, 0, flags::none));
  CHECK(!verify(spk, amount, tx_non_der_sig, 0, flags::dersig));
}

TEST_CASE("p2sh multisig verification")
{
  // Spending a multisig P2SH output using a mainnet tx with id
  // 3cd7f78499632d6f672d8a9412ae756b29c41342954c97846e0d153c7753a37e
  auto const spk =
    bitcoin::script{"a914fc8b5799cb5ae54c1be1fd97844a1cd97e820c5587"_hex};
  auto const tx_valid = *bitcoin::parse_transaction(
    "0100000001dd320ee7e290ddd042332f85dd064d2ee052257a9f4761929c237a7674ff1f0d"
    "01000000fdfe0000483045022100f808cadda09bf753740a9d1f012fe9224d670d2b4337af"
    "61858e9a61d1415a6a0220296e83ac33055c8e58bcd4f7a1afc010b6da0787e41b0add9ced"
    "70bf1b5694c901483045022100ed525f5b43420c4fe745a19276e851bf21270bbb81717e4e"
    "ad7d7919a1be267802201b48b9e6c92cc698f6ceb0c170321deb253d9d94c355f00bb0c672"
    "7a567d3dcf014c69522102239bbabd01dc2e4974d60dd658ca8547924f3f3fa5e583f4dea1"
    "16c5a330b7d32102135d7f51e7c3aced9d0b7a5c0dc374eea814afce5e5a075f3bacf143b3"
    "3af2e62102606e72be62d5fcff8764807ff676d31e7e99b5f56b79e38fdbb794d2796bbbfa"
    "53aeffffffff02846a8700000000001976a914932850c5373a1dda47027c51125b0493c026"
    "c9a388ac4da06c000000000017a91498dd7103a99f268f443fee4424a240af3d4a5aeb8700"
    "000000"_hex);
  auto const amount = 0 * bitcoin::units::satoshi;

  CHECK(verify(spk, amount, tx_valid, 0, flags::p2sh));
  CHECK(verify(spk, amount, tx_valid, 0, flags::all & ~flags::taproot));

  // same tx but with corrupted signature
  auto const tx_corrupted_sig = *bitcoin::parse_transaction(
    "0100000001dd320ee7e290ddd042332f85dd064d2ee052257a9f4761929c237a7674ff1f0d"
    "01000000fdfe0000483045023100f808cadda09bf753740a9d1f012fe9224d670d2b4337af"
    "61858e9a61d1415a6a0220296e83ac33055c8e58bcd4f7a1afc010b6da0787e41b0add9ced"
    "70bf1b5694c901483045022100ed525f5b43420c4fe745a19276e851bf21270bbb81717e4e"
    "ad7d7919a1be267802201b48b9e6c92cc698f6ceb0c170321deb253d9d94c355f00bb0c672"
    "7a567d3dcf014c69522102239bbabd01dc2e4974d60dd658ca8547924f3f3fa5e583f4dea1"
    "16c5a330b7d32102135d7f51e7c3aced9d0b7a5c0dc374eea814afce5e5a075f3bacf143b3"
    "3af2e62102606e72be62d5fcff8764807ff676d31e7e99b5f56b79e38fdbb794d2796bbbfa"
    "53aeffffffff02846a8700000000001976a914932850c5373a1dda47027c51125b0493c026"
    "c9a388ac4da06c000000000017a91498dd7103a99f268f443fee4424a240af3d4a5aeb8700"
    "000000"_hex);
  CHECK(!verify(spk, amount, tx_corrupted_sig, 0, flags::p2sh));
  CHECK(verify(spk, amount, tx_corrupted_sig, 0, flags::none));

  // same tx but with a non-null dummy stack element
  auto const tx_non_null_dummy = *bitcoin::parse_transaction(
    "0100000001dd320ee7e290ddd042332f85dd064d2ee052257a9f4761929c237a7674ff1f0d"
    "01000000fdfe0051483045022100f808cadda09bf753740a9d1f012fe9224d670d2b4337af"
    "61858e9a61d1415a6a0220296e83ac33055c8e58bcd4f7a1afc010b6da0787e41b0add9ced"
    "70bf1b5694c901483045022100ed525f5b43420c4fe745a19276e851bf21270bbb81717e4e"
    "ad7d7919a1be267802201b48b9e6c92cc698f6ceb0c170321deb253d9d94c355f00bb0c672"
    "7a567d3dcf014c69522102239bbabd01dc2e4974d60dd658ca8547924f3f3fa5e583f4dea1"
    "16c5a330b7d32102135d7f51e7c3aced9d0b7a5c0dc374eea814afce5e5a075f3bacf143b3"
    "3af2e62102606e72be62d5fcff8764807ff676d31e7e99b5f56b79e38fdbb794d2796bbbfa"
    "53aeffffffff02846a8700000000001976a914932850c5373a1dda47027c51125b0493c026"
    "c9a388ac4da06c000000000017a91498dd7103a99f268f443fee4424a240af3d4a5aeb8700"
    "000000"_hex);
  CHECK(verify(spk, amount, tx_non_null_dummy, 0, flags::p2sh));
  CHECK(
    !verify(spk, amount, tx_non_null_dummy, 0, flags::p2sh | flags::nulldummy));
}

TEST_CASE("cltv verification")
{
  // Spending a CLTV-locked P2SH output (locked to block 100)
  auto const spk =
    bitcoin::script{"a914e6855a94d0e499a0d554b2476cb779885986575b87"_hex};

  // tx with locktime 100 (satisfying CLTV condition)
  auto const tx_locktime_100 = *bitcoin::parse_transaction(
    "0200000001738df4ccd15745a9539ff632cbe903f1050807edf35a1a3835c5bca619078f19"
    "010000007047304402202124e252c265a905c02063b1235b77c2406ae3c44d5749117f0ac0"
    "86e2350ba2022061b65765be68ac21b60ede916d2a6deb1e6f8c9f723e66859bf4f06d98f7"
    "112b01270164b1752102d8019ae39403a4c0b49e98a0be4ed9ad0b1ba20f324fd6268c7455"
    "841deddd0dac000000000118ddf50500000000015164000000"_hex);
  auto const amount = 0 * bitcoin::units::satoshi;

  CHECK(verify(spk, amount, tx_locktime_100, 0,
               flags::p2sh | flags::checklocktimeverify));
  CHECK(verify(spk, amount, tx_locktime_100, 0, flags::all & ~flags::taproot));

  // tx with locktime 50 (not satisfying CLTV condition)
  auto const tx_locktime_50 = *bitcoin::parse_transaction(
    "0200000001738df4ccd15745a9539ff632cbe903f1050807edf35a1a3835c5bca619078f19"
    "0100000071483045022100bdaefb402ddf25738762c57978b9f02adb9007e7c835673d8cbd"
    "a6bc0b58ee78022054011b17b5d7d8b516d1ced26d124b435a5c4cccc7492778ab2a6661f5"
    "ef365801270164b1752102d8019ae39403a4c0b49e98a0be4ed9ad0b1ba20f324fd6268c74"
    "55841deddd0dac000000000118ddf50500000000015132000000"_hex);
  CHECK(!verify(spk, amount, tx_locktime_50, 0,
                flags::p2sh | flags::checklocktimeverify));
  CHECK(verify(spk, amount, tx_locktime_50, 0, flags::p2sh));
}

TEST_CASE("csv verification")
{
  // Spending a CSV-locked P2SH output (locked to sequence 10)
  auto const spk =
    bitcoin::script{"a914284e9e01049bc9bfe2a1f06e6e78cd29a717ffb987"_hex};

  // tx with input sequence 10 (satisfying CSV condition)
  auto const tx_sequence_10 = *bitcoin::parse_transaction(
    "0200000001b4b3b5eed405f118a43e411d51eace0b7a48ad6ec061e0c6d1e2a5dbc50fa178"
    "0100000071483045022100fc82daf200e56d9500d3cdb4708a5d9cf1b62a3299dedc9cdf7b"
    "6e6412b7fa280220144b6d6c535a6a66f60d25b68c6b6ae95c0e45c8801e4007b76d0cfcf2"
    "64ec4d0127010ab275210291c420b3afc1c75796653268a727d61df2edd606c243b261df61"
    "dd22f388553fac0a0000000118ddf50500000000015100000000"_hex);
  auto const amount = 0 * bitcoin::units::satoshi;

  CHECK(verify(spk, amount, tx_sequence_10, 0,
               flags::p2sh | flags::checksequenceverify));
  CHECK(verify(spk, amount, tx_sequence_10, 0, flags::all & ~flags::taproot));

  // tx with input sequence 5 (not satisfying CSV condition)
  auto const tx_sequence_5 = *bitcoin::parse_transaction(
    "0200000001b4b3b5eed405f118a43e411d51eace0b7a48ad6ec061e0c6d1e2a5dbc50fa178"
    "010000007148304502210080c15d7432d18c78529b95f6677dfc57ae22024789eefed400db"
    "febcdc8341da02204775fed6dedc3b540eea67c3bf8058e1211248970cdab9980c0ed09736"
    "e13fbb0127010ab275210291c420b3afc1c75796653268a727d61df2edd606c243b261df61"
    "dd22f388553fac050000000118ddf50500000000015100000000"_hex);
  CHECK(!verify(spk, amount, tx_sequence_5, 0,
                flags::p2sh | flags::checksequenceverify));
  CHECK(verify(spk, amount, tx_sequence_5, 0, flags::p2sh));
}

TEST_CASE("p2sh p2wpkh verification")
{
  // Spending a P2SH-P2WPKH output using a mainnet tx with id
  // 07dea5918a500d7476b1d116d80507a66bc2167681b2e6ca7dd99dbc6d95c31d
  auto const spk =
    bitcoin::script{"a91434c06f8c87e355e123bdc6dda4ffabc64b6989ef87"_hex};
  auto const tx_valid = *bitcoin::parse_transaction(
    "01000000000101d9fd94d0ff0026d307c994d0003180a5f248146efb6371d040c5973f5f66"
    "d9df0400000017160014b31b31a6cb654cfab3c50567bcf124f48a0beaecffffffff012cbd"
    "1c000000000017a914233b74bf0823fa58bbbd26dfc3bb4ae715547167870247304402206f"
    "60569cac136c114a58aedd80f6fa1c51b49093e7af883e605c212bdafcd8d202200e91a55f"
    "408a021ad2631bc29a67bd6915b2d7e9ef0265627eabd7f7234455f6012103e7e802f50344"
    "303c76d12c089c8724c1b230e3b745693bbe16aad536293d15e300000000"_hex);
  auto const amount = 1900000 * bitcoin::units::satoshi;

  CHECK(verify(spk, amount, tx_valid, 0, flags::p2sh | flags::witness));
  CHECK(verify(spk, amount, tx_valid, 0, flags::all & ~flags::taproot));

  // same tx but with corrupted signature
  auto const tx_corrupted_sig = *bitcoin::parse_transaction(
    "01000000000101d9fd94d0ff0026d307c994d0003180a5f248146efb6371d040c5973f5f66"
    "d9df0400000017160014b31b31a6cb654cfab3c50567bcf124f48a0beaecffffffff012cbd"
    "1c000000000017a914233b74bf0823fa58bbbd26dfc3bb4ae715547167870247304402206f"
    "60569cac136c114a58aedd80f6fa1c51b49093e7af883e615c212bdafcd8d202200e91a55f"
    "408a021ad2631bc29a67bd6915b2d7e9ef0265627eabd7f7234455f6012103e7e802f50344"
    "303c76d12c089c8724c1b230e3b745693bbe16aad536293d15e300000000"_hex);
  CHECK(
    !verify(spk, amount, tx_corrupted_sig, 0, flags::p2sh | flags::witness));
  CHECK(verify(spk, amount, tx_corrupted_sig, 0, flags::p2sh));
}

TEST_CASE("p2sh p2wsh verification")
{
  // Spending a P2SH-P2WSH output using a mainnet tx with id
  // 017be55761bf5a3920c73778810a6be4c3315dc6efa4f31b590bc3bc1da9d75f
  auto const spk =
    bitcoin::script{"a91469abb4763c2074e22a2ab2e06208f552bf7c654387"_hex};
  auto const tx_valid = *bitcoin::parse_transaction(
    "020000000001018d30fa8c3023ae9e72f44cea3525b4fe084a816830efb98b605678230cc9"
    "c48d0100000023220020a9cd0514e7793003d378df96bbe21c901421ff089ad5598a1a9b5a"
    "3cf6eaef0afdffffff0201ed11a60000000017a914194b877577228c0012e4ca3c3780198d"
    "8b09d14c87bccb510200000000160014db65e96bfbf0d6ce727ab2518a2f45635367f8b304"
    "0047304402202e679780ebe920a1e367482cace23cebaa9d8be7e7d1bd727d7439fb62c833"
    "2802204008e33b111161206cd09c947588b43114011a60c7dbaf0679c60d891e38fbb70147"
    "3044022029233663e8fdbaead30f4ff806018d7ec939505fb4e11cd53195e3ca83f36f5302"
    "202010174a3b84d1128272ef1fabf23f1add09e8e074ed2443688a718be86947e401695221"
    "02282a387d21d8784fbbf28e6d0cb3f9984996771e7283bd8f5c32dc4b7a961ba821029458"
    "6ab0277cae2a8b138527816bf9f36a7203265d83e08bbeaebdd7e1568b2c21030e724028e6"
    "4b78c0479a35e7c9b4cc3c13456f12ba993a8f19a38ee3b2d7743953ae00000000"_hex);
  auto const amount = 2825108081 * bitcoin::units::satoshi;

  CHECK(verify(spk, amount, tx_valid, 0, flags::p2sh | flags::witness));
  CHECK(verify(spk, amount, tx_valid, 0, flags::all & ~flags::taproot));

  // same tx but with corrupted signature
  auto const tx_corrupted_sig = *bitcoin::parse_transaction(
    "020000000001018d30fa8c3023ae9e72f44cea3525b4fe084a816830efb98b605678230cc9"
    "c48d0100000023220020a9cd0514e7793003d378df96bbe21c901421ff089ad5598a1a9b5a"
    "3cf6eaef0afdffffff0201ed11a60000000017a914194b877577228c0012e4ca3c3780198d"
    "8b09d14c87bccb510200000000160014db65e96bfbf0d6ce727ab2518a2f45635367f8b304"
    "0047304402202e679780ebe920a1e367482cace23cebaa9d8be7e7d1bd727d7439fb62c833"
    "2802203008e33b111161206cd09c947588b43114011a60c7dbaf0679c60d891e38fbb70147"
    "3044022029233663e8fdbaead30f4ff806018d7ec939505fb4e11cd53195e3ca83f36f5302"
    "202010174a3b84d1128272ef1fabf23f1add09e8e074ed2443688a718be86947e401695221"
    "02282a387d21d8784fbbf28e6d0cb3f9984996771e7283bd8f5c32dc4b7a961ba821029458"
    "6ab0277cae2a8b138527816bf9f36a7203265d83e08bbeaebdd7e1568b2c21030e724028e6"
    "4b78c0479a35e7c9b4cc3c13456f12ba993a8f19a38ee3b2d7743953ae00000000"_hex);
  CHECK(
    !verify(spk, amount, tx_corrupted_sig, 0, flags::p2sh | flags::witness));
  CHECK(verify(spk, amount, tx_corrupted_sig, 0, flags::p2sh));
}

TEST_CASE("p2wpkh verification")
{
  // Spending a native P2WPKH output using a mainnet tx with id
  // 00000000102d4e899ec7cc3656d91ab83aa8e95807dabb90fbe16a1a9e70b6ab
  auto const spk =
    bitcoin::script{"0014141e536966344275512b7c2f49be5b8fbe7fbd05"_hex};
  auto const tx_valid = *bitcoin::parse_transaction(
    "0200000000010118cd99a3898c2b63da66ec9b7e1d15928453a0b3c2fa74fd748830420000"
    "00000000000000ffffffff011d13000000000000160014f222ad02300df72ab7129602f279"
    "b47d83b453ca02483045022100b1da3d290132155acd68dafee7c794e84922f364cf9acb1b"
    "65e806dc41bd702b02200af6003caf19a291488649be0396edb1274888beb5bb3a96e1d8e6"
    "f4903e0e7401210364b35b722e1e3590575994dad5c6c25b8b24d3777964a28cedea41bbaa"
    "297da555d2d73d"_hex);
  auto const amount = 5003 * bitcoin::units::satoshi;

  CHECK(verify(spk, amount, tx_valid, 0, flags::p2sh | flags::witness));
  CHECK(verify(spk, amount, tx_valid, 0, flags::all & ~flags::taproot));

  // using wrong amount
  CHECK(!verify(spk, 5002 * bitcoin::units::satoshi, tx_valid, 0,
                flags::p2sh | flags::witness));
  CHECK(verify(spk, 5002 * bitcoin::units::satoshi, tx_valid, 0, flags::p2sh));
}

TEST_CASE("p2wsh verification")
{
  // Spending a P2WSH output using a mainnet tx with id
  // 12fc05be6778b06e77191e8fb18fee632b2d92efa0b6830e1cf63e28723a8b8f
  auto const spk = bitcoin::script{
    "0020b38c970d115bffbc7d16c5f3fc858cefe3448c8d141a679b65554de78a88a0cd"_hex};
  auto const tx_valid = *bitcoin::parse_transaction(
    "01000000000102e1434357ad4d08274ed106e21f2694d35000b46e66b1dd714d63e9cd3f2a"
    "b4740000000000ffffffffb9de6f14166a841772ec53c6e384adb3dc151ba1e7b2c0a92cc9"
    "f194690be1240000000000ffffffff0198282d0000000000160014c95362dd904e547af461"
    "dbf4cc4d251a8fa1295205483045022100c8553375207dd6ef92439a22707a268629b4af08"
    "ae94d02e92c8acc355f5911902207ed6b01a97a9cc52c1fb8c6c6999b27e9c34f703a03917"
    "d8a640a45e0589ed9901210200c503dc20b66731af9d189f0a0981b148b40cb2c26f3ff15c"
    "c90037b812b6e22017e3c268fe7c34ea02effd3975038fc09ce0326b063fe0ec9356b6f558"
    "ba55ff0101616382012088a820be72730c5ba3ae4d924ef26be8a20b2e820d63916bb9db4f"
    "38214f739b897e1d8876a9148d1fe4411b3cd3a0bccafb8b57ee0c071e5abd79670428c189"
    "69b17576a914bec3dacae92ec1cd60843a8704119b0e202c6d346888ac05483045022100ae"
    "691ea4c91edf52f44e56e4f53d1ab6bb2562bb34f4f56f4f0003ea52f91fa7022001ca8196"
    "e6961b29cff64e1b5ac8917ba90a162f8230860bbc7b050624578fb701210200c503dc20b6"
    "6731af9d189f0a0981b148b40cb2c26f3ff15cc90037b812b6e220362fc6c6a5b56532c27f"
    "701a067eda066ec2be426b5cd696a3a030abffc446ce0101616382012088a820360537c727"
    "eed2694b8d5493c1b0b5d79289042af10f6cdb3361f14d3fab523e8876a9148d1fe4411b3c"
    "d3a0bccafb8b57ee0c071e5abd7967043cc18969b17576a914ad27aa467040ddf17bbea8be"
    "0794e3e6953c09066888ac00000000"_hex);
  auto const amount = 1480000 * bitcoin::units::satoshi;

  CHECK(verify(spk, amount, tx_valid, 1, flags::p2sh | flags::witness));
  CHECK(verify(spk, amount, tx_valid, 1, flags::all & ~flags::taproot));

  // same tx but with corrupted signature
  auto const tx_corrupted_sig = *bitcoin::parse_transaction(
    "01000000000102e1434357ad4d08274ed106e21f2694d35000b46e66b1dd714d63e9cd3f2a"
    "b4740000000000ffffffffb9de6f14166a841772ec53c6e384adb3dc151ba1e7b2c0a92cc9"
    "f194690be1240000000000ffffffff0198282d0000000000160014c95362dd904e547af461"
    "dbf4cc4d251a8fa1295205483045022100c8553375207dd6ef92439a22707a268629b4af08"
    "ae94d02e92c8acc355f5911902207ed6b01a97a9cc52c1fb8c6c6999b27e9c34f703a03917"
    "d8a640a45e0589ed9901210200c503dc20b66731af9d189f0a0981b148b40cb2c26f3ff15c"
    "c90037b812b6e22017e3c268fe7c34ea02effd3975038fc09ce0326b063fe0ec9356b6f558"
    "ba55ff0101616382012088a820be72730c5ba3ae4d924ef26be8a20b2e820d63916bb9db4f"
    "38214f739b897e1d8876a9148d1fe4411b3cd3a0bccafb8b57ee0c071e5abd79670428c189"
    "69b17576a914bec3dacae92ec1cd60843a8704119b0e202c6d346888ac05483045022100ae"
    "691ea4c91edf52f44e56e4f53d1ab6bb2562bb34f4f56f4f0003ea52f91fa7022001ca2196"
    "e6961b29cff64e1b5ac8917ba90a162f8230860bbc7b050624578fb701210200c503dc20b6"
    "6731af9d189f0a0981b148b40cb2c26f3ff15cc90037b812b6e220362fc6c6a5b56532c27f"
    "701a067eda066ec2be426b5cd696a3a030abffc446ce0101616382012088a820360537c727"
    "eed2694b8d5493c1b0b5d79289042af10f6cdb3361f14d3fab523e8876a9148d1fe4411b3c"
    "d3a0bccafb8b57ee0c071e5abd7967043cc18969b17576a914ad27aa467040ddf17bbea8be"
    "0794e3e6953c09066888ac00000000"_hex);
  CHECK(
    !verify(spk, amount, tx_corrupted_sig, 1, flags::p2sh | flags::witness));
  CHECK(verify(spk, amount, tx_corrupted_sig, 1, flags::p2sh));
}

TEST_CASE("p2tr keypath verification")
{
  // Spending a P2TR output via the key-path using a mainnet tx with id
  // 33e794d097969002ee05d336686fc03c9e15a597c1b9827669460fac98799036
  auto const spk = bitcoin::script{
    "5120339ce7e165e67d93adb3fef88a6d4beed33f01fa876f05a225242b82a631abc0"_hex};
  auto const tx_valid = *bitcoin::parse_transaction(
    "01000000000101d1f1c1f8cdf6759167b90f52c9ad358a369f95284e841d7a2536cef31c05"
    "49580100000000fdffffff020000000000000000316a2f49206c696b65205363686e6f7272"
    "207369677320616e6420492063616e6e6f74206c69652e204062697462756734329e060100"
    "00000000225120a37c3903c8d0db6512e2b40b0dffa05e5a3ab73603ce8c9c4b7771e54123"
    "28f90140a60c383f71bac0ec919b1d7dbc3eb72dd56e7aa99583615564f9f99b8ae4e837b7"
    "58773a5b2e4c51348854c8389f008e05029db7f464a5ff2e01d5e6e626174affd30a00"_hex);
  auto const amount = 88480 * bitcoin::units::satoshi;
  auto const output = bitcoin::tx_output{amount, spk};
  auto const base_flags = flags::p2sh | flags::witness | flags::taproot;

  CHECK(verify(spk, amount, tx_valid, 0, base_flags, std::span{&output, 1}));
  CHECK(verify(spk, amount, tx_valid, 0, flags::all, std::span{&output, 1}));

  // same tx but with corrupted signature
  auto const tx_corrupted_sig = *bitcoin::parse_transaction(
    "01000000000101d1f1c1f8cdf6759167b90f52c9ad358a369f95284e841d7a2536cef31c05"
    "49580100000000fdffffff020000000000000000316a2f49206c696b65205363686e6f7272"
    "207369677320616e6420492063616e6e6f74206c69652e204062697462756734329e060100"
    "00000000225120a37c3903c8d0db6512e2b40b0dffa05e5a3ab73603ce8c9c4b7771e54123"
    "28f90140a60c383f71bac0ec919b1d7dbc3eb72dd56e7aa99583615564f9f99b8ae4e837b7"
    "58772a5b2e4c51348854c8389f008e05029db7f464a5ff2e01d5e6e626174affd30a00"_hex);
  CHECK(!verify(spk, amount, tx_corrupted_sig, 0, base_flags,
                std::span{&output, 1}));
  CHECK(verify(spk, amount, tx_corrupted_sig, 0, base_flags & ~flags::taproot,
               std::span{&output, 1}));
}

TEST_CASE("p2tr scriptpath verification")
{
  // Spending a P2TR output via the script-path using a mainnet tx with id
  // 1ba232a8bf936cf24155292c9a4330298278f572bacc78455eb68e3552197c30
  auto const spk = bitcoin::script{
    "5120e687f4f55e3de5264cf4c4f43b53edb5c26e4adae3a3098ce918a663582785bd"_hex};
  auto const tx_valid = *bitcoin::parse_transaction(
    "02000000000102761402258bf42275f52db288dbbc8fdfe30b35dea86c5425a57feef1a400"
    "8b0b0100000000ffffffff3847ba0ccc4e1b63ed2f3b4a677bd247940f4d5669cc91d9a4eb"
    "096e7615badc0000000000ffffffff0291ad070000000000225120059715a12766bbbee852"
    "9b53dce51fe708e9895f50c6babec988c7917ff5958464d11a0000000000225120bee1246f"
    "13735551e5e5c2b5631014501a6c77f8ee2d16d33dc109096f22b2a40140ac4e4af854be64"
    "5890275c8144869343752d5ceee9b361cfab3de0726c10a449cc7491a295417f7c457961fd"
    "de59bde483330364b42fddddeef65cd1d97150fc0440b78c0a5065343d451a93dcb499edd3"
    "d8994697932322be5e27fa218f5a99be8484a8ef802c3054dee442baced3c170b8afe18ec9"
    "758c860876fe4f06a5e3ccb240984299fc968b71d999354af2e991e089908adec84e1b1f04"
    "da8149aa0ffce28311b363dfd2dfc456de77746919c263e95a14952080f433ddb87b13b884"
    "812bda4420b5095be39b9f2f96a77235854af7635dd09d0324569e9b3d587fe5fb7c44720c"
    "ad202b74c2011af089c849383ee527c72325de52df6a788428b68d49e9174053aabaac41c1"
    "50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0675a94484b"
    "3d55d76af4a2275d327a47c1ec5c7d2232596a09fc883d40bb237e00000000"_hex);
  auto const amount = 503185 * bitcoin::units::satoshi;
  auto const outputs = std::array{
    bitcoin::tx_output{
      1757828 * bitcoin::units::satoshi,
      bitcoin::script{
        "51207ee3c4ab9c8144be0e39fc849fab95e70da97fb0d70754b34553c25f9d325fa0"_hex},
    },
    bitcoin::tx_output{amount, spk},
  };
  auto const base_flags = flags::p2sh | flags::witness | flags::taproot;

  CHECK(verify(spk, amount, tx_valid, 1, base_flags, outputs));
  CHECK(verify(spk, amount, tx_valid, 1, flags::all, outputs));

  // same tx but with corrupted signature
  auto const tx_corrupted_sig = *bitcoin::parse_transaction(
    "02000000000102761402258bf42275f52db288dbbc8fdfe30b35dea86c5425a57feef1a400"
    "8b0b0100000000ffffffff3847ba0ccc4e1b63ed2f3b4a677bd247940f4d5669cc91d9a4eb"
    "096e7615badc0000000000ffffffff0291ad070000000000225120059715a12766bbbee852"
    "9b53dce51fe708e9895f50c6babec988c7917ff5958464d11a0000000000225120bee1246f"
    "13735551e5e5c2b5631014501a6c77f8ee2d16d33dc109096f22b2a40140ac4e4af854be64"
    "5890275c8144869343752d5ceee9b361cfab3de0726c10a449cc7491a295417f7c457961fd"
    "de59bde483330364b42fddddeef65cd1d97150fc0440b78c0a5065343d451a93dcb499edd3"
    "d8994697932322be5e27fa218f5a99be8484a8ef802c3054dee442baced3c170b8afe18ec9"
    "758c860876fe4f06a5e3ccb240984299fc968b71d999354af2e991e089908adec84e1b1f04"
    "da8149aa0ffce28211b363dfd2dfc456de77746919c263e95a14952080f433ddb87b13b884"
    "812bda4420b5095be39b9f2f96a77235854af7635dd09d0324569e9b3d587fe5fb7c44720c"
    "ad202b74c2011af089c849383ee527c72325de52df6a788428b68d49e9174053aabaac41c1"
    "50929b74c1a04954b78b4b6035e97a5e078a5a0f28ec96d547bfee9ace803ac0675a94484b"
    "3d55d76af4a2275d327a47c1ec5c7d2232596a09fc883d40bb237e00000000"_hex);
  CHECK(!verify(spk, amount, tx_corrupted_sig, 1, base_flags, outputs));
  CHECK(verify(spk, amount, tx_corrupted_sig, 1, base_flags & ~flags::taproot,
               outputs));
}
