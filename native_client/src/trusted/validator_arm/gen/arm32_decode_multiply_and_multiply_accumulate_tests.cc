/*
 * Copyright 2012 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error This file is not meant for use in the TCB
#endif


#include "gtest/gtest.h"
#include "native_client/src/trusted/validator_arm/actual_vs_baseline.h"
#include "native_client/src/trusted/validator_arm/actual_classes.h"
#include "native_client/src/trusted/validator_arm/baseline_classes.h"
#include "native_client/src/trusted/validator_arm/inst_classes_testers.h"

using nacl_arm_dec::Instruction;
using nacl_arm_dec::ClassDecoder;
using nacl_arm_dec::Register;
using nacl_arm_dec::RegisterList;

namespace nacl_arm_test {

// The following classes are derived class decoder testers that
// add row pattern constraints and decoder restrictions to each tester.
// This is done so that it can be used to make sure that the
// corresponding pattern is not tested for cases that would be excluded
//  due to row checks, or restrictions specified by the row restrictions.


// Neutral case:
// inst(23:20)=0100
//    = {baseline: 'Binary4RegisterDualResult',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16)},
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=0100
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       baseline: Binary4RegisterDualResult,
//       constraints: ,
//       defs: {RdLo,RdHi},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE]}
class Binary4RegisterDualResultTesterCase0
    : public Binary4RegisterDualResultTester {
 public:
  Binary4RegisterDualResultTesterCase0(const NamedClassDecoder& decoder)
    : Binary4RegisterDualResultTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Binary4RegisterDualResultTesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  if ((inst.Bits() & 0x00F00000) != 0x00400000 /* op(23:20)=~0100 */) return false;

  // Check other preconditions defined for the base decoder.
  return Binary4RegisterDualResultTester::
      PassesParsePreconditions(inst, decoder);
}

bool Binary4RegisterDualResultTesterCase0
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Binary4RegisterDualResultTester::ApplySanityChecks(inst, decoder));

  // safety: Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE
  EXPECT_TRUE(!((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) || (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) || (((15) == ((inst.Bits() & 0x0000000F)))) || (((15) == (((inst.Bits() & 0x00000F00) >> 8))))));

  // safety: RdHi == RdLo => UNPREDICTABLE
  EXPECT_TRUE(((((inst.Bits() & 0x000F0000) >> 16)) != (((inst.Bits() & 0x0000F000) >> 12))));

  // defs: {RdLo,RdHi};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().Add(Register(((inst.Bits() & 0x0000F000) >> 12))).Add(Register(((inst.Bits() & 0x000F0000) >> 16)))));

  return true;
}

// Neutral case:
// inst(23:20)=0101
//    = {baseline: 'UndefinedCondDecoder',
//       constraints: }
//
// Representaive case:
// op(23:20)=0101
//    = {baseline: UndefinedCondDecoder,
//       constraints: }
class UnsafeCondDecoderTesterCase1
    : public UnsafeCondDecoderTester {
 public:
  UnsafeCondDecoderTesterCase1(const NamedClassDecoder& decoder)
    : UnsafeCondDecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UnsafeCondDecoderTesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  if ((inst.Bits() & 0x00F00000) != 0x00500000 /* op(23:20)=~0101 */) return false;

  // Check other preconditions defined for the base decoder.
  return UnsafeCondDecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// Neutral case:
// inst(23:20)=0110
//    = {baseline: 'Binary4RegisterDualOp',
//       constraints: ,
//       defs: {inst(19:16)},
//       safety: ['15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) || 15 == inst(15:12) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=0110
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       baseline: Binary4RegisterDualOp,
//       constraints: ,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       safety: [Pc in {Rd,Rn,Rm,Ra} => UNPREDICTABLE]}
class Binary4RegisterDualOpTesterCase2
    : public Binary4RegisterDualOpTester {
 public:
  Binary4RegisterDualOpTesterCase2(const NamedClassDecoder& decoder)
    : Binary4RegisterDualOpTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Binary4RegisterDualOpTesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  if ((inst.Bits() & 0x00F00000) != 0x00600000 /* op(23:20)=~0110 */) return false;

  // Check other preconditions defined for the base decoder.
  return Binary4RegisterDualOpTester::
      PassesParsePreconditions(inst, decoder);
}

bool Binary4RegisterDualOpTesterCase2
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Binary4RegisterDualOpTester::ApplySanityChecks(inst, decoder));

  // safety: Pc in {Rd,Rn,Rm,Ra} => UNPREDICTABLE
  EXPECT_TRUE(!((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) || (((15) == ((inst.Bits() & 0x0000000F)))) || (((15) == (((inst.Bits() & 0x00000F00) >> 8)))) || (((15) == (((inst.Bits() & 0x0000F000) >> 12))))));

  // defs: {Rd};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().Add(Register(((inst.Bits() & 0x000F0000) >> 16)))));

  return true;
}

