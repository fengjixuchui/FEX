
#include "Interface/Core/JIT/Arm64/JITClass.h"
#include "Interface/IR/Passes/RegisterAllocationPass.h"

namespace FEXCore::CPU {
static uint64_t LUDIV(uint64_t SrcHigh, uint64_t SrcLow, uint64_t Divisor) {
  __uint128_t Source = (static_cast<__uint128_t>(SrcHigh) << 64) | SrcLow;
  __uint128_t Res = Source / Divisor;
  return Res;
}

static int64_t LDIV(int64_t SrcHigh, int64_t SrcLow, int64_t Divisor) {
  __int128_t Source = (static_cast<__int128_t>(SrcHigh) << 64) | SrcLow;
  __int128_t Res = Source / Divisor;
  return Res;
}

static uint64_t LUREM(uint64_t SrcHigh, uint64_t SrcLow, uint64_t Divisor) {
  __uint128_t Source = (static_cast<__uint128_t>(SrcHigh) << 64) | SrcLow;
  __uint128_t Res = Source % Divisor;
  return Res;
}

static int64_t LREM(int64_t SrcHigh, int64_t SrcLow, int64_t Divisor) {
  __int128_t Source = (static_cast<__int128_t>(SrcHigh) << 64) | SrcLow;
  __int128_t Res = Source % Divisor;
  return Res;
}

using namespace vixl;
using namespace vixl::aarch64;
#define DEF_OP(x) void JITCore::Op_##x(FEXCore::IR::IROp_Header *IROp, uint32_t Node)
DEF_OP(TruncElementPair) {
  auto Op = IROp->C<IR::IROp_TruncElementPair>();

  switch (Op->Size) {
    case 4: {
      auto Dst = GetSrcPair<RA_32>(Node);
      auto Src = GetSrcPair<RA_32>(Op->Header.Args[0].ID());
      mov(Dst.first, Src.first);
      mov(Dst.second, Src.second);
      break;
    }
    default: LogMan::Msg::A("Unhandled Truncation size: %d", Op->Size); break;
  }
}

DEF_OP(Constant) {
  auto Op = IROp->C<IR::IROp_Constant>();
  auto Dst = GetReg<RA_64>(Node);
  LoadConstant(Dst, Op->Constant);
}

DEF_OP(CycleCounter) {
#ifdef DEBUG_CYCLES
  movz(GetReg<RA_64>(Node), 0);
#else
  mrs(GetReg<RA_64>(Node), CNTVCT_EL0);
#endif
}

DEF_OP(Add) {
  auto Op = IROp->C<IR::IROp_Add>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 4:
      add(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()), GetReg<RA_32>(Op->Header.Args[1].ID()));
      break;
    case 8:
      add(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
      break;
    default: LogMan::Msg::A("Unsupported Add size: %d", OpSize);
  }
}

DEF_OP(Sub) {
  auto Op = IROp->C<IR::IROp_Sub>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 4:
      sub(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()), GetReg<RA_32>(Op->Header.Args[1].ID()));
      break;
    case 8:
      sub(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
      break;
    default: LogMan::Msg::A("Unsupported Sub size: %d", OpSize);
  }

}

DEF_OP(Neg) {
  auto Op = IROp->C<IR::IROp_Neg>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 4:
      neg(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()));
      break;
    case 8:
      neg(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()));
      break;
    default: LogMan::Msg::A("Unsupported Not size: %d", OpSize);
  }
}

