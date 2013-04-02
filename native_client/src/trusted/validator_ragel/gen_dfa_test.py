#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys
import unittest

import def_format
import gen_dfa


class TestLegacyPrefixes(unittest.TestCase):

  def test_empty(self):
    self.assertEquals(
        gen_dfa.GenerateLegacyPrefixes([], []),
        [()])

  def test_one(self):
    self.assertEquals(
        gen_dfa.GenerateLegacyPrefixes(['req'], []),
        [('req',)])
    self.assertEquals(
        set(gen_dfa.GenerateLegacyPrefixes([], ['opt'])),
        set([
            (),
            ('opt',)]))

  def test_many(self):
    self.assertEquals(
        set(gen_dfa.GenerateLegacyPrefixes(['req'], ['opt1', 'opt2'])),
        set([
            ('req',),
            ('opt1', 'req'),
            ('opt2', 'req'),
            ('req', 'opt1'),
            ('req', 'opt2'),
            ('opt1', 'opt2', 'req'),
            ('opt1', 'req', 'opt2'),
            ('opt2', 'opt1', 'req'),
            ('opt2', 'req', 'opt1'),
            ('req', 'opt1', 'opt2'),
            ('req', 'opt2', 'opt1')]))


class TestOperand(unittest.TestCase):

  def test_default(self):
    op = gen_dfa.Operand.Parse(
        'r',
        default_rw=def_format.OperandReadWriteMode.READ)

    assert op.Readable() and not op.Writable()
    self.assertEquals(op.arg_type, def_format.OperandType.REGISTER_IN_OPCODE)
    self.assertEquals(op.size, '')
    assert not op.implicit

    self.assertEquals(str(op), '=r')


class TestInstruction(unittest.TestCase):

  def test_name_and_operands(self):
    instr = gen_dfa.Instruction()
    instr.ParseNameAndOperands('add G E')
    self.assertEquals(instr.name, 'add')
    self.assertEquals(len(instr.operands), 2)

    op1, op2 = instr.operands
    assert op1.Readable() and not op1.Writable()
    assert op2.Readable() and op2.Writable()

    self.assertEquals(str(instr), 'add =G &E')

  def test_modrm_presence(self):
    instr = gen_dfa.Instruction.Parse(
        'add G E, 0x00, lock nacl-amd64-zero-extends')
    assert instr.HasModRM()

    instr = gen_dfa.Instruction.Parse(
        'mov O !a, 0xa0, ia32')
    assert not instr.HasModRM()

    # Technically, mfence has ModRM byte (0xf0), but for our purposes
    # we treat it as part of opcode.
    instr = gen_dfa.Instruction.Parse(
        'mfence, 0x0f 0xae 0xf0, CPUFeature_SSE2')
    assert not instr.HasModRM()

  def test_collect_prefixes(self):
    instr = gen_dfa.Instruction.Parse('pause, 0xf3 0x90, norex')

    instr.CollectPrefixes()

    self.assertEquals(instr.required_prefixes, ['0xf3'])
    self.assertEquals(instr.opcodes, ['0x90'])


class TestPrinterParts(unittest.TestCase):

  def test_nop_signature(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        '"nopw   0x0(%eax,%eax,1)", 0x66 0x0f 0x1f 0x44 0x00 0x00, ia32 norex')

    printer._PrintSignature(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        @instruction_nopw___0x0__eax__eax_1_
        @operands_count_is_0
        """.split())

  def test_simple_mov_signature(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse('mov =Ob !ab, 0xa0, ia32')

    printer._PrintSignature(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        @instruction_mov
        @operands_count_is_2
        @operand0_8bit
        @operand1_8bit
        """.split())

  def test_nop_opcode(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'nop, 0x90, norex')

    printer._PrintOpcode(instr)

    self.assertEquals(printer.GetContent(), '0x90')

  def test_register_in_opcode(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'bswap ry, 0x0f 0xc8')

    printer._PrintOpcode(instr)

    self.assertEquals(
        printer.GetContent(),
        '0x0f (0xc8|0xc9|0xca|0xcb|0xcc|0xcd|0xce|0xcf)')

  def test_opcode_in_modrm(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'add I E, 0x80 /0, lock nacl-amd64-zero-extends')

    printer._PrintOpcode(instr)

    self.assertEquals(printer.GetContent(), '0x80')


class TestInstructionPrinter(unittest.TestCase):

  def test_no_modrm(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse(
        'mov Ob !ab, 0xa0, ia32')

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        0xa0
        @instruction_mov
        @operands_count_is_2
        @operand0_8bit
        @operand1_8bit
        @operand1_rax
        @operand0_absolute_disp
        disp32
        """.split())

  def test_relative(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 32)
    instr = gen_dfa.Instruction.Parse('jmp Jb, 0xeb')

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        0xeb
        @instruction_jmp
        @operands_count_is_1
        @operand0_8bit
        rel8
        @operand0_jmp_to
        """.split())

  def test_no_modrm_with_rex(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 64)
    instr = gen_dfa.Instruction.Parse(
        'movabs Od !ad, 0xa0, amd64 nacl-forbidden')

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        REX_RXB?
        0xa0
        @instruction_movabs
        @operands_count_is_2
        @operand0_32bit
        @operand1_32bit
        @set_spurious_rex_b
        @set_spurious_rex_x
        @set_spurious_rex_r
        @operand1_rax
        @operand0_absolute_disp
        disp64
        """.split())

  def test_nop_with_rex(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 64)
    instr = gen_dfa.Instruction.Parse(
        'nop, 0x40 0x90, amd64 norex')

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        0x40 0x90
        @instruction_nop
        @operands_count_is_0
        @set_spurious_rex_b
        @set_spurious_rex_x
        @set_spurious_rex_r
        """.split())

  def test_with_prefixes(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.VALIDATOR, 32)
    instr = gen_dfa.Instruction.Parse(
        'movsb =Xb &Yb, 0xa4, rep')

    instr.CollectPrefixes()

    printer.PrintInstructionWithoutModRM(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        (rep)?
        0xa4
        """.split())

  def test_modrm_reg(self):
    printer = gen_dfa.InstructionPrinter(gen_dfa.DECODER, 64)
    instr = gen_dfa.Instruction.Parse(
        'mov Gw !Rw, 0x89')

    printer.PrintInstructionWithModRMReg(instr)

    self.assertEquals(
        printer.GetContent().split(),
        """
        REX_RXB?
        0x89
        @instruction_mov
        @operands_count_is_2
        @operand0_16bit
        @operand1_16bit
        modrm_registers
        @operand0_from_modrm_reg
        @operand1_from_modrm_rm
        @set_spurious_rex_x
        """.split())


class TestParser(unittest.TestCase):

  def test_instruction_definitions(self):
    def_filenames = sys.argv[1:]
    assert len(def_filenames) > 0
    for filename in def_filenames:
      gen_dfa.ParseDefFile(filename)


if __name__ == '__main__':
  unittest.main(argv=[sys.argv[0], '--verbose'])