// Neutral case:
// inst(23:20)=0111
//    = {baseline: 'UndefinedCondDecoder',
//       constraints: }
//
// Representaive case:
// op(23:20)=0111
//    = {baseline: UndefinedCondDecoder,
//       constraints: }
class UnsafeCondDecoderTesterCase3
    : public UnsafeCondDecoderTester {
 public:
  UnsafeCondDecoderTesterCase3(const NamedClassDecoder& decoder)
    : UnsafeCondDecoderTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool UnsafeCondDecoderTesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  if ((inst.Bits() & 0x00F00000) != 0x00700000 /* op(23:20)=~0111 */) return false;

  // Check other preconditions defined for the base decoder.
  return UnsafeCondDecoderTester::
      PassesParsePreconditions(inst, decoder);
}

// Neutral case:
// inst(23:20)=000x & inst(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {baseline: 'Binary3RegisterOpAltA',
//       constraints: ,
//       defs: {inst(19:16),16 if inst(20)=1 else 32},
//       safety: ['15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', '(ArchVersion() < 6 && inst(19:16) == inst(3:0)) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=000x & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       baseline: Binary3RegisterOpAltA,
//       constraints: ,
//       defs: {Rd,NZCV if setflags else None},
//       fields: [S(20), Rd(19:16), Rm(11:8), Rn(3:0)],
//       safety: [Pc in {Rd,Rn,Rm} => UNPREDICTABLE, (ArchVersion() < 6 && Rd == Rn) => UNPREDICTABLE],
//       setflags: S(20)=1}
class Binary3RegisterOpAltATesterCase4
    : public Binary3RegisterOpAltATester {
 public:
  Binary3RegisterOpAltATesterCase4(const NamedClassDecoder& decoder)
    : Binary3RegisterOpAltATester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Binary3RegisterOpAltATesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  if ((inst.Bits() & 0x00E00000) != 0x00000000 /* op(23:20)=~000x */) return false;
  if ((inst.Bits() & 0x0000F000) != 0x00000000 /* $pattern(31:0)=~xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx */) return false;

  // Check other preconditions defined for the base decoder.
  return Binary3RegisterOpAltATester::
      PassesParsePreconditions(inst, decoder);
}

bool Binary3RegisterOpAltATesterCase4
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Binary3RegisterOpAltATester::ApplySanityChecks(inst, decoder));

  // safety: Pc in {Rd,Rn,Rm} => UNPREDICTABLE
  EXPECT_TRUE(!((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) || (((15) == ((inst.Bits() & 0x0000000F)))) || (((15) == (((inst.Bits() & 0x00000F00) >> 8))))));

  // safety: (ArchVersion() < 6 && Rd == Rn) => UNPREDICTABLE
  EXPECT_TRUE((!((((nacl_arm_dec::ArchVersion()) < (6))) && (((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))))));

  // defs: {Rd,NZCV if setflags else None};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().Add(Register(((inst.Bits() & 0x000F0000) >> 16))).Add(Register(((inst.Bits() & 0x00100000) == 0x00100000 ? 16 : 32)))));

  return true;
}

// Neutral case:
// inst(23:20)=001x
//    = {baseline: 'Binary4RegisterDualOpLtV6RdNotRn',
//       constraints: ,
//       defs: {inst(19:16),16 if inst(20)=1 else 32},
//       safety: ['15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) || 15 == inst(15:12) => UNPREDICTABLE', '(ArchVersion() < 6 && inst(19:16) == inst(3:0)) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=001x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       baseline: Binary4RegisterDualOpLtV6RdNotRn,
//       constraints: ,
//       defs: {Rd,NZCV if setflags else None},
//       fields: [S(20), Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       safety: [Pc in {Rd,Rn,Rm,Ra} => UNPREDICTABLE, (ArchVersion() < 6 && Rd == Rn) => UNPREDICTABLE],
//       setflags: S(20)=1}
class Binary4RegisterDualOpTesterCase5
    : public Binary4RegisterDualOpTester {
 public:
  Binary4RegisterDualOpTesterCase5(const NamedClassDecoder& decoder)
    : Binary4RegisterDualOpTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Binary4RegisterDualOpTesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  if ((inst.Bits() & 0x00E00000) != 0x00200000 /* op(23:20)=~001x */) return false;

  // Check other preconditions defined for the base decoder.
  return Binary4RegisterDualOpTester::
      PassesParsePreconditions(inst, decoder);
}

bool Binary4RegisterDualOpTesterCase5
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Binary4RegisterDualOpTester::ApplySanityChecks(inst, decoder));

  // safety: Pc in {Rd,Rn,Rm,Ra} => UNPREDICTABLE
  EXPECT_TRUE(!((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) || (((15) == ((inst.Bits() & 0x0000000F)))) || (((15) == (((inst.Bits() & 0x00000F00) >> 8)))) || (((15) == (((inst.Bits() & 0x0000F000) >> 12))))));

  // safety: (ArchVersion() < 6 && Rd == Rn) => UNPREDICTABLE
  EXPECT_TRUE((!((((nacl_arm_dec::ArchVersion()) < (6))) && (((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))))));

  // defs: {Rd,NZCV if setflags else None};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().Add(Register(((inst.Bits() & 0x000F0000) >> 16))).Add(Register(((inst.Bits() & 0x00100000) == 0x00100000 ? 16 : 32)))));

  return true;
}

