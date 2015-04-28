/*******************************************************************************
Copyright (C) 2015 OLogN Technologies AG

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as 
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*******************************************************************************/


#if !defined __SACCP_PROTOCOL_CONSTANTS_H__
#define __SACCP_PROTOCOL_CONSTANTS_H__


// packet types
#define SACCP_PAIRING 0x0            /*Master: sends; Slave: receives*/
#define SACCP_PROGRAMMING 0x1        /*Master: sends; Slave: receives*/
#define SACCP_NEW_PROGRAM 0x2        /*Master: sends; Slave: receives*/
#define SACCP_REPEAT_OLD_PROGRAM 0x3 /*Master: sends; Slave: receives*/
#define SACCP_REUSE_OLD_PROGRAM 0x4  /*Master: sends; Slave: receives*/

#define SACCP_PAIRING_RESPONSE 0x7     /*Master: receives; Slave: sends*/
#define SACCP_PROGRAMMING_RESPONSE 0x6 /*Master: receives; Slave: sends*/
#define SACCP_REPLY_OK 0x0             /*Master: receives; Slave: sends*/
#define SACCP_REPLY_EXCEPTION 0x1      /*Master: receives; Slave: sends*/
#define SACCP_REPLY_ERROR 0x2          /*Master: receives; Slave: sends*/


// extra headers types
#define END_OF_HEADERS 0
#define ENABLE_ZEPTOERR 1


// error codes
#define SACCP_ERROR_INVALID_FORMAT 1
#define SACCP_ERROR_OLD_PROGRAM_CHECKSUM_DOESNT_MATCH 2


// op codes

#define ZEPTOVM_OP_DEVICECAPS 0x1
#define ZEPTOVM_OP_EXEC 0x2
#define ZEPTOVM_OP_PUSHREPLY 0x3
#define ZEPTOVM_OP_SLEEP 0x4
#define ZEPTOVM_OP_TRANSMITTER 0x5
#define ZEPTOVM_OP_MCUSLEEP 0x6
#define ZEPTOVM_OP_POPREPLIES 0x7  /* limited support in Zepto VM-One, full support from Zepto VM-Tiny */
#define ZEPTOVM_OP_EXIT 0x8
#define ZEPTOVM_OP_APPENDTOREPLY 0x9 /* limited support in Zepto VM-One, full support from Zepto VM-Tiny */
/* starting from the next opcode, instructions are not supported by Zepto VM-One */
#define ZEPTOVM_OP_JMP 0xA
#define ZEPTOVM_OP_JMPIFREPLYFIELD_LT 0xB
#define ZEPTOVM_OP_JMPIFREPLYFIELD_GT 0xC
#define ZEPTOVM_OP_JMPIFREPLYFIELD_EQ 0xD
#define ZEPTOVM_OP_JMPIFREPLYFIELD_NE 0xE
#define ZEPTOVM_OP_MOVEREPLYTOFRONT 0xF
/* starting from the next opcode, instructions are not supported by Zepto VM-Tiny and below */
#define ZEPTOVM_OP_PUSHEXPR_CONSTANT 0x10
#define ZEPTOVM_OP_PUSHEXPR_REPLYFIELD 0x11
#define ZEPTOVM_OP_EXPRUNOP 0x12
#define ZEPTOVM_OP_EXPRUNOP_EX 0x13
#define ZEPTOVM_OP_EXPRUNOP_EX2 0x14
#define ZEPTOVM_OP_EXPRBINOP 0x15
#define ZEPTOVM_OP_EXPRBINOP_EX 0x16
#define ZEPTOVM_OP_EXPRBINOP_EX2 0x17
#define ZEPTOVM_OP_JMPIFEXPR_LT 0x18
#define ZEPTOVM_OP_JMPIFEXPR_GT 0x19
#define ZEPTOVM_OP_JMPIFEXPR_EQ 0x1A
#define ZEPTOVM_OP_JMPIFEXPR_NE 0x1B
#define ZEPTOVM_OP_JMPIFEXPR_EX_LT 0x1C
#define ZEPTOVM_OP_JMPIFEXPR_EX_GT 0x1D
#define ZEPTOVM_OP_JMPIFEXPR_EX_EQ 0x1E
#define ZEPTOVM_OP_JMPIFEXPR_EX_NE 0x1F
#define ZEPTOVM_OP_CALL 0x20
#define ZEPTOVM_OP_RET 0x21
#define ZEPTOVM_OP_SWITCH 0x22
#define ZEPTOVM_OP_SWITCH_EX 0x23
#define ZEPTOVM_OP_INCANDJMPIF 0x24
#define ZEPTOVM_OP_DECANDJMPIF 0x25
/* starting from the next opcode, instructions are not supported by Zepto VM-Small and below */
#define ZEPTOVM_OP_PARALLEL 0x26


#endif