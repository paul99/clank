/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#ifndef NACL_TRUSTED_BUT_NOT_TCB
#error This file is not meant for use in the TCB
#endif


#include "gtest/gtest.h"
#include "native_client/src/trusted/validator_arm/actual_vs_baseline.h"
#include "native_client/src/trusted/validator_arm/baseline_vs_baseline.h"
#include "native_client/src/trusted/validator_arm/actual_classes.h"
#include "native_client/src/trusted/validator_arm/baseline_classes.h"
#include "native_client/src/trusted/validator_arm/inst_classes_testers.h"
#include "native_client/src/trusted/validator_arm/arm_helpers.h"
#include "native_client/src/trusted/validator_arm/gen/arm32_decode_named_bases.h"

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


// op1(24:20)=11000 & op2(7:5)=000 & Rd(15:12)=~1111
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
//       generated_baseline: USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class Binary4RegisterDualOpTesterCase0
    : public Binary4RegisterDualOpTester {
 public:
  Binary4RegisterDualOpTesterCase0(const NamedClassDecoder& decoder)
    : Binary4RegisterDualOpTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Binary4RegisterDualOpTesterCase0
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~11000
  if ((inst.Bits() & 0x01F00000)  !=
          0x01800000) return false;
  // op2(7:5)=~000
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000000) return false;
  // Rd(15:12)=1111
  if ((inst.Bits() & 0x0000F000)  ==
          0x0000F000) return false;

  // Check other preconditions defined for the base decoder.
  return Binary4RegisterDualOpTester::
      PassesParsePreconditions(inst, decoder);
}

bool Binary4RegisterDualOpTesterCase0
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Binary4RegisterDualOpTester::
               ApplySanityChecks(inst, decoder));

  // safety: Ra  ==
  //          Pc => DECODER_ERROR
  EXPECT_TRUE(((((inst.Bits() & 0x0000F000) >> 12)) != (15)));

  // safety: Pc in {Rd, Rn, Rm} => UNPREDICTABLE
  EXPECT_TRUE(!((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8))))));

  // defs: {Rd};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))));

  return true;
}

// op1(24:20)=11000 & op2(7:5)=000 & Rd(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Binary3RegisterOpAltA,
//       baseline: Binary3RegisterOpAltA,
//       constraints: ,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       generated_baseline: USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class Binary3RegisterOpAltATesterCase1
    : public Binary3RegisterOpAltATester {
 public:
  Binary3RegisterOpAltATesterCase1(const NamedClassDecoder& decoder)
    : Binary3RegisterOpAltATester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Binary3RegisterOpAltATesterCase1
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~11000
  if ((inst.Bits() & 0x01F00000)  !=
          0x01800000) return false;
  // op2(7:5)=~000
  if ((inst.Bits() & 0x000000E0)  !=
          0x00000000) return false;
  // Rd(15:12)=~1111
  if ((inst.Bits() & 0x0000F000)  !=
          0x0000F000) return false;

  // Check other preconditions defined for the base decoder.
  return Binary3RegisterOpAltATester::
      PassesParsePreconditions(inst, decoder);
}

bool Binary3RegisterOpAltATesterCase1
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Binary3RegisterOpAltATester::
               ApplySanityChecks(inst, decoder));

  // safety: Pc in {Rd, Rn, Rm} => UNPREDICTABLE
  EXPECT_TRUE(!((((15) == (((inst.Bits() & 0x000F0000) >> 16)))) ||
       (((15) == ((inst.Bits() & 0x0000000F)))) ||
       (((15) == (((inst.Bits() & 0x00000F00) >> 8))))));

  // defs: {Rd};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().
   Add(Register(((inst.Bits() & 0x000F0000) >> 16)))));

  return true;
}

// op1(24:20)=11111 & op2(7:5)=111
//    = {actual: PermanentlyUndefined,
//       baseline: PermanentlyUndefined,
//       constraints: }
class PermanentlyUndefinedTesterCase2
    : public PermanentlyUndefinedTester {
 public:
  PermanentlyUndefinedTesterCase2(const NamedClassDecoder& decoder)
    : PermanentlyUndefinedTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
};