// Neutral case:
// inst(23:20)=100x
//    = {baseline: 'Binary4RegisterDualResultUsesRnRm',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16),16 if inst(20)=1 else 32},
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE', '(ArchVersion() < 6 && (inst(19:16) == inst(3:0) || inst(15:12) == inst(3:0))) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=100x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       baseline: Binary4RegisterDualResultUsesRnRm,
//       constraints: ,
//       defs: {RdLo,RdHi,NZCV if setflags else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE, (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1}
class Binary4RegisterDualResultTesterCase6
    : public Binary4RegisterDualResultTester {
 public:
  Binary4RegisterDualResultTesterCase6(const NamedClassDecoder& decoder)
    : Binary4RegisterDualResultTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Binary4RegisterDualResultTesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  if ((inst.Bits() & 0x00E00000) != 0x00800000 /* op(23:20)=~100x */) return false;

  // Check other preconditions defined for the base decoder.
  return Binary4RegisterDualResultTester::
      PassesParsePreconditions(inst, decoder);
}

bool Binary4RegisterDualResultTesterCase6
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Binary4RegisterDualResultTester::ApplySanityChecks(inst, decoder));

  // safety: Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE
  EXPECT_TRUE(!((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) || (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) || (((15) == ((inst.Bits() & 0x0000000F)))) || (((15) == (((inst.Bits() & 0x00000F00) >> 8))))));

  // safety: RdHi == RdLo => UNPREDICTABLE
  EXPECT_TRUE(((((inst.Bits() & 0x000F0000) >> 16)) != (((inst.Bits() & 0x0000F000) >> 12))));

  // safety: (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE
  EXPECT_TRUE((!((((nacl_arm_dec::ArchVersion()) < (6))) && (((((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))) || (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F)))))))));

  // defs: {RdLo,RdHi,NZCV if setflags else None};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().Add(Register(((inst.Bits() & 0x0000F000) >> 12))).Add(Register(((inst.Bits() & 0x000F0000) >> 16))).Add(Register(((inst.Bits() & 0x00100000) == 0x00100000 ? 16 : 32)))));

  return true;
}

// Neutral case:
// inst(23:20)=101x
//    = {baseline: 'Binary4RegisterDualResultLtV6RdHiLoNotRn',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16),16 if inst(20)=1 else 32},
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE', '(ArchVersion() < 6 && (inst(19:16) == inst(3:0) || inst(15:12) == inst(3:0))) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=101x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       baseline: Binary4RegisterDualResultLtV6RdHiLoNotRn,
//       constraints: ,
//       defs: {RdLo,RdHi,NZCV if setflags else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE, (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1}
class Binary4RegisterDualResultTesterCase7
    : public Binary4RegisterDualResultTester {
 public:
  Binary4RegisterDualResultTesterCase7(const NamedClassDecoder& decoder)
    : Binary4RegisterDualResultTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Binary4RegisterDualResultTesterCase7
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  if ((inst.Bits() & 0x00E00000) != 0x00A00000 /* op(23:20)=~101x */) return false;

  // Check other preconditions defined for the base decoder.
  return Binary4RegisterDualResultTester::
      PassesParsePreconditions(inst, decoder);
}

bool Binary4RegisterDualResultTesterCase7
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Binary4RegisterDualResultTester::ApplySanityChecks(inst, decoder));

  // safety: Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE
  EXPECT_TRUE(!((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) || (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) || (((15) == ((inst.Bits() & 0x0000000F)))) || (((15) == (((inst.Bits() & 0x00000F00) >> 8))))));

  // safety: RdHi == RdLo => UNPREDICTABLE
  EXPECT_TRUE(((((inst.Bits() & 0x000F0000) >> 16)) != (((inst.Bits() & 0x0000F000) >> 12))));

  // safety: (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE
  EXPECT_TRUE((!((((nacl_arm_dec::ArchVersion()) < (6))) && (((((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))) || (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F)))))))));

  // defs: {RdLo,RdHi,NZCV if setflags else None};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().Add(Register(((inst.Bits() & 0x0000F000) >> 12))).Add(Register(((inst.Bits() & 0x000F0000) >> 16))).Add(Register(((inst.Bits() & 0x00100000) == 0x00100000 ? 16 : 32)))));

  return true;
}

// Neutral case:
// inst(23:20)=110x
//    = {baseline: 'Binary4RegisterDualResultUsesRnRm',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16),16 if inst(20)=1 else 32},
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE', '(ArchVersion() < 6 && (inst(19:16) == inst(3:0) || inst(15:12) == inst(3:0))) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=110x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       baseline: Binary4RegisterDualResultUsesRnRm,
//       constraints: ,
//       defs: {RdLo,RdHi,NZCV if setflags else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE, (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1}
class Binary4RegisterDualResultTesterCase8
    : public Binary4RegisterDualResultTester {
 public:
  Binary4RegisterDualResultTesterCase8(const NamedClassDecoder& decoder)
    : Binary4RegisterDualResultTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Binary4RegisterDualResultTesterCase8
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  if ((inst.Bits() & 0x00E00000) != 0x00C00000 /* op(23:20)=~110x */) return false;

  // Check other preconditions defined for the base decoder.
  return Binary4RegisterDualResultTester::
      PassesParsePreconditions(inst, decoder);
}