DEF_OP(Mul) {
  auto Op = IROp->C<IR::IROp_Mul>();
  uint8_t OpSize = IROp->Size;
  auto Dst = GetReg<RA_64>(Node);

  switch (OpSize) {
    case 1:
      sxtb(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      sxtb(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mul(Dst, TMP1, TMP2);
      sxtb(Dst, Dst);
    break;
    case 2:
      sxth(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      sxth(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mul(Dst, TMP1, TMP2);
      sxth(Dst, Dst);
    break;
    case 4:
      sxtw(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      sxtw(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mul(Dst.W(), TMP1.W(), TMP2.W());
      sxtw(Dst, Dst);
    break;
    case 8:
      mul(Dst, GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
    break;
    default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
  }
}

DEF_OP(UMul) {
  auto Op = IROp->C<IR::IROp_UMul>();
  uint8_t OpSize = IROp->Size;
  auto Dst = GetReg<RA_64>(Node);

  switch (OpSize) {
    case 1:
      uxtb(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      uxtb(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mul(Dst, TMP1, TMP2);
      uxtb(Dst, Dst);
    break;
    case 2:
      uxth(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      uxth(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mul(Dst, TMP1, TMP2);
      uxth(Dst, Dst);
    break;
    case 4:
      uxtw(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      uxtw(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mul(Dst.W(), TMP1.W(), TMP2.W());
      uxtw(Dst, Dst);
    break;
    case 8:
      mul(Dst, GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
    break;
    default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
  }
}

DEF_OP(Div) {
  auto Op = IROp->C<IR::IROp_Div>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
    case 1: {
      auto Dividend = GetReg<RA_32>(Op->Header.Args[0].ID());
      auto Divisor = GetReg<RA_32>(Op->Header.Args[1].ID());
      sxtb(w2, Dividend);
      sxtb(w3, Divisor);

      sdiv(GetReg<RA_32>(Node), w2, w3);
    break;
    }
    case 2: {
      auto Dividend = GetReg<RA_32>(Op->Header.Args[0].ID());
      auto Divisor = GetReg<RA_32>(Op->Header.Args[1].ID());
      sxth(w2, Dividend);
      sxth(w3, Divisor);

      sdiv(GetReg<RA_32>(Node), w2, w3);
    break;
    }
    case 4: {
      sdiv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()), GetReg<RA_32>(Op->Header.Args[1].ID()));
    break;
    }
    case 8: {
      sdiv(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
    break;
    }
    default: LogMan::Msg::A("Unknown DIV Size: %d", Size); break;
  }
}

DEF_OP(UDiv) {
  auto Op = IROp->C<IR::IROp_UDiv>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
    case 1: {
      udiv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()), GetReg<RA_32>(Op->Header.Args[1].ID()));
      break;
    }
    case 2: {
      udiv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()), GetReg<RA_32>(Op->Header.Args[1].ID()));
      break;
    }
    case 4: {
      udiv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()), GetReg<RA_32>(Op->Header.Args[1].ID()));
      break;
    }
    case 8: {
      udiv(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
      break;
    }
    default: LogMan::Msg::A("Unknown UDIV Size: %d", Size); break;
  }
}

DEF_OP(Rem) {
  auto Op = IROp->C<IR::IROp_Rem>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 1: {
      auto Dividend = GetReg<RA_32>(Op->Header.Args[0].ID());
      auto Divisor = GetReg<RA_32>(Op->Header.Args[1].ID());
      sxtb(w2, Dividend);
      sxtb(w3, Divisor);

      sdiv(TMP1.W(), w2, w3);
      msub(GetReg<RA_32>(Node), TMP1.W(), w3, w2);
    break;
    }
    case 2: {
      auto Dividend = GetReg<RA_32>(Op->Header.Args[0].ID());
      auto Divisor = GetReg<RA_32>(Op->Header.Args[1].ID());

      sxth(w2, Dividend);
      sxth(w3, Divisor);

      sdiv(TMP1.W(), w2, w3);
      msub(GetReg<RA_32>(Node), TMP1.W(), w3, w2);
    break;
    }
    case 4: {
      auto Dividend = GetReg<RA_32>(Op->Header.Args[0].ID());
      auto Divisor = GetReg<RA_32>(Op->Header.Args[1].ID());

      sdiv(TMP1.W(), Dividend, Divisor);
      msub(GetReg<RA_32>(Node), TMP1, Divisor, Dividend);
    break;
    }
    case 8: {
      auto Dividend = GetReg<RA_64>(Op->Header.Args[0].ID());
      auto Divisor = GetReg<RA_64>(Op->Header.Args[1].ID());

      sdiv(TMP1, Dividend, Divisor);
      msub(GetReg<RA_64>(Node), TMP1, Divisor, Dividend);
    break;
    }
    default: LogMan::Msg::A("Unknown REM Size: %d", OpSize); break;
  }
}

DEF_OP(URem) {
  auto Op = IROp->C<IR::IROp_URem>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 1: {
      auto Dividend = GetReg<RA_32>(Op->Header.Args[0].ID());
      auto Divisor = GetReg<RA_32>(Op->Header.Args[1].ID());

      udiv(TMP1.W(), Dividend, Divisor);
      msub(GetReg<RA_32>(Node), TMP1, Divisor, Dividend);
    break;
    }
    case 2: {
      auto Dividend = GetReg<RA_32>(Op->Header.Args[0].ID());
      auto Divisor = GetReg<RA_32>(Op->Header.Args[1].ID());

      udiv(TMP1.W(), Dividend, Divisor);
      msub(GetReg<RA_32>(Node), TMP1, Divisor, Dividend);
    break;
    }
    case 4: {
      auto Dividend = GetReg<RA_32>(Op->Header.Args[0].ID());
      auto Divisor = GetReg<RA_32>(Op->Header.Args[1].ID());

      udiv(TMP1.W(), Dividend, Divisor);
      msub(GetReg<RA_32>(Node), TMP1, Divisor, Dividend);
    break;
    }
    case 8: {
      auto Dividend = GetReg<RA_64>(Op->Header.Args[0].ID());
      auto Divisor = GetReg<RA_64>(Op->Header.Args[1].ID());

      udiv(TMP1, Dividend, Divisor);
      msub(GetReg<RA_64>(Node), TMP1, Divisor, Dividend);
    break;
    }
    default: LogMan::Msg::A("Unknown UREM Size: %d", OpSize); break;
  }
}

DEF_OP(MulH) {
  auto Op = IROp->C<IR::IROp_MulH>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 1:
      sxtb(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      sxtb(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mul(TMP1, TMP1, TMP2);
      sbfx(GetReg<RA_64>(Node), TMP1, 8, 8);
    break;
    case 2:
      sxth(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      sxth(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mul(TMP1, TMP1, TMP2);
      sbfx(GetReg<RA_64>(Node), TMP1, 16, 16);
    break;
    case 4:
      sxtw(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      sxtw(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mul(TMP1, TMP1, TMP2);
      sbfx(GetReg<RA_64>(Node), TMP1, 32, 32);
    break;
    case 8:
      smulh(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
    break;
    default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
  }
}

DEF_OP(UMulH) {
  auto Op = IROp->C<IR::IROp_UMulH>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 1:
      uxtb(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      uxtb(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mul(TMP1, TMP1, TMP2);
      ubfx(GetReg<RA_64>(Node), TMP1, 8, 8);
    break;
    case 2:
      uxth(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      uxth(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mul(TMP1, TMP1, TMP2);
      ubfx(GetReg<RA_64>(Node), TMP1, 16, 16);
    break;
    case 4:
      uxtw(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      uxtw(TMP2, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mul(TMP1, TMP1, TMP2);
      ubfx(GetReg<RA_64>(Node), TMP1, 32, 32);
    break;
    case 8:
      umulh(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
    break;
    default: LogMan::Msg::A("Unknown Sext size: %d", OpSize);
  }
}

DEF_OP(Or) {
  auto Op = IROp->C<IR::IROp_Or>();
  orr(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
}

DEF_OP(And) {
  auto Op = IROp->C<IR::IROp_And>();
  and_(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
}

DEF_OP(Xor) {
  auto Op = IROp->C<IR::IROp_Xor>();
  eor(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
}

DEF_OP(Lshl) {
  auto Op = IROp->C<IR::IROp_Lshl>();
  uint8_t OpSize = IROp->Size;
  if (OpSize == 8)
    lslv(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
  else
    lslv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()), GetReg<RA_32>(Op->Header.Args[1].ID()));
}

DEF_OP(Lshr) {
  auto Op = IROp->C<IR::IROp_Lshr>();
  uint8_t OpSize = IROp->Size;
  if (OpSize == 8)
    lsrv(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
  else
    lsrv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()), GetReg<RA_32>(Op->Header.Args[1].ID()));
}

DEF_OP(Ashr) {
  auto Op = IROp->C<IR::IROp_Ashr>();
  uint8_t OpSize = IROp->Size;
  if (OpSize == 8)
    asrv(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
  else if (OpSize == 4) {
    asrv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()), GetReg<RA_32>(Op->Header.Args[1].ID()));
  }
  else {
    sbfx(TMP1.X(), GetReg<RA_64>(Op->Header.Args[0].ID()), 0, OpSize * 8);
    asrv(GetReg<RA_64>(Node), TMP1.X(), GetReg<RA_64>(Op->Header.Args[1].ID()));
    ubfx(GetReg<RA_64>(Node),GetReg<RA_64>(Node), 0, OpSize * 8);
  }
}

DEF_OP(Rol) {
  auto Op = IROp->C<IR::IROp_Rol>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 1: {
      movz(TMP1, 8);
      sub(TMP1.W(), TMP1.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));

      mov(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()));
      bfi(GetReg<RA_32>(Node), GetReg<RA_32>(Node), 8, 8);
      bfi(GetReg<RA_32>(Node), GetReg<RA_32>(Node), 16, 8);
      bfi(GetReg<RA_32>(Node), GetReg<RA_32>(Node), 24, 8);
      rorv(GetReg<RA_32>(Node), GetReg<RA_32>(Node), TMP1.W());
      and_(GetReg<RA_32>(Node), GetReg<RA_32>(Node), 0xFF);
      break;
    }
    case 2: {
      movz(TMP1, 16);
      sub(TMP1.W(), TMP1.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));

      mov(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()));
      bfi(GetReg<RA_32>(Node), GetReg<RA_32>(Node), 16, 16);
      rorv(GetReg<RA_32>(Node), GetReg<RA_32>(Node), TMP1.W());
      and_(GetReg<RA_32>(Node), GetReg<RA_32>(Node), 0xFFFF);
      break;
    }
    case 4: {
      movz(TMP1, 32);
      sub(TMP1.W(), TMP1.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
      rorv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()), TMP1.W());
    break;
    }
    case 8: {
      movz(TMP1, 64);
      sub(TMP1, TMP1, GetReg<RA_64>(Op->Header.Args[1].ID()));
      rorv(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), TMP1);
    break;
    }
    default: LogMan::Msg::A("Unhandled ROL size: %d", OpSize);
  }
}

DEF_OP(Ror) {
  auto Op = IROp->C<IR::IROp_Ror>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 1: {
      mov(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()));
      bfi(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()), 8, 8);
      bfi(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()), 16, 8);
      bfi(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()), 24, 8);
      rorv(GetReg<RA_32>(Node), TMP1.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
    break;
    }
    case 2: {
      mov(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()));
      bfi(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()), 16, 16);
      rorv(GetReg<RA_32>(Node), TMP1.W(), GetReg<RA_32>(Op->Header.Args[1].ID()));
    break;
    }
    case 4: {
      rorv(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()), GetReg<RA_32>(Op->Header.Args[1].ID()));
    break;
    }
    case 8: {
      rorv(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));
    break;
    }

    default: LogMan::Msg::A("Unhandled ROR size: %d", OpSize);
  }
}

DEF_OP(Extr) {
  auto Op = IROp->C<IR::IROp_Extr>();
  uint8_t OpSize = IROp->Size;

  switch (OpSize) {
    case 4: {
      extr(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()), GetReg<RA_32>(Op->Header.Args[1].ID()), Op->LSB);
    break;
    }
    case 8: {
      extr(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()), Op->LSB);
    break;
    }

    default: LogMan::Msg::A("Unhandled EXTR size: %d", OpSize);
  }
}