bool PermanentlyUndefinedTesterCase2
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~11111
  if ((inst.Bits() & 0x01F00000)  !=
          0x01F00000) return false;
  // op2(7:5)=~111
  if ((inst.Bits() & 0x000000E0)  !=
          0x000000E0) return false;

  // Check other preconditions defined for the base decoder.
  return PermanentlyUndefinedTester::
      PassesParsePreconditions(inst, decoder);
}

// op1(24:20)=1101x & op2(7:5)=x10
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Binary2RegisterBitRangeNotRnIsPcBitfieldExtract,
//       baseline: Binary2RegisterBitRangeNotRnIsPcBitfieldExtract,
//       constraints: ,
//       defs: {Rd},
//       fields: [widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       generated_baseline: SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0,
//       lsb: lsb(11:7),
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//         lsb + widthm1  >
//               31 => UNPREDICTABLE],
//       uses: {Rn},
//       widthm1: widthm1(20:16)}
class Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTesterCase3
    : public Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester {
 public:
  Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTesterCase3(const NamedClassDecoder& decoder)
    : Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTesterCase3
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1101x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01A00000) return false;
  // op2(7:5)=~x10
  if ((inst.Bits() & 0x00000060)  !=
          0x00000040) return false;

  // Check other preconditions defined for the base decoder.
  return Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester::
      PassesParsePreconditions(inst, decoder);
}

bool Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTesterCase3
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester::
               ApplySanityChecks(inst, decoder));

  // safety: Pc in {Rd, Rn} => UNPREDICTABLE
  EXPECT_TRUE(!((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F))))));

  // safety: lsb + widthm1  >
  //          31 => UNPREDICTABLE
  EXPECT_TRUE(((((inst.Bits() & 0x00000F80) >> 7) + ((inst.Bits() & 0x001F0000) >> 16)) <= (31)));

  // defs: {Rd};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)))));

  return true;
}

// op1(24:20)=1110x & op2(7:5)=x00 & Rn(3:0)=~1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Binary2RegisterBitRangeMsbGeLsb,
//       baseline: Binary2RegisterBitRangeMsbGeLsb,
//       constraints: ,
//       defs: {Rd},
//       fields: [msb(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       generated_baseline: BFI_cccc0111110mmmmmddddlllll001nnnn_case_0,
//       lsb: lsb(11:7),
//       msb: msb(20:16),
//       safety: [Rn  ==
//               Pc => DECODER_ERROR,
//         Rd  ==
//               Pc => UNPREDICTABLE,
//         msb  <
//               lsb => UNPREDICTABLE],
//       uses: {Rn, Rd}}
class Binary2RegisterBitRangeMsbGeLsbTesterCase4
    : public Binary2RegisterBitRangeMsbGeLsbTester {
 public:
  Binary2RegisterBitRangeMsbGeLsbTesterCase4(const NamedClassDecoder& decoder)
    : Binary2RegisterBitRangeMsbGeLsbTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Binary2RegisterBitRangeMsbGeLsbTesterCase4
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1110x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01C00000) return false;
  // op2(7:5)=~x00
  if ((inst.Bits() & 0x00000060)  !=
          0x00000000) return false;
  // Rn(3:0)=1111
  if ((inst.Bits() & 0x0000000F)  ==
          0x0000000F) return false;

  // Check other preconditions defined for the base decoder.
  return Binary2RegisterBitRangeMsbGeLsbTester::
      PassesParsePreconditions(inst, decoder);
}

bool Binary2RegisterBitRangeMsbGeLsbTesterCase4
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Binary2RegisterBitRangeMsbGeLsbTester::
               ApplySanityChecks(inst, decoder));

  // safety: Rn  ==
  //          Pc => DECODER_ERROR
  EXPECT_TRUE((((inst.Bits() & 0x0000000F)) != (15)));

  // safety: Rd  ==
  //          Pc => UNPREDICTABLE
  EXPECT_TRUE(((((inst.Bits() & 0x0000F000) >> 12)) != (15)));

  // safety: msb  <
  //          lsb => UNPREDICTABLE
  EXPECT_TRUE(((((inst.Bits() & 0x001F0000) >> 16)) >= (((inst.Bits() & 0x00000F80) >> 7))));

  // defs: {Rd};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)))));

  return true;
}

