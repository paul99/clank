/*
 * THIS FILE IS AUTO-GENERATED. DO NOT EDIT.
 * Compiled for x86-32 bit mode.
 *
 * You must include ncopcode_desc.h before this file.
 */

static int NaClGpSubregIndex[NaClOpKindEnumSize] = {
  /*      Unknown_Operand */ NACL_REGISTER_UNDEFINED,
  /*            A_Operand */ NACL_REGISTER_UNDEFINED,
  /*            E_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Eb_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Ew_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Ev_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Eo_Operand */ NACL_REGISTER_UNDEFINED,
  /*          Edq_Operand */ NACL_REGISTER_UNDEFINED,
  /*            G_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Gb_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Gw_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Gv_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Go_Operand */ NACL_REGISTER_UNDEFINED,
  /*          Gdq_Operand */ NACL_REGISTER_UNDEFINED,
  /*        Seg_G_Operand */ NACL_REGISTER_UNDEFINED,
  /*         G_OpcodeBase */ NACL_REGISTER_UNDEFINED,
  /*            I_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Ib_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Iw_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Iv_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Io_Operand */ NACL_REGISTER_UNDEFINED,
  /*           I2_Operand */ NACL_REGISTER_UNDEFINED,
  /*            J_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Jb_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Jw_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Jv_Operand */ NACL_REGISTER_UNDEFINED,
  /*            M_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Mb_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Mw_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Mv_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Mo_Operand */ NACL_REGISTER_UNDEFINED,
  /*          Mdq_Operand */ NACL_REGISTER_UNDEFINED,
  /*          Mpw_Operand */ NACL_REGISTER_UNDEFINED,
  /*          Mpv_Operand */ NACL_REGISTER_UNDEFINED,
  /*          Mpo_Operand */ NACL_REGISTER_UNDEFINED,
  /*        Mmx_E_Operand */ NACL_REGISTER_UNDEFINED,
  /*        Mmx_N_Operand */ NACL_REGISTER_UNDEFINED,
  /*        Mmx_G_Operand */ NACL_REGISTER_UNDEFINED,
  /*       Mmx_Gd_Operand */ NACL_REGISTER_UNDEFINED,
  /*        Xmm_E_Operand */ NACL_REGISTER_UNDEFINED,
  /*       Xmm_Eo_Operand */ NACL_REGISTER_UNDEFINED,
  /*        Xmm_G_Operand */ NACL_REGISTER_UNDEFINED,
  /*       Xmm_Go_Operand */ NACL_REGISTER_UNDEFINED,
  /*            C_Operand */ NACL_REGISTER_UNDEFINED,
  /*            D_Operand */ NACL_REGISTER_UNDEFINED,
  /*            O_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Ob_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Ow_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Ov_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Oo_Operand */ NACL_REGISTER_UNDEFINED,
  /*            S_Operand */ NACL_REGISTER_UNDEFINED,
  /*           St_Operand */ NACL_REGISTER_UNDEFINED,
  /*           RegUnknown */ NACL_REGISTER_UNDEFINED,
  /*                RegAL */   0,
  /*                RegBL */   3,
  /*                RegCL */   1,
  /*                RegDL */   2,
  /*                RegAH */   0,
  /*                RegBH */   3,
  /*                RegCH */   1,
  /*                RegDH */   2,
  /*               RegDIL */ NACL_REGISTER_UNDEFINED,
  /*               RegSIL */ NACL_REGISTER_UNDEFINED,
  /*               RegBPL */ NACL_REGISTER_UNDEFINED,
  /*               RegSPL */ NACL_REGISTER_UNDEFINED,
  /*               RegR8B */ NACL_REGISTER_UNDEFINED,
  /*               RegR9B */ NACL_REGISTER_UNDEFINED,
  /*              RegR10B */ NACL_REGISTER_UNDEFINED,
  /*              RegR11B */ NACL_REGISTER_UNDEFINED,
  /*              RegR12B */ NACL_REGISTER_UNDEFINED,
  /*              RegR13B */ NACL_REGISTER_UNDEFINED,
  /*              RegR14B */ NACL_REGISTER_UNDEFINED,
  /*              RegR15B */ NACL_REGISTER_UNDEFINED,
  /*                RegAX */   0,
  /*                RegBX */   3,
  /*                RegCX */   1,
  /*                RegDX */   2,
  /*                RegSI */   6,
  /*                RegDI */   7,
  /*                RegBP */   5,
  /*                RegSP */   4,
  /*               RegR8W */ NACL_REGISTER_UNDEFINED,
  /*               RegR9W */ NACL_REGISTER_UNDEFINED,
  /*              RegR10W */ NACL_REGISTER_UNDEFINED,
  /*              RegR11W */ NACL_REGISTER_UNDEFINED,
  /*              RegR12W */ NACL_REGISTER_UNDEFINED,
  /*              RegR13W */ NACL_REGISTER_UNDEFINED,
  /*              RegR14W */ NACL_REGISTER_UNDEFINED,
  /*              RegR15W */ NACL_REGISTER_UNDEFINED,
  /*               RegEAX */   0,
  /*               RegEBX */   3,
  /*               RegECX */   1,
  /*               RegEDX */   2,
  /*               RegESI */   6,
  /*               RegEDI */   7,
  /*               RegEBP */   5,
  /*               RegESP */   4,
  /*               RegR8D */ NACL_REGISTER_UNDEFINED,
  /*               RegR9D */ NACL_REGISTER_UNDEFINED,
  /*              RegR10D */ NACL_REGISTER_UNDEFINED,
  /*              RegR11D */ NACL_REGISTER_UNDEFINED,
  /*              RegR12D */ NACL_REGISTER_UNDEFINED,
  /*              RegR13D */ NACL_REGISTER_UNDEFINED,
  /*              RegR14D */ NACL_REGISTER_UNDEFINED,
  /*              RegR15D */ NACL_REGISTER_UNDEFINED,
  /*                RegCS */ NACL_REGISTER_UNDEFINED,
  /*                RegDS */ NACL_REGISTER_UNDEFINED,
  /*                RegSS */ NACL_REGISTER_UNDEFINED,
  /*                RegES */ NACL_REGISTER_UNDEFINED,
  /*                RegFS */ NACL_REGISTER_UNDEFINED,
  /*                RegGS */ NACL_REGISTER_UNDEFINED,
  /*               RegCR0 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR1 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR2 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR3 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR4 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR5 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR6 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR7 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR8 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR9 */ NACL_REGISTER_UNDEFINED,
  /*              RegCR10 */ NACL_REGISTER_UNDEFINED,
  /*              RegCR11 */ NACL_REGISTER_UNDEFINED,
  /*              RegCR12 */ NACL_REGISTER_UNDEFINED,
  /*              RegCR13 */ NACL_REGISTER_UNDEFINED,
  /*              RegCR14 */ NACL_REGISTER_UNDEFINED,
  /*              RegCR15 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR0 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR1 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR2 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR3 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR4 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR5 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR6 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR7 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR8 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR9 */ NACL_REGISTER_UNDEFINED,
  /*              RegDR10 */ NACL_REGISTER_UNDEFINED,
  /*              RegDR11 */ NACL_REGISTER_UNDEFINED,
  /*              RegDR12 */ NACL_REGISTER_UNDEFINED,
  /*              RegDR13 */ NACL_REGISTER_UNDEFINED,
  /*              RegDR14 */ NACL_REGISTER_UNDEFINED,
  /*              RegDR15 */ NACL_REGISTER_UNDEFINED,
  /*            RegEFLAGS */ NACL_REGISTER_UNDEFINED,
  /*            RegRFLAGS */ NACL_REGISTER_UNDEFINED,
  /*               RegEIP */ NACL_REGISTER_UNDEFINED,
  /*               RegRIP */ NACL_REGISTER_UNDEFINED,
  /*               RegRAX */ NACL_REGISTER_UNDEFINED,
  /*               RegRBX */ NACL_REGISTER_UNDEFINED,
  /*               RegRCX */ NACL_REGISTER_UNDEFINED,
  /*               RegRDX */ NACL_REGISTER_UNDEFINED,
  /*               RegRSI */ NACL_REGISTER_UNDEFINED,
  /*               RegRDI */ NACL_REGISTER_UNDEFINED,
  /*               RegRBP */ NACL_REGISTER_UNDEFINED,
  /*               RegRSP */ NACL_REGISTER_UNDEFINED,
  /*                RegR8 */ NACL_REGISTER_UNDEFINED,
  /*                RegR9 */ NACL_REGISTER_UNDEFINED,
  /*               RegR10 */ NACL_REGISTER_UNDEFINED,
  /*               RegR11 */ NACL_REGISTER_UNDEFINED,
  /*               RegR12 */ NACL_REGISTER_UNDEFINED,
  /*               RegR13 */ NACL_REGISTER_UNDEFINED,
  /*               RegR14 */ NACL_REGISTER_UNDEFINED,
  /*               RegR15 */ NACL_REGISTER_UNDEFINED,
  /*              RegREIP */ NACL_REGISTER_UNDEFINED,
  /*              RegREAX */ NACL_REGISTER_UNDEFINED,
  /*              RegREBX */ NACL_REGISTER_UNDEFINED,
  /*              RegRECX */ NACL_REGISTER_UNDEFINED,
  /*              RegREDX */ NACL_REGISTER_UNDEFINED,
  /*              RegRESP */ NACL_REGISTER_UNDEFINED,
  /*              RegREBP */ NACL_REGISTER_UNDEFINED,
  /*              RegRESI */ NACL_REGISTER_UNDEFINED,
  /*              RegREDI */ NACL_REGISTER_UNDEFINED,
  /*             RegREAXa */ NACL_REGISTER_UNDEFINED,
  /*            RegDS_ESI */ NACL_REGISTER_UNDEFINED,
  /*            RegDS_EDI */ NACL_REGISTER_UNDEFINED,
  /*            RegES_EDI */ NACL_REGISTER_UNDEFINED,
  /*            RegDS_EBX */ NACL_REGISTER_UNDEFINED,
  /*               RegST0 */ NACL_REGISTER_UNDEFINED,
  /*               RegST1 */ NACL_REGISTER_UNDEFINED,
  /*               RegST2 */ NACL_REGISTER_UNDEFINED,
  /*               RegST3 */ NACL_REGISTER_UNDEFINED,
  /*               RegST4 */ NACL_REGISTER_UNDEFINED,
  /*               RegST5 */ NACL_REGISTER_UNDEFINED,
  /*               RegST6 */ NACL_REGISTER_UNDEFINED,
  /*               RegST7 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX0 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX1 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX2 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX3 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX4 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX5 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX6 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX7 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM0 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM1 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM2 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM3 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM4 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM5 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM6 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM7 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM8 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM9 */ NACL_REGISTER_UNDEFINED,
  /*             RegXMM10 */ NACL_REGISTER_UNDEFINED,
  /*             RegXMM11 */ NACL_REGISTER_UNDEFINED,
  /*             RegXMM12 */ NACL_REGISTER_UNDEFINED,
  /*             RegXMM13 */ NACL_REGISTER_UNDEFINED,
  /*             RegXMM14 */ NACL_REGISTER_UNDEFINED,
  /*             RegXMM15 */ NACL_REGISTER_UNDEFINED,
  /*               RegGP7 */ NACL_REGISTER_UNDEFINED,
  /*              Const_1 */ NACL_REGISTER_UNDEFINED,
};

