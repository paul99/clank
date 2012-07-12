/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <sstream>
#include <vector>

#include "native_client/src/debug_server/gdb_rsp/packet.h"
#include "native_client/src/debug_server/gdb_rsp/test.h"

using gdb_rsp::Packet;

int VerifyPacket(Packet *wr, Packet *rd, void *ctx, PacketFunc_t tx) {
  int errs = 0;
  char ch;
  std::string str;

  uint8_t  byte = 0;
  uint16_t word16 = 0;
  uint32_t word32 = 0;

  // Clear the packet may be reused
  wr->Clear();

  // Verify non alligned writes
  wr->AddRawChar('X');
  wr->AddWord16(0x1234);
  wr->AddWord32(0x56789ABC);
  wr->AddWord8(0xFF);
  if (tx) tx(ctx, wr, rd);
  rd->GetString(&str);
  if (strcmp("X3412bc9a7856ff", str.data())) errs++;

  // Verify rewind
  rd->Rewind();
  rd->GetRawChar(&ch);
  if (ch != 'X') errs++;

  rd->GetWord16(&word16);
  if (word16 != 0x1234) errs++;

  rd->GetWord32(&word32);
  if (word32 != 0x56789ABC) errs++;

  rd->GetWord8(&byte);
  if (byte != 0xFF) errs++;

  // Check Empty Send
  wr->Clear();
  if (tx) tx(ctx, wr, rd);
  if (rd->GetRawChar(&ch)) {
    errs++;
    printf("Was expecting an empty packet\n");
  }

  // Check nibble/byte order
  // Check edge cases of 1, 0, -1, -2
  wr->AddNumberSep(0x123, ',');
  wr->AddNumberSep(1, ',');
  wr->AddNumberSep(0, ',');
  wr->AddNumberSep(static_cast<uint64_t>(-1), ',');
  wr->AddNumberSep(static_cast<uint64_t>(-2), 0);
  if (tx) tx(ctx, wr, rd);
  rd->GetString(&str);
  if (strcmp("123,1,0,-1,fffffffffffffffe", str.data()) != 0) errs++;

  // We push "compress" which was generated by a real gdbserver, through into
  // our packet, then pull it out as a block to decompress, then check against
  // the 32b values generated by a real gdb debugger. (Inline golden file)
  const char *compress = "0*488c7280038c72800d4c72800030**fd00637702020* 23"
                         "0*\"2b0*\"2b0*\"2b0*\"530*\"2b0*}00080f83f00c022f8"
                         "07c022f80f3c80040*&80ff3f0**80ff3f7f030* 30*\"0f*"
                         " 0* 58050* 85f7196c2b0*\"b0b801010*}0*}0*b801f0* ";
  uint32_t vals[16] = { 0, 0, 0, 0x28C788, 0x28C738, 0x28c7d4, 3, 0, 0x776300FD,
                        0x202, 0x23, 0x2B, 0x2B, 0x2B, 0x53, 0x2B };
  uint32_t tmp[16];

  wr->Clear();
  wr->AddString(compress);
  if (tx) tx(ctx, wr, rd);
  rd->GetBlock(tmp, sizeof(vals));
  if (memcmp(tmp, vals, sizeof(vals))) {
    errs++;
    printf("Failed to decompress as expected.\n");
  }

  if (errs)
    printf("FAILED PACKET TEST\n");

  return errs;
}

int TestPacket() {
  int errs = 0;
  Packet pkt;

  errs += VerifyPacket(&pkt, &pkt, NULL, NULL);
  return errs;
}