// op1(24:20)=1110x & op2(7:5)=x00 & Rn(3:0)=1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       actual: Unary1RegisterBitRangeMsbGeLsb,
//       baseline: Unary1RegisterBitRangeMsbGeLsb,
//       constraints: ,
//       defs: {Rd},
//       fields: [msb(20:16), Rd(15:12), lsb(11:7)],
//       generated_baseline: BFC_cccc0111110mmmmmddddlllll0011111_case_0,
//       lsb: lsb(11:7),
//       msb: msb(20:16),
//       safety: [Rd  ==
//               Pc => UNPREDICTABLE,
//         msb  <
//               lsb => UNPREDICTABLE],
//       uses: {Rd}}
class Unary1RegisterBitRangeMsbGeLsbTesterCase5
    : public Unary1RegisterBitRangeMsbGeLsbTester {
 public:
  Unary1RegisterBitRangeMsbGeLsbTesterCase5(const NamedClassDecoder& decoder)
    : Unary1RegisterBitRangeMsbGeLsbTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Unary1RegisterBitRangeMsbGeLsbTesterCase5
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1110x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01C00000) return false;
  // op2(7:5)=~x00
  if ((inst.Bits() & 0x00000060)  !=
          0x00000000) return false;
  // Rn(3:0)=~1111
  if ((inst.Bits() & 0x0000000F)  !=
          0x0000000F) return false;

  // Check other preconditions defined for the base decoder.
  return Unary1RegisterBitRangeMsbGeLsbTester::
      PassesParsePreconditions(inst, decoder);
}

bool Unary1RegisterBitRangeMsbGeLsbTesterCase5
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Unary1RegisterBitRangeMsbGeLsbTester::
               ApplySanityChecks(inst, decoder));

  // safety: Rd  ==
  //          Pc => UNPREDICTABLE
  EXPECT_TRUE(((((inst.Bits() & 0x0000F000) >> 12)) != (15)));

  // safety: msb  <
  //          lsb => UNPREDICTABLE
  EXPECT_TRUE(((((inst.Bits() & 0x001F0000) >> 16)) >= (((inst.Bits() & 0x00000F80) >> 7))));

  // defs: {Rd};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)))));

  return true;
}

// op1(24:20)=1111x & op2(7:5)=x10
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Binary2RegisterBitRangeNotRnIsPcBitfieldExtract,
//       baseline: Binary2RegisterBitRangeNotRnIsPcBitfieldExtract,
//       constraints: ,
//       defs: {Rd},
//       fields: [widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       generated_baseline: UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0,
//       lsb: lsb(11:7),
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//         lsb + widthm1  >
//               31 => UNPREDICTABLE],
//       uses: {Rn},
//       widthm1: widthm1(20:16)}
class Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTesterCase6
    : public Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester {
 public:
  Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTesterCase6(const NamedClassDecoder& decoder)
    : Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester(decoder) {}
  virtual bool PassesParsePreconditions(
      nacl_arm_dec::Instruction inst,
      const NamedClassDecoder& decoder);
  virtual bool ApplySanityChecks(nacl_arm_dec::Instruction inst,
                                 const NamedClassDecoder& decoder);
};

bool Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTesterCase6
::PassesParsePreconditions(
     nacl_arm_dec::Instruction inst,
     const NamedClassDecoder& decoder) {

  // Check that row patterns apply to pattern being checked.'
  // op1(24:20)=~1111x
  if ((inst.Bits() & 0x01E00000)  !=
          0x01E00000) return false;
  // op2(7:5)=~x10
  if ((inst.Bits() & 0x00000060)  !=
          0x00000040) return false;

  // Check other preconditions defined for the base decoder.
  return Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester::
      PassesParsePreconditions(inst, decoder);
}

bool Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTesterCase6
::ApplySanityChecks(nacl_arm_dec::Instruction inst,
                    const NamedClassDecoder& decoder) {
  NC_PRECOND(Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester::
               ApplySanityChecks(inst, decoder));

  // safety: Pc in {Rd, Rn} => UNPREDICTABLE
  EXPECT_TRUE(!((((15) == (((inst.Bits() & 0x0000F000) >> 12)))) ||
       (((15) == ((inst.Bits() & 0x0000000F))))));

  // safety: lsb + widthm1  >
  //          31 => UNPREDICTABLE
  EXPECT_TRUE(((((inst.Bits() & 0x00000F80) >> 7) + ((inst.Bits() & 0x001F0000) >> 16)) <= (31)));

  // defs: {Rd};
  EXPECT_TRUE(decoder.defs(inst).IsSame(RegisterList().
   Add(Register(((inst.Bits() & 0x0000F000) >> 12)))));

  return true;
}

