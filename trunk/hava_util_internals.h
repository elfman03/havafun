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

#endif
