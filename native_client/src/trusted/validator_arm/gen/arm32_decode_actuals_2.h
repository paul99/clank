/*
 * Copyright 2013 The Native Client Authors.  All rights reserved.
 * Use of this source code is governed by a BSD-style license that can
 * be found in the LICENSE file.
 */

// DO NOT EDIT: GENERATED CODE

#ifndef NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_ACTUALS_2_H_
#define NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_ACTUALS_2_H_

#include "native_client/src/trusted/validator_arm/inst_classes.h"
#include "native_client/src/trusted/validator_arm/arm_helpers.h"

namespace nacl_arm_dec {

// Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)(0)=1 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1 ||
//         inst(15:12)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100110d1snnnndddd1101nqm0mmmm,
//    rule: VABD_floating_point,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VACGE_111100110dssnnnndddd1110nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100110dssnnnndddd1110nqm1mmmm,
//    rule: VACGE,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VACGT_111100110dssnnnndddd1110nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100110dssnnnndddd1110nqm1mmmm,
//    rule: VACGT,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VADD_floating_point_A1_111100100d0snnnndddd1101nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100d0snnnndddd1101nqm0mmmm,
//    rule: VADD_floating_point_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCEQ_register_A2_111100100d0snnnndddd1110nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100d0snnnndddd1110nqm0mmmm,
//    rule: VCEQ_register_A2,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCGE_register_A2_111100110d0snnnndddd1110nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100110d0snnnndddd1110nqm0mmmm,
//    rule: VCGE_register_A2,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCGT_register_A2_111100110d1snnnndddd1110nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100110d1snnnndddd1110nqm0mmmm,
//    rule: VCGT_register_A2,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMDv2,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VFMA_A1_111100100d00nnnndddd1100nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100d00nnnndddd1100nqm1mmmm,
//    rule: VFMA_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMDv2,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VFMS_A1_111100100d10nnnndddd1100nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100d10nnnndddd1100nqm1mmmm,
//    rule: VFMS_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMAX_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100dssnnnndddd1111nqm0mmmm,
//    rule: VMAX_floating_point,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMIN_floating_point_111100100dssnnnndddd1111nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100dssnnnndddd1111nqm0mmmm,
//    rule: VMIN_floating_point,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMLA_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100dpsnnnndddd1101nqm1mmmm,
//    rule: VMLA_floating_point_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMLS_floating_point_A1_111100100dpsnnnndddd1101nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100dpsnnnndddd1101nqm1mmmm,
//    rule: VMLS_floating_point_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMUL_floating_point_A1_111100110d0snnnndddd1101nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100110d0snnnndddd1101nqm1mmmm,
//    rule: VMUL_floating_point_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VRECPS_111100100d0snnnndddd1111nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100d0snnnndddd1111nqm1mmmm,
//    rule: VRECPS,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VRSQRTS_111100100d1snnnndddd1111nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100d1snnnndddd1111nqm1mmmm,
//    rule: VRSQRTS,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32_DQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSUB_floating_point_A1_111100100d1snnnndddd1101nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100100d1snnnndddd1101nqm0mmmm,
//    rule: VSUB_floating_point_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(0)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
class Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VABD_floating_point_111100110d1snnnndddd1101nqm0mmmm_case_1);
};

// Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=11 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f110qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VABS_A1,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_1,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f010qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCEQ_immediate_0,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_1,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f001qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCGE_immediate_0,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_1,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f000qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCGT_immediate_0,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_1,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f011qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCLE_immediate_0,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCLS_111100111d11ss00dddd01000qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd01000qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCLS,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_1,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f100qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCLT_immediate_0,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCLZ_111100111d11ss00dddd01001qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd01001qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCLZ,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VNEG_111100111d11ss01dddd0f111qm0mmmm_case_1,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f111qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VNEG,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VPADAL_111100111d11ss00dddd0110p1m0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd0110p1m0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VPADAL,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    unsigned: (op(0)=1),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VPADDL_111100111d11ss00dddd0010p1m0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd0010p1m0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VPADDL,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    unsigned: (op(0)=1),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQABS_111100111d11ss00dddd01110qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd01110qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQABS,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQNEG_111100111d11ss00dddd01111qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd01111qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQNEG,
//    safety: [size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_1);
};

// Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=~10 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_F32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f110qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VABS_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_F32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCEQ_immediate_0_111100111d11ss01dddd0f010qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f010qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCEQ_immediate_0,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_F32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCGE_immediate_0_111100111d11ss01dddd0f001qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f001qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCGE_immediate_0,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_F32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCGT_immediate_0_111100111d11ss01dddd0f000qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f000qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCGT_immediate_0,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_F32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCLE_immediate_0_111100111d11ss01dddd0f011qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f011qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCLE_immediate_0,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_F32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCLT_immediate_0_111100111d11ss01dddd0f100qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f100qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCLT_immediate_0,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_CVT_F2I,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCVT_111100111d11ss11dddd011ppqm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss11dddd011ppqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCVT,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    to_integer: op(1)=1,
//    unsigned: op(0)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_F32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VNEG_111100111d11ss01dddd0f111qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss01dddd0f111qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VNEG,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_F32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    floating_point: F(10)=1,
//    generated_baseline: VRECPE_111100111d11ss11dddd010f0qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss11dddd010f0qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRECPE,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_F32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    floating_point: F(10)=1,
//    generated_baseline: VRSQRTE_111100111d11ss11dddd010f1qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss11dddd010f1qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRSQRTE,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      size(19:18)=~10 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2
     : public ClassDecoder {
 public:
  Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VABS_A1_111100111d11ss01dddd0f110qm0mmmm_case_2);
};

// Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [true => MAY_BE_SAFE],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    advsimd: false,
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), M(5), Vm(3:0)],
//    generated_baseline: VABS_cccc11101d110000dddd101s11m0mmmm_case_0,
//    m: Vm:D
//         if sz(8)=0
//         else M:Vm,
//    pattern: cccc11101d110000dddd101s11m0mmmm,
//    rule: VABS,
//    safety: [true => MAY_BE_SAFE],
//    sz: sz(8),
//    true: true,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    E: E(7),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), E(7), M(5), Vm(3:0)],
//    generated_baseline: VCMP_VCMPE_cccc11101d110100dddd101se1m0mmmm_case_0,
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    pattern: cccc11101d110100dddd101se1m0mmmm,
//    quiet_nan_exc: E(7)=1,
//    rule: VCMP_VCMPE,
//    safety: [true => MAY_BE_SAFE],
//    sz: sz(8),
//    true: true,
//    uses: {},
//    with_zero: false}
//
// Baseline:
//   {D: D(22),
//    E: E(7),
//    Vd: Vd(15:12),
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    dp_operation: sz(8)=1,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), E(7)],
//    generated_baseline: VCMP_VCMPE_cccc11101d110101dddd101se1000000_case_0,
//    pattern: cccc11101d110101dddd101se1000000,
//    quiet_nan_exc: E(7)=1,
//    rule: VCMP_VCMPE,
//    safety: [true => MAY_BE_SAFE],
//    sz: sz(8),
//    true: true,
//    uses: {},
//    with_zero: true}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    T: T(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: VFPv3HP,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D,
//    defs: {},
//    fields: [cond(31:28), D(22), op(16), Vd(15:12), T(7), M(5), Vm(3:0)],
//    generated_baseline: VCVTB_VCVTT_cccc11101d11001odddd1010t1m0mmmm_case_0,
//    half_to_single: op(16)=0,
//    lowbit: 16
//         if T(7)=1
//         else 0,
//    m: Vm:M,
//    op: op(16),
//    pattern: cccc11101d11001odddd1010t1m0mmmm,
//    rule: VCVTB_VCVTT,
//    safety: [true => MAY_BE_SAFE],
//    true: true,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    double_to_single: sz(8)=1,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), M(5), Vm(3:0)],
//    generated_baseline: VCVT_between_double_precision_and_single_precision_cccc11101d110111dddd101s11m0mmmm_case_0,
//    m: Vm:D
//         if sz(8)=0
//         else M:Vm,
//    pattern: cccc11101d110111dddd101s11m0mmmm,
//    rule: VCVT_between_double_precision_and_single_precision,
//    safety: [true => MAY_BE_SAFE],
//    sz: sz(8),
//    true: true,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    Vd: Vd(15:12),
//    advsimd: false,
//    arch: VFPv3,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      imm4H(19:16),
//      Vd(15:12),
//      sz(8),
//      imm4L(3:0)],
//    generated_baseline: VMOV_immediate_cccc11101d11iiiidddd101s0000iiii_case_0,
//    imm32: VFPExpandImm(imm4H:imm4L, 32),
//    imm4H: imm4H(19:16),
//    imm4L: imm4L(3:0),
//    imm64: VFPExpandImm(imm4H:imm4L, 64),
//    pattern: cccc11101d11iiiidddd101s0000iiii,
//    regs: 1,
//    rule: VMOV_immediate,
//    safety: [true => MAY_BE_SAFE],
//    single_register: sz(8)=0,
//    sz: sz(8),
//    true: true,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    advsimd: false,
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), M(5), Vm(3:0)],
//    generated_baseline: VMOV_register_cccc11101d110000dddd101s01m0mmmm_case_0,
//    m: Vm:D
//         if sz(8)=0
//         else M:Vm,
//    pattern: cccc11101d110000dddd101s01m0mmmm,
//    regs: 1,
//    rule: VMOV_register,
//    safety: [true => MAY_BE_SAFE],
//    single_register: sz(8)=0,
//    sz: sz(8),
//    true: true,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    advsimd: false,
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), M(5), Vm(3:0)],
//    generated_baseline: VNEG_cccc11101d110001dddd101s01m0mmmm_case_0,
//    m: Vm:D
//         if sz(8)=0
//         else M:Vm,
//    pattern: cccc11101d110001dddd101s01m0mmmm,
//    rule: VNEG,
//    safety: [true => MAY_BE_SAFE],
//    sz: sz(8),
//    true: true,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    advsimd: false,
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), sz(8), M(5), Vm(3:0)],
//    generated_baseline: VSQRT_cccc11101d110001dddd101s11m0mmmm_case_0,
//    m: Vm:D
//         if sz(8)=0
//         else M:Vm,
//    pattern: cccc11101d110001dddd101s11m0mmmm,
//    rule: VSQRT,
//    safety: [true => MAY_BE_SAFE],
//    sz: sz(8),
//    true: true,
//    uses: {}}
class Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VABS_cccc11101d110000dddd101s11m0mmmm_case_1);
};

// Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:16)(0)=1 ||
//         inst(3:0)(0)=1 => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    baseline: VectorBinary3RegisterDifferentLength_I16_32_64,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 111100101dssnnnndddd0100n0m0mmmm,
//    rule: VADDHN,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      Vn(0)=1 ||
//         Vm(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    baseline: VectorBinary3RegisterDifferentLength_I16_32_64,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VRADDHN_111100111dssnnnndddd0100n0m0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 111100111dssnnnndddd0100n0m0mmmm,
//    rule: VRADDHN,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      Vn(0)=1 ||
//         Vm(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    baseline: VectorBinary3RegisterDifferentLength_I16_32_64,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VRSUBHN_111100111dssnnnndddd0110n0m0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 111100111dssnnnndddd0110n0m0mmmm,
//    rule: VRSUBHN,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      Vn(0)=1 ||
//         Vm(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    baseline: VectorBinary3RegisterDifferentLength_I16_32_64,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSUBHN_111100101dssnnnndddd0110n0m0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 111100101dssnnnndddd0110n0m0mmmm,
//    rule: VSUBHN,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      Vn(0)=1 ||
//         Vm(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VADDHN_111100101dssnnnndddd0100n0m0mmmm_case_1);
};

// Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(15:12)(0)=1 ||
//         (inst(8)=1 &&
//         inst(19:16)(0)=1) => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    baseline: VectorBinary3RegisterDifferentLength_I8_16_32,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_0,
//    is_vaddw: U(24)=1,
//    is_w: op(8)=1,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 1111001u1dssnnnndddd000pn0m0mmmm,
//    rule: VADDL_VADDW,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      Vd(0)=1 ||
//         (op(8)=1 &&
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    baseline: VectorBinary3RegisterDifferentLength_I8_16_32,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSUBL_VSUBW_1111001u1dssnnnndddd001pn0m0mmmm_case_0,
//    is_w: op(8)=1,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 1111001u1dssnnnndddd001pn0m0mmmm,
//    rule: VSUBL_VSUBW,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      Vd(0)=1 ||
//         (op(8)=1 &&
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VADDL_VADDW_1111001u1dssnnnndddd000pn0m0mmmm_case_1);
};

// Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(31:28)=1111 => DECODER_ERROR],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    advsimd: false,
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_0,
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    pattern: cccc11100d11nnnndddd101sn0m0mmmm,
//    rule: VADD_floating_point,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    advsimd: false,
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VDIV_cccc11101d00nnnndddd101sn0m0mmmm_case_0,
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    pattern: cccc11101d00nnnndddd101sn0m0mmmm,
//    rule: VDIV,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    advsimd: false,
//    arch: VFPv4,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      op(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VFMA_VFMS_cccc11101d10nnnndddd101snom0mmmm_case_0,
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    op: op(6),
//    op1_neg: op(6)=1,
//    pattern: cccc11101d10nnnndddd101snom0mmmm,
//    rule: VFMA_VFMS,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: VFPv4,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      op(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VFNMA_VFNMS_cccc11101d01nnnndddd101snom0mmmm_case_0,
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    op: op(6),
//    op1_neg: op(6)=1,
//    pattern: cccc11101d01nnnndddd101snom0mmmm,
//    rule: VFNMA_VFNMS,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    add: op(6)=0,
//    advsimd: false,
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      op(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMLA_VMLS_floating_point_cccc11100d00nnnndddd101snom0mmmm_case_0,
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    op: op(6),
//    pattern: cccc11100d00nnnndddd101snom0mmmm,
//    rule: VMLA_VMLS_floating_point,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    advsimd: false,
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMUL_floating_point_cccc11100d10nnnndddd101sn0m0mmmm_case_0,
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    pattern: cccc11100d10nnnndddd101sn0m0mmmm,
//    rule: VMUL_floating_point,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    VFPNegMul_VNMLA: 1,
//    VFPNegMul_VNMLS: 2,
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      op(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VNMLA_VNMLS_cccc11100d01nnnndddd101snom0mmmm_case_0,
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    op: op(6),
//    pattern: cccc11100d01nnnndddd101snom0mmmm,
//    rule: VNMLA_VNMLS,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    type: VFPNegMul_VNMLA
//         if op(6)=1
//         else VFPNegMul_VNMLS,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    VFPNegMul_VNMUL: 3,
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VNMUL_cccc11100d10nnnndddd101sn1m0mmmm_case_0,
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    pattern: cccc11100d10nnnndddd101sn1m0mmmm,
//    rule: VNMUL,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    type: VFPNegMul_VNMUL,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    advsimd: false,
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    false: false,
//    fields: [cond(31:28),
//      D(22),
//      Vn(19:16),
//      Vd(15:12),
//      sz(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSUB_floating_point_cccc11100d11nnnndddd101sn1m0mmmm_case_0,
//    m: M:Vm
//         if dp_operation
//         else Vm:M,
//    n: N:Vn
//         if dp_operation
//         else Vn:N,
//    pattern: cccc11100d11nnnndddd101sn1m0mmmm,
//    rule: VSUB_floating_point,
//    safety: [cond(31:28)=1111 => DECODER_ERROR],
//    sz: sz(8),
//    uses: {}}
class Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VADD_floating_point_cccc11100d11nnnndddd101sn0m0mmmm_case_1);
};

// Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1 ||
//         inst(15:12)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100100dssnnnndddd1000nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VADD_integer,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VAND_register_111100100d00nnnndddd0001nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100100d00nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VAND_register,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VBIC_register_111100100d01nnnndddd0001nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100100d01nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VBIC_register,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VBIF_111100110d11nnnndddd0001nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100110d11nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VBIF,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VBIT_111100110d10nnnndddd0001nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100110d10nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VBIT,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VBSL_111100110d01nnnndddd0001nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100110d01nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VBSL,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VEOR_111100110d00nnnndddd0001nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100110d00nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VEOR,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VORN_register_111100100d11nnnndddd0001nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100100d11nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VORN_register,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VORR_register_or_VMOV_register_A1_111100100d10nnnndddd0001nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100100d10nnnndddd0001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VORR_register_or_VMOV_register_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQADD_1111001u0dssnnnndddd0000nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0000nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQADD,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQRSHL_1111001u0dssnnnndddd0101nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0101nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQRSHL,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQSHL_register_1111001u0dssnnnndddd0100nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0100nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQSHL_register,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQSUB_1111001u0dssnnnndddd0010nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0010nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQSUB,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VRSHL_1111001u0dssnnnndddd0101nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0101nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRSHL,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSHL_register_1111001u0dssnnnndddd0100nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd0100nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSHL_register,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQ,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSUB_integer_111100110dssnnnndddd1000nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100110dssnnnndddd1000nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSUB_integer,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VADD_integer_111100100dssnnnndddd1000nqm0mmmm_case_1);
};

// Actual_VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1
//
// Actual:
//   {safety: [inst(11:8)(0)=0 ||
//         inst(11:8)(3:2)=11 => DECODER_ERROR,
//      inst(6)=1 &&
//         inst(15:12)(0)=1 => UNDEFINED]}
//
// Baseline:
//   {D: D(22),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    arch: ASIMD,
//    baseline: Vector1RegisterImmediate_BIT,
//    cmode: cmode(11:8),
//    constraints: ,
//    d: D:Vd,
//    fields: [i(24),
//      D(22),
//      imm3(18:16),
//      Vd(15:12),
//      cmode(11:8),
//      Q(6),
//      op(5),
//      imm4(3:0)],
//    generated_baseline: VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0,
//    i: i(24),
//    imm3: imm3(18:16),
//    imm4: imm4(3:0),
//    imm64: AdvSIMDExpandImm(op, cmode, i:imm3:imm4),
//    op: op(5),
//    pattern: 1111001i1d000mmmddddcccc0q11mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VBIC_immediate,
//    safety: [cmode(0)=0 ||
//         cmode(3:2)=11 => DECODER_ERROR,
//      Q(6)=1 &&
//         Vd(0)=1 => UNDEFINED]}
//
// Baseline:
//   {D: D(22),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    arch: ASIMD,
//    baseline: Vector1RegisterImmediate_BIT,
//    cmode: cmode(11:8),
//    constraints: ,
//    d: D:Vd,
//    fields: [i(24),
//      D(22),
//      imm3(18:16),
//      Vd(15:12),
//      cmode(11:8),
//      Q(6),
//      op(5),
//      imm4(3:0)],
//    generated_baseline: VORR_immediate_1111001i1d000mmmddddcccc0q01mmmm_case_0,
//    i: i(24),
//    imm3: imm3(18:16),
//    imm4: imm4(3:0),
//    imm64: AdvSIMDExpandImm(op, cmode, i:imm3:imm4),
//    op: op(5),
//    pattern: 1111001i1d000mmmddddcccc0q01mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VORR_immediate,
//    safety: [cmode(0)=0 ||
//         cmode(3:2)=11 => DECODER_ERROR,
//      Q(6)=1 &&
//         Vd(0)=1 => UNDEFINED]}
class Actual_VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1()
     : ClassDecoder() {}
  virtual SafetyLevel safety(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VBIC_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1);
};

// Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=~00 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCNT_111100111d11ss00dddd01010qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd01010qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCNT,
//    safety: [size(19:18)=~00 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMVN_register_111100111d11ss00dddd01011qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd01011qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMVN_register,
//    safety: [size(19:18)=~00 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VCNT_111100111d11ss00dddd01010qm0mmmm_case_1);
};

// Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(18:16)=~000 &&
//         inst(18:16)=~10x => DECODER_ERROR],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: VFPv2,
//    baseline: CondVfpOp,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D
//         if to_integer
//         else D:Vd
//         if dp_operation
//         else Vd:D,
//    defs: {},
//    dp_operation: sz(8)=1,
//    fields: [cond(31:28),
//      D(22),
//      opc2(18:16),
//      Vd(15:12),
//      sz(8),
//      op(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_0,
//    m: Vm:M
//         if not to_integer
//         else M:Vm
//         if dp_operation
//         else Vm:M,
//    op: op(7),
//    opc2: opc2(18:16),
//    pattern: cccc11101d111ooodddd101sp1m0mmmm,
//    round_nearest: not to_integer,
//    round_zero: to_integer &&
//         op(7)=1,
//    rule: VCVT_VCVTR_between_floating_point_and_integer_Floating_point,
//    safety: [opc2(18:16)=~000 &&
//         opc2(18:16)=~10x => DECODER_ERROR],
//    sz: sz(8),
//    to_integer: opc2(2)=1,
//    true: true,
//    unsigned: opc2(0)=0
//         if to_integer
//         else op(7)=0,
//    uses: {}}
class Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VCVT_VCVTR_between_floating_point_and_integer_Floating_point_cccc11101d111ooodddd101sp1m0mmmm_case_1);
};

// Actual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:16)=000xxx => DECODER_ERROR,
//      inst(21:16)=0xxxxx => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_CVT,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    frac_bits: 64 - imm6,
//    generated_baseline: VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd111p0qm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VCVT_between_floating_point_and_fixed_point,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//      imm6(21:16)=0xxxxx => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    to_fixed: op(8)=1,
//    unsigned: U(24)=1,
//    uses: {}}
class Actual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VCVT_between_floating_point_and_fixed_point_1111001u1diiiiiidddd111p0qm1mmmm_case_1);
};

// Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1
//
// Actual:
//   {defs: {},
//    safety: [16
//         if inst(7)=0
//         else 32 - inst(3:0):inst(5)  <
//            0 => UNPREDICTABLE],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    U: U(16),
//    Vd: Vd(12),
//    arch: VFPv3,
//    baseline: VcvtPtAndFixedPoint_FloatingPoint,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D
//         if sz(8)=0
//         else D:Vd,
//    defs: {},
//    dp_operation: sf(8)=1,
//    fields: [cond(31:28),
//      D(22),
//      op(18),
//      U(16),
//      Vd(12),
//      sf(8),
//      sx(7),
//      i(5),
//      imm4(3:0)],
//    frac_bits: size - imm4:i,
//    generated_baseline: VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_0,
//    i: i(5),
//    imm4: imm4(3:0),
//    op: op(18),
//    pattern: cccc11101d111o1udddd101fx1i0iiii,
//    round_nearest: not to_fixed,
//    round_zero: to_fixed,
//    rule: VCVT_between_floating_point_and_fixed_point_Floating_point,
//    safety: [frac_bits  <
//            0 => UNPREDICTABLE],
//    sf: sf(8),
//    size: 16
//         if sx(7)=0
//         else 32,
//    sx: sx(7),
//    sz: sz(8),
//    to_fixed: op(18)=1,
//    true: true,
//    unsigned: U(16)=1,
//    uses: {}}
class Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1
     : public ClassDecoder {
 public:
  Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VCVT_between_floating_point_and_fixed_point_Floating_point_cccc11101d111o1udddd101fx1i0iiii_case_1);
};

// Actual_VDUP_arm_core_register_cccc11101bq0ddddtttt1011d0e10000_case_1
//
// Actual:
//   {defs: {},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(21)=1 &&
//         inst(19:16)(0)=1 => UNDEFINED,
//      inst(22):inst(5)(1:0)=11 => UNDEFINED],
//    uses: {inst(15:12)}}
//
// Baseline:
//   {B: B(22),
//    D: D(7),
//    E: E(5),
//    Pc: 15,
//    Q: Q(21),
//    Rt: Rt(15:12),
//    Vd: Vd(19:16),
//    arch: AdvSIMD,
//    baseline: DuplicateToAdvSIMDRegisters,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2
//         if B:E(1:0)=00
//         else 4
//         if B:E(1:0)=01
//         else 8
//         if B:E(1:0)=10
//         else 0,
//    esize: 32
//         if B:E(1:0)=00
//         else 16
//         if B:E(1:0)=01
//         else 8
//         if B:E(1:0)=10
//         else 0,
//    fields: [cond(31:28),
//      B(22),
//      Q(21),
//      Vd(19:16),
//      Rt(15:12),
//      D(7),
//      E(5)],
//    generated_baseline: VDUP_arm_core_register_cccc11101bq0ddddtttt1011d0e10000_case_0,
//    pattern: cccc11101bq0ddddtttt1011d0e10000,
//    regs: 1
//         if Q(21)=0
//         else 2,
//    rule: VDUP_arm_core_register,
//    safety: [Q(21)=1 &&
//         Vd(0)=1 => UNDEFINED,
//      B:E(1:0)=11 => UNDEFINED,
//      t  ==
//            Pc => UNPREDICTABLE],
//    sel: B:E,
//    t: Rt,
//    uses: {Rt}}
class Actual_VDUP_arm_core_register_cccc11101bq0ddddtttt1011d0e10000_case_1
     : public ClassDecoder {
 public:
  Actual_VDUP_arm_core_register_cccc11101bq0ddddtttt1011d0e10000_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VDUP_arm_core_register_cccc11101bq0ddddtttt1011d0e10000_case_1);
};

// Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         32  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:8)=0111
//         else 2
//         if inst(11:8)=1010
//         else 3
//         if inst(11:8)=0110
//         else 4
//         if inst(11:8)=0010
//         else 0 => UNPREDICTABLE,
//      inst(11:8)=0110 &&
//         inst(5:4)(1)=1 => UNDEFINED,
//      inst(11:8)=0111 &&
//         inst(5:4)(1)=1 => UNDEFINED,
//      inst(11:8)=1010 &&
//         inst(5:4)=11 => UNDEFINED,
//      not inst(11:8)=0111 ||
//         inst(11:8)=1010 ||
//         inst(11:8)=0110 ||
//         inst(11:8)=0010 => DECODER_ERROR],
//    small_imm_base_wb: not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(5:4)=00
//         else 4 << align,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreMultiple1,
//    constraints: ,
//    d: D:Vd,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    generated_baseline: VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_0,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d10nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    regs: 1
//         if type(11:8)=0111
//         else 2
//         if type(11:8)=1010
//         else 3
//         if type(11:8)=0110
//         else 4
//         if type(11:8)=0010
//         else 0,
//    rule: VLD1_multiple_single_elements,
//    safety: [type(11:8)=0111 &&
//         align(1)=1 => UNDEFINED,
//      type(11:8)=1010 &&
//         align(5:4)=11 => UNDEFINED,
//      type(11:8)=0110 &&
//         align(1)=1 => UNDEFINED,
//      not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(5:4)=00
//         else 4 << align,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreMultiple1,
//    constraints: ,
//    d: D:Vd,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    generated_baseline: VST1_multiple_single_elements_111101000d00nnnnddddttttssaammmm_case_0,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d00nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    regs: 1
//         if type(11:8)=0111
//         else 2
//         if type(11:8)=1010
//         else 3
//         if type(11:8)=0110
//         else 4
//         if type(11:8)=0010
//         else 0,
//    rule: VST1_multiple_single_elements,
//    safety: [type(11:8)=0111 &&
//         align(1)=1 => UNDEFINED,
//      type(11:8)=1010 &&
//         align(5:4)=11 => UNDEFINED,
//      type(11:8)=0110 &&
//         align(1)=1 => UNDEFINED,
//      not type in bitset {'0111', '1010', '0110', '0010'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
class Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1
     : public ClassDecoder {
 public:
  Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLD1_multiple_single_elements_111101000d10nnnnddddttttssaammmm_case_1);
};

// Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         32  <=
//            inst(22):inst(15:12) + 1
//         if inst(5)=0
//         else 2 => UNPREDICTABLE,
//      inst(7:6)=11 ||
//         (inst(7:6)=00 &&
//         inst(4)=1) => UNDEFINED],
//    small_imm_base_wb: not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    T: T(5),
//    Vd: Vd(15:12),
//    a: a(4),
//    alignment: 1
//         if a(4)=0
//         else ebytes,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadSingle1AllLanes,
//    constraints: ,
//    d: D:Vd,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(7:6),
//      T(5),
//      a(4),
//      Rm(3:0)],
//    generated_baseline: VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_0,
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnndddd1100sstammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    regs: 1
//         if T(5)=0
//         else 2,
//    rule: VLD1_single_element_to_all_lanes,
//    safety: [size(7:6)=11 ||
//         (size(7:6)=00 &&
//         a(4)=1) => UNDEFINED,
//      n  ==
//            Pc ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
class Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1
     : public ClassDecoder {
 public:
  Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLD1_single_element_to_all_lanes_111101001d10nnnndddd1100sstammmm_case_1);
};

// Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) => UNPREDICTABLE,
//      inst(11:10)=00 &&
//         inst(7:4)(0)=~0 => UNDEFINED,
//      inst(11:10)=01 &&
//         inst(7:4)(1)=~0 => UNDEFINED,
//      inst(11:10)=10 &&
//         inst(7:4)(1:0)=~00 &&
//         inst(7:4)(1:0)=~11 => UNDEFINED,
//      inst(11:10)=10 &&
//         inst(7:4)(2)=~0 => UNDEFINED,
//      inst(11:10)=11 => UNDEFINED],
//    small_imm_base_wb: not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: 1
//         if size(11:10)=00
//         else (1
//         if index_align(0)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(1:0)=00
//         else 4)
//         if size(11:10)=10
//         else 0,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreSingle1,
//    constraints: ,
//    d: D:Vd,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    generated_baseline: VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_0,
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnnddddss00aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD1_single_element_to_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=00 &&
//         index_align(0)=~0 => UNDEFINED,
//      size(11:10)=01 &&
//         index_align(1)=~0 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(2)=~0 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1:0)=~00 &&
//         index_align(1:0)=~11 => UNDEFINED,
//      n  ==
//            Pc => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: 1
//         if size(11:10)=00
//         else (1
//         if index_align(0)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(1:0)=00
//         else 4)
//         if size(11:10)=10
//         else 0,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreSingle1,
//    constraints: ,
//    d: D:Vd,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    generated_baseline: VST1_single_element_from_one_lane_111101001d00nnnnddddss00aaaammmm_case_0,
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d00nnnnddddss00aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VST1_single_element_from_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=00 &&
//         index_align(0)=~0 => UNDEFINED,
//      size(11:10)=01 &&
//         index_align(1)=~0 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(2)=~0 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1:0)=~00 &&
//         index_align(1:0)=~11 => UNDEFINED,
//      n  ==
//            Pc => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
class Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1
     : public ClassDecoder {
 public:
  Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLD1_single_element_to_one_lane_111101001d10nnnnddddss00aaaammmm_case_1);
};

// Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         32  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:8)=1000
//         else 2 + 1
//         if inst(11:8)=1000 ||
//         inst(11:8)=1001
//         else 2 => UNPREDICTABLE,
//      inst(11:8)=1000 ||
//         inst(11:8)=1001 &&
//         inst(5:4)=11 => UNDEFINED,
//      inst(7:6)=11 => UNDEFINED,
//      not inst(11:8)=1000 ||
//         inst(11:8)=1001 ||
//         inst(11:8)=0011 => DECODER_ERROR],
//    small_imm_base_wb: not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(5:4)=00
//         else 4 << align,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreMultiple2,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    generated_baseline: VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//    inc: 1
//         if type(11:8)=1000
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d10nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    regs: 1
//         if type in bitset {'1000', '1001'}
//         else 2,
//    rule: VLD2_multiple_2_element_structures,
//    safety: [size(7:6)=11 => UNDEFINED,
//      type in bitset {'1000', '1001'} &&
//         align(5:4)=11 => UNDEFINED,
//      not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d2 + regs  >
//            32 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(5:4)=00
//         else 4 << align,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreMultiple2,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    generated_baseline: VST2_multiple_2_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//    inc: 1
//         if type(11:8)=1000
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d00nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    regs: 1
//         if type in bitset {'1000', '1001'}
//         else 2,
//    rule: VST2_multiple_2_element_structures,
//    safety: [size(7:6)=11 => UNDEFINED,
//      type in bitset {'1000', '1001'} &&
//         align(5:4)=11 => UNDEFINED,
//      not type in bitset {'1000', '1001', '0011'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d2 + regs  >
//            32 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
class Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1
     : public ClassDecoder {
 public:
  Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLD2_multiple_2_element_structures_111101000d10nnnnddddttttssaammmm_case_1);
};

// Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(5)=0
//         else 2 => UNPREDICTABLE,
//      inst(7:6)=11 => UNDEFINED],
//    small_imm_base_wb: not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    T: T(5),
//    Vd: Vd(15:12),
//    a: a(4),
//    alignment: 1
//         if a(4)=0
//         else 2 * ebytes,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadSingle2AllLanes,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(7:6),
//      T(5),
//      a(4),
//      Rm(3:0)],
//    generated_baseline: VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_0,
//    inc: 1
//         if T(5)=0
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnndddd1101sstammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD2_single_2_element_structure_to_all_lanes,
//    safety: [size(7:6)=11 => UNDEFINED,
//      n  ==
//            Pc ||
//         d2  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
class Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1
     : public ClassDecoder {
 public:
  Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLD2_single_2_element_structure_to_all_lanes_111101001d10nnnndddd1101sstammmm_case_1);
};

// Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:10)=00
//         else (1
//         if inst(7:4)(1)=0
//         else 2)
//         if inst(11:10)=01
//         else (1
//         if inst(7:4)(2)=0
//         else 2)
//         if inst(11:10)=10
//         else 0 => UNPREDICTABLE,
//      inst(11:10)=10 &&
//         inst(7:4)(1)=~0 => UNDEFINED,
//      inst(11:10)=11 => UNDEFINED],
//    small_imm_base_wb: not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: (1
//         if index_align(0)=0
//         else 2)
//         if size(11:10)=00
//         else (1
//         if index_align(0)=0
//         else 4)
//         if size(11:10)=01
//         else (1
//         if index_align(0)=0
//         else 8)
//         if size(11:10)=10
//         else 0,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreSingle2,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    generated_baseline: VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_0,
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnnddddss01aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD2_single_2_element_structure_to_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1)=~0 => UNDEFINED,
//      n  ==
//            Pc ||
//         d2  >
//            31 => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: (1
//         if index_align(0)=0
//         else 2)
//         if size(11:10)=00
//         else (1
//         if index_align(0)=0
//         else 4)
//         if size(11:10)=01
//         else (1
//         if index_align(0)=0
//         else 8)
//         if size(11:10)=10
//         else 0,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreSingle2,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    generated_baseline: VST2_single_2_element_structure_from_one_lane_111101001d00nnnnddddss01aaaammmm_case_0,
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d00nnnnddddss01aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VST2_single_2_element_structure_from_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1)=~0 => UNDEFINED,
//      n  ==
//            Pc ||
//         d2  >
//            31 => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
class Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1
     : public ClassDecoder {
 public:
  Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLD2_single_2_element_structure_to_one_lane_111101001d10nnnnddddss01aaaammmm_case_1);
};

// Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:8)=0100
//         else 2 + 1
//         if inst(11:8)=0100
//         else 2 => UNPREDICTABLE,
//      inst(7:6)=11 ||
//         inst(5:4)(1)=1 => UNDEFINED,
//      not inst(11:8)=0100 ||
//         inst(11:8)=0101 => DECODER_ERROR],
//    small_imm_base_wb: not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(0)=0
//         else 8,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreMultiple3,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    generated_baseline: VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//    inc: 1
//         if type(11:8)=0100
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d10nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD3_multiple_3_element_structures,
//    safety: [size(7:6)=11 ||
//         align(1)=1 => UNDEFINED,
//      not type in bitset {'0100', '0101'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d3  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(0)=0
//         else 8,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreMultiple3,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    generated_baseline: VST3_multiple_3_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//    inc: 1
//         if type(11:8)=0100
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d00nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VST3_multiple_3_element_structures,
//    safety: [size(7:6)=11 ||
//         align(1)=1 => UNDEFINED,
//      not type in bitset {'0100', '0101'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d3  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
class Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1
     : public ClassDecoder {
 public:
  Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLD3_multiple_3_element_structures_111101000d10nnnnddddttttssaammmm_case_1);
};

// Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(5)=0
//         else 2 + 1
//         if inst(5)=0
//         else 2 => UNPREDICTABLE,
//      inst(7:6)=11 ||
//         inst(4)=1 => UNDEFINED],
//    small_imm_base_wb: not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    T: T(5),
//    Vd: Vd(15:12),
//    a: a(4),
//    alignment: 1,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadSingle3AllLanes,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(7:6),
//      T(5),
//      a(4),
//      Rm(3:0)],
//    generated_baseline: VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_0,
//    inc: 1
//         if T(5)=0
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnndddd1110sstammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD3_single_3_element_structure_to_all_lanes,
//    safety: [size(7:6)=11 ||
//         a(4)=1 => UNDEFINED,
//      n  ==
//            Pc ||
//         d3  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
class Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1
     : public ClassDecoder {
 public:
  Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLD3_single_3_element_structure_to_all_lanes_111101001d10nnnndddd1110sstammmm_case_1);
};

// Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:10)=00
//         else (1
//         if inst(7:4)(1)=0
//         else 2)
//         if inst(11:10)=01
//         else (1
//         if inst(7:4)(2)=0
//         else 2)
//         if inst(11:10)=10
//         else 0 + 1
//         if inst(11:10)=00
//         else (1
//         if inst(7:4)(1)=0
//         else 2)
//         if inst(11:10)=01
//         else (1
//         if inst(7:4)(2)=0
//         else 2)
//         if inst(11:10)=10
//         else 0 => UNPREDICTABLE,
//      inst(11:10)=00 &&
//         inst(7:4)(0)=~0 => UNDEFINED,
//      inst(11:10)=01 &&
//         inst(7:4)(0)=~0 => UNDEFINED,
//      inst(11:10)=10 &&
//         inst(7:4)(1:0)=~00 => UNDEFINED,
//      inst(11:10)=11 => UNDEFINED],
//    small_imm_base_wb: not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: 1,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreSingle3,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    generated_baseline: VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_0,
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnnddddss10aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD3_single_3_element_structure_to_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=00 &&
//         index_align(0)=~0 => UNDEFINED,
//      size(11:10)=01 &&
//         index_align(0)=~0 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1:0)=~00 => UNDEFINED,
//      n  ==
//            Pc ||
//         d3  >
//            31 => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: 1,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreSingle3,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    generated_baseline: VST3_single_3_element_structure_from_one_lane_111101001d00nnnnddddss10aaaammmm_case_0,
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d00nnnnddddss10aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VST3_single_3_element_structure_from_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=00 &&
//         index_align(0)=~0 => UNDEFINED,
//      size(11:10)=01 &&
//         index_align(0)=~0 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1:0)=~00 => UNDEFINED,
//      n  ==
//            Pc ||
//         d3  >
//            31 => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
class Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1
     : public ClassDecoder {
 public:
  Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLD3_single_3_element_structure_to_one_lane_111101001d10nnnnddddss10aaaammmm_case_1);
};

// Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:8)=0000
//         else 2 + 1
//         if inst(11:8)=0000
//         else 2 + 1
//         if inst(11:8)=0000
//         else 2 => UNPREDICTABLE,
//      inst(7:6)=11 => UNDEFINED,
//      not inst(11:8)=0000 ||
//         inst(11:8)=0001 => DECODER_ERROR],
//    small_imm_base_wb: not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(5:4)=00
//         else 4 << align,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreMultiple4,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    d4: d3 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    generated_baseline: VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_0,
//    inc: 1
//         if type(11:8)=0000
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d10nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD4_multiple_4_element_structures,
//    safety: [size(7:6)=11 => UNDEFINED,
//      not type in bitset {'0000', '0001'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d4  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    align: align(5:4),
//    alignment: 1
//         if align(5:4)=00
//         else 4 << align,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreMultiple4,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    d4: d3 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      type(11:8),
//      size(7:6),
//      align(5:4),
//      Rm(3:0)],
//    generated_baseline: VST4_multiple_4_element_structures_111101000d00nnnnddddttttssaammmm_case_0,
//    inc: 1
//         if type(11:8)=0000
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101000d00nnnnddddttttssaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VST4_multiple_4_element_structures,
//    safety: [size(7:6)=11 => UNDEFINED,
//      not type in bitset {'0000', '0001'} => DECODER_ERROR,
//      n  ==
//            Pc ||
//         d4  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: not register_index,
//    type: type(11:8),
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
class Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1
     : public ClassDecoder {
 public:
  Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLD4_multiple_4_element_structures_111101000d10nnnnddddttttssaammmm_case_1);
};

// Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(5)=0
//         else 2 + 1
//         if inst(5)=0
//         else 2 + 1
//         if inst(5)=0
//         else 2 => UNPREDICTABLE,
//      inst(7:6)=11 &&
//         inst(4)=0 => UNDEFINED],
//    small_imm_base_wb: not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    T: T(5),
//    Vd: Vd(15:12),
//    a: a(4),
//    alignment: 16
//         if size(7:6)=11
//         else (1
//         if a(4)=0
//         else 8)
//         if size(7:6)=10
//         else (1
//         if a(4)=0
//         else 4 * ebytes),
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadSingle4AllLanes,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    d4: d3 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    elements: 8 / ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(7:6),
//      T(5),
//      a(4),
//      Rm(3:0)],
//    generated_baseline: VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_0,
//    inc: 1
//         if T(5)=0
//         else 2,
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnndddd1111sstammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD4_single_4_element_structure_to_all_lanes,
//    safety: [size(7:6)=11 &&
//         a(4)=0 => UNDEFINED,
//      n  ==
//            Pc ||
//         d4  >
//            31 => UNPREDICTABLE],
//    size: size(7:6),
//    small_imm_base_wb: not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
class Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1
     : public ClassDecoder {
 public:
  Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLD4_single_4_element_structure_to_all_lanes_111101001d10nnnndddd1111sstammmm_case_1);
};

// Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)}
//         if (15  !=
//            inst(3:0))
//         else {},
//    safety: [15  ==
//            inst(19:16) ||
//         31  <=
//            inst(22):inst(15:12) + 1
//         if inst(11:10)=00
//         else (1
//         if inst(7:4)(1)=0
//         else 2)
//         if inst(11:10)=01
//         else (1
//         if inst(7:4)(2)=0
//         else 2)
//         if inst(11:10)=10
//         else 0 + 1
//         if inst(11:10)=00
//         else (1
//         if inst(7:4)(1)=0
//         else 2)
//         if inst(11:10)=01
//         else (1
//         if inst(7:4)(2)=0
//         else 2)
//         if inst(11:10)=10
//         else 0 + 1
//         if inst(11:10)=00
//         else (1
//         if inst(7:4)(1)=0
//         else 2)
//         if inst(11:10)=01
//         else (1
//         if inst(7:4)(2)=0
//         else 2)
//         if inst(11:10)=10
//         else 0 => UNPREDICTABLE,
//      inst(11:10)=10 &&
//         inst(7:4)(1:0)=11 => UNDEFINED,
//      inst(11:10)=11 => UNDEFINED],
//    small_imm_base_wb: not (15  !=
//            inst(3:0) &&
//         13  !=
//            inst(3:0)),
//    uses: {inst(3:0)
//         if (15  !=
//            inst(3:0))
//         else 32, inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: (1
//         if index_align(0)=0
//         else 4)
//         if size(11:10)=00
//         else (1
//         if index_align(0)=0
//         else 8)
//         if size(11:10)=01
//         else (1
//         if index_align(1:0)=00
//         else 4 << index_align(1:0))
//         if size(11:10)=10
//         else 0,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreSingle4,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    d4: d3 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    generated_baseline: VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_0,
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d10nnnnddddss11aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VLD4_single_4_element_structure_to_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1:0)=11 => UNDEFINED,
//      n  ==
//            Pc ||
//         d4  >
//            31 => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    Pc: 15,
//    Rm: Rm(3:0),
//    Rn: Rn(19:16),
//    Sp: 13,
//    Vd: Vd(15:12),
//    alignment: (1
//         if index_align(0)=0
//         else 4)
//         if size(11:10)=00
//         else (1
//         if index_align(0)=0
//         else 8)
//         if size(11:10)=01
//         else (1
//         if index_align(1:0)=00
//         else 4 << index_align(1:0))
//         if size(11:10)=10
//         else 0,
//    arch: ASIMD,
//    base: n,
//    baseline: VectorLoadStoreSingle4,
//    constraints: ,
//    d: D:Vd,
//    d2: d + inc,
//    d3: d2 + inc,
//    d4: d3 + inc,
//    defs: {base}
//         if wback
//         else {},
//    ebytes: 1 << size,
//    esize: 8 * ebytes,
//    fields: [D(22),
//      Rn(19:16),
//      Vd(15:12),
//      size(11:10),
//      index_align(7:4),
//      Rm(3:0)],
//    generated_baseline: VST4_single_4_element_structure_form_one_lane_111101001d00nnnnddddss11aaaammmm_case_0,
//    inc: 1
//         if size(11:10)=00
//         else (1
//         if index_align(1)=0
//         else 2)
//         if size(11:10)=01
//         else (1
//         if index_align(2)=0
//         else 2)
//         if size(11:10)=10
//         else 0,
//    index: index_align(3:1)
//         if size(11:10)=00
//         else index_align(3:2)
//         if size(11:10)=01
//         else index_align(3)
//         if size(11:10)=10
//         else 0,
//    index_align: index_align(7:4),
//    m: Rm,
//    n: Rn,
//    pattern: 111101001d00nnnnddddss11aaaammmm,
//    register_index: (m  !=
//            Pc &&
//         m  !=
//            Sp),
//    rule: VST4_single_4_element_structure_form_one_lane,
//    safety: [size(11:10)=11 => UNDEFINED,
//      size(11:10)=10 &&
//         index_align(1:0)=11 => UNDEFINED,
//      n  ==
//            Pc ||
//         d4  >
//            31 => UNPREDICTABLE],
//    size: size(11:10),
//    small_imm_base_wb: not register_index,
//    uses: {m
//         if wback
//         else None, n},
//    wback: (m  !=
//            Pc)}
class Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1
     : public ClassDecoder {
 public:
  Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLD4_single_4_element_structure_to_one_lane_111101001d10nnnnddddss11aaaammmm_case_1);
};

// Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(21)=1
//         else 32},
//    safety: [0  ==
//            inst(7:0) ||
//         32  <=
//            inst(15:12):inst(22) + inst(7:0) => UNPREDICTABLE,
//      15  ==
//            inst(19:16) &&
//         inst(21)=1 => UNPREDICTABLE,
//      inst(23)  ==
//            inst(24) &&
//         inst(21)=1 => UNDEFINED,
//      inst(24)=0 &&
//         inst(23)=0 &&
//         inst(21)=0 => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(23)=1 &&
//         inst(21)=1 &&
//         13  ==
//            inst(19:16) => DECODER_ERROR,
//      inst(24)=1 &&
//         inst(21)=0 => DECODER_ERROR],
//    small_imm_base_wb: inst(21)=1,
//    uses: {inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Sp: 13,
//    U: U(23),
//    Vd: Vd(15:12),
//    W: W(21),
//    add: U(23)=1,
//    arch: VFPv2,
//    base: Rn,
//    baseline: LoadVectorRegisterList,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D,
//    defs: {Rn
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      D(22),
//      W(21),
//      Rn(19:16),
//      Vd(15:12),
//      imm8(7:0)],
//    generated_baseline: VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_0,
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    n: Rn,
//    pattern: cccc110pudw1nnnndddd1010iiiiiiii,
//    regs: imm8,
//    rule: VLDM,
//    safety: [P(24)=0 &&
//         U(23)=0 &&
//         W(21)=0 => DECODER_ERROR,
//      P(24)=1 &&
//         W(21)=0 => DECODER_ERROR,
//      P  ==
//            U &&
//         W(21)=1 => UNDEFINED,
//      n  ==
//            Pc &&
//         wback => UNPREDICTABLE,
//      P(24)=0 &&
//         U(23)=1 &&
//         W(21)=1 &&
//         Rn  ==
//            Sp => DECODER_ERROR,
//      regs  ==
//            0 ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    single_regs: true,
//    small_imm_base_wb: wback,
//    true: true,
//    uses: {Rn},
//    wback: W(21)=1}
class Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLDM_cccc110pudw1nnnndddd1010iiiiiiii_case_1);
};

// Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(21)=1
//         else 32},
//    safety: [0  ==
//            inst(7:0) / 2 ||
//         16  <=
//            inst(7:0) / 2 ||
//         32  <=
//            inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE,
//      15  ==
//            inst(19:16) &&
//         inst(21)=1 => UNPREDICTABLE,
//      VFPSmallRegisterBank() &&
//         16  <=
//            inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE,
//      inst(23)  ==
//            inst(24) &&
//         inst(21)=1 => UNDEFINED,
//      inst(24)=0 &&
//         inst(23)=0 &&
//         inst(21)=0 => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(23)=1 &&
//         inst(21)=1 &&
//         13  ==
//            inst(19:16) => DECODER_ERROR,
//      inst(24)=1 &&
//         inst(21)=0 => DECODER_ERROR],
//    small_imm_base_wb: inst(21)=1,
//    uses: {inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Sp: 13,
//    U: U(23),
//    Vd: Vd(15:12),
//    W: W(21),
//    add: U(23)=1,
//    arch: ['VFPv2', 'AdvSIMD'],
//    base: Rn,
//    baseline: LoadVectorRegisterList,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd,
//    defs: {Rn
//         if wback
//         else None},
//    false: false,
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      D(22),
//      W(21),
//      Rn(19:16),
//      Vd(15:12),
//      imm8(7:0)],
//    generated_baseline: VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_0,
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    n: Rn,
//    pattern: cccc110pudw1nnnndddd1011iiiiiiii,
//    regs: imm8 / 2,
//    rule: VLDM,
//    safety: [P(24)=0 &&
//         U(23)=0 &&
//         W(21)=0 => DECODER_ERROR,
//      P(24)=1 &&
//         W(21)=0 => DECODER_ERROR,
//      P  ==
//            U &&
//         W(21)=1 => UNDEFINED,
//      n  ==
//            Pc &&
//         wback => UNPREDICTABLE,
//      P(24)=0 &&
//         U(23)=1 &&
//         W(21)=1 &&
//         Rn  ==
//            Sp => DECODER_ERROR,
//      regs  ==
//            0 ||
//         regs  >
//            16 ||
//         d + regs  >
//            32 => UNPREDICTABLE,
//      VFPSmallRegisterBank() &&
//         d + regs  >
//            16 => UNPREDICTABLE],
//    single_regs: false,
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    wback: W(21)=1}
class Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLDM_cccc110pudw1nnnndddd1011iiiiiiii_case_1);
};

// Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {},
//    is_literal_load: 15  ==
//            inst(19:16),
//    uses: {inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    Pc: 15,
//    Rn: Rn(19:16),
//    U: U(23),
//    Vd: Vd(15:12),
//    add: U(23)=1,
//    arch: VFPv2,
//    base: Rn,
//    baseline: LoadVectorRegister,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    fields: [cond(31:28), U(23), D(22), Rn(19:16), Vd(15:12), imm8(7:0)],
//    generated_baseline: VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_0,
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    is_literal_load: Rn  ==
//            Pc,
//    n: Rn,
//    pattern: cccc1101ud01nnnndddd1010iiiiiiii,
//    rule: VLDR,
//    single_reg: true,
//    true: true,
//    uses: {Rn}}
//
// Baseline:
//   {D: D(22),
//    Pc: 15,
//    Rn: Rn(19:16),
//    U: U(23),
//    Vd: Vd(15:12),
//    add: U(23)=1,
//    arch: ['VFPv2', 'AdvSIMD'],
//    base: Rn,
//    baseline: LoadVectorRegister,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    false: false,
//    fields: [cond(31:28), U(23), D(22), Rn(19:16), Vd(15:12), imm8(7:0)],
//    generated_baseline: VLDR_cccc1101ud01nnnndddd1011iiiiiiii_case_0,
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    is_literal_load: Rn  ==
//            Pc,
//    n: Rn,
//    pattern: cccc1101ud01nnnndddd1011iiiiiiii,
//    rule: VLDR,
//    single_reg: false,
//    uses: {Rn}}
class Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual bool is_literal_load(Instruction i) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VLDR_cccc1101ud01nnnndddd1010iiiiiiii_case_1);
};

// Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [(inst(21:20)=00 ||
//         inst(15:12)(0)=1) => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_I16_32L,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0,
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001u1dssnnnndddd0p10n1m0mmmm,
//    regs: 1,
//    rule: VMLAL_by_scalar_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: Q(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_I16_32L,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMLSL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_0,
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001u1dssnnnndddd0p10n1m0mmmm,
//    regs: 1,
//    rule: VMLSL_by_scalar_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: Q(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_I16_32L,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMULL_by_scalar_A2_1111001u1dssnnnndddd1010n1m0mmmm_case_0,
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001u1dssnnnndddd1010n1m0mmmm,
//    regs: 1,
//    rule: VMULL_by_scalar_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: Q(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_I16_32L,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQDMLAL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0,
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 111100101dssnnnndddd0p11n1m0mmmm,
//    regs: 1,
//    rule: VQDMLAL_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: Q(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_I16_32L,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQDMLSL_A1_111100101dssnnnndddd0p11n1m0mmmm_case_0,
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 111100101dssnnnndddd0p11n1m0mmmm,
//    regs: 1,
//    rule: VQDMLSL_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: Q(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_I16_32L,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQDMULL_A2_111100101dssnnnndddd1011n1m0mmmm_case_0,
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 111100101dssnnnndddd1011n1m0mmmm,
//    regs: 1,
//    rule: VQDMULL_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         Vd(0)=1) => UNDEFINED],
//    size: size(21:20),
//    unsigned: Q(24)=1,
//    uses: {}}
class Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMLAL_by_scalar_A2_1111001u1dssnnnndddd0p10n1m0mmmm_case_1);
};

// Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [(inst(21:20)=00 ||
//         inst(21:20)=01) => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR,
//      inst(24)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_F32,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0,
//    index: M,
//    m: Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VMLA_by_scalar_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         size(21:20)=01) => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_F32,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_0,
//    index: M,
//    m: Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VMLS_by_scalar_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         size(21:20)=01) => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_F32,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_0,
//    index: M,
//    m: Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd100fn1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VMUL_by_scalar_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      (size(21:20)=00 ||
//         size(21:20)=01) => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
class Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1);
};

// Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)=00 => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR,
//      inst(24)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_I16_32,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VMLA_by_scalar_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_I16_32,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMLS_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_1,
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd0p0fn1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VMLS_by_scalar_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_I16_32,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMUL_by_scalar_A1_1111001q1dssnnnndddd100fn1m0mmmm_case_1,
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd100fn1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VMUL_by_scalar_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_I16_32,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQDMULH_A2_1111001q1dssnnnndddd1100n1m0mmmm_case_0,
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd1100n1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VQDMULH_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(8),
//    M: M(5),
//    N: N(7),
//    Q: Q(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterScalar_I16_32,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [Q(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(10),
//      F(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQRDMULH_1111001q1dssnnnndddd1101n1m0mmmm_case_0,
//    index: M:Vm(3)
//         if size(21:20)=01
//         else M,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(10),
//    pattern: 1111001q1dssnnnndddd1101n1m0mmmm,
//    regs: 1
//         if Q(24)=0
//         else 2,
//    rule: VQRDMULH,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 => UNDEFINED,
//      Q(24)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1) => UNDEFINED],
//    size: size(21:20),
//    uses: {}}
class Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2
     : public ClassDecoder {
 public:
  Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMLA_by_scalar_A1_1111001q1dssnnnndddd0p0fn1m0mmmm_case_2);
};

// Actual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=11 => UNDEFINED,
//      inst(3:0)(0)=1 => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V16_32_64N,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMOVN_111100111d11ss10dddd001000m0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss10dddd001000m0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMOVN,
//    safety: [size(19:18)=11 => UNDEFINED, Vm(0)=1 => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class Actual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMOVN_111100111d11ss10dddd001000m0mmmm_case_1);
};

// Actual_VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_1
//
// Actual:
//   {defs: {},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE,
//      inst(22:21):inst(6:5)(3:0)=0x10 => UNDEFINED],
//    uses: {inst(15:12)}}
//
// Baseline:
//   {D: D(7),
//    Pc: 15,
//    Rt: Rt(15:12),
//    Vd: Vd(19:16),
//    advsimd: sel in bitset {'1xxx', '0xx1'},
//    arch: ['VFPv2', 'AdvSIMD'],
//    baseline: MoveVfpRegisterOpWithTypeSel,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    esize: 8
//         if opc1:opc2(3:0)=1xxx
//         else 16
//         if opc1:opc2(3:0)=0xx1
//         else 32
//         if opc1:opc2(3:0)=0x00
//         else 0,
//    fields: [cond(31:28),
//      opc1(22:21),
//      Vd(19:16),
//      Rt(15:12),
//      D(7),
//      opc2(6:5)],
//    generated_baseline: VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_0,
//    index: opc1(0):opc2
//         if opc1:opc2(3:0)=1xxx
//         else opc1(0):opc2(1)
//         if opc1:opc2(3:0)=0xx1
//         else opc1(0)
//         if opc1:opc2(3:0)=0x00
//         else 0,
//    opc1: opc1(22:21),
//    opc2: opc2(6:5),
//    pattern: cccc11100ii0ddddtttt1011dii10000,
//    rule: VMOV_ARM_core_register_to_scalar,
//    safety: [opc1:opc2(3:0)=0x10 => UNDEFINED,
//      t  ==
//            Pc => UNPREDICTABLE],
//    sel: opc1:opc2,
//    t: Rt,
//    uses: {Rt}}
class Actual_VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_1
     : public ClassDecoder {
 public:
  Actual_VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMOV_ARM_core_register_to_scalar_cccc11100ii0ddddtttt1011dii10000_case_1);
};

// Actual_VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_1
//
// Actual:
//   {defs: {inst(15:12)
//         if inst(20)=1
//         else 32},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE],
//    uses: {inst(15:12)
//         if not inst(20)=1
//         else 32}}
//
// Baseline:
//   {N: N(7),
//    None: 32,
//    Pc: 15,
//    Rt: Rt(15:12),
//    Vn: Vn(19:16),
//    arch: VFPv2,
//    baseline: MoveVfpRegisterOp,
//    cond: cond(31:28),
//    constraints: ,
//    defs: {Rt
//         if to_arm_register
//         else None},
//    fields: [cond(31:28), op(20), Vn(19:16), Rt(15:12), N(7)],
//    generated_baseline: VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_0,
//    n: Vn:N,
//    op: op(20),
//    pattern: cccc1110000onnnntttt1010n0010000,
//    rule: VMOV_between_ARM_core_register_and_single_precision_register,
//    safety: [t  ==
//            Pc => UNPREDICTABLE],
//    t: Rt,
//    to_arm_register: op(20)=1,
//    uses: {Rt
//         if not to_arm_register
//         else None}}
//
// Baseline:
//   {N: N(7),
//    None: 32,
//    Pc: 15,
//    Rt: Rt(15:12),
//    Vn: Vn(19:16),
//    arch: VFPv2,
//    baseline: MoveVfpRegisterOp,
//    cond: cond(31:28),
//    constraints: ,
//    defs: {Rt
//         if to_arm_register
//         else None},
//    fields: [cond(31:28), op(20), Vn(19:16), Rt(15:12), N(7)],
//    generated_baseline: VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000xnnnntttt1010n0010000_case_0,
//    n: Vn:N,
//    op: op(20),
//    pattern: cccc1110000xnnnntttt1010n0010000,
//    rule: VMOV_between_ARM_core_register_and_single_precision_register,
//    safety: [t  ==
//            Pc => UNPREDICTABLE],
//    t: Rt,
//    to_arm_register: op(20)=1,
//    uses: {Rt
//         if not to_arm_register
//         else None}}
class Actual_VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_1
     : public ClassDecoder {
 public:
  Actual_VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMOV_between_ARM_core_register_and_single_precision_register_cccc1110000onnnntttt1010n0010000_case_1);
};

// Actual_VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12), inst(19:16)}
//         if inst(20)=1
//         else {},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) => UNPREDICTABLE,
//      inst(20)=1 &&
//         inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {}
//         if inst(20)=1
//         else {inst(15:12), inst(19:16)}}
//
// Baseline:
//   {M: M(5),
//    Pc: 15,
//    Rt: Rt(15:12),
//    Rt2: Rt2(19:16),
//    Vm: Vm(3:0),
//    arch: ['VFPv2', 'AdvSIMD'],
//    baseline: MoveDoubleVfpRegisterOp,
//    cond: cond(31:28),
//    constraints: ,
//    defs: {Rt, Rt2}
//         if to_arm_registers
//         else {},
//    fields: [cond(31:28), op(20), Rt2(19:16), Rt(15:12), M(5), Vm(3:0)],
//    generated_baseline: VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_0,
//    m: M:Vm,
//    op: op(20),
//    pattern: cccc1100010otttttttt101100m1mmmm,
//    rule: VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register,
//    safety: [Pc in {t, t2} => UNPREDICTABLE,
//      to_arm_registers &&
//         t  ==
//            t2 => UNPREDICTABLE],
//    t: Rt,
//    t2: Rt2,
//    to_arm_registers: op(20)=1,
//    uses: {}
//         if to_arm_registers
//         else {Rt, Rt2}}
class Actual_VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMOV_between_two_ARM_core_registers_and_a_doubleword_extension_register_cccc1100010otttttttt101100m1mmmm_case_1);
};