DEF_OP(LDiv) {
  auto Op = IROp->C<IR::IROp_LDiv>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
    case 2: {
      uxth(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()));
      bfi(TMP1.W(), GetReg<RA_32>(Op->Header.Args[1].ID()), 16, 16);
      sxth(TMP2.W(), GetReg<RA_32>(Op->Header.Args[2].ID()));
      sdiv(GetReg<RA_32>(Node), TMP1.W(), TMP2.W());
    break;
    }
    case 4: {
      mov(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      bfi(TMP1, GetReg<RA_64>(Op->Header.Args[1].ID()), 32, 32);
      sxtw(TMP2, GetReg<RA_32>(Op->Header.Args[2].ID()));
      sdiv(GetReg<RA_64>(Node), TMP1, TMP2);
    break;
    }
    case 8: {
      uint64_t SPOffset = AlignUp((RA64.size() + 1) * 8, 16);

      sub(sp, sp, SPOffset);
      int i = 0;
      for (auto RA : RA64) {
        str(RA, MemOperand(sp, i * 8));
        i++;
      }
      str(lr,       MemOperand(sp, RA64.size() * 8 + 0 * 8));

      mov(x0, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mov(x1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      mov(x2, GetReg<RA_64>(Op->Header.Args[2].ID()));

#if _M_X86_64
      CallRuntime(LDIV);
#else
      LoadConstant(x3, reinterpret_cast<uint64_t>(LDIV));
      blr(x3);
#endif

      // Result is now in x0
      // Fix the stack and any values that were stepped on
      i = 0;
      for (auto RA : RA64) {
        ldr(RA, MemOperand(sp, i * 8));
        i++;
      }

      // Move result to its destination register
      mov(GetReg<RA_64>(Node), x0);

      ldr(lr,       MemOperand(sp, RA64.size() * 8 + 0 * 8));

      add(sp, sp, SPOffset);
    break;
    }
    default: LogMan::Msg::A("Unknown LDIV Size: %d", Size); break;
  }
}

DEF_OP(LUDiv) {
  auto Op = IROp->C<IR::IROp_LUDiv>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
    case 2: {
      uxth(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()));
      bfi(TMP1.W(), GetReg<RA_32>(Op->Header.Args[1].ID()), 16, 16);
      udiv(GetReg<RA_32>(Node), TMP1.W(), GetReg<RA_32>(Op->Header.Args[2].ID()));
    break;
    }
    case 4: {
      mov(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      bfi(TMP1, GetReg<RA_64>(Op->Header.Args[1].ID()), 32, 32);
      udiv(GetReg<RA_64>(Node), TMP1, GetReg<RA_64>(Op->Header.Args[2].ID()));
    break;
    }
    case 8: {
      uint64_t SPOffset = AlignUp((RA64.size() + 1) * 8, 16);

      sub(sp, sp, SPOffset);
      int i = 0;
      for (auto RA : RA64) {
        str(RA, MemOperand(sp, i * 8));
        i++;
      }
      str(lr,       MemOperand(sp, RA64.size() * 8 + 0 * 8));

      mov(x0, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mov(x1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      mov(x2, GetReg<RA_64>(Op->Header.Args[2].ID()));

#if _M_X86_64
      CallRuntime(LUDIV);
#else
      LoadConstant(x3, reinterpret_cast<uint64_t>(LUDIV));
      blr(x3);
#endif

      // Result is now in x0
      // Fix the stack and any values that were stepped on
      i = 0;
      for (auto RA : RA64) {
        ldr(RA, MemOperand(sp, i * 8));
        i++;
      }

      // Move result to its destination register
      mov(GetReg<RA_64>(Node), x0);

      ldr(lr,       MemOperand(sp, RA64.size() * 8 + 0 * 8));

      add(sp, sp, SPOffset);
    break;
    }
    default: LogMan::Msg::A("Unknown LUDIV Size: %d", Size); break;
  }
}

DEF_OP(LRem) {
  auto Op = IROp->C<IR::IROp_LRem>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  auto Size = OpSize;
  switch (Size) {
    case 2: {
      auto Divisor = GetReg<RA_32>(Op->Header.Args[2].ID());

      uxth(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()));
      bfi(TMP1.W(), GetReg<RA_32>(Op->Header.Args[1].ID()), 16, 16);
      sxth(w3, Divisor);
      sdiv(TMP2.W(), TMP1.W(), w3);

      msub(GetReg<RA_32>(Node), TMP2.W(), w3, TMP1.W());
    break;
    }
    case 4: {
      auto Divisor = GetReg<RA_64>(Op->Header.Args[2].ID());

      mov(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      bfi(TMP1, GetReg<RA_64>(Op->Header.Args[1].ID()), 32, 32);
      sxtw(x3, Divisor);
      sdiv(TMP2, TMP1, x3);

      msub(GetReg<RA_32>(Node), TMP2.W(), w3, TMP1.W());
    break;
    }
    case 8: {
              uint64_t SPOffset = AlignUp((RA64.size() + 1) * 8, 16);

      sub(sp, sp, SPOffset);
      int i = 0;
      for (auto RA : RA64) {
        str(RA, MemOperand(sp, i * 8));
        i++;
      }
      str(lr,       MemOperand(sp, RA64.size() * 8 + 0 * 8));

      mov(x0, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mov(x1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      mov(x2, GetReg<RA_64>(Op->Header.Args[2].ID()));

#if _M_X86_64
      CallRuntime(LREM);
#else
      LoadConstant(x3, reinterpret_cast<uint64_t>(LREM));
      blr(x3);
#endif

      // Result is now in x0
      // Fix the stack and any values that were stepped on
      i = 0;
      for (auto RA : RA64) {
        ldr(RA, MemOperand(sp, i * 8));
        i++;
      }

      // Move result to its destination register
      mov(GetReg<RA_64>(Node), x0);

      ldr(lr,       MemOperand(sp, RA64.size() * 8 + 0 * 8));

      add(sp, sp, SPOffset);
    break;
    }
    default: LogMan::Msg::A("Unknown LREM Size: %d", Size); break;
  }
}

DEF_OP(LURem) {
  auto Op = IROp->C<IR::IROp_LURem>();
  uint8_t OpSize = IROp->Size;
  // Each source is OpSize in size
  // So you can have up to a 128bit divide from x86-64
  switch (OpSize) {
    case 2: {
      auto Divisor = GetReg<RA_32>(Op->Header.Args[2].ID());

      uxth(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()));
      bfi(TMP1.W(), GetReg<RA_32>(Op->Header.Args[1].ID()), 16, 16);
      udiv(TMP2.W(), TMP1.W(), Divisor);
      msub(GetReg<RA_32>(Node), TMP2.W(), Divisor, TMP1.W());
    break;
    }
    case 4: {
      auto Divisor = GetReg<RA_64>(Op->Header.Args[2].ID());

      mov(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      bfi(TMP1, GetReg<RA_64>(Op->Header.Args[1].ID()), 32, 32);
      udiv(TMP2, TMP1, Divisor);

      msub(GetReg<RA_64>(Node), TMP2, Divisor, TMP1);
    break;
    }
    case 8: {
      uint64_t SPOffset = AlignUp((RA64.size() + 1) * 8, 16);

      sub(sp, sp, SPOffset);
      int i = 0;
      for (auto RA : RA64) {
        str(RA, MemOperand(sp, i * 8));
        i++;
      }
      str(lr,       MemOperand(sp, RA64.size() * 8 + 0 * 8));

      mov(x0, GetReg<RA_64>(Op->Header.Args[1].ID()));
      mov(x1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      mov(x2, GetReg<RA_64>(Op->Header.Args[2].ID()));

#if _M_X86_64
      CallRuntime(LUREM);
#else
      LoadConstant(x3, reinterpret_cast<uint64_t>(LUREM));
      blr(x3);
#endif

      // Result is now in x0
      // Fix the stack and any values that were stepped on
      i = 0;
      for (auto RA : RA64) {
        ldr(RA, MemOperand(sp, i * 8));
        i++;
      }

      // Move result to its destination register
      mov(GetReg<RA_64>(Node), x0);

      ldr(lr,       MemOperand(sp, RA64.size() * 8 + 0 * 8));

      add(sp, sp, SPOffset);
    break;
    }
    default: LogMan::Msg::A("Unknown LUREM Size: %d", OpSize); break;
  }
}

DEF_OP(Sext) {
  auto Op = IROp->C<IR::IROp_Sext>();
  LogMan::Throw::A(Op->SrcSize <= 64, "Can't support Zext of size: %ld", Op->SrcSize);
  auto Dst = GetReg<RA_64>(Node);

  switch (Op->SrcSize / 8) {
    case 1:
      sxtb(Dst, GetReg<RA_64>(Op->Header.Args[0].ID()));
      break;
    case 2:
      sxth(Dst, GetReg<RA_64>(Op->Header.Args[0].ID()));
      break;
    case 4:
      sxtw(Dst, GetReg<RA_64>(Op->Header.Args[0].ID()));
      break;
    case 8:
      mov(Dst, GetReg<RA_64>(Op->Header.Args[0].ID()));
      break;
    default: LogMan::Msg::A("Unknown Sext size: %d", Op->SrcSize / 8);
  }
}

DEF_OP(Not) {
  auto Op = IROp->C<IR::IROp_Not>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 4:
      mvn(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()));
      break;
    case 8:
      mvn(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()));
      break;
    default: LogMan::Msg::A("Unsupported Not size: %d", OpSize);
  }
}

DEF_OP(Popcount) {
  auto Op = IROp->C<IR::IROp_Popcount>();
  auto Dst = GetReg<RA_64>(Node);
  fmov(VTMP1.D(), GetReg<RA_64>(Op->Header.Args[0].ID()));
  cnt(VTMP1.V8B(), VTMP1.V8B());
  addv(VTMP1.B(), VTMP1.V8B());
  umov(Dst.W(), VTMP1.B(), 0);
}

DEF_OP(FindLSB) {
  auto Op = IROp->C<IR::IROp_FindLSB>();
  uint8_t OpSize = IROp->Size;
  auto Dst = GetReg<RA_64>(Node);
  auto Src = GetReg<RA_64>(Op->Header.Args[0].ID());
  if (OpSize != 8) {
    ubfx(TMP1, Src, 0, OpSize * 8);
    cmp(TMP1, 0);
    rbit(TMP1, TMP1);
    clz(Dst, TMP1);
    csinv(Dst, Dst, xzr, ne);
  }
  else {
    rbit(TMP1, Src);
    cmp(Src, 0);
    clz(Dst, TMP1);
    csinv(Dst, Dst, xzr, ne);
  }
}

DEF_OP(FindMSB) {
  auto Op = IROp->C<IR::IROp_FindMSB>();
  uint8_t OpSize = IROp->Size;
  auto Dst = GetReg<RA_64>(Node);
  switch (OpSize) {
    case 2:
      movz(TMP1, OpSize * 8 - 1);
      lsl(Dst.W(), GetReg<RA_32>(Op->Header.Args[0].ID()), 16);
      orr(Dst.W(), Dst.W(), 0x8000);
      clz(Dst.W(), Dst.W());
      sub(Dst, TMP1, Dst);
    break;
    case 4:
      movz(TMP1, OpSize * 8 - 1);
      clz(Dst.W(), GetReg<RA_32>(Op->Header.Args[0].ID()));
      sub(Dst, TMP1, Dst);
      break;
    case 8:
      movz(TMP1, OpSize * 8 - 1);
      clz(Dst, GetReg<RA_64>(Op->Header.Args[0].ID()));
      sub(Dst, TMP1, Dst);
      break;
    default: LogMan::Msg::A("Unknown REV size: %d", OpSize); break;
  }
}

DEF_OP(FindTrailingZeros) {
  auto Op = IROp->C<IR::IROp_FindTrailingZeros>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 2:
      rbit(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()));
      orr(GetReg<RA_32>(Node), GetReg<RA_32>(Node), 0x8000);
      clz(GetReg<RA_32>(Node), GetReg<RA_32>(Node));
    break;
    case 4:
      rbit(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()));
      clz(GetReg<RA_32>(Node), GetReg<RA_32>(Node));
      break;
    case 8:
      rbit(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()));
      clz(GetReg<RA_64>(Node), GetReg<RA_64>(Node));
      break;
    default: LogMan::Msg::A("Unknown size: %d", OpSize); break;
  }
}