static int NaClGpReg64Index[NaClOpKindEnumSize] = {
  /*      Unknown_Operand */ NACL_REGISTER_UNDEFINED,
  /*            A_Operand */ NACL_REGISTER_UNDEFINED,
  /*            E_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Eb_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Ew_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Ev_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Eo_Operand */ NACL_REGISTER_UNDEFINED,
  /*          Edq_Operand */ NACL_REGISTER_UNDEFINED,
  /*            G_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Gb_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Gw_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Gv_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Go_Operand */ NACL_REGISTER_UNDEFINED,
  /*          Gdq_Operand */ NACL_REGISTER_UNDEFINED,
  /*        Seg_G_Operand */ NACL_REGISTER_UNDEFINED,
  /*         G_OpcodeBase */ NACL_REGISTER_UNDEFINED,
  /*            I_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Ib_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Iw_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Iv_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Io_Operand */ NACL_REGISTER_UNDEFINED,
  /*           I2_Operand */ NACL_REGISTER_UNDEFINED,
  /*            J_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Jb_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Jw_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Jv_Operand */ NACL_REGISTER_UNDEFINED,
  /*            M_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Mb_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Mw_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Mv_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Mo_Operand */ NACL_REGISTER_UNDEFINED,
  /*          Mdq_Operand */ NACL_REGISTER_UNDEFINED,
  /*          Mpw_Operand */ NACL_REGISTER_UNDEFINED,
  /*          Mpv_Operand */ NACL_REGISTER_UNDEFINED,
  /*          Mpo_Operand */ NACL_REGISTER_UNDEFINED,
  /*        Mmx_E_Operand */ NACL_REGISTER_UNDEFINED,
  /*        Mmx_N_Operand */ NACL_REGISTER_UNDEFINED,
  /*        Mmx_G_Operand */ NACL_REGISTER_UNDEFINED,
  /*       Mmx_Gd_Operand */ NACL_REGISTER_UNDEFINED,
  /*        Xmm_E_Operand */ NACL_REGISTER_UNDEFINED,
  /*       Xmm_Eo_Operand */ NACL_REGISTER_UNDEFINED,
  /*        Xmm_G_Operand */ NACL_REGISTER_UNDEFINED,
  /*       Xmm_Go_Operand */ NACL_REGISTER_UNDEFINED,
  /*            C_Operand */ NACL_REGISTER_UNDEFINED,
  /*            D_Operand */ NACL_REGISTER_UNDEFINED,
  /*            O_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Ob_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Ow_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Ov_Operand */ NACL_REGISTER_UNDEFINED,
  /*           Oo_Operand */ NACL_REGISTER_UNDEFINED,
  /*            S_Operand */ NACL_REGISTER_UNDEFINED,
  /*           St_Operand */ NACL_REGISTER_UNDEFINED,
  /*           RegUnknown */ NACL_REGISTER_UNDEFINED,
  /*                RegAL */ NACL_REGISTER_UNDEFINED,
  /*                RegBL */ NACL_REGISTER_UNDEFINED,
  /*                RegCL */ NACL_REGISTER_UNDEFINED,
  /*                RegDL */ NACL_REGISTER_UNDEFINED,
  /*                RegAH */ NACL_REGISTER_UNDEFINED,
  /*                RegBH */ NACL_REGISTER_UNDEFINED,
  /*                RegCH */ NACL_REGISTER_UNDEFINED,
  /*                RegDH */ NACL_REGISTER_UNDEFINED,
  /*               RegDIL */ NACL_REGISTER_UNDEFINED,
  /*               RegSIL */ NACL_REGISTER_UNDEFINED,
  /*               RegBPL */ NACL_REGISTER_UNDEFINED,
  /*               RegSPL */ NACL_REGISTER_UNDEFINED,
  /*               RegR8B */ NACL_REGISTER_UNDEFINED,
  /*               RegR9B */ NACL_REGISTER_UNDEFINED,
  /*              RegR10B */ NACL_REGISTER_UNDEFINED,
  /*              RegR11B */ NACL_REGISTER_UNDEFINED,
  /*              RegR12B */ NACL_REGISTER_UNDEFINED,
  /*              RegR13B */ NACL_REGISTER_UNDEFINED,
  /*              RegR14B */ NACL_REGISTER_UNDEFINED,
  /*              RegR15B */ NACL_REGISTER_UNDEFINED,
  /*                RegAX */ NACL_REGISTER_UNDEFINED,
  /*                RegBX */ NACL_REGISTER_UNDEFINED,
  /*                RegCX */ NACL_REGISTER_UNDEFINED,
  /*                RegDX */ NACL_REGISTER_UNDEFINED,
  /*                RegSI */ NACL_REGISTER_UNDEFINED,
  /*                RegDI */ NACL_REGISTER_UNDEFINED,
  /*                RegBP */ NACL_REGISTER_UNDEFINED,
  /*                RegSP */ NACL_REGISTER_UNDEFINED,
  /*               RegR8W */ NACL_REGISTER_UNDEFINED,
  /*               RegR9W */ NACL_REGISTER_UNDEFINED,
  /*              RegR10W */ NACL_REGISTER_UNDEFINED,
  /*              RegR11W */ NACL_REGISTER_UNDEFINED,
  /*              RegR12W */ NACL_REGISTER_UNDEFINED,
  /*              RegR13W */ NACL_REGISTER_UNDEFINED,
  /*              RegR14W */ NACL_REGISTER_UNDEFINED,
  /*              RegR15W */ NACL_REGISTER_UNDEFINED,
  /*               RegEAX */ NACL_REGISTER_UNDEFINED,
  /*               RegEBX */ NACL_REGISTER_UNDEFINED,
  /*               RegECX */ NACL_REGISTER_UNDEFINED,
  /*               RegEDX */ NACL_REGISTER_UNDEFINED,
  /*               RegESI */ NACL_REGISTER_UNDEFINED,
  /*               RegEDI */ NACL_REGISTER_UNDEFINED,
  /*               RegEBP */ NACL_REGISTER_UNDEFINED,
  /*               RegESP */ NACL_REGISTER_UNDEFINED,
  /*               RegR8D */ NACL_REGISTER_UNDEFINED,
  /*               RegR9D */ NACL_REGISTER_UNDEFINED,
  /*              RegR10D */ NACL_REGISTER_UNDEFINED,
  /*              RegR11D */ NACL_REGISTER_UNDEFINED,
  /*              RegR12D */ NACL_REGISTER_UNDEFINED,
  /*              RegR13D */ NACL_REGISTER_UNDEFINED,
  /*              RegR14D */ NACL_REGISTER_UNDEFINED,
  /*              RegR15D */ NACL_REGISTER_UNDEFINED,
  /*                RegCS */ NACL_REGISTER_UNDEFINED,
  /*                RegDS */ NACL_REGISTER_UNDEFINED,
  /*                RegSS */ NACL_REGISTER_UNDEFINED,
  /*                RegES */ NACL_REGISTER_UNDEFINED,
  /*                RegFS */ NACL_REGISTER_UNDEFINED,
  /*                RegGS */ NACL_REGISTER_UNDEFINED,
  /*               RegCR0 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR1 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR2 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR3 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR4 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR5 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR6 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR7 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR8 */ NACL_REGISTER_UNDEFINED,
  /*               RegCR9 */ NACL_REGISTER_UNDEFINED,
  /*              RegCR10 */ NACL_REGISTER_UNDEFINED,
  /*              RegCR11 */ NACL_REGISTER_UNDEFINED,
  /*              RegCR12 */ NACL_REGISTER_UNDEFINED,
  /*              RegCR13 */ NACL_REGISTER_UNDEFINED,
  /*              RegCR14 */ NACL_REGISTER_UNDEFINED,
  /*              RegCR15 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR0 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR1 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR2 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR3 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR4 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR5 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR6 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR7 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR8 */ NACL_REGISTER_UNDEFINED,
  /*               RegDR9 */ NACL_REGISTER_UNDEFINED,
  /*              RegDR10 */ NACL_REGISTER_UNDEFINED,
  /*              RegDR11 */ NACL_REGISTER_UNDEFINED,
  /*              RegDR12 */ NACL_REGISTER_UNDEFINED,
  /*              RegDR13 */ NACL_REGISTER_UNDEFINED,
  /*              RegDR14 */ NACL_REGISTER_UNDEFINED,
  /*              RegDR15 */ NACL_REGISTER_UNDEFINED,
  /*            RegEFLAGS */ NACL_REGISTER_UNDEFINED,
  /*            RegRFLAGS */ NACL_REGISTER_UNDEFINED,
  /*               RegEIP */ NACL_REGISTER_UNDEFINED,
  /*               RegRIP */ NACL_REGISTER_UNDEFINED,
  /*               RegRAX */ NACL_REGISTER_UNDEFINED,
  /*               RegRBX */ NACL_REGISTER_UNDEFINED,
  /*               RegRCX */ NACL_REGISTER_UNDEFINED,
  /*               RegRDX */ NACL_REGISTER_UNDEFINED,
  /*               RegRSI */ NACL_REGISTER_UNDEFINED,
  /*               RegRDI */ NACL_REGISTER_UNDEFINED,
  /*               RegRBP */ NACL_REGISTER_UNDEFINED,
  /*               RegRSP */ NACL_REGISTER_UNDEFINED,
  /*                RegR8 */ NACL_REGISTER_UNDEFINED,
  /*                RegR9 */ NACL_REGISTER_UNDEFINED,
  /*               RegR10 */ NACL_REGISTER_UNDEFINED,
  /*               RegR11 */ NACL_REGISTER_UNDEFINED,
  /*               RegR12 */ NACL_REGISTER_UNDEFINED,
  /*               RegR13 */ NACL_REGISTER_UNDEFINED,
  /*               RegR14 */ NACL_REGISTER_UNDEFINED,
  /*               RegR15 */ NACL_REGISTER_UNDEFINED,
  /*              RegREIP */ NACL_REGISTER_UNDEFINED,
  /*              RegREAX */ NACL_REGISTER_UNDEFINED,
  /*              RegREBX */ NACL_REGISTER_UNDEFINED,
  /*              RegRECX */ NACL_REGISTER_UNDEFINED,
  /*              RegREDX */ NACL_REGISTER_UNDEFINED,
  /*              RegRESP */ NACL_REGISTER_UNDEFINED,
  /*              RegREBP */ NACL_REGISTER_UNDEFINED,
  /*              RegRESI */ NACL_REGISTER_UNDEFINED,
  /*              RegREDI */ NACL_REGISTER_UNDEFINED,
  /*             RegREAXa */ NACL_REGISTER_UNDEFINED,
  /*            RegDS_ESI */ NACL_REGISTER_UNDEFINED,
  /*            RegDS_EDI */ NACL_REGISTER_UNDEFINED,
  /*            RegES_EDI */ NACL_REGISTER_UNDEFINED,
  /*            RegDS_EBX */ NACL_REGISTER_UNDEFINED,
  /*               RegST0 */ NACL_REGISTER_UNDEFINED,
  /*               RegST1 */ NACL_REGISTER_UNDEFINED,
  /*               RegST2 */ NACL_REGISTER_UNDEFINED,
  /*               RegST3 */ NACL_REGISTER_UNDEFINED,
  /*               RegST4 */ NACL_REGISTER_UNDEFINED,
  /*               RegST5 */ NACL_REGISTER_UNDEFINED,
  /*               RegST6 */ NACL_REGISTER_UNDEFINED,
  /*               RegST7 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX0 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX1 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX2 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX3 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX4 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX5 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX6 */ NACL_REGISTER_UNDEFINED,
  /*              RegMMX7 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM0 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM1 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM2 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM3 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM4 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM5 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM6 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM7 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM8 */ NACL_REGISTER_UNDEFINED,
  /*              RegXMM9 */ NACL_REGISTER_UNDEFINED,
  /*             RegXMM10 */ NACL_REGISTER_UNDEFINED,
  /*             RegXMM11 */ NACL_REGISTER_UNDEFINED,
  /*             RegXMM12 */ NACL_REGISTER_UNDEFINED,
  /*             RegXMM13 */ NACL_REGISTER_UNDEFINED,
  /*             RegXMM14 */ NACL_REGISTER_UNDEFINED,
  /*             RegXMM15 */ NACL_REGISTER_UNDEFINED,
  /*               RegGP7 */ NACL_REGISTER_UNDEFINED,
  /*              Const_1 */ NACL_REGISTER_UNDEFINED,
};