// Actual_VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_1
//
// Actual:
//   {defs: {inst(15:12), inst(19:16)}
//         if inst(20)=1
//         else {},
//    safety: [15  ==
//            inst(15:12) ||
//         15  ==
//            inst(19:16) ||
//         31  ==
//            inst(3:0):inst(5) => UNPREDICTABLE,
//      inst(20)=1 &&
//         inst(15:12)  ==
//            inst(19:16) => UNPREDICTABLE],
//    uses: {}
//         if inst(20)=1
//         else {inst(15:12), inst(19:16)}}
//
// Baseline:
//   {M: M(5),
//    Pc: 15,
//    Rt: Rt(15:12),
//    Rt2: Rt2(19:16),
//    Vm: Vm(3:0),
//    arch: ['VFPv2'],
//    baseline: MoveDoubleVfpRegisterOp,
//    cond: cond(31:28),
//    constraints: ,
//    defs: {Rt, Rt2}
//         if to_arm_registers
//         else {},
//    fields: [cond(31:28), op(20), Rt2(19:16), Rt(15:12), M(5), Vm(3:0)],
//    generated_baseline: VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_0,
//    m: Vm:M,
//    op: op(20),
//    pattern: cccc1100010otttttttt101000m1mmmm,
//    rule: VMOV_between_two_ARM_core_registers_and_two_single_precision_registers,
//    safety: [Pc in {t, t2} ||
//         m  ==
//            31 => UNPREDICTABLE,
//      to_arm_registers &&
//         t  ==
//            t2 => UNPREDICTABLE],
//    t: Rt,
//    t2: Rt2,
//    to_arm_registers: op(20)=1,
//    uses: {}
//         if to_arm_registers
//         else {Rt, Rt2}}
class Actual_VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMOV_between_two_ARM_core_registers_and_two_single_precision_registers_cccc1100010otttttttt101000m1mmmm_case_1);
};

// Actual_VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_1
//
// Actual:
//   {safety: [inst(5)=0 &&
//         inst(11:8)(0)=1 &&
//         inst(11:8)(3:2)=~11 => DECODER_ERROR,
//      inst(5)=1 &&
//         inst(11:8)=~1110 => DECODER_ERROR,
//      inst(6)=1 &&
//         inst(15:12)(0)=1 => UNDEFINED]}
//
// Baseline:
//   {D: D(22),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    arch: ASIMD,
//    baseline: Vector1RegisterImmediate_MOV,
//    cmode: cmode(11:8),
//    constraints: ,
//    d: D:Vd,
//    false: false,
//    fields: [i(24),
//      D(22),
//      imm3(18:16),
//      Vd(15:12),
//      cmode(11:8),
//      Q(6),
//      op(5),
//      imm4(3:0)],
//    generated_baseline: VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_0,
//    i: i(24),
//    imm3: imm3(18:16),
//    imm4: imm4(3:0),
//    imm64: AdvSIMDExpandImm(op, cmode, i:imm3:imm4),
//    op: op(5),
//    pattern: 1111001m1d000mmmddddcccc0qp1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMOV_immediate_A1,
//    safety: [op(5)=0 &&
//         cmode(0)=1 &&
//         cmode(3:2)=~11 => DECODER_ERROR,
//      op(5)=1 &&
//         cmode(11:8)=~1110 => DECODER_ERROR,
//      Q(6)=1 &&
//         Vd(0)=1 => UNDEFINED],
//    single_register: false}
class Actual_VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_1()
     : ClassDecoder() {}
  virtual SafetyLevel safety(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMOV_immediate_A1_1111001m1d000mmmddddcccc0qp1mmmm_case_1);
};

// Actual_VMRS_cccc111011110001tttt101000010000_case_1
//
// Actual:
//   {defs: {16
//         if 15  ==
//            inst(15:12)
//         else inst(15:12)}}
//
// Baseline:
//   {NZCV: 16,
//    Pc: 15,
//    Rt: Rt(15:12),
//    arch: ['VFPv2', 'AdvSIMD'],
//    baseline: VfpMrsOp,
//    cond: cond(31:28),
//    constraints: ,
//    defs: {NZCV
//         if t  ==
//            Pc
//         else Rt},
//    fields: [cond(31:28), Rt(15:12)],
//    generated_baseline: VMRS_cccc111011110001tttt101000010000_case_0,
//    pattern: cccc111011110001tttt101000010000,
//    rule: VMRS,
//    t: Rt}
class Actual_VMRS_cccc111011110001tttt101000010000_case_1
     : public ClassDecoder {
 public:
  Actual_VMRS_cccc111011110001tttt101000010000_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMRS_cccc111011110001tttt101000010000_case_1);
};

// Actual_VMSR_cccc111011100001tttt101000010000_case_1
//
// Actual:
//   {defs: {},
//    safety: [15  ==
//            inst(15:12) => UNPREDICTABLE],
//    uses: {inst(15:12)}}
//
// Baseline:
//   {Pc: 15,
//    Rt: Rt(15:12),
//    arch: ['VFPv2', 'AdvSIMD'],
//    baseline: VfpUsesRegOp,
//    cond: cond(31:28),
//    constraints: ,
//    defs: {},
//    fields: [cond(31:28), Rt(15:12)],
//    generated_baseline: VMSR_cccc111011100001tttt101000010000_case_0,
//    pattern: cccc111011100001tttt101000010000,
//    rule: VMSR,
//    safety: [t  ==
//            Pc => UNPREDICTABLE],
//    t: Rt,
//    uses: {Rt}}
class Actual_VMSR_cccc111011100001tttt101000010000_case_1
     : public ClassDecoder {
 public:
  Actual_VMSR_cccc111011100001tttt101000010000_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMSR_cccc111011100001tttt101000010000_case_1);
};

// Actual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(15:12)(0)=1 => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR,
//      inst(24)=1 ||
//         inst(21:20)=~00 => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    baseline: VectorBinary3RegisterDifferentLength_P8,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 1111001u1dssnnnndddd11p0n0m0mmmm,
//    rule: VMULL_polynomial_A2,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      U(24)=1 ||
//         size(21:20)=~00 => UNDEFINED,
//      Vd(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class Actual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMULL_polynomial_A2_1111001u1dssnnnndddd11p0n0m0mmmm_case_1);
};

// Actual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)=~00 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1 ||
//         inst(15:12)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQI8P,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    false: false,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd1001nqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMUL_polynomial_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      size(21:20)=~00 => UNDEFINED],
//    size: size(21:20),
//    unsigned: false,
//    uses: {}}
class Actual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMUL_polynomial_A1_1111001u0dssnnnndddd1001nqm1mmmm_case_1);
};

// Actual_VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1
//
// Actual:
//   {safety: [(inst(11:8)(0)=1 &&
//         inst(11:8)(3:2)=~11) ||
//         inst(11:8)(3:1)=111 => DECODER_ERROR,
//      inst(6)=1 &&
//         inst(15:12)(0)=1 => UNDEFINED]}
//
// Baseline:
//   {D: D(22),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    arch: ASIMD,
//    baseline: Vector1RegisterImmediate_MVN,
//    cmode: cmode(11:8),
//    constraints: ,
//    d: D:Vd,
//    fields: [i(24),
//      D(22),
//      imm3(18:16),
//      Vd(15:12),
//      cmode(11:8),
//      Q(6),
//      op(5),
//      imm4(3:0)],
//    generated_baseline: VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_0,
//    i: i(24),
//    imm3: imm3(18:16),
//    imm4: imm4(3:0),
//    imm64: AdvSIMDExpandImm(op, cmode, i:imm3:imm4),
//    op: op(5),
//    pattern: 1111001i1d000mmmddddcccc0q11mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VMVN_immediate,
//    safety: [(cmode(0)=1 &&
//         cmode(3:2)=~11) ||
//         cmode(3:1)=111 => DECODER_ERROR,
//      Q(6)=1 &&
//         Vd(0)=1 => UNDEFINED]}
class Actual_VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1()
     : ClassDecoder() {}
  virtual SafetyLevel safety(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VMVN_immediate_1111001i1d000mmmddddcccc0q11mmmm_case_1);
};

// Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)(0)=1 ||
//         inst(6)=1 => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32P,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100110d0snnnndddd1101nqm0mmmm,
//    rule: VPADD_floating_point,
//    safety: [size(0)=1 ||
//         Q(6)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32P,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VPMAX_111100110dssnnnndddd1111nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100110dssnnnndddd1111nqm0mmmm,
//    rule: VPMAX,
//    safety: [size(0)=1 ||
//         Q(6)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLength32P,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 2,
//    esize: 32,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VPMIN_111100110dssnnnndddd1111nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    op1_neg: size(1),
//    pattern: 111100110dssnnnndddd1111nqm0mmmm,
//    rule: VPMIN,
//    safety: [size(0)=1 ||
//         Q(6)=1 => UNDEFINED],
//    size: size(21:20),
//    sz: size(0),
//    uses: {}}
class Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VPADD_floating_point_111100110d0snnnndddd1101nqm0mmmm_case_1);
};

// Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)=11 => UNDEFINED,
//      inst(6)=1 => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDI,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100100dssnnnndddd1011n0m1mmmm,
//    rule: VPADD_integer,
//    safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDI,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VPMAX_1111001u0dssnnnndddd1010n0m0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd1010n0m0mmmm,
//    rule: VPMAX,
//    safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDI,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VPMIN_1111001u0dssnnnndddd1010n0m1mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 1111001u0dssnnnndddd1010n0m1mmmm,
//    rule: VPMIN,
//    safety: [size(21:20)=11 => UNDEFINED, Q(6)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VPADD_integer_111100100dssnnnndddd1011n0m1mmmm_case_1);
};

// Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1
//
// Actual:
//   {base: 13,
//    defs: {13},
//    safety: [0  ==
//            inst(7:0) ||
//         32  <=
//            inst(15:12):inst(22) + inst(7:0) => UNPREDICTABLE],
//    small_imm_base_wb: true,
//    uses: {13}}
//
// Baseline:
//   {D: D(22),
//    Sp: 13,
//    Vd: Vd(15:12),
//    arch: VFPv2,
//    base: Sp,
//    baseline: LoadVectorRegisterList,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D,
//    defs: {Sp},
//    fields: [cond(31:28), D(22), Vd(15:12), imm8(7:0)],
//    generated_baseline: VPOP_cccc11001d111101dddd1010iiiiiiii_case_0,
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    pattern: cccc11001d111101dddd1010iiiiiiii,
//    regs: imm8,
//    rule: VPOP,
//    safety: [regs  ==
//            0 ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    single_regs: true,
//    small_imm_base_wb: true,
//    true: true,
//    uses: {Sp}}
//
// Baseline:
//   {D: D(22),
//    Sp: 13,
//    Vd: Vd(15:12),
//    arch: VFPv2,
//    base: Sp,
//    baseline: StoreVectorRegisterList,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D,
//    defs: {Sp},
//    fields: [cond(31:28), D(22), Vd(15:12), imm8(7:0)],
//    generated_baseline: VPUSH_cccc11010d101101dddd1010iiiiiiii_case_0,
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    pattern: cccc11010d101101dddd1010iiiiiiii,
//    regs: imm8,
//    rule: VPUSH,
//    safety: [regs  ==
//            0 ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    single_regs: true,
//    small_imm_base_wb: true,
//    true: true,
//    uses: {Sp}}
class Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VPOP_cccc11001d111101dddd1010iiiiiiii_case_1);
};

// Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1
//
// Actual:
//   {base: 13,
//    defs: {13},
//    safety: [0  ==
//            inst(7:0) / 2 ||
//         16  <=
//            inst(7:0) / 2 ||
//         32  <=
//            inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE,
//      VFPSmallRegisterBank() &&
//         16  <=
//            inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE],
//    small_imm_base_wb: true,
//    uses: {13}}
//
// Baseline:
//   {D: D(22),
//    Sp: 13,
//    Vd: Vd(15:12),
//    arch: ['VFPv2', 'AdvSIMD'],
//    base: Sp,
//    baseline: LoadVectorRegisterList,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd,
//    defs: {Sp},
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), imm8(7:0)],
//    generated_baseline: VPOP_cccc11001d111101dddd1011iiiiiiii_case_0,
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    pattern: cccc11001d111101dddd1011iiiiiiii,
//    regs: imm8 / 2,
//    rule: VPOP,
//    safety: [regs  ==
//            0 ||
//         regs  >
//            16 ||
//         d + regs  >
//            32 => UNPREDICTABLE,
//      VFPSmallRegisterBank() &&
//         d + regs  >
//            16 => UNPREDICTABLE],
//    single_regs: false,
//    small_imm_base_wb: true,
//    true: true,
//    uses: {Sp}}
//
// Baseline:
//   {D: D(22),
//    Sp: 13,
//    Vd: Vd(15:12),
//    arch: ['VFPv2', 'AdvSIMD'],
//    base: Sp,
//    baseline: StoreVectorRegisterList,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd,
//    defs: {Sp},
//    false: false,
//    fields: [cond(31:28), D(22), Vd(15:12), imm8(7:0)],
//    generated_baseline: VPUSH_cccc11010d101101dddd1011iiiiiiii_case_0,
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    pattern: cccc11010d101101dddd1011iiiiiiii,
//    regs: imm8 / 2,
//    rule: VPUSH,
//    safety: [regs  ==
//            0 ||
//         regs  >
//            16 ||
//         d + regs  >
//            32 => UNPREDICTABLE,
//      VFPSmallRegisterBank() &&
//         d + regs  >
//            16 => UNPREDICTABLE],
//    single_regs: false,
//    small_imm_base_wb: true,
//    true: true,
//    uses: {Sp}}
class Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VPOP_cccc11001d111101dddd1011iiiiiiii_case_1);
};

// Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:20)=00 ||
//         inst(15:12)(0)=1 => UNDEFINED,
//      inst(21:20)=11 => DECODER_ERROR],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    add: op(8)=0,
//    baseline: VectorBinary3RegisterDifferentLength_I16_32L,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_0,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 111100101dssnnnndddd10p1n0m0mmmm,
//    rule: VQDMLAL_VQDMLSL_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 ||
//         Vd(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    add: op(8)=0,
//    baseline: VectorBinary3RegisterDifferentLength_I16_32L,
//    constraints: & size(21:20)=~11 ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(8),
//      N(7),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQDMULL_A1_111100101dssnnnndddd1101n0m0mmmm_case_0,
//    m: Vm(2:0)
//         if size(21:20)=01
//         else Vm,
//    n: N:Vn,
//    op: op(8),
//    pattern: 111100101dssnnnndddd1101n0m0mmmm,
//    rule: VQDMULL_A1,
//    safety: [size(21:20)=11 => DECODER_ERROR,
//      size(21:20)=00 ||
//         Vd(0)=1 => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VQDMLAL_VQDMLSL_A1_111100101dssnnnndddd10p1n0m0mmmm_case_1);
};

// Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [(inst(21:20)=11 ||
//         inst(21:20)=00) => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(19:16)(0)=1 ||
//         inst(15:12)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQI16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100100dssnnnndddd1011nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQDMULH_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      (size(21:20)=11 ||
//         size(21:20)=00) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    N: N(7),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    Vn: Vn(19:16),
//    arch: ASIMD,
//    baseline: VectorBinary3RegisterSameLengthDQI16_32,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [U(24),
//      D(22),
//      size(21:20),
//      Vn(19:16),
//      Vd(15:12),
//      op(9),
//      N(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQRDMULH_A1_111100110dssnnnndddd1011nqm0mmmm_case_0,
//    m: M:Vm,
//    n: N:Vn,
//    op: op(9),
//    pattern: 111100110dssnnnndddd1011nqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQRDMULH_A1,
//    safety: [Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vn(0)=1 ||
//         Vd(0)=1) => UNDEFINED,
//      (size(21:20)=11 ||
//         size(21:20)=00) => UNDEFINED],
//    size: size(21:20),
//    unsigned: U(24)=1,
//    uses: {}}
class Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VQDMULH_A1_111100100dssnnnndddd1011nqm0mmmm_case_1);
};

// Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1
//
// Actual:
//   {safety: [inst(19:18)=11 ||
//         inst(3:0)(0)=1 => UNDEFINED,
//      inst(7:6)=00 => DECODER_ERROR]}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_I16_32_64N,
//    constraints: ,
//    d: D:Vd,
//    dest_unsigned: op(0)=1,
//    fields: [D(22), size(19:18), Vd(15:12), op(7:6), M(5), Vm(3:0)],
//    generated_baseline: VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_0,
//    m: M:Vm,
//    op: op(7:6),
//    pattern: 111100111d11ss10dddd0010ppm0mmmm,
//    rule: VQMOVN,
//    safety: [op(7:6)=00 => DECODER_ERROR,
//      size(19:18)=11 ||
//         Vm(0)=1 => UNDEFINED],
//    size: size(19:18),
//    src_unsigned: op(7:6)=11}
//
// Baseline:
//   {D: D(22),
//    M: M(5),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_I16_32_64N,
//    constraints: ,
//    d: D:Vd,
//    dest_unsigned: op(0)=1,
//    fields: [D(22), size(19:18), Vd(15:12), op(7:6), M(5), Vm(3:0)],
//    generated_baseline: VQMOVUN_111100111d11ss10dddd0010ppm0mmmm_case_0,
//    m: M:Vm,
//    op: op(7:6),
//    pattern: 111100111d11ss10dddd0010ppm0mmmm,
//    rule: VQMOVUN,
//    safety: [op(7:6)=00 => DECODER_ERROR,
//      size(19:18)=11 ||
//         Vm(0)=1 => UNDEFINED],
//    size: size(19:18),
//    src_unsigned: op(7:6)=11}
class Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1()
     : ClassDecoder() {}
  virtual SafetyLevel safety(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VQMOVN_111100111d11ss10dddd0010ppm0mmmm_case_1);
};

// Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:16)=000xxx => DECODER_ERROR,
//      inst(24)=0 &&
//         inst(8)=0 => DECODER_ERROR,
//      inst(3:0)(0)=1 => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_N16_32_64RS,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: U(24)=1,
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd100p01m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQRSHRN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//      Vm(0)=1 => UNDEFINED,
//      U(24)=0 &&
//         op(8)=0 => DECODER_ERROR],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    src_unsigned: U(24)=1 &&
//         op(8)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_N16_32_64RS,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: U(24)=1,
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQRSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQRSHRUN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//      Vm(0)=1 => UNDEFINED,
//      U(24)=0 &&
//         op(8)=0 => DECODER_ERROR],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    src_unsigned: U(24)=1 &&
//         op(8)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_N16_32_64RS,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: U(24)=1,
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQRSHRUN_1111001u1diiiiiidddd100p01m1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd100p01m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQRSHRUN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//      Vm(0)=1 => UNDEFINED,
//      U(24)=0 &&
//         op(8)=0 => DECODER_ERROR],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    src_unsigned: U(24)=1 &&
//         op(8)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_N16_32_64RS,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: U(24)=1,
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQSHRN_1111001u1diiiiiidddd100p00m1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQSHRN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//      Vm(0)=1 => UNDEFINED,
//      U(24)=0 &&
//         op(8)=0 => DECODER_ERROR],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    src_unsigned: U(24)=1 &&
//         op(8)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_N16_32_64RS,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: U(24)=1,
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQSHRUN_1111001u1diiiiiidddd100p00m1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd100p00m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQSHRUN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR,
//      Vm(0)=1 => UNDEFINED,
//      U(24)=0 &&
//         op(8)=0 => DECODER_ERROR],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    src_unsigned: U(24)=1 &&
//         op(8)=1,
//    uses: {}}
class Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VQRSHRN_1111001u1diiiiiidddd100p01m1mmmm_case_1);
};

// Actual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(24)=0 &&
//         inst(8)=0 => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED,
//      inst(7):inst(21:16)(6:0)=0000xxx => DECODER_ERROR],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_ILS,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    dest_unsigned: U(24)=1,
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd011plqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VQSHL_VQSHLU_immediate,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED,
//      U(24)=0 &&
//         op(8)=0 => UNDEFINED],
//    shift_amount: imm6 - 8
//         if L:imm6(6:0)=0001xxx
//         else imm6 - 16
//         if L:imm6(6:0)=001xxxx
//         else imm6 - 32
//         if L:imm6(6:0)=01xxxxx
//         else imm6
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    src_unsigned: U(24)=1 &&
//         op(8)=1,
//    unsigned: U(24)=1,
//    uses: {}}
class Actual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VQSHL_VQSHLU_immediate_1111001u1diiiiiidddd011plqm1mmmm_case_1);
};

// Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [3  <
//            inst(8:7) + inst(19:18) => UNDEFINED,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_RG,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VREV16_111100111d11ss00dddd000ppqm0mmmm_case_0,
//    groupsize: rev_groupsize(op, size),
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd000ppqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    reverse_mask: rev_mask(groupsize, esize),
//    rule: VREV16,
//    safety: [op + size  >=
//            3 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_RG,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VREV32_111100111d11ss00dddd000ppqm0mmmm_case_0,
//    groupsize: rev_groupsize(op, size),
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd000ppqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    reverse_mask: rev_mask(groupsize, esize),
//    rule: VREV32,
//    safety: [op + size  >=
//            3 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_RG,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VREV64_111100111d11ss00dddd000ppqm0mmmm_case_0,
//    groupsize: rev_groupsize(op, size),
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss00dddd000ppqm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    reverse_mask: rev_mask(groupsize, esize),
//    rule: VREV64,
//    safety: [op + size  >=
//            3 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VREV16_111100111d11ss00dddd000ppqm0mmmm_case_1);
};

// Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(21:16)=000xxx => DECODER_ERROR,
//      inst(3:0)(0)=1 => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_N16_32_64R,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VRSHRN_111100101diiiiiidddd100001m1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 111100101diiiiiidddd100001m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRSHRN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vm(0)=1 => UNDEFINED],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_N16_32_64R,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSHRN_111100101diiiiiidddd100000m1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 111100101diiiiiidddd100000m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSHRN,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vm(0)=1 => UNDEFINED],
//    shift_amount: 16 - imm6
//         if imm6(21:16)=001xxx
//         else 32 - imm6
//         if imm6(21:16)=01xxxx
//         else 64 - imm6
//         if imm6(21:16)=1xxxxx
//         else 0,
//    uses: {}}
class Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VRSHRN_111100101diiiiiidddd100001m1mmmm_case_1);
};

// Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED,
//      inst(7):inst(21:16)(6:0)=0000xxx => DECODER_ERROR],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_I,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd0010lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRSHR,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: 16 - imm6
//         if L:imm6(6:0)=0001xxx
//         else 32 - imm6
//         if L:imm6(6:0)=001xxxx
//         else 64 - imm6,
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_I,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VRSRA_1111001u1diiiiiidddd0011lqm1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd0011lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VRSRA,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: 16 - imm6
//         if L:imm6(6:0)=0001xxx
//         else 32 - imm6
//         if L:imm6(6:0)=001xxxx
//         else 64 - imm6,
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_I,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSHL_immediate_111100101diiiiiidddd0101lqm1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 111100101diiiiiidddd0101lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSHL_immediate,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: imm6 - 8
//         if L:imm6(6:0)=0001xxx
//         else imm6 - 16
//         if L:imm6(6:0)=001xxxx
//         else imm6 - 32
//         if L:imm6(6:0)=01xxxxx
//         else imm6
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_I,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSHR_1111001u1diiiiiidddd0000lqm1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd0000lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSHR,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: 16 - imm6
//         if L:imm6(6:0)=0001xxx
//         else 32 - imm6
//         if L:imm6(6:0)=001xxxx
//         else 64 - imm6,
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_I,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSLI_111100111diiiiiidddd0101lqm1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 111100111diiiiiidddd0101lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSLI,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: imm6 - 8
//         if L:imm6(6:0)=0001xxx
//         else imm6 - 16
//         if L:imm6(6:0)=001xxxx
//         else imm6 - 32
//         if L:imm6(6:0)=01xxxxx
//         else imm6
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_I,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSRA_1111001u1diiiiiidddd0001lqm1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd0001lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSRA,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: 16 - imm6
//         if L:imm6(6:0)=0001xxx
//         else 32 - imm6
//         if L:imm6(6:0)=001xxxx
//         else 64 - imm6,
//    unsigned: U(24)=1,
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_I,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if L:imm6(6:0)=0001xxx
//         else 4
//         if L:imm6(6:0)=001xxxx
//         else 2
//         if L:imm6(6:0)=01xxxxx
//         else 1
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    esize: 8
//         if L:imm6(6:0)=0001xxx
//         else 16
//         if L:imm6(6:0)=001xxxx
//         else 32
//         if L:imm6(6:0)=01xxxxx
//         else 64
//         if L:imm6(6:0)=1xxxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSRI_111100111diiiiiidddd0100lqm1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 111100111diiiiiidddd0100lqm1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSRI,
//    safety: [L:imm6(6:0)=0000xxx => DECODER_ERROR,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    shift_amount: 16 - imm6
//         if L:imm6(6:0)=0001xxx
//         else 32 - imm6
//         if L:imm6(6:0)=001xxxx
//         else 64 - imm6,
//    unsigned: U(24)=1,
//    uses: {}}
class Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VRSHR_1111001u1diiiiiidddd0010lqm1mmmm_case_1);
};