bool Binary4RegisterDualResultTesterCase8
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Binary4RegisterDualResultTester::ApplySanityChecks(inst, decoder));

  // safety: Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE
  EXPECT_TRUE(!((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) || (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) || (((15) == ((inst.Bits() & 0x0000000F)))) || (((15) == (((inst.Bits() & 0x00000F00) >> 8))))));

  // safety: RdHi == RdLo => UNPREDICTABLE
  EXPECT_TRUE(((((inst.Bits() & 0x000F0000) >> 16)) != (((inst.Bits() & 0x0000F000) >> 12))));

  // safety: (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE
  EXPECT_TRUE((!((((nacl_arm_dec::ArchVersion()) < (6))) && (((((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))) || (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F)))))))));

  // defs: {RdLo,RdHi,NZCV if setflags else None};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().Add(Register(((inst.Bits() & 0x0000F000) >> 12))).Add(Register(((inst.Bits() & 0x000F0000) >> 16))).Add(Register(((inst.Bits() & 0x00100000) == 0x00100000 ? 16 : 32)))));

  return true;
}

// Neutral case:
// inst(23:20)=111x
//    = {baseline: 'Binary4RegisterDualResultLtV6RdHiLoNotRn',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16),16 if inst(20)=1 else 32},
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE', '(ArchVersion() < 6 && (inst(19:16) == inst(3:0) || inst(15:12) == inst(3:0))) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=111x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       baseline: Binary4RegisterDualResultLtV6RdHiLoNotRn,
//       constraints: ,
//       defs: {RdLo,RdHi,NZCV if setflags else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE, (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1}
class Binary4RegisterDualResultTesterCase9
    : public Binary4RegisterDualResultTester {
 public:
  Binary4RegisterDualResultTesterCase9(const NamedClassDecoder& decoder)
    : Binary4RegisterDualResultTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Binary4RegisterDualResultTesterCase9
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  if ((inst.Bits() & 0x00E00000) != 0x00E00000 /* op(23:20)=~111x */) return false;

  // Check other preconditions defined for the base decoder.
  return Binary4RegisterDualResultTester::
      PassesParsePreconditions(inst, decoder);
}

bool Binary4RegisterDualResultTesterCase9
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Binary4RegisterDualResultTester::ApplySanityChecks(inst, decoder));

  // safety: Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE
  EXPECT_TRUE(!((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) || (((15) == (((inst.Bits() & 0x000F0000) >> 16)))) || (((15) == ((inst.Bits() & 0x0000000F)))) || (((15) == (((inst.Bits() & 0x00000F00) >> 8))))));

  // safety: RdHi == RdLo => UNPREDICTABLE
  EXPECT_TRUE(((((inst.Bits() & 0x000F0000) >> 16)) != (((inst.Bits() & 0x0000F000) >> 12))));

  // safety: (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE
  EXPECT_TRUE((!((((nacl_arm_dec::ArchVersion()) < (6))) && (((((((inst.Bits() & 0x000F0000) >> 16)) == ((inst.Bits() & 0x0000000F)))) || (((((inst.Bits() & 0x0000F000) >> 12)) == ((inst.Bits() & 0x0000000F)))))))));

  // defs: {RdLo,RdHi,NZCV if setflags else None};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().Add(Register(((inst.Bits() & 0x0000F000) >> 12))).Add(Register(((inst.Bits() & 0x000F0000) >> 16))).Add(Register(((inst.Bits() & 0x00100000) == 0x00100000 ? 16 : 32)))));

  return true;
}

// The following are derived class decoder testers for decoder actions
// associated with a pattern of an action. These derived classes introduce
// a default constructor that automatically initializes the expected decoder
// to the corresponding instance in the generated DecoderState.

// Neutral case:
// inst(23:20)=0100
//    = {baseline: 'Binary4RegisterDualResult',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16)},
//       rule: 'UMAAL_A1',
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE']}
//
// Representative case:
// op(23:20)=0100
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       baseline: Binary4RegisterDualResult,
//       constraints: ,
//       defs: {RdLo,RdHi},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       rule: UMAAL_A1,
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE]}
class Binary4RegisterDualResultTester_Case0
    : public Binary4RegisterDualResultTesterCase0 {
 public:
  Binary4RegisterDualResultTester_Case0()
    : Binary4RegisterDualResultTesterCase0(
      state_.Binary4RegisterDualResult_UMAAL_A1_instance_)
  {}
};

// Neutral case:
// inst(23:20)=0101
//    = {baseline: 'UndefinedCondDecoder',
//       constraints: }
//
// Representative case:
// op(23:20)=0101
//    = {baseline: UndefinedCondDecoder,
//       constraints: }
class UndefinedCondDecoderTester_Case1
    : public UnsafeCondDecoderTesterCase1 {
 public:
  UndefinedCondDecoderTester_Case1()
    : UnsafeCondDecoderTesterCase1(
      state_.UndefinedCondDecoder_None_instance_)
  {}
};