DEF_OP(CountLeadingZeroes) {
  auto Op = IROp->C<IR::IROp_CountLeadingZeroes>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 2:
      lsl(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()), 16);
      orr(GetReg<RA_32>(Node), GetReg<RA_32>(Node), 0x8000);
      clz(GetReg<RA_32>(Node), GetReg<RA_32>(Node));
    break;
    case 4:
      clz(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()));
      break;
    case 8:
      clz(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()));
      break;
    default: LogMan::Msg::A("Unknown size: %d", OpSize); break;
  }
}

DEF_OP(Rev) {
  auto Op = IROp->C<IR::IROp_Rev>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 2:
      rev(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()));
      lsr(GetReg<RA_32>(Node), GetReg<RA_32>(Node), 16);
    break;
    case 4:
      rev(GetReg<RA_32>(Node), GetReg<RA_32>(Op->Header.Args[0].ID()));
      break;
    case 8:
      rev(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[0].ID()));
      break;
    default: LogMan::Msg::A("Unknown REV size: %d", OpSize); break;
  }
}

DEF_OP(Bfi) {
  auto Op = IROp->C<IR::IROp_Bfi>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 1:
    case 2:
    case 4: {
      auto Dst = GetReg<RA_32>(Node);
      mov(TMP1.W(), GetReg<RA_32>(Op->Header.Args[0].ID()));
      bfi(TMP1.W(), GetReg<RA_32>(Op->Header.Args[1].ID()), Op->lsb, Op->Width);
      ubfx(Dst, TMP1.W(), 0, OpSize * 8);
      break;
    }
    case 8:
      mov(TMP1, GetReg<RA_64>(Op->Header.Args[0].ID()));
      bfi(TMP1, GetReg<RA_64>(Op->Header.Args[1].ID()), Op->lsb, Op->Width);
      mov(GetReg<RA_64>(Node), TMP1);
      break;
    default: LogMan::Msg::A("Unknown BFI size: %d", OpSize); break;
  }
}

