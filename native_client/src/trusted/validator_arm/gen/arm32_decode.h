/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE


#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_H_

#include "native_client/src/trusted/validator_arm/decode.h"
#include "native_client/src/trusted/validator_arm/actual_classes.h"
#include "native_client/src/trusted/validator_arm/baseline_classes.h"
#include "native_client/src/trusted/validator_arm/gen/arm32_decode_actuals.h"

namespace nacl_arm_dec {

// Defines a decoder class selector for instructions.
class Arm32DecoderState : DecoderState {
 public:
   explicit Arm32DecoderState();

   // Parses the given instruction, returning the decoder to use.
   virtual const ClassDecoder& decode(const Instruction) const;

 private:

  // The following list of methods correspond to each decoder table,
  // and implements the pattern matching of the corresponding bit
  // patterns. After matching the corresponding bit patterns, they
  // either call other methods in this list (corresponding to another
  // decoder table), or they return the instance field that implements
  // the class decoder that should be used to decode the particular
  // instruction.

  inline const ClassDecoder& decode_ARMv7(
      const Instruction inst) const;

  inline const ClassDecoder& decode_advanced_simd_data_processing_instructions(
      const Instruction inst) const;

  inline const ClassDecoder& decode_advanced_simd_element_or_structure_load_store_instructions(
      const Instruction inst) const;

  inline const ClassDecoder& decode_branch_branch_with_link_and_block_data_transfer(
      const Instruction inst) const;

  inline const ClassDecoder& decode_coprocessor_instructions_and_supervisor_call(
      const Instruction inst) const;

  inline const ClassDecoder& decode_data_processing_and_miscellaneous_instructions(
      const Instruction inst) const;

  inline const ClassDecoder& decode_data_processing_immediate(
      const Instruction inst) const;

  inline const ClassDecoder& decode_data_processing_register(
      const Instruction inst) const;

  inline const ClassDecoder& decode_data_processing_register_shifted_register(
      const Instruction inst) const;

  inline const ClassDecoder& decode_extension_register_load_store_instructions(
      const Instruction inst) const;

  inline const ClassDecoder& decode_extra_load_store_instructions(
      const Instruction inst) const;

  inline const ClassDecoder& decode_floating_point_data_processing_instructions(
      const Instruction inst) const;

  inline const ClassDecoder& decode_halfword_multiply_and_multiply_accumulate(
      const Instruction inst) const;

  inline const ClassDecoder& decode_load_store_word_and_unsigned_byte(
      const Instruction inst) const;

  inline const ClassDecoder& decode_media_instructions(
      const Instruction inst) const;

  inline const ClassDecoder& decode_memory_hints_advanced_simd_instructions_and_miscellaneous_instructions(
      const Instruction inst) const;

  inline const ClassDecoder& decode_miscellaneous_instructions(
      const Instruction inst) const;

  inline const ClassDecoder& decode_msr_immediate_and_hints(
      const Instruction inst) const;

  inline const ClassDecoder& decode_multiply_and_multiply_accumulate(
      const Instruction inst) const;

  inline const ClassDecoder& decode_other_floating_point_data_processing_instructions(
      const Instruction inst) const;

  inline const ClassDecoder& decode_packing_unpacking_saturation_and_reversal(
      const Instruction inst) const;

  inline const ClassDecoder& decode_parallel_addition_and_subtraction_signed(
      const Instruction inst) const;

  inline const ClassDecoder& decode_parallel_addition_and_subtraction_unsigned(
      const Instruction inst) const;

  inline const ClassDecoder& decode_saturating_addition_and_subtraction(
      const Instruction inst) const;

  inline const ClassDecoder& decode_signed_multiply_signed_and_unsigned_divide(
      const Instruction inst) const;

  inline const ClassDecoder& decode_simd_dp_1imm(
      const Instruction inst) const;

  inline const ClassDecoder& decode_simd_dp_2misc(
      const Instruction inst) const;

  inline const ClassDecoder& decode_simd_dp_2scalar(
      const Instruction inst) const;

  inline const ClassDecoder& decode_simd_dp_2shift(
      const Instruction inst) const;