// Neutral case:
// inst(23:20)=0110
//    = {baseline: 'Binary4RegisterDualOp',
//       constraints: ,
//       defs: {inst(19:16)},
//       rule: 'MLS_A1',
//       safety: ['15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) || 15 == inst(15:12) => UNPREDICTABLE']}
//
// Representative case:
// op(23:20)=0110
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       baseline: Binary4RegisterDualOp,
//       constraints: ,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       rule: MLS_A1,
//       safety: [Pc in {Rd,Rn,Rm,Ra} => UNPREDICTABLE]}
class Binary4RegisterDualOpTester_Case2
    : public Binary4RegisterDualOpTesterCase2 {
 public:
  Binary4RegisterDualOpTester_Case2()
    : Binary4RegisterDualOpTesterCase2(
      state_.Binary4RegisterDualOp_MLS_A1_instance_)
  {}
};

// Neutral case:
// inst(23:20)=0111
//    = {baseline: 'UndefinedCondDecoder',
//       constraints: }
//
// Representative case:
// op(23:20)=0111
//    = {baseline: UndefinedCondDecoder,
//       constraints: }
class UndefinedCondDecoderTester_Case3
    : public UnsafeCondDecoderTesterCase3 {
 public:
  UndefinedCondDecoderTester_Case3()
    : UnsafeCondDecoderTesterCase3(
      state_.UndefinedCondDecoder_None_instance_)
  {}
};

// Neutral case:
// inst(23:20)=000x & inst(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {baseline: 'Binary3RegisterOpAltA',
//       constraints: ,
//       defs: {inst(19:16),16 if inst(20)=1 else 32},
//       rule: 'MUL_A1',
//       safety: ['15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', '(ArchVersion() < 6 && inst(19:16) == inst(3:0)) => UNPREDICTABLE']}
//
// Representative case:
// op(23:20)=000x & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       baseline: Binary3RegisterOpAltA,
//       constraints: ,
//       defs: {Rd,NZCV if setflags else None},
//       fields: [S(20), Rd(19:16), Rm(11:8), Rn(3:0)],
//       rule: MUL_A1,
//       safety: [Pc in {Rd,Rn,Rm} => UNPREDICTABLE, (ArchVersion() < 6 && Rd == Rn) => UNPREDICTABLE],
//       setflags: S(20)=1}
class Binary3RegisterOpAltATester_Case4
    : public Binary3RegisterOpAltATesterCase4 {
 public:
  Binary3RegisterOpAltATester_Case4()
    : Binary3RegisterOpAltATesterCase4(
      state_.Binary3RegisterOpAltA_MUL_A1_instance_)
  {}
};

// Neutral case:
// inst(23:20)=001x
//    = {baseline: 'Binary4RegisterDualOpLtV6RdNotRn',
//       constraints: ,
//       defs: {inst(19:16),16 if inst(20)=1 else 32},
//       rule: 'MLA_A1',
//       safety: ['15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) || 15 == inst(15:12) => UNPREDICTABLE', '(ArchVersion() < 6 && inst(19:16) == inst(3:0)) => UNPREDICTABLE']}
//
// Representative case:
// op(23:20)=001x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       baseline: Binary4RegisterDualOpLtV6RdNotRn,
//       constraints: ,
//       defs: {Rd,NZCV if setflags else None},
//       fields: [S(20), Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       rule: MLA_A1,
//       safety: [Pc in {Rd,Rn,Rm,Ra} => UNPREDICTABLE, (ArchVersion() < 6 && Rd == Rn) => UNPREDICTABLE],
//       setflags: S(20)=1}
class Binary4RegisterDualOpLtV6RdNotRnTester_Case5
    : public Binary4RegisterDualOpTesterCase5 {
 public:
  Binary4RegisterDualOpLtV6RdNotRnTester_Case5()
    : Binary4RegisterDualOpTesterCase5(
      state_.Binary4RegisterDualOpLtV6RdNotRn_MLA_A1_instance_)
  {}
};

// Neutral case:
// inst(23:20)=100x
//    = {baseline: 'Binary4RegisterDualResultUsesRnRm',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16),16 if inst(20)=1 else 32},
//       rule: 'UMULL_A1',
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE', '(ArchVersion() < 6 && (inst(19:16) == inst(3:0) || inst(15:12) == inst(3:0))) => UNPREDICTABLE']}
//
// Representative case:
// op(23:20)=100x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       baseline: Binary4RegisterDualResultUsesRnRm,
//       constraints: ,
//       defs: {RdLo,RdHi,NZCV if setflags else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       rule: UMULL_A1,
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE, (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1}
class Binary4RegisterDualResultUsesRnRmTester_Case6
    : public Binary4RegisterDualResultTesterCase6 {
 public:
  Binary4RegisterDualResultUsesRnRmTester_Case6()
    : Binary4RegisterDualResultTesterCase6(
      state_.Binary4RegisterDualResultUsesRnRm_UMULL_A1_instance_)
  {}
};