// Actual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(15:12)(0)=1 => UNDEFINED,
//      inst(21:16)=000xxx => DECODER_ERROR],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    L: L(7),
//    M: M(5),
//    Q: Q(6),
//    U: U(24),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: VectorBinary2RegisterShiftAmount_E8_16_32L,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 8
//         if imm6(21:16)=001xxx
//         else 4
//         if imm6(21:16)=01xxxx
//         else 2
//         if imm6(21:16)=1xxxxx
//         else 0,
//    esize: 8
//         if imm6(21:16)=001xxx
//         else 16
//         if imm6(21:16)=01xxxx
//         else 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    fields: [U(24),
//      D(22),
//      imm6(21:16),
//      Vd(15:12),
//      op(8),
//      L(7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_0,
//    imm6: imm6(21:16),
//    n: M:Vm,
//    op: op(8),
//    pattern: 1111001u1diiiiiidddd101000m1mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSHLL_A1_or_VMOVL,
//    safety: [imm6(21:16)=000xxx => DECODER_ERROR, Vd(0)=1 => UNDEFINED],
//    shift_amount: imm6 - 8
//         if imm6(21:16)=001xxx
//         else imm6 - 16
//         if imm6(21:16)=01xxxx
//         else imm6 - 32
//         if imm6(21:16)=1xxxxx
//         else 0,
//    uses: {}}
class Actual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VSHLL_A1_or_VMOVL_1111001u1diiiiiidddd101000m1mmmm_case_1);
};

// Actual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=11 ||
//         inst(15:12)(0)=1 => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_I8_16_32L,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss10dddd001100m0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSHLL_A2,
//    safety: [size(19:18)=11 ||
//         Vd(0)=1 => UNDEFINED],
//    shift_amount: esize,
//    size: size(19:18),
//    uses: {}}
class Actual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VSHLL_A2_111100111d11ss10dddd001100m0mmmm_case_1);
};

// Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(21)=1
//         else 32},
//    safety: [0  ==
//            inst(7:0) ||
//         32  <=
//            inst(15:12):inst(22) + inst(7:0) => UNPREDICTABLE,
//      15  ==
//            inst(19:16) &&
//         inst(21)=1 => UNPREDICTABLE,
//      inst(23)  ==
//            inst(24) &&
//         inst(21)=1 => UNDEFINED,
//      inst(24)=0 &&
//         inst(23)=0 &&
//         inst(21)=0 => DECODER_ERROR,
//      inst(24)=1 &&
//         inst(21)=0 => DECODER_ERROR,
//      inst(24)=1 &&
//         inst(23)=0 &&
//         inst(21)=1 &&
//         13  ==
//            inst(19:16) => DECODER_ERROR],
//    small_imm_base_wb: inst(21)=1,
//    uses: {inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Sp: 13,
//    U: U(23),
//    Vd: Vd(15:12),
//    W: W(21),
//    add: U(23)=1,
//    arch: VFPv2,
//    base: Rn,
//    baseline: StoreVectorRegisterList,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D,
//    defs: {Rn
//         if wback
//         else None},
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      D(22),
//      W(21),
//      Rn(19:16),
//      Vd(15:12),
//      imm8(7:0)],
//    generated_baseline: VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_0,
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    n: Rn,
//    pattern: cccc110pudw0nnnndddd1010iiiiiiii,
//    regs: imm8,
//    rule: VSTM,
//    safety: [P(24)=0 &&
//         U(23)=0 &&
//         W(21)=0 => DECODER_ERROR,
//      P(24)=1 &&
//         W(21)=0 => DECODER_ERROR,
//      P  ==
//            U &&
//         W(21)=1 => UNDEFINED,
//      n  ==
//            Pc &&
//         wback => UNPREDICTABLE,
//      P(24)=1 &&
//         U(23)=0 &&
//         W(21)=1 &&
//         Rn  ==
//            Sp => DECODER_ERROR,
//      regs  ==
//            0 ||
//         d + regs  >
//            32 => UNPREDICTABLE],
//    single_regs: true,
//    small_imm_base_wb: wback,
//    true: true,
//    uses: {Rn},
//    wback: W(21)=1}
class Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VSTM_cccc110pudw0nnnndddd1010iiiiiiii_case_1);
};

// Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1
//
// Actual:
//   {base: inst(19:16),
//    defs: {inst(19:16)
//         if inst(21)=1
//         else 32},
//    safety: [0  ==
//            inst(7:0) / 2 ||
//         16  <=
//            inst(7:0) / 2 ||
//         32  <=
//            inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE,
//      15  ==
//            inst(19:16) &&
//         inst(21)=1 => UNPREDICTABLE,
//      VFPSmallRegisterBank() &&
//         16  <=
//            inst(22):inst(15:12) + inst(7:0) / 2 => UNPREDICTABLE,
//      inst(23)  ==
//            inst(24) &&
//         inst(21)=1 => UNDEFINED,
//      inst(24)=0 &&
//         inst(23)=0 &&
//         inst(21)=0 => DECODER_ERROR,
//      inst(24)=1 &&
//         inst(21)=0 => DECODER_ERROR,
//      inst(24)=1 &&
//         inst(23)=0 &&
//         inst(21)=1 &&
//         13  ==
//            inst(19:16) => DECODER_ERROR],
//    small_imm_base_wb: inst(21)=1,
//    uses: {inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    None: 32,
//    P: P(24),
//    Pc: 15,
//    Rn: Rn(19:16),
//    Sp: 13,
//    U: U(23),
//    Vd: Vd(15:12),
//    W: W(21),
//    add: U(23)=1,
//    arch: ['VFPv2', 'AdvSIMD'],
//    base: Rn,
//    baseline: StoreVectorRegisterList,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd,
//    defs: {Rn
//         if wback
//         else None},
//    false: false,
//    fields: [cond(31:28),
//      P(24),
//      U(23),
//      D(22),
//      W(21),
//      Rn(19:16),
//      Vd(15:12),
//      imm8(7:0)],
//    generated_baseline: VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_0,
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    n: Rn,
//    pattern: cccc110pudw0nnnndddd1011iiiiiiii,
//    regs: imm8 / 2,
//    rule: VSTM,
//    safety: [P(24)=0 &&
//         U(23)=0 &&
//         W(21)=0 => DECODER_ERROR,
//      P(24)=1 &&
//         W(21)=0 => DECODER_ERROR,
//      P  ==
//            U &&
//         W(21)=1 => UNDEFINED,
//      n  ==
//            Pc &&
//         wback => UNPREDICTABLE,
//      P(24)=1 &&
//         U(23)=0 &&
//         W(21)=1 &&
//         Rn  ==
//            Sp => DECODER_ERROR,
//      regs  ==
//            0 ||
//         regs  >
//            16 ||
//         d + regs  >
//            32 => UNPREDICTABLE,
//      VFPSmallRegisterBank() &&
//         d + regs  >
//            16 => UNPREDICTABLE],
//    single_regs: false,
//    small_imm_base_wb: wback,
//    uses: {Rn},
//    wback: W(21)=1}
class Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1()
     : ClassDecoder() {}
  virtual Register base_address_register(Instruction i) const;
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual bool base_address_register_writeback_small_immediate(
      Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VSTM_cccc110pudw0nnnndddd1011iiiiiiii_case_1);
};

// Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1
//
// Actual:
//   {defs: {},
//    safety: [15  ==
//            inst(19:16) => FORBIDDEN_OPERANDS],
//    uses: {inst(19:16)}}
//
// Baseline:
//   {D: D(22),
//    Pc: 15,
//    Rn: Rn(19:16),
//    U: U(23),
//    Vd: Vd(15:12),
//    add: U(23)=1,
//    arch: VFPv2,
//    baseline: StoreVectorRegister,
//    cond: cond(31:28),
//    constraints: ,
//    d: Vd:D,
//    defs: {},
//    fields: [cond(31:28), U(23), D(22), Rn(19:16), Vd(15:12), imm8(7:0)],
//    generated_baseline: VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_0,
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    n: Rn,
//    pattern: cccc1101ud00nnnndddd1010iiiiiiii,
//    rule: VSTR,
//    safety: [n  ==
//            Pc => FORBIDDEN_OPERANDS],
//    single_reg: true,
//    true: true,
//    uses: {Rn}}
//
// Baseline:
//   {D: D(22),
//    Pc: 15,
//    Rn: Rn(19:16),
//    U: U(23),
//    Vd: Vd(15:12),
//    add: U(23)=1,
//    arch: ['VFPv2', 'AdvSIMD'],
//    baseline: StoreVectorRegister,
//    cond: cond(31:28),
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    false: false,
//    fields: [cond(31:28), U(23), D(22), Rn(19:16), Vd(15:12), imm8(7:0)],
//    generated_baseline: VSTR_cccc1101ud00nnnndddd1011iiiiiiii_case_0,
//    imm32: ZeroExtend(imm8:'00'(1:0), 32),
//    imm8: imm8(7:0),
//    n: Rn,
//    pattern: cccc1101ud00nnnndddd1011iiiiiiii,
//    rule: VSTR,
//    safety: [n  ==
//            Pc => FORBIDDEN_OPERANDS],
//    single_reg: false,
//    uses: {Rn}}
class Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1
     : public ClassDecoder {
 public:
  Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VSTR_cccc1101ud00nnnndddd1010iiiiiiii_case_1);
};

// Actual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=~00 => UNDEFINED,
//      inst(22):inst(15:12)  ==
//            inst(5):inst(3:0) => UNKNOWN,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8S,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VSWP_111100111d11ss10dddd00000qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss10dddd00000qm0mmmm,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VSWP,
//    safety: [d  ==
//            m => UNKNOWN,
//      size(19:18)=~00 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class Actual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VSWP_111100111d11ss10dddd00000qm0mmmm_case_1);
};

// Actual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=11 => UNDEFINED,
//      inst(22):inst(15:12)  ==
//            inst(5):inst(3:0) => UNKNOWN,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32T,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VTRN_111100111d11ss10dddd00001qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss10dddd00001qm0mmmm,
//    quadword_operation: Q(6)=1,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VTRN,
//    safety: [d  ==
//            m => UNKNOWN,
//      size(19:18)=11 => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class Actual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VTRN_111100111d11ss10dddd00001qm0mmmm_case_1);
};

// Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1
//
// Actual:
//   {defs: {},
//    safety: [inst(19:18)=11 ||
//         (inst(6)=0 &&
//         inst(19:18)=10) => UNDEFINED,
//      inst(22):inst(15:12)  ==
//            inst(5):inst(3:0) => UNKNOWN,
//      inst(6)=1 &&
//         (inst(15:12)(0)=1 ||
//         inst(3:0)(0)=1) => UNDEFINED],
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32I,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VUZP_111100111d11ss10dddd00010qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss10dddd00010qm0mmmm,
//    quadword_operation: Q(6)=1,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VUZP,
//    safety: [d  ==
//            m => UNKNOWN,
//      size(19:18)=11 ||
//         (Q(6)=0 &&
//         size(19:18)=10) => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
//
// Baseline:
//   {D: D(22),
//    F: F(10),
//    M: M(5),
//    Q: Q(6),
//    Vd: Vd(15:12),
//    Vm: Vm(3:0),
//    arch: ASIMD,
//    baseline: Vector2RegisterMiscellaneous_V8_16_32I,
//    constraints: ,
//    d: D:Vd,
//    defs: {},
//    elements: 64 / esize,
//    esize: 8 << size,
//    fields: [D(22),
//      size(19:18),
//      Vd(15:12),
//      F(10),
//      op(8:7),
//      Q(6),
//      M(5),
//      Vm(3:0)],
//    generated_baseline: VZIP_111100111d11ss10dddd00011qm0mmmm_case_0,
//    m: M:Vm,
//    op: op(8:7),
//    pattern: 111100111d11ss10dddd00011qm0mmmm,
//    quadword_operation: Q(6)=1,
//    regs: 1
//         if Q(6)=0
//         else 2,
//    rule: VZIP,
//    safety: [d  ==
//            m => UNKNOWN,
//      size(19:18)=11 ||
//         (Q(6)=0 &&
//         size(19:18)=10) => UNDEFINED,
//      Q(6)=1 &&
//         (Vd(0)=1 ||
//         Vm(0)=1) => UNDEFINED],
//    size: size(19:18),
//    uses: {}}
class Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1
     : public ClassDecoder {
 public:
  Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1()
     : ClassDecoder() {}
  virtual RegisterList defs(Instruction inst) const;
  virtual SafetyLevel safety(Instruction i) const;
  virtual RegisterList uses(Instruction i) const;
 private:
  NACL_DISALLOW_COPY_AND_ASSIGN(
      Actual_VUZP_111100111d11ss10dddd00010qm0mmmm_case_1);
};

} // namespace nacl_arm_test

#endif  // NATIVE_CLIENT_SRC_TRUSTED_VALIDATOR_ARM_GEN_ARM32_DECODE_ACTUALS_2_H_
