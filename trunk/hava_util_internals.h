/*
 * HavaFun code component
 * Internal bits needed to implement the protocol to talk to Hava
 *
 * Copyright (C) 2009 Chris Elford
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, see <http://www.gnu.org/licenses>.
 *
 * Linking havafun statically or dynamically with other modules is making a 
 * combined work based on havafun. Thus, the terms and conditions of the GNU 
 * General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of havafun give 
 * you permission to combine havafun with free software programs or libraries 
 * that are released under the GNU LGPL and with code included in the standard 
 * release of "myhava.com Hava Player platform SDKs (e.g., linux, mac, windows)"
 * under "myhava.com's SDK license" (or modified versions of such code, with 
 * unchanged license). You may copy and distribute such a system following the 
 * terms of the GNU GPL for havafun and the licenses of the other code concerned
 * {, provided that you include the source code of that other code when and as 
 * the GNU GPL requires distribution of source code}.
 *
 * Note that people who make modified versions of havafun are not obligated to 
 * grant this special exception for their modified versions; it is their choice
 * whether to do so. The GNU General Public License gives permission to release
 * a modified version without this exception; this exception also makes it 
 * possible to release a modified version which carries forward this exception.
 *
 */
#ifndef HAVA_UTIL_INT_H_
#define HAVA_UTIL_INT_H_ 1

typedef struct FrameHeader {
  unsigned char  cmdhi;
  unsigned char  cmdlo;
  unsigned char  seqhi;             // hi order byte of frame sequence no
  unsigned char  seqlo;             // lo order byte of frame sequence no
  unsigned short s4;
  unsigned short s6;
  unsigned short s8;
  unsigned char  c10;
  unsigned char  stream_remaining;  // cntdown to 0 then send continue packet
  unsigned short c12;
  unsigned short c14;
  unsigned char  payload;           // the payload starts here
} FrameHeader ;

// A MPEG file header
const unsigned char mpeg_hdr[51]={ 0x00,0x00,0x01,0xba,0x44,0x00,0x04,0x00,
                                   0x05,0xb1,0x09,0x27,0xc3,0xf8,0x00,0x00,
                                   0x01,0xbb,0x00,0x0c,0x80,0xc4,0xe1,0x04,
                                   0xe1,0xff,0xe0,0xe0,0xe8,0xc0,0xc0,0x20,
                                   0x00,0x00,0x01,0xe0,0x07,0xec,0x80,0xc1,
                                   0x0d,0x31,0x00,0x01,0x51,0xe9,0x11,0x00,
                                   0x01,0x22,0xfb};

// Initialization packet
const unsigned char init_pkt[24]={ 0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,
                                   0x00,0x1d,0x6a,0xe1,0x2a,0x93,0x00,0x00,
                                   0x00,0x00,0x00,0x09,0x00,0x1f,0x06,0xd9, };

// Basic form of a channel changing packet
/* jason
const unsigned char channel_set[32]={ 0x03,0x07,0xfa,0xce,0x00,0x00,0x00,0x14,
                                      0x00,0x00,0x00,0xa8,0x00,0x00,0x06,0x69,
                                      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08, 
                                      0xde,0xad,0xbe,0xef,0x00,0x00,0x00,0x5c };
*/
const unsigned char channel_set[32]={ 0x03,0x07,0xfa,0xce,0x00,0x00,0x00,0x14,
                                      0x00,0x00,0x00,0x10,0x00,0x00,0x06,0x69,
                                      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08, 
                                      0xde,0xad,0xbe,0xef,0x00,0x00,0x00,0x03 };
#define CHANNEL_OFFSET 24

// Basic form of a remote control button push
const unsigned char button_push[56]={ 0x03,0x07,0xfa,0xce,0x00,0x00,0x00,0x14,
                                      0x00,0x00,0x00,0x0b,0x00,0x00,0x08,0x23,
                                      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,
                                      0x08,0x08,0x00,0x01,0x00,0x01,0x00,0x00,
                                      0x00,0x00,0x00,0x0f,0x00,0x10,0x00,0x00,
                                      0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,
                                      0x07,0x01,0x33,0x07,0x01,0x80,0x00,0x00 };
#define BUTTON_OFFSET 52

// Basic form of a start packet
const unsigned char start_pkt[8]={ 0x03,0x0c,0xfe,0xed,0x00,0x00,0x00,0x02, };