// Neutral case:
// inst(23:20)=101x
//    = {baseline: 'Binary4RegisterDualResultLtV6RdHiLoNotRn',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16),16 if inst(20)=1 else 32},
//       rule: 'UMLAL_A1',
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE', '(ArchVersion() < 6 && (inst(19:16) == inst(3:0) || inst(15:12) == inst(3:0))) => UNPREDICTABLE']}
//
// Representative case:
// op(23:20)=101x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       baseline: Binary4RegisterDualResultLtV6RdHiLoNotRn,
//       constraints: ,
//       defs: {RdLo,RdHi,NZCV if setflags else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       rule: UMLAL_A1,
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE, (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1}
class Binary4RegisterDualResultLtV6RdHiLoNotRnTester_Case7
    : public Binary4RegisterDualResultTesterCase7 {
 public:
  Binary4RegisterDualResultLtV6RdHiLoNotRnTester_Case7()
    : Binary4RegisterDualResultTesterCase7(
      state_.Binary4RegisterDualResultLtV6RdHiLoNotRn_UMLAL_A1_instance_)
  {}
};

// Neutral case:
// inst(23:20)=110x
//    = {baseline: 'Binary4RegisterDualResultUsesRnRm',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16),16 if inst(20)=1 else 32},
//       rule: 'SMULL_A1',
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE', '(ArchVersion() < 6 && (inst(19:16) == inst(3:0) || inst(15:12) == inst(3:0))) => UNPREDICTABLE']}
//
// Representative case:
// op(23:20)=110x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       baseline: Binary4RegisterDualResultUsesRnRm,
//       constraints: ,
//       defs: {RdLo,RdHi,NZCV if setflags else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       rule: SMULL_A1,
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE, (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1}
class Binary4RegisterDualResultUsesRnRmTester_Case8
    : public Binary4RegisterDualResultTesterCase8 {
 public:
  Binary4RegisterDualResultUsesRnRmTester_Case8()
    : Binary4RegisterDualResultTesterCase8(
      state_.Binary4RegisterDualResultUsesRnRm_SMULL_A1_instance_)
  {}
};

// Neutral case:
// inst(23:20)=111x
//    = {baseline: 'Binary4RegisterDualResultLtV6RdHiLoNotRn',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16),16 if inst(20)=1 else 32},
//       rule: 'SMLAL_A1',
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE', '(ArchVersion() < 6 && (inst(19:16) == inst(3:0) || inst(15:12) == inst(3:0))) => UNPREDICTABLE']}
//
// Representative case:
// op(23:20)=111x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       baseline: Binary4RegisterDualResultLtV6RdHiLoNotRn,
//       constraints: ,
//       defs: {RdLo,RdHi,NZCV if setflags else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       rule: SMLAL_A1,
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE, (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1}
class Binary4RegisterDualResultLtV6RdHiLoNotRnTester_Case9
    : public Binary4RegisterDualResultTesterCase9 {
 public:
  Binary4RegisterDualResultLtV6RdHiLoNotRnTester_Case9()
    : Binary4RegisterDualResultTesterCase9(
      state_.Binary4RegisterDualResultLtV6RdHiLoNotRn_SMLAL_A1_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// Neutral case:
// inst(23:20)=0100
//    = {actual: 'Binary4RegisterDualResult',
//       baseline: 'Binary4RegisterDualResult',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16)},
//       pattern: 'cccc00000100hhhhllllmmmm1001nnnn',
//       rule: 'UMAAL_A1',
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=0100
//    = {Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Binary4RegisterDualResult,
//       baseline: Binary4RegisterDualResult,
//       constraints: ,
//       defs: {RdLo,RdHi},
//       fields: [RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00000100hhhhllllmmmm1001nnnn,
//       rule: UMAAL_A1,
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE]}
TEST_F(Arm32DecoderStateTests,
       Binary4RegisterDualResultTester_Case0_TestCase0) {
  Binary4RegisterDualResultTester_Case0 tester;
  tester.Test("cccc00000100hhhhllllmmmm1001nnnn");
}

// Neutral case:
// inst(23:20)=0101
//    = {actual: 'Undefined',
//       baseline: 'UndefinedCondDecoder',
//       constraints: ,
//       pattern: 'cccc00000101xxxxxxxxxxxx1001xxxx'}
//
// Representative case:
// op(23:20)=0101
//    = {actual: Undefined,
//       baseline: UndefinedCondDecoder,
//       constraints: ,
//       pattern: cccc00000101xxxxxxxxxxxx1001xxxx}
TEST_F(Arm32DecoderStateTests,
       UndefinedCondDecoderTester_Case1_TestCase1) {
  UndefinedCondDecoderTester_Case1 baseline_tester;
  NamedUndefined_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00000101xxxxxxxxxxxx1001xxxx");
}