DEF_OP(Bfe) {
  auto Op = IROp->C<IR::IROp_Bfe>();
  uint8_t OpSize = IROp->Size;
  LogMan::Throw::A(OpSize <= 8, "OpSize is too large for BFE: %d", OpSize);
  LogMan::Throw::A(Op->Width != 0, "Invalid BFE width of 0");

  auto Dst = GetReg<RA_64>(Node);
  ubfx(Dst, GetReg<RA_64>(Op->Header.Args[0].ID()), Op->lsb, Op->Width);
}

DEF_OP(Sbfe) {
  auto Op = IROp->C<IR::IROp_Bfe>();
  uint8_t OpSize = IROp->Size;

  auto Dst = GetReg<RA_64>(Node);
  if (OpSize == 8) {
    sbfx(Dst, GetReg<RA_64>(Op->Header.Args[0].ID()), Op->lsb, Op->Width);
  } else {
    LogMan::Msg::D("Unimplemented Sbfe size");
  }
}

DEF_OP(Select) {
  auto Op = IROp->C<IR::IROp_Select>();

  cmp(GetReg<RA_64>(Op->Header.Args[0].ID()), GetReg<RA_64>(Op->Header.Args[1].ID()));

  switch (Op->Cond.Val) {
  case FEXCore::IR::COND_EQ:
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[2].ID()), GetReg<RA_64>(Op->Header.Args[3].ID()), Condition::eq);
  break;
  case FEXCore::IR::COND_NEQ:
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[2].ID()), GetReg<RA_64>(Op->Header.Args[3].ID()), Condition::ne);
  break;
  case FEXCore::IR::COND_SGE:
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[2].ID()), GetReg<RA_64>(Op->Header.Args[3].ID()), Condition::ge);
  break;
  case FEXCore::IR::COND_SLT:
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[2].ID()), GetReg<RA_64>(Op->Header.Args[3].ID()), Condition::lt);
  break;
  case FEXCore::IR::COND_SGT:
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[2].ID()), GetReg<RA_64>(Op->Header.Args[3].ID()), Condition::gt);
  break;
  case FEXCore::IR::COND_SLE:
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[2].ID()), GetReg<RA_64>(Op->Header.Args[3].ID()), Condition::le);
  break;
  case FEXCore::IR::COND_UGE:
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[2].ID()), GetReg<RA_64>(Op->Header.Args[3].ID()), Condition::cs);
  break;
  case FEXCore::IR::COND_ULT:
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[2].ID()), GetReg<RA_64>(Op->Header.Args[3].ID()), Condition::cc);
  break;
  case FEXCore::IR::COND_UGT:
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[2].ID()), GetReg<RA_64>(Op->Header.Args[3].ID()), Condition::hi);
  break;
  case FEXCore::IR::COND_ULE:
    csel(GetReg<RA_64>(Node), GetReg<RA_64>(Op->Header.Args[2].ID()), GetReg<RA_64>(Op->Header.Args[3].ID()), Condition::ls);
  break;
  case FEXCore::IR::COND_MI:
  case FEXCore::IR::COND_PL:
  case FEXCore::IR::COND_VS:
  case FEXCore::IR::COND_VC:
  default:
  LogMan::Msg::A("Unsupported compare type");
  break;
  }
}