// The following are derived class decoder testers for decoder actions
// associated with a pattern of an action. These derived classes introduce
// a default constructor that automatically initializes the expected decoder
// to the corresponding instance in the generated DecoderState.

// op1(24:20)=11000 & op2(7:5)=000 & Rd(15:12)=~1111
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
//       generated_baseline: USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0,
//       rule: USADA8,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
class Binary4RegisterDualOpTester_Case0
    : public Binary4RegisterDualOpTesterCase0 {
 public:
  Binary4RegisterDualOpTester_Case0()
    : Binary4RegisterDualOpTesterCase0(
      state_.Binary4RegisterDualOp_USADA8_instance_)
  {}
};

// op1(24:20)=11000 & op2(7:5)=000 & Rd(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Binary3RegisterOpAltA,
//       baseline: Binary3RegisterOpAltA,
//       constraints: ,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       generated_baseline: USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0,
//       rule: USAD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
class Binary3RegisterOpAltATester_Case1
    : public Binary3RegisterOpAltATesterCase1 {
 public:
  Binary3RegisterOpAltATester_Case1()
    : Binary3RegisterOpAltATesterCase1(
      state_.Binary3RegisterOpAltA_USAD8_instance_)
  {}
};

// op1(24:20)=11111 & op2(7:5)=111
//    = {actual: PermanentlyUndefined,
//       baseline: PermanentlyUndefined,
//       constraints: ,
//       rule: UDF}
class PermanentlyUndefinedTester_Case2
    : public PermanentlyUndefinedTesterCase2 {
 public:
  PermanentlyUndefinedTester_Case2()
    : PermanentlyUndefinedTesterCase2(
      state_.PermanentlyUndefined_UDF_instance_)
  {}
};

// op1(24:20)=1101x & op2(7:5)=x10
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Binary2RegisterBitRangeNotRnIsPcBitfieldExtract,
//       baseline: Binary2RegisterBitRangeNotRnIsPcBitfieldExtract,
//       constraints: ,
//       defs: {Rd},
//       fields: [widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       generated_baseline: SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0,
//       lsb: lsb(11:7),
//       rule: SBFX,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//         lsb + widthm1  >
//               31 => UNPREDICTABLE],
//       uses: {Rn},
//       widthm1: widthm1(20:16)}
class Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester_Case3
    : public Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTesterCase3 {
 public:
  Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester_Case3()
    : Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTesterCase3(
      state_.Binary2RegisterBitRangeNotRnIsPcBitfieldExtract_SBFX_instance_)
  {}
};

// op1(24:20)=1110x & op2(7:5)=x00 & Rn(3:0)=~1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Binary2RegisterBitRangeMsbGeLsb,
//       baseline: Binary2RegisterBitRangeMsbGeLsb,
//       constraints: ,
//       defs: {Rd},
//       fields: [msb(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       generated_baseline: BFI_cccc0111110mmmmmddddlllll001nnnn_case_0,
//       lsb: lsb(11:7),
//       msb: msb(20:16),
//       rule: BFI,
//       safety: [Rn  ==
//               Pc => DECODER_ERROR,
//         Rd  ==
//               Pc => UNPREDICTABLE,
//         msb  <
//               lsb => UNPREDICTABLE],
//       uses: {Rn, Rd}}
class Binary2RegisterBitRangeMsbGeLsbTester_Case4
    : public Binary2RegisterBitRangeMsbGeLsbTesterCase4 {
 public:
  Binary2RegisterBitRangeMsbGeLsbTester_Case4()
    : Binary2RegisterBitRangeMsbGeLsbTesterCase4(
      state_.Binary2RegisterBitRangeMsbGeLsb_BFI_instance_)
  {}
};