// Neutral case:
// inst(23:20)=0110
//    = {actual: 'Binary4RegisterDualOp',
//       baseline: 'Binary4RegisterDualOp',
//       constraints: ,
//       defs: {inst(19:16)},
//       pattern: 'cccc00000110ddddaaaammmm1001nnnn',
//       rule: 'MLS_A1',
//       safety: ['15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) || 15 == inst(15:12) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=0110
//    = {Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Binary4RegisterDualOp,
//       baseline: Binary4RegisterDualOp,
//       constraints: ,
//       defs: {Rd},
//       fields: [Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc00000110ddddaaaammmm1001nnnn,
//       rule: MLS_A1,
//       safety: [Pc in {Rd,Rn,Rm,Ra} => UNPREDICTABLE]}
TEST_F(Arm32DecoderStateTests,
       Binary4RegisterDualOpTester_Case2_TestCase2) {
  Binary4RegisterDualOpTester_Case2 tester;
  tester.Test("cccc00000110ddddaaaammmm1001nnnn");
}

// Neutral case:
// inst(23:20)=0111
//    = {actual: 'Undefined',
//       baseline: 'UndefinedCondDecoder',
//       constraints: ,
//       pattern: 'cccc00000111xxxxxxxxxxxx1001xxxx'}
//
// Representative case:
// op(23:20)=0111
//    = {actual: Undefined,
//       baseline: UndefinedCondDecoder,
//       constraints: ,
//       pattern: cccc00000111xxxxxxxxxxxx1001xxxx}
TEST_F(Arm32DecoderStateTests,
       UndefinedCondDecoderTester_Case3_TestCase3) {
  UndefinedCondDecoderTester_Case3 baseline_tester;
  NamedUndefined_None actual;
  ActualVsBaselineTester a_vs_b_tester(actual, baseline_tester);
  a_vs_b_tester.Test("cccc00000111xxxxxxxxxxxx1001xxxx");
}

// Neutral case:
// inst(23:20)=000x & inst(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {actual: 'Binary3RegisterOpAltA',
//       baseline: 'Binary3RegisterOpAltA',
//       constraints: ,
//       defs: {inst(19:16),16 if inst(20)=1 else 32},
//       pattern: 'cccc0000000sdddd0000mmmm1001nnnn',
//       rule: 'MUL_A1',
//       safety: ['15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', '(ArchVersion() < 6 && inst(19:16) == inst(3:0)) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=000x & $pattern(31:0)=xxxxxxxxxxxxxxxx0000xxxxxxxxxxxx
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Binary3RegisterOpAltA,
//       baseline: Binary3RegisterOpAltA,
//       constraints: ,
//       defs: {Rd,NZCV if setflags else None},
//       fields: [S(20), Rd(19:16), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000000sdddd0000mmmm1001nnnn,
//       rule: MUL_A1,
//       safety: [Pc in {Rd,Rn,Rm} => UNPREDICTABLE, (ArchVersion() < 6 && Rd == Rn) => UNPREDICTABLE],
//       setflags: S(20)=1}
TEST_F(Arm32DecoderStateTests,
       Binary3RegisterOpAltATester_Case4_TestCase4) {
  Binary3RegisterOpAltATester_Case4 tester;
  tester.Test("cccc0000000sdddd0000mmmm1001nnnn");
}

// Neutral case:
// inst(23:20)=001x
//    = {actual: 'Binary4RegisterDualOpLtV6RdNotRn',
//       baseline: 'Binary4RegisterDualOpLtV6RdNotRn',
//       constraints: ,
//       defs: {inst(19:16),16 if inst(20)=1 else 32},
//       pattern: 'cccc0000001sddddaaaammmm1001nnnn',
//       rule: 'MLA_A1',
//       safety: ['15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) || 15 == inst(15:12) => UNPREDICTABLE', '(ArchVersion() < 6 && inst(19:16) == inst(3:0)) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=001x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       Ra: Ra(15:12),
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Binary4RegisterDualOpLtV6RdNotRn,
//       baseline: Binary4RegisterDualOpLtV6RdNotRn,
//       constraints: ,
//       defs: {Rd,NZCV if setflags else None},
//       fields: [S(20), Rd(19:16), Ra(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000001sddddaaaammmm1001nnnn,
//       rule: MLA_A1,
//       safety: [Pc in {Rd,Rn,Rm,Ra} => UNPREDICTABLE, (ArchVersion() < 6 && Rd == Rn) => UNPREDICTABLE],
//       setflags: S(20)=1}
TEST_F(Arm32DecoderStateTests,
       Binary4RegisterDualOpLtV6RdNotRnTester_Case5_TestCase5) {
  Binary4RegisterDualOpLtV6RdNotRnTester_Case5 tester;
  tester.Test("cccc0000001sddddaaaammmm1001nnnn");
}

// Neutral case:
// inst(23:20)=100x
//    = {actual: 'Binary4RegisterDualResultUsesRnRm',
//       baseline: 'Binary4RegisterDualResultUsesRnRm',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16),16 if inst(20)=1 else 32},
//       pattern: 'cccc0000100shhhhllllmmmm1001nnnn',
//       rule: 'UMULL_A1',
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE', '(ArchVersion() < 6 && (inst(19:16) == inst(3:0) || inst(15:12) == inst(3:0))) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=100x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Binary4RegisterDualResultUsesRnRm,
//       baseline: Binary4RegisterDualResultUsesRnRm,
//       constraints: ,
//       defs: {RdLo,RdHi,NZCV if setflags else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000100shhhhllllmmmm1001nnnn,
//       rule: UMULL_A1,
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE, (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1}
TEST_F(Arm32DecoderStateTests,
       Binary4RegisterDualResultUsesRnRmTester_Case6_TestCase6) {
  Binary4RegisterDualResultUsesRnRmTester_Case6 tester;
  tester.Test("cccc0000100shhhhllllmmmm1001nnnn");
}