DEF_OP(VExtractToGPR) {
  auto Op = IROp->C<IR::IROp_VExtractToGPR>();
  uint8_t OpSize = IROp->Size;
  switch (OpSize) {
    case 1:
      umov(GetReg<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()).V16B(), Op->Idx);
    break;
    case 2:
      umov(GetReg<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()).V8H(), Op->Idx);
    break;
    case 4:
      umov(GetReg<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()).V4S(), Op->Idx);
    break;
    case 8:
      umov(GetReg<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()).V2D(), Op->Idx);
    break;
    default:  LogMan::Msg::A("Unhandled ExtractElementSize: %d", OpSize);
  }
}

DEF_OP(Float_ToGPR_ZU) {
  LogMan::Msg::D("Unimplemented");
}

DEF_OP(Float_ToGPR_ZS) {
  auto Op = IROp->C<IR::IROp_Float_ToGPR_ZS>();
  if (Op->Header.ElementSize == 8) {
    fcvtzs(GetReg<RA_64>(Node), GetSrc(Op->Header.Args[0].ID()).D());
  }
  else {
    fcvtzs(GetReg<RA_32>(Node), GetSrc(Op->Header.Args[0].ID()).S());
  }
}

DEF_OP(Float_ToGPR_U) {
  LogMan::Msg::D("Unimplemented");
}