// Basic form of a continuation packet
//
// 0xff,0xff should be replaced with the next sequence number
// 0x20,021,0x21 are the number of packets allowed before next continuation
// For some reason, it is an unsigned byte repeated as X-1,X,X
//
const unsigned char continue_pkt[22]={ 0x03,0x03,0xff,0xff,0x0f,0xb1,0xda,0xbc,
                                       0x00,0x03,0x92,0x94,0x00,0x00,0x74,0x90,
                                       0x20,0x21,0x21,0x04,0x00,0x00, };
#define SEQ_OFFSET 2

#define HAVA_MAXTRIES 8
#define HAVA_BUFFSIZE 4096

const char *Hava_buttons[256]={
  0, // 0(0x00)
  "Power",      // 1(0x01)
  "PowerOn",    // 2(0x02)
  "PowerOff",   // 3(0x03)
  0, // 4(0x04)
  0, // 5(0x05)
  0, // 6(0x06)
  0, // 7(0x07)
  0, // 8(0x08)
  0, // 9(0x09)
  0, // 10(0x0a)
  0, // 11(0x0b)
  0, // 12(0x0c)
  0, // 13(0x0d)
  0, // 14(0x0e)
  0, // 15(0x0f)
  0, // 16(0x10)
  0, // 17(0x11)
  0, // 18(0x12)
  0, // 19(0x13)
  0, // 20(0x14)
  0, // 21(0x15)
  0, // 22(0x16)
  0, // 23(0x17)
  0, // 24(0x18)
  0, // 25(0x19)
  0, // 26(0x1a)
  0, // 27(0x1b)
  0, // 28(0x1c)
  0, // 29(0x1d)
  0, // 30(0x1e)
  0, // 31(0x1f)
  0, // 32(0x20)
  0, // 33(0x21)
  0, // 34(0x22)
  0, // 35(0x23)
  0, // 36(0x24)
  0, // 37(0x25)
  0, // 38(0x26)
  0, // 39(0x27)
  0, // 40(0x28)
  0, // 41(0x29)
  "Select", // 42(0x2a)
  0, // 43(0x2b)
  0, // 44(0x2c)
  0, // 45(0x2d)
  0, // 46(0x2e)
  0, // 47(0x2f)
  0, // 48(0x30)
  0, // 49(0x31)
  0, // 50(0x32)
  0, // 51(0x33)
  0, // 52(0x34)
  0, // 53(0x35)
  0, // 54(0x36)
  0, // 55(0x37)
  0, // 56(0x38)
  0, // 57(0x39)
  0, // 58(0x3a)
  0, // 59(0x3b)
  0, // 60(0x3c)
  0, // 61(0x3d)
  0, // 62(0x3e)
  0, // 63(0x3f)
  0, // 64(0x40)
  0, // 65(0x41)
  0, // 66(0x42)
  0, // 67(0x43)
  0, // 68(0x44)
  0, // 69(0x45)
  0, // 70(0x46)
  0, // 71(0x47)
  0, // 72(0x48)
  0, // 73(0x49)
  0, // 74(0x4a)
  0, // 75(0x4b)
  0, // 76(0x4c)
  0, // 77(0x4d)
  0, // 78(0x4e)
  0, // 79(0x4f)
  0, // 80(0x50)
  0, // 81(0x51)
  0, // 82(0x52)
  0, // 83(0x53)
  0, // 84(0x54)
  0, // 85(0x55)
  0, // 86(0x56)
  0, // 87(0x57)
  0, // 88(0x58)
  0, // 89(0x59)
  0, // 90(0x5a)
  0, // 91(0x5b)
  0, // 92(0x5c)
  0, // 93(0x5d)
  0, // 94(0x5e)
  0, // 95(0x5f)
  0, // 96(0x60)
  0, // 97(0x61)
  0, // 98(0x62)
  0, // 99(0x63)
  0, // 100(0x64)
  0, // 101(0x65)
  0, // 102(0x66)
  0, // 103(0x67)
  0, // 104(0x68)
  0, // 105(0x69)
  0, // 106(0x6a)
  0, // 107(0x6b)
  0, // 108(0x6c)
  0, // 109(0x6d)
  0, // 110(0x6e)
  0, // 111(0x6f)
  0, // 112(0x70)
  0, // 113(0x71)
  0, // 114(0x72)
  0, // 115(0x73)
  0, // 116(0x74)
  0, // 117(0x75)
  0, // 118(0x76)
  0, // 119(0x77)
  0, // 120(0x78)
  0, // 121(0x79)
  0, // 122(0x7a)
  0, // 123(0x7b)
  0, // 124(0x7c)
  0, // 125(0x7d)
  0, // 126(0x7e)
  0, // 127(0x7f)
  0, // 128(0x80)
  0, // 129(0x81)
  0, // 130(0x82)
  0, // 131(0x83)
  0, // 132(0x84)
  0, // 133(0x85)
  0, // 134(0x86)
  0, // 135(0x87)
  0, // 136(0x88)
  0, // 137(0x89)
  0, // 138(0x8a)
  0, // 139(0x8b)
  0, // 140(0x8c)
  0, // 141(0x8d)
  0, // 142(0x8e)
  0, // 143(0x8f)
  0, // 144(0x90)
  0, // 145(0x91)
  0, // 146(0x92)
  0, // 147(0x93)
  0, // 148(0x94)
  0, // 149(0x95)
  0, // 150(0x96)
  0, // 151(0x97)
  0, // 152(0x98)
  0, // 153(0x99)
  0, // 154(0x9a)
  0, // 155(0x9b)
  0, // 156(0x9c)
  0, // 157(0x9d)
  0, // 158(0x9e)
  0, // 159(0x9f)
  0, // 160(0xa0)
  0, // 161(0xa1)
  0, // 162(0xa2)
  0, // 163(0xa3)
  0, // 164(0xa4)
  0, // 165(0xa5)
  0, // 166(0xa6)
  0, // 167(0xa7)
  0, // 168(0xa8)
  0, // 169(0xa9)
  0, // 170(0xaa)
  0, // 171(0xab)
  0, // 172(0xac)
  0, // 173(0xad)
  0, // 174(0xae)
  0, // 175(0xaf)
  0, // 176(0xb0)
  0, // 177(0xb1)
  0, // 178(0xb2)
  0, // 179(0xb3)
  0, // 180(0xb4)
  0, // 181(0xb5)
  0, // 182(0xb6)
  0, // 183(0xb7)
  0, // 184(0xb8)
  0, // 185(0xb9)
  0, // 186(0xba)
  0, // 187(0xbb)
  0, // 188(0xbc)
  0, // 189(0xbd)
  0, // 190(0xbe)
  0, // 191(0xbf)
  0, // 192(0xc0)
  0, // 193(0xc1)
  0, // 194(0xc2)
  0, // 195(0xc3)
  0, // 196(0xc4)
  0, // 197(0xc5)
  0, // 198(0xc6)
  0, // 199(0xc7)
  0, // 200(0xc8)
  0, // 201(0xc9)
  0, // 202(0xca)
  0, // 203(0xcb)
  0, // 204(0xcc)
  0, // 205(0xcd)
  0, // 206(0xce)
  0, // 207(0xcf)
  0, // 208(0xd0)
  0, // 209(0xd1)
  0, // 210(0xd2)
  0, // 211(0xd3)
  0, // 212(0xd4)
  0, // 213(0xd5)
  0, // 214(0xd6)
  0, // 215(0xd7)
  0, // 216(0xd8)
  0, // 217(0xd9)
  0, // 218(0xda)
  0, // 219(0xdb)
  0, // 220(0xdc)
  0, // 221(0xdd)
  0, // 222(0xde)
  0, // 223(0xdf)
  0, // 224(0xe0)
  0, // 225(0xe1)
  0, // 226(0xe2)
  0, // 227(0xe3)
  0, // 228(0xe4)
  0, // 229(0xe5)
  0, // 230(0xe6)
  0, // 231(0xe7)
  0, // 232(0xe8)
  0, // 233(0xe9)
  0, // 234(0xea)
  0, // 235(0xeb)
  0, // 236(0xec)
  0, // 237(0xed)
  0, // 238(0xee)
  0, // 239(0xef)
  0, // 240(0xf0)
  0, // 241(0xf1)
  0, // 242(0xf2)
  0, // 243(0xf3)
  0, // 244(0xf4)
  0, // 245(0xf5)
  0, // 246(0xf6)
  0, // 247(0xf7)
  0, // 248(0xf8)
  0, // 249(0xf9)
  0, // 250(0xfa)
  0, // 251(0xfb)
  0, // 252(0xfc)
  0, // 253(0xfd)
  0, // 254(0xfe)
  0, // 255(0xff)
} ;

#endif