// Neutral case:
// inst(23:20)=101x
//    = {actual: 'Binary4RegisterDualResultLtV6RdHiLoNotRn',
//       baseline: 'Binary4RegisterDualResultLtV6RdHiLoNotRn',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16),16 if inst(20)=1 else 32},
//       pattern: 'cccc0000101shhhhllllmmmm1001nnnn',
//       rule: 'UMLAL_A1',
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE', '(ArchVersion() < 6 && (inst(19:16) == inst(3:0) || inst(15:12) == inst(3:0))) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=101x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Binary4RegisterDualResultLtV6RdHiLoNotRn,
//       baseline: Binary4RegisterDualResultLtV6RdHiLoNotRn,
//       constraints: ,
//       defs: {RdLo,RdHi,NZCV if setflags else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000101shhhhllllmmmm1001nnnn,
//       rule: UMLAL_A1,
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE, (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1}
TEST_F(Arm32DecoderStateTests,
       Binary4RegisterDualResultLtV6RdHiLoNotRnTester_Case7_TestCase7) {
  Binary4RegisterDualResultLtV6RdHiLoNotRnTester_Case7 tester;
  tester.Test("cccc0000101shhhhllllmmmm1001nnnn");
}

// Neutral case:
// inst(23:20)=110x
//    = {actual: 'Binary4RegisterDualResultUsesRnRm',
//       baseline: 'Binary4RegisterDualResultUsesRnRm',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16),16 if inst(20)=1 else 32},
//       pattern: 'cccc0000110shhhhllllmmmm1001nnnn',
//       rule: 'SMULL_A1',
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE', '(ArchVersion() < 6 && (inst(19:16) == inst(3:0) || inst(15:12) == inst(3:0))) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=110x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Binary4RegisterDualResultUsesRnRm,
//       baseline: Binary4RegisterDualResultUsesRnRm,
//       constraints: ,
//       defs: {RdLo,RdHi,NZCV if setflags else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000110shhhhllllmmmm1001nnnn,
//       rule: SMULL_A1,
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE, (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1}
TEST_F(Arm32DecoderStateTests,
       Binary4RegisterDualResultUsesRnRmTester_Case8_TestCase8) {
  Binary4RegisterDualResultUsesRnRmTester_Case8 tester;
  tester.Test("cccc0000110shhhhllllmmmm1001nnnn");
}

// Neutral case:
// inst(23:20)=111x
//    = {actual: 'Binary4RegisterDualResultLtV6RdHiLoNotRn',
//       baseline: 'Binary4RegisterDualResultLtV6RdHiLoNotRn',
//       constraints: ,
//       defs: {inst(15:12),inst(19:16),16 if inst(20)=1 else 32},
//       pattern: 'cccc0000111shhhhllllmmmm1001nnnn',
//       rule: 'SMLAL_A1',
//       safety: ['15 == inst(15:12) || 15 == inst(19:16) || 15 == inst(3:0) || 15 == inst(11:8) => UNPREDICTABLE', 'inst(15:12) == inst(19:16) => UNPREDICTABLE', '(ArchVersion() < 6 && (inst(19:16) == inst(3:0) || inst(15:12) == inst(3:0))) => UNPREDICTABLE']}
//
// Representaive case:
// op(23:20)=111x
//    = {NZCV: 16,
//       None: 32,
//       Pc: 15,
//       RdHi: RdHi(19:16),
//       RdLo: RdLo(15:12),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       S: S(20),
//       actual: Binary4RegisterDualResultLtV6RdHiLoNotRn,
//       baseline: Binary4RegisterDualResultLtV6RdHiLoNotRn,
//       constraints: ,
//       defs: {RdLo,RdHi,NZCV if setflags else None},
//       fields: [S(20), RdHi(19:16), RdLo(15:12), Rm(11:8), Rn(3:0)],
//       pattern: cccc0000111shhhhllllmmmm1001nnnn,
//       rule: SMLAL_A1,
//       safety: [Pc in {RdLo,RdHi,Rn,Rm} => UNPREDICTABLE, RdHi == RdLo => UNPREDICTABLE, (ArchVersion() < 6 && (RdHi == Rn || RdLo == Rn)) => UNPREDICTABLE],
//       setflags: S(20)=1}
TEST_F(Arm32DecoderStateTests,
       Binary4RegisterDualResultLtV6RdHiLoNotRnTester_Case9_TestCase9) {
  Binary4RegisterDualResultLtV6RdHiLoNotRnTester_Case9 tester;
  tester.Test("cccc0000111shhhhllllmmmm1001nnnn");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