// op1(24:20)=1110x & op2(7:5)=x00 & Rn(3:0)=1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       actual: Unary1RegisterBitRangeMsbGeLsb,
//       baseline: Unary1RegisterBitRangeMsbGeLsb,
//       constraints: ,
//       defs: {Rd},
//       fields: [msb(20:16), Rd(15:12), lsb(11:7)],
//       generated_baseline: BFC_cccc0111110mmmmmddddlllll0011111_case_0,
//       lsb: lsb(11:7),
//       msb: msb(20:16),
//       rule: BFC,
//       safety: [Rd  ==
//               Pc => UNPREDICTABLE,
//         msb  <
//               lsb => UNPREDICTABLE],
//       uses: {Rd}}
class Unary1RegisterBitRangeMsbGeLsbTester_Case5
    : public Unary1RegisterBitRangeMsbGeLsbTesterCase5 {
 public:
  Unary1RegisterBitRangeMsbGeLsbTester_Case5()
    : Unary1RegisterBitRangeMsbGeLsbTesterCase5(
      state_.Unary1RegisterBitRangeMsbGeLsb_BFC_instance_)
  {}
};

// op1(24:20)=1111x & op2(7:5)=x10
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Binary2RegisterBitRangeNotRnIsPcBitfieldExtract,
//       baseline: Binary2RegisterBitRangeNotRnIsPcBitfieldExtract,
//       constraints: ,
//       defs: {Rd},
//       fields: [widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       generated_baseline: UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0,
//       lsb: lsb(11:7),
//       rule: UBFX,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//         lsb + widthm1  >
//               31 => UNPREDICTABLE],
//       uses: {Rn},
//       widthm1: widthm1(20:16)}
class Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester_Case6
    : public Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTesterCase6 {
 public:
  Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester_Case6()
    : Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTesterCase6(
      state_.Binary2RegisterBitRangeNotRnIsPcBitfieldExtract_UBFX_instance_)
  {}
};

// Defines a gtest testing harness for tests.
class Arm32DecoderStateTests : public ::testing::Test {
 protected:
  Arm32DecoderStateTests() {}
};

// The following functions test each pattern specified in parse
// decoder tables.

// op1(24:20)=11000 & op2(7:5)=000 & Rd(15:12)=~1111
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
//       generated_baseline: USADA8_cccc01111000ddddaaaammmm0001nnnn_case_0,
//       pattern: cccc01111000ddddaaaammmm0001nnnn,
//       rule: USADA8,
//       safety: [Ra  ==
//               Pc => DECODER_ERROR,
//         Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm, Ra}}
TEST_F(Arm32DecoderStateTests,
       Binary4RegisterDualOpTester_Case0_TestCase0) {
  Binary4RegisterDualOpTester_Case0 tester;
  tester.Test("cccc01111000ddddaaaammmm0001nnnn");
}

// op1(24:20)=11000 & op2(7:5)=000 & Rd(15:12)=1111
//    = {Pc: 15,
//       Rd: Rd(19:16),
//       Rm: Rm(11:8),
//       Rn: Rn(3:0),
//       actual: Binary3RegisterOpAltA,
//       baseline: Binary3RegisterOpAltA,
//       constraints: ,
//       defs: {Rd},
//       fields: [Rd(19:16), Rm(11:8), Rn(3:0)],
//       generated_baseline: USAD8_cccc01111000dddd1111mmmm0001nnnn_case_0,
//       pattern: cccc01111000dddd1111mmmm0001nnnn,
//       rule: USAD8,
//       safety: [Pc in {Rd, Rn, Rm} => UNPREDICTABLE],
//       uses: {Rn, Rm}}
TEST_F(Arm32DecoderStateTests,
       Binary3RegisterOpAltATester_Case1_TestCase1) {
  Binary3RegisterOpAltATester_Case1 tester;
  tester.Test("cccc01111000dddd1111mmmm0001nnnn");
}

// op1(24:20)=11111 & op2(7:5)=111
//    = {actual: PermanentlyUndefined,
//       baseline: PermanentlyUndefined,
//       constraints: ,
//       pattern: cccc01111111iiiiiiiiiiii1111iiii,
//       rule: UDF}
TEST_F(Arm32DecoderStateTests,
       PermanentlyUndefinedTester_Case2_TestCase2) {
  PermanentlyUndefinedTester_Case2 tester;
  tester.Test("cccc01111111iiiiiiiiiiii1111iiii");
}