DEF_OP(Float_ToGPR_S) {
  auto Op = IROp->C<IR::IROp_Float_ToGPR_S>();
  if (Op->Header.ElementSize == 8) {
    frinti(VTMP1.D(), GetSrc(Op->Header.Args[0].ID()).D());
    fcvtzs(GetReg<RA_64>(Node), VTMP1.D());
  }
  else {
    frinti(VTMP1.S(), GetSrc(Op->Header.Args[0].ID()).S());
    fcvtzs(GetReg<RA_32>(Node), VTMP1.S());
  }
}

DEF_OP(FCmp) {
  auto Op = IROp->C<IR::IROp_FCmp>();

  if (Op->ElementSize == 4) {
    fcmp(GetSrc(Op->Header.Args[0].ID()).S(), GetSrc(Op->Header.Args[1].ID()).S());
  }
  else {
    fcmp(GetSrc(Op->Header.Args[0].ID()).D(), GetSrc(Op->Header.Args[1].ID()).D());
  }
  auto Dst = GetReg<RA_64>(Node);
  eor(Dst, Dst, Dst);

  if (Op->Flags & (1 << IR::FCMP_FLAG_LT)) {
    cset(TMP2, Condition::mi);
    lsl(TMP2, TMP2, IR::FCMP_FLAG_LT);
    orr(Dst, Dst, TMP2);
  }
  if (Op->Flags & (1 << IR::FCMP_FLAG_EQ)) {
    cset(TMP2, Condition::eq);
    lsl(TMP2, TMP2, IR::FCMP_FLAG_EQ);
    orr(Dst, Dst, TMP2);
  }
  if (Op->Flags & (1 << IR::FCMP_FLAG_UNORDERED)) {
    cset(TMP2, Condition::vs);
    lsl(TMP2, TMP2, IR::FCMP_FLAG_UNORDERED);
    orr(Dst, Dst, TMP2);
  }
}