  inline const ClassDecoder& decode_simd_dp_3diff(
      const Instruction inst) const;

  inline const ClassDecoder& decode_simd_dp_3same(
      const Instruction inst) const;

  inline const ClassDecoder& decode_synchronization_primitives(
      const Instruction inst) const;

  inline const ClassDecoder& decode_transfer_between_arm_core_and_extension_register_8_16_and_32_bit(
      const Instruction inst) const;

  inline const ClassDecoder& decode_transfer_between_arm_core_and_extension_registers_64_bit(
      const Instruction inst) const;

  inline const ClassDecoder& decode_unconditional_instructions(
      const Instruction inst) const;

  // The following fields define the set of class decoders
  // that can be returned by the API function "decode". They
  // are created once as instance fields, and then returned
  // by the table methods above. This speeds up the code since
  // the class decoders need to only be built once (and reused
  // for each call to "decode").
  const Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1 Actual_ADC_immediate_cccc0010101snnnnddddiiiiiiiiiiii_case_1_instance_;
  const Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1 Actual_ADC_register_cccc0000101snnnnddddiiiiitt0mmmm_case_1_instance_;
  const Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1 Actual_ADC_register_shifted_register_cccc0000101snnnnddddssss0tt1mmmm_case_1_instance_;
  const Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1 Actual_ADD_immediate_cccc0010100snnnnddddiiiiiiiiiiii_case_1_instance_;
  const Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1 Actual_ADR_A1_cccc001010001111ddddiiiiiiiiiiii_case_1_instance_;
  const Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1 Actual_ASR_immediate_cccc0001101s0000ddddiiiii100mmmm_case_1_instance_;
  const Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1 Actual_ASR_register_cccc0001101s0000ddddmmmm0101nnnn_case_1_instance_;
  const Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1 Actual_BIC_immediate_cccc0011110snnnnddddiiiiiiiiiiii_case_1_instance_;
  const Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1 Actual_BKPT_cccc00010010iiiiiiiiiiii0111iiii_case_1_instance_;
  const Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1 Actual_BLX_immediate_1111101hiiiiiiiiiiiiiiiiiiiiiiii_case_1_instance_;
  const Actual_BLX_register_cccc000100101111111111110011mmmm_case_1 Actual_BLX_register_cccc000100101111111111110011mmmm_case_1_instance_;
  const Actual_Bx_cccc000100101111111111110001mmmm_case_1 Actual_Bx_cccc000100101111111111110001mmmm_case_1_instance_;
  const Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1 Actual_CLZ_cccc000101101111dddd11110001mmmm_case_1_instance_;
  const Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1 Actual_CMN_immediate_cccc00110111nnnn0000iiiiiiiiiiii_case_1_instance_;
  const Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1 Actual_CMN_register_cccc00010111nnnn0000iiiiitt0mmmm_case_1_instance_;
  const Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1 Actual_CMN_register_shifted_register_cccc00010111nnnn0000ssss0tt1mmmm_case_1_instance_;
  const Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1 Actual_LDRD_immediate_cccc000pu1w0nnnnttttiiii1101iiii_case_1_instance_;
  const Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1 Actual_LDRD_literal_cccc0001u1001111ttttiiii1101iiii_case_1_instance_;
  const Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1 Actual_LDRD_register_cccc000pu0w0nnnntttt00001101mmmm_case_1_instance_;
  const Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1 Actual_LDREXB_cccc00011101nnnntttt111110011111_case_1_instance_;
  const Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1 Actual_LDREXD_cccc00011011nnnntttt111110011111_case_1_instance_;
  const Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1 Actual_LDRH_immediate_cccc000pu1w1nnnnttttiiii1011iiii_case_1_instance_;
  const Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1 Actual_LDRH_literal_cccc000pu1w11111ttttiiii1011iiii_case_1_instance_;
  const Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1 Actual_LDRH_register_cccc000pu0w1nnnntttt00001011mmmm_case_1_instance_;
  const Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1 Actual_LSL_immediate_cccc0001101s0000ddddiiiii000mmmm_case_1_instance_;
  const Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1 Actual_MLA_A1_cccc0000001sddddaaaammmm1001nnnn_case_1_instance_;
  const Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1 Actual_MLS_A1_cccc00000110ddddaaaammmm1001nnnn_case_1_instance_;
  const Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1 Actual_MOVT_cccc00110100iiiiddddiiiiiiiiiiii_case_1_instance_;
  const Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1 Actual_MOV_immediate_A1_cccc0011101s0000ddddiiiiiiiiiiii_case_1_instance_;
  const Actual_MRS_cccc00010r001111dddd000000000000_case_1 Actual_MRS_cccc00010r001111dddd000000000000_case_1_instance_;
  const Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1 Actual_MSR_immediate_cccc00110010mm001111iiiiiiiiiiii_case_1_instance_;
  const Actual_MSR_register_cccc00010010mm00111100000000nnnn_case_1 Actual_MSR_register_cccc00010010mm00111100000000nnnn_case_1_instance_;
  const Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1 Actual_MUL_A1_cccc0000000sdddd0000mmmm1001nnnn_case_1_instance_;
  const Actual_NOP_cccc0011001000001111000000000000_case_1 Actual_NOP_cccc0011001000001111000000000000_case_1_instance_;
  const Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1 Actual_ORR_immediate_cccc0011100snnnnddddiiiiiiiiiiii_case_1_instance_;
  const Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1 Actual_PKH_cccc01101000nnnnddddiiiiit01mmmm_case_1_instance_;
  const Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1 Actual_SMLALBB_SMLALBT_SMLALTB_SMLALTT_cccc00010100hhhhllllmmmm1xx0nnnn_case_1_instance_;
  const Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1 Actual_SMLAL_A1_cccc0000111shhhhllllmmmm1001nnnn_case_1_instance_;
  const Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1 Actual_SMULBB_SMULBT_SMULTB_SMULTT_cccc00010110dddd0000mmmm1xx0nnnn_case_1_instance_;
  const Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1 Actual_SMULL_A1_cccc0000110shhhhllllmmmm1001nnnn_case_1_instance_;
  const Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1 Actual_STRD_immediate_cccc000pu1w0nnnnttttiiii1111iiii_case_1_instance_;
  const Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1 Actual_STRD_register_cccc000pu0w0nnnntttt00001111mmmm_case_1_instance_;
  const Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1 Actual_STREXB_cccc00011100nnnndddd11111001tttt_case_1_instance_;
  const Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1 Actual_STREXD_cccc00011010nnnndddd11111001tttt_case_1_instance_;
  const Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1 Actual_STRH_immediate_cccc000pu1w0nnnnttttiiii1011iiii_case_1_instance_;
  const Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1 Actual_STRH_register_cccc000pu0w0nnnntttt00001011mmmm_case_1_instance_;
  const Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1 Actual_TST_immediate_cccc00110001nnnn0000iiiiiiiiiiii_case_1_instance_;
  const Actual_Unnamed_case_1 Actual_Unnamed_case_1_instance_;
  const Binary2RegisterBitRangeMsbGeLsb Binary2RegisterBitRangeMsbGeLsb_instance_;
  const Binary2RegisterBitRangeNotRnIsPcBitfieldExtract Binary2RegisterBitRangeNotRnIsPcBitfieldExtract_instance_;
  const Binary3RegisterOpAltA Binary3RegisterOpAltA_instance_;
  const Binary3RegisterOpAltANoCondsUpdate Binary3RegisterOpAltANoCondsUpdate_instance_;
  const Binary3RegisterOpAltBNoCondUpdates Binary3RegisterOpAltBNoCondUpdates_instance_;
  const Binary4RegisterDualOp Binary4RegisterDualOp_instance_;
  const Binary4RegisterDualOpNoCondsUpdate Binary4RegisterDualOpNoCondsUpdate_instance_;
  const Binary4RegisterDualResultNoCondsUpdate Binary4RegisterDualResultNoCondsUpdate_instance_;
  const BranchImmediate24 BranchImmediate24_instance_;
  const CondVfpOp CondVfpOp_instance_;
  const DataBarrier DataBarrier_instance_;
  const Deprecated Deprecated_instance_;
  const DuplicateToAdvSIMDRegisters DuplicateToAdvSIMDRegisters_instance_;
  const Forbidden Forbidden_instance_;
  const ForbiddenCondDecoder ForbiddenCondDecoder_instance_;
  const InstructionBarrier InstructionBarrier_instance_;
  const LdrImmediateOp LdrImmediateOp_instance_;
  const Load2RegisterImm12Op Load2RegisterImm12Op_instance_;
  const Load3RegisterImm5Op Load3RegisterImm5Op_instance_;
  const LoadRegisterList LoadRegisterList_instance_;
  const LoadVectorRegister LoadVectorRegister_instance_;
  const LoadVectorRegisterList LoadVectorRegisterList_instance_;
  const MoveDoubleVfpRegisterOp MoveDoubleVfpRegisterOp_instance_;
  const MoveVfpRegisterOp MoveVfpRegisterOp_instance_;
  const MoveVfpRegisterOpWithTypeSel MoveVfpRegisterOpWithTypeSel_instance_;
  const PermanentlyUndefined PermanentlyUndefined_instance_;
  const PreloadRegisterImm12Op PreloadRegisterImm12Op_instance_;
  const PreloadRegisterPairOp PreloadRegisterPairOp_instance_;
  const Store2RegisterImm12Op Store2RegisterImm12Op_instance_;
  const Store3RegisterImm5Op Store3RegisterImm5Op_instance_;
  const StoreRegisterList StoreRegisterList_instance_;
  const StoreVectorRegister StoreVectorRegister_instance_;
  const StoreVectorRegisterList StoreVectorRegisterList_instance_;
  const Unary1RegisterBitRangeMsbGeLsb Unary1RegisterBitRangeMsbGeLsb_instance_;
  const Unary2RegisterImmedShiftedOp Unary2RegisterImmedShiftedOp_instance_;
  const Unary2RegisterSatImmedShiftedOp Unary2RegisterSatImmedShiftedOp_instance_;
  const Undefined Undefined_instance_;
  const Unpredictable Unpredictable_instance_;
  const VcvtPtAndFixedPoint_FloatingPoint VcvtPtAndFixedPoint_FloatingPoint_instance_;
  const Vector1RegisterImmediate_BIT Vector1RegisterImmediate_BIT_instance_;
  const Vector1RegisterImmediate_MOV Vector1RegisterImmediate_MOV_instance_;
  const Vector1RegisterImmediate_MVN Vector1RegisterImmediate_MVN_instance_;
  const Vector2RegisterMiscellaneous_CVT_F2I Vector2RegisterMiscellaneous_CVT_F2I_instance_;
  const Vector2RegisterMiscellaneous_CVT_H2S Vector2RegisterMiscellaneous_CVT_H2S_instance_;
  const Vector2RegisterMiscellaneous_F32 Vector2RegisterMiscellaneous_F32_instance_;
  const Vector2RegisterMiscellaneous_I16_32_64N Vector2RegisterMiscellaneous_I16_32_64N_instance_;
  const Vector2RegisterMiscellaneous_I8_16_32L Vector2RegisterMiscellaneous_I8_16_32L_instance_;
  const Vector2RegisterMiscellaneous_RG Vector2RegisterMiscellaneous_RG_instance_;
  const Vector2RegisterMiscellaneous_V16_32_64N Vector2RegisterMiscellaneous_V16_32_64N_instance_;
  const Vector2RegisterMiscellaneous_V8 Vector2RegisterMiscellaneous_V8_instance_;
  const Vector2RegisterMiscellaneous_V8S Vector2RegisterMiscellaneous_V8S_instance_;
  const Vector2RegisterMiscellaneous_V8_16_32 Vector2RegisterMiscellaneous_V8_16_32_instance_;
  const Vector2RegisterMiscellaneous_V8_16_32I Vector2RegisterMiscellaneous_V8_16_32I_instance_;
  const Vector2RegisterMiscellaneous_V8_16_32T Vector2RegisterMiscellaneous_V8_16_32T_instance_;
  const VectorBinary2RegisterScalar_F32 VectorBinary2RegisterScalar_F32_instance_;
  const VectorBinary2RegisterScalar_I16_32 VectorBinary2RegisterScalar_I16_32_instance_;
  const VectorBinary2RegisterScalar_I16_32L VectorBinary2RegisterScalar_I16_32L_instance_;
  const VectorBinary2RegisterShiftAmount_CVT VectorBinary2RegisterShiftAmount_CVT_instance_;
  const VectorBinary2RegisterShiftAmount_E8_16_32L VectorBinary2RegisterShiftAmount_E8_16_32L_instance_;
  const VectorBinary2RegisterShiftAmount_I VectorBinary2RegisterShiftAmount_I_instance_;
  const VectorBinary2RegisterShiftAmount_ILS VectorBinary2RegisterShiftAmount_ILS_instance_;
  const VectorBinary2RegisterShiftAmount_N16_32_64R VectorBinary2RegisterShiftAmount_N16_32_64R_instance_;
  const VectorBinary2RegisterShiftAmount_N16_32_64RS VectorBinary2RegisterShiftAmount_N16_32_64RS_instance_;
  const VectorBinary3RegisterDifferentLength_I16_32L VectorBinary3RegisterDifferentLength_I16_32L_instance_;
  const VectorBinary3RegisterDifferentLength_I16_32_64 VectorBinary3RegisterDifferentLength_I16_32_64_instance_;
  const VectorBinary3RegisterDifferentLength_I8_16_32 VectorBinary3RegisterDifferentLength_I8_16_32_instance_;
  const VectorBinary3RegisterDifferentLength_I8_16_32L VectorBinary3RegisterDifferentLength_I8_16_32L_instance_;
  const VectorBinary3RegisterDifferentLength_P8 VectorBinary3RegisterDifferentLength_P8_instance_;
  const VectorBinary3RegisterImmOp VectorBinary3RegisterImmOp_instance_;
  const VectorBinary3RegisterLookupOp VectorBinary3RegisterLookupOp_instance_;
  const VectorBinary3RegisterSameLength32P VectorBinary3RegisterSameLength32P_instance_;
  const VectorBinary3RegisterSameLength32_DQ VectorBinary3RegisterSameLength32_DQ_instance_;
  const VectorBinary3RegisterSameLengthDI VectorBinary3RegisterSameLengthDI_instance_;
  const VectorBinary3RegisterSameLengthDQ VectorBinary3RegisterSameLengthDQ_instance_;
  const VectorBinary3RegisterSameLengthDQI16_32 VectorBinary3RegisterSameLengthDQI16_32_instance_;
  const VectorBinary3RegisterSameLengthDQI8P VectorBinary3RegisterSameLengthDQI8P_instance_;
  const VectorBinary3RegisterSameLengthDQI8_16_32 VectorBinary3RegisterSameLengthDQI8_16_32_instance_;
  const VectorLoadSingle1AllLanes VectorLoadSingle1AllLanes_instance_;
  const VectorLoadSingle2AllLanes VectorLoadSingle2AllLanes_instance_;
  const VectorLoadSingle3AllLanes VectorLoadSingle3AllLanes_instance_;
  const VectorLoadSingle4AllLanes VectorLoadSingle4AllLanes_instance_;
  const VectorLoadStoreMultiple1 VectorLoadStoreMultiple1_instance_;
  const VectorLoadStoreMultiple2 VectorLoadStoreMultiple2_instance_;
  const VectorLoadStoreMultiple3 VectorLoadStoreMultiple3_instance_;
  const VectorLoadStoreMultiple4 VectorLoadStoreMultiple4_instance_;
  const VectorLoadStoreSingle1 VectorLoadStoreSingle1_instance_;
  const VectorLoadStoreSingle2 VectorLoadStoreSingle2_instance_;
  const VectorLoadStoreSingle3 VectorLoadStoreSingle3_instance_;
  const VectorLoadStoreSingle4 VectorLoadStoreSingle4_instance_;
  const VectorUnary2RegisterDup VectorUnary2RegisterDup_instance_;
  const VfpMrsOp VfpMrsOp_instance_;
  const VfpUsesRegOp VfpUsesRegOp_instance_;
  const NotImplemented not_implemented_;
};

}  // namespace nacl_arm_dec
#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_H_