// op1(24:20)=1101x & op2(7:5)=x10
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Binary2RegisterBitRangeNotRnIsPcBitfieldExtract,
//       baseline: Binary2RegisterBitRangeNotRnIsPcBitfieldExtract,
//       constraints: ,
//       defs: {Rd},
//       fields: [widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       generated_baseline: SBFX_cccc0111101wwwwwddddlllll101nnnn_case_0,
//       lsb: lsb(11:7),
//       pattern: cccc0111101wwwwwddddlllll101nnnn,
//       rule: SBFX,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//         lsb + widthm1  >
//               31 => UNPREDICTABLE],
//       uses: {Rn},
//       widthm1: widthm1(20:16)}
TEST_F(Arm32DecoderStateTests,
       Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester_Case3_TestCase3) {
  Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester_Case3 tester;
  tester.Test("cccc0111101wwwwwddddlllll101nnnn");
}

// op1(24:20)=1110x & op2(7:5)=x00 & Rn(3:0)=~1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Binary2RegisterBitRangeMsbGeLsb,
//       baseline: Binary2RegisterBitRangeMsbGeLsb,
//       constraints: ,
//       defs: {Rd},
//       fields: [msb(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       generated_baseline: BFI_cccc0111110mmmmmddddlllll001nnnn_case_0,
//       lsb: lsb(11:7),
//       msb: msb(20:16),
//       pattern: cccc0111110mmmmmddddlllll001nnnn,
//       rule: BFI,
//       safety: [Rn  ==
//               Pc => DECODER_ERROR,
//         Rd  ==
//               Pc => UNPREDICTABLE,
//         msb  <
//               lsb => UNPREDICTABLE],
//       uses: {Rn, Rd}}
TEST_F(Arm32DecoderStateTests,
       Binary2RegisterBitRangeMsbGeLsbTester_Case4_TestCase4) {
  Binary2RegisterBitRangeMsbGeLsbTester_Case4 tester;
  tester.Test("cccc0111110mmmmmddddlllll001nnnn");
}

// op1(24:20)=1110x & op2(7:5)=x00 & Rn(3:0)=1111
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       actual: Unary1RegisterBitRangeMsbGeLsb,
//       baseline: Unary1RegisterBitRangeMsbGeLsb,
//       constraints: ,
//       defs: {Rd},
//       fields: [msb(20:16), Rd(15:12), lsb(11:7)],
//       generated_baseline: BFC_cccc0111110mmmmmddddlllll0011111_case_0,
//       lsb: lsb(11:7),
//       msb: msb(20:16),
//       pattern: cccc0111110mmmmmddddlllll0011111,
//       rule: BFC,
//       safety: [Rd  ==
//               Pc => UNPREDICTABLE,
//         msb  <
//               lsb => UNPREDICTABLE],
//       uses: {Rd}}
TEST_F(Arm32DecoderStateTests,
       Unary1RegisterBitRangeMsbGeLsbTester_Case5_TestCase5) {
  Unary1RegisterBitRangeMsbGeLsbTester_Case5 tester;
  tester.Test("cccc0111110mmmmmddddlllll0011111");
}

// op1(24:20)=1111x & op2(7:5)=x10
//    = {Pc: 15,
//       Rd: Rd(15:12),
//       Rn: Rn(3:0),
//       actual: Binary2RegisterBitRangeNotRnIsPcBitfieldExtract,
//       baseline: Binary2RegisterBitRangeNotRnIsPcBitfieldExtract,
//       constraints: ,
//       defs: {Rd},
//       fields: [widthm1(20:16), Rd(15:12), lsb(11:7), Rn(3:0)],
//       generated_baseline: UBFX_cccc0111111mmmmmddddlllll101nnnn_case_0,
//       lsb: lsb(11:7),
//       pattern: cccc0111111mmmmmddddlllll101nnnn,
//       rule: UBFX,
//       safety: [Pc in {Rd, Rn} => UNPREDICTABLE,
//         lsb + widthm1  >
//               31 => UNPREDICTABLE],
//       uses: {Rn},
//       widthm1: widthm1(20:16)}
TEST_F(Arm32DecoderStateTests,
       Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester_Case6_TestCase6) {
  Binary2RegisterBitRangeNotRnIsPcBitfieldExtractTester_Case6 tester;
  tester.Test("cccc0111111mmmmmddddlllll101nnnn");
}

}  // namespace nacl_arm_test

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