DEF_OP(F80Cmp) {
  LogMan::Msg::D("Unimplemented");
}

#undef DEF_OP

void JITCore::RegisterALUHandlers() {
#define REGISTER_OP(op, x) OpHandlers[FEXCore::IR::IROps::OP_##op] = &JITCore::Op_##x
  REGISTER_OP(TRUNCELEMENTPAIR,  TruncElementPair);
  REGISTER_OP(CONSTANT,          Constant);
  REGISTER_OP(CYCLECOUNTER,      CycleCounter);
  REGISTER_OP(ADD,               Add);
  REGISTER_OP(SUB,               Sub);
  REGISTER_OP(NEG,               Neg);
  REGISTER_OP(MUL,               Mul);
  REGISTER_OP(UMUL,              UMul);
  REGISTER_OP(DIV,               Div);
  REGISTER_OP(UDIV,              UDiv);
  REGISTER_OP(REM,               Rem);
  REGISTER_OP(UREM,              URem);
  REGISTER_OP(MULH,              MulH);
  REGISTER_OP(UMULH,             UMulH);
  REGISTER_OP(OR,                Or);
  REGISTER_OP(AND,               And);
  REGISTER_OP(XOR,               Xor);
  REGISTER_OP(LSHL,              Lshl);
  REGISTER_OP(LSHR,              Lshr);
  REGISTER_OP(ASHR,              Ashr);
  REGISTER_OP(ROL,               Rol);
  REGISTER_OP(ROR,               Ror);
  REGISTER_OP(EXTR,              Extr);
  REGISTER_OP(LDIV,              LDiv);
  REGISTER_OP(LUDIV,             LUDiv);
  REGISTER_OP(LREM,              LRem);
  REGISTER_OP(LUREM,             LURem);
  REGISTER_OP(SEXT,              Sext);
  REGISTER_OP(NOT,               Not);
  REGISTER_OP(POPCOUNT,          Popcount);
  REGISTER_OP(FINDLSB,           FindLSB);
  REGISTER_OP(FINDMSB,           FindMSB);
  REGISTER_OP(FINDTRAILINGZEROS, FindTrailingZeros);
  REGISTER_OP(COUNTLEADINGZEROES, CountLeadingZeroes);
  REGISTER_OP(REV,               Rev);
  REGISTER_OP(BFI,               Bfi);
  REGISTER_OP(BFE,               Bfe);
  REGISTER_OP(SBFE,              Sbfe);
  REGISTER_OP(SELECT,            Select);
  REGISTER_OP(VEXTRACTTOGPR,     VExtractToGPR);
  REGISTER_OP(FLOAT_TOGPR_ZU,    Float_ToGPR_ZU);
  REGISTER_OP(FLOAT_TOGPR_ZS,    Float_ToGPR_ZS);
  REGISTER_OP(FLOAT_TOGPR_U,     Float_ToGPR_U);
  REGISTER_OP(FLOAT_TOGPR_S,     Float_ToGPR_S);
  REGISTER_OP(FCMP,              FCmp);
  REGISTER_OP(F80CMP,            F80Cmp);

#undef REGISTER_OP
}

}
