// SPDX-License-Identifier: BSL-1.0

#include <bitcoin/script.hpp>

namespace {

using bitcoin::opcode;

auto str(opcode code) -> std::string_view
{
  switch (code) {
    case opcode::op_0:                   return "OP_0"; // OP_FALSE
    case opcode::op_pushdata1:           return "OP_PUSHDATA1";
    case opcode::op_pushdata2:           return "OP_PUSHDATA2";
    case opcode::op_pushdata4:           return "OP_PUSHDATA4";
    case opcode::op_1negate:             return "OP_1NEGATE";
    case opcode::op_reserved:            return "OP_RESERVED";
    case opcode::op_1:                   return "OP_1"; // OP_TRUE
    case opcode::op_2:                   return "OP_2";
    case opcode::op_3:                   return "OP_3";
    case opcode::op_4:                   return "OP_4";
    case opcode::op_5:                   return "OP_5";
    case opcode::op_6:                   return "OP_6";
    case opcode::op_7:                   return "OP_7";
    case opcode::op_8:                   return "OP_8";
    case opcode::op_9:                   return "OP_9";
    case opcode::op_10:                  return "OP_10";
    case opcode::op_11:                  return "OP_11";
    case opcode::op_12:                  return "OP_12";
    case opcode::op_13:                  return "OP_13";
    case opcode::op_14:                  return "OP_14";
    case opcode::op_15:                  return "OP_15";
    case opcode::op_16:                  return "OP_16";

    case opcode::op_nop:                 return "OP_NOP";
    case opcode::op_ver:                 return "OP_VER";
    case opcode::op_if:                  return "OP_IF";
    case opcode::op_notif:               return "OP_NOTIF";
    case opcode::op_verif:               return "OP_VERIF";
    case opcode::op_vernotif:            return "OP_VERNOTIF";
    case opcode::op_else:                return "OP_ELSE";
    case opcode::op_endif:               return "OP_ENDIF";
    case opcode::op_verify:              return "OP_VERIFY";
    case opcode::op_return:              return "OP_RETURN";

    case opcode::op_toaltstack:          return "OP_TOALTSTACK";
    case opcode::op_fromaltstack:        return "OP_FROMALTSTACK";
    case opcode::op_2drop:               return "OP_2DROP";
    case opcode::op_2dup:                return "OP_2DUP";
    case opcode::op_3dup:                return "OP_3DUP";
    case opcode::op_2over:               return "OP_2OVER";
    case opcode::op_2rot:                return "OP_2ROT";
    case opcode::op_2swap:               return "OP_2SWAP";
    case opcode::op_ifdup:               return "OP_IFDUP";
    case opcode::op_depth:               return "OP_DEPTH";
    case opcode::op_drop:                return "OP_DROP";
    case opcode::op_dup:                 return "OP_DUP";
    case opcode::op_nip:                 return "OP_NIP";
    case opcode::op_over:                return "OP_OVER";
    case opcode::op_pick:                return "OP_PICK";
    case opcode::op_roll:                return "OP_ROLL";
    case opcode::op_rot:                 return "OP_ROT";
    case opcode::op_swap:                return "OP_SWAP";
    case opcode::op_tuck:                return "OP_TUCK";

    case opcode::op_cat:                 return "OP_CAT";
    case opcode::op_substr:              return "OP_SUBSTR";
    case opcode::op_left:                return "OP_LEFT";
    case opcode::op_right:               return "OP_RIGHT";
    case opcode::op_size:                return "OP_SIZE";

    case opcode::op_invert:              return "OP_INVERT";
    case opcode::op_and:                 return "OP_AND";
    case opcode::op_or:                  return "OP_OR";
    case opcode::op_xor:                 return "OP_XOR";
    case opcode::op_equal:               return "OP_EQUAL";
    case opcode::op_equalverify:         return "OP_EQUALVERIFY";
    case opcode::op_reserved1:           return "OP_RESERVED1";
    case opcode::op_reserved2:           return "OP_RESERVED2";

    case opcode::op_1add:                return "OP_1ADD";
    case opcode::op_1sub:                return "OP_1SUB";
    case opcode::op_2mul:                return "OP_2MUL";
    case opcode::op_2div:                return "OP_2DIV";
    case opcode::op_negate:              return "OP_NEGATE";
    case opcode::op_abs:                 return "OP_ABS";
    case opcode::op_not:                 return "OP_NOT";
    case opcode::op_0notequal:           return "OP_0NOTEQUAL";

    case opcode::op_add:                 return "OP_ADD";
    case opcode::op_sub:                 return "OP_SUB";
    case opcode::op_mul:                 return "OP_MUL";
    case opcode::op_div:                 return "OP_DIV";
    case opcode::op_mod:                 return "OP_MOD";
    case opcode::op_lshift:              return "OP_LSHIFT";
    case opcode::op_rshift:              return "OP_RSHIFT";

    case opcode::op_booland:             return "OP_BOOLAND";
    case opcode::op_boolor:              return "OP_BOOLOR";
    case opcode::op_numequal:            return "OP_NUMEQUAL";
    case opcode::op_numequalverify:      return "OP_NUMEQUALVERIFY";
    case opcode::op_numnotequal:         return "OP_NUMNOTEQUAL";
    case opcode::op_lessthan:            return "OP_LESSTHAN";
    case opcode::op_greaterthan:         return "OP_GREATERTHAN";
    case opcode::op_lessthanorequal:     return "OP_LESSTHANOREQUAL";
    case opcode::op_greaterthanorequal:  return "OP_GREATERTHANOREQUAL";
    case opcode::op_min:                 return "OP_MIN";
    case opcode::op_max:                 return "OP_MAX";

    case opcode::op_within:              return "OP_WITHIN";

    case opcode::op_ripemd160:           return "OP_RIPEMD160";
    case opcode::op_sha1:                return "OP_SHA1";
    case opcode::op_sha256:              return "OP_SHA256";
    case opcode::op_hash160:             return "OP_HASH160";
    case opcode::op_hash256:             return "OP_HASH256";
    case opcode::op_codeseparator:       return "OP_CODESEPARATOR";
    case opcode::op_checksig:            return "OP_CHECKSIG";
    case opcode::op_checksigverify:      return "OP_CHECKSIGVERIFY";
    case opcode::op_checkmultisig:       return "OP_CHECKMULTISIG";
    case opcode::op_checkmultisigverify: return "OP_CHECKMULTISIGVERIFY";

    case opcode::op_nop1:                return "OP_NOP1";
    case opcode::op_checklocktimeverify: return "OP_CHECKLOCKTIMEVERIFY";
    case opcode::op_checksequenceverify: return "OP_CHECKSEQUENCEVERIFY";
    case opcode::op_nop4:                return "OP_NOP4";
    case opcode::op_nop5:                return "OP_NOP5";
    case opcode::op_nop6:                return "OP_NOP6";
    case opcode::op_nop7:                return "OP_NOP7";
    case opcode::op_nop8:                return "OP_NOP8";
    case opcode::op_nop9:                return "OP_NOP9";
    case opcode::op_nop10:               return "OP_NOP10";

    case opcode::op_checksigadd:         return "OP_CHECKSIGADD";

    default:                             return "OP_INVALIDOPCODE";
  }
}

} // namespace

auto std::formatter<bitcoin::opcode>::format(bitcoin::opcode code,
                                             std::format_context& ctx) const
  -> std::format_context::iterator
{
  return std::formatter<string_view>::format(str(code), ctx);
}
