/**************************************************************************
 	This file is part of Minimalist Casio Assembler (MCA).

    MCA is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MCA is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MCA.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/


/**
  * This file contain many opcodes-related definitions for the Minimalist Casio Assembler.
  * 
  */

#ifndef MCA_OPCODES_H
#define MCA_OPCODES_H


// Definition the registers R0~R15 :
#define REG_R0	(0b0000)
#define REG_R1	(0b0001)
#define REG_R2	(0b0010)
#define REG_R3	(0b0011)
#define REG_R4	(0b0100)
#define REG_R5	(0b0101)
#define REG_R6	(0b0110)
#define REG_R7	(0b0111)
#define REG_R8	(0b1000)
#define REG_R9	(0b1001)
#define REG_R10	(0b1010)
#define REG_R11	(0b1011)
#define REG_R12	(0b1100)
#define REG_R13	(0b1101)
#define REG_R14	(0b1110)
#define REG_R15	(0b1111)



// Definition of the SH3 opcodes :
/**********************************
	Data-transfer instructions
 **********************************/
/* MOV		#imm,Rn */
#define OPC_MOV_IMM_RN(imm, rn)				(0b1110000000000000 | ((imm)&0x00FF) | (((rn)<<8)&0x0F00))
/* MOV.W	@(disp,PC),Rn */
#define OPC_MOVW_ATDISP_RN(disp, rn)		(0b1001000000000000 | (((disp)>>1)&0x00FF) | (((rn)<<8)&0x0F00))
/* MOV.L	@(disp,PC),Rn */
#define OPC_MOVL_ATDISP_RN(disp, rn)		(0b1101000000000000 | (((disp)>>2)&0x00FF) | (((rn)<<8)&0x0F00))
/* MOV 		Rm,Rn */
#define OPC_MOV_RM_RN(rm, rn)				(0b0110000000000011 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))

/* MOV.x	Rm,@Rn */
#define OPC_MOVB_RM_ATRN(rm, rn)			(0b0010000000000000 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
#define OPC_MOVW_RM_ATRN(rm, rn)			(0b0010000000000001 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
#define OPC_MOVL_RM_ATRN(rm, rn)			(0b0010000000000010 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* MOV.x	Rm,@-Rn */
#define OPC_MOVB_RM_ATDECRN(rm, rn)			(0b0010000000000100 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
#define OPC_MOVW_RM_ATDECRN(rm, rn)			(0b0010000000000101 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
#define OPC_MOVL_RM_ATDECRN(rm, rn)			(0b0010000000000110 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))

/* MOV.x	@Rm,Rn */
#define OPC_MOVB_ATRM_RN(rm, rn)			(0b0110000000000000 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
#define OPC_MOVW_ATRM_RN(rm, rn)			(0b0110000000000001 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
#define OPC_MOVL_ATRM_RN(rm, rn)			(0b0110000000000010 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* MOV.w	@Rm+,Rn */
#define OPC_MOVB_ATRMINC_RN(rm, rn)			(0b0110000000000100 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
#define OPC_MOVW_ATRMINC_RN(rm, rn)			(0b0110000000000101 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
#define OPC_MOVL_ATRMINC_RN(rm, rn)			(0b0110000000000110 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))

//Don't forget the displacement address are converted by divide by 1 for byte, 2 for word, and 4 for long...
/* MOV.B	R0,@(disp,Rn) */
#define OPC_MOVB_R0_ATDISPRN(disp, rn)		(0b1000000000000000 | (((disp)>>0)&0x000F) | (((rn)<<4)&0x00F0))
/* MOV.W	R0,@(disp,Rn) */
#define OPC_MOVW_R0_ATDISPRN(disp, rn)		(0b1000000100000000 | (((disp)>>1)&0x000F) | (((rn)<<4)&0x00F0))
/* MOV.L 	Rm,@(disp,Rn) */
#define OPC_MOVL_RM_ATDISPRN(rm, disp, rn)	(0b0001000000000000 | (((rm)<<4)&0x00F0) | (((disp)>>2)&0x000F) | (((rn)<<8)&0x0F00))

/* MOV.B	@(disp,Rm),R0 */
#define OPC_MOVB_ATDISPRM_R0(disp, rm)		(0b1000010000000000 | (((disp)>>0)&0x000F) | (((rm)<<4)&0x00F0))
/* MOV.W	@(disp,Rm),R0 */
#define OPC_MOVW_ATDISPRM_R0(disp, rm)		(0b1000010100000000 | (((disp)>>1)&0x000F) | (((rm)<<4)&0x00F0))
/* MOV.L 	@(disp,Rm),Rn */
#define OPC_MOVL_ATDISPRM_RN(disp, rm, rn)	(0b0101000000000000 | (((rm)<<4)&0x00F0) | (((disp)>>2)&0x000F) | (((rn)<<8)&0x0F00))

/* MOV.x	Rm,@(R0,Rn) */
#define OPC_MOVB_RM_ATR0RN(rm, rn)			(0b0000000000000100 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
#define OPC_MOVW_RM_ATR0RN(rm, rn)			(0b0000000000000101 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
#define OPC_MOVL_RM_ATR0RN(rm, rn)			(0b0000000000000110 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* MOV.x	@(R0,Rm),Rn */
#define OPC_MOVB_ATR0RM_RN(rm, rn)			(0b0000000000001100 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
#define OPC_MOVW_ATR0RM_RN(rm, rn)			(0b0000000000001101 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
#define OPC_MOVL_ATR0RM_RN(rm, rn)			(0b0000000000001110 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))

/* MOV.x	R0,@(disp,GBR) */
#define OPC_MOVB_R0_ATDISPGBR(disp)			(0b1100000000000000 | (((disp)>>0)&0x00FF))
#define OPC_MOVW_R0_ATDISPGBR(disp)			(0b1100000100000000 | (((disp)>>1)&0x00FF))
#define OPC_MOVL_R0_ATDISPGBR(disp)			(0b1100001000000000 | (((disp)>>2)&0x00FF))
/* MOV.x	@(disp,GBR),R0 */
#define OPC_MOVB_ATDISPGBR_R0(disp)			(0b1100010000000000 | (((disp)>>0)&0x00FF))
#define OPC_MOVW_ATDISPGBR_R0(disp)			(0b1100010100000000 | (((disp)>>1)&0x00FF))
#define OPC_MOVL_ATDISPGBR_R0(disp)			(0b1100011000000000 | (((disp)>>2)&0x00FF))

/* MOVA		@(disp,PC),R0 */
#define OPC_MOVA(disp)						(0b1100011100000000 | (((disp)>>2)&0x00FF))
/* MOVT		Rn */
#define OPC_MOVT(rn)						(0b0000000000101001 | (((rn)<<8)&0x0F00))
/* PREF		@Rn */
#define OPC_PREF(rn)						(0b0000000010000011 | (((rn)<<8)&0x0F00))
/* SWAP.B	Rm,Rn */
#define OPC_SWAPB(rm, rn)					(0b0110000000001000 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* SWAP.W	Rm,Rn */
#define OPC_SWAPW(rm, rn)					(0b0110000000001001 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* XTRCT	Rm,Rn */
#define OPC_XTRCT(rm, rn)					(0b0010000000001101 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))

/**********************************
	Arithmetic Instructions
 **********************************/
/* ADD		Rm,Rn */
#define OPC_ADD(rm, rn)						(0b0011000000001100 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* ADD		#imm,Rn */
#define OPC_ADD_IMM(imm, rn)				(0b0111000000000000 | ((imm)&0x00FF) | (((rn)<<8)&0x0F00))
/* ADDC		Rm,Rn */
#define OPC_ADDC(rm, rn)					(0b0011000000001110 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* ADDV		Rm,Rn */
#define OPC_ADDV(rm, rn)					(0b0011000000001111 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* CMP/EQ	#imm,R0 */
#define OPC_CMP_EQ_IMM(imm)					(0b1000100000000000 | ((imm)&0x00FF))
/* CMP/EQ	Rm,Rn */
#define OPC_CMP_EQ(rm, rn)					(0b0011000000000000 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* CMP/HS	Rm,Rn */
#define OPC_CMP_HS(rm, rn)					(0b0011000000000010 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* CMP/GE	Rm,Rn */
#define OPC_CMP_GE(rm, rn)					(0b0011000000000011 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* CMP/HI	Rm,Rn */
#define OPC_CMP_HI(rm, rn)					(0b0011000000000110 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* CMP/GT	Rm,Rn */
#define OPC_CMP_GT(rm, rn)					(0b0011000000000111 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* CMP/PZ	Rn */
#define OPC_CMP_PZ(rn)						(0b0100000000010001 | (((rn)<<8)&0x0F00))
/* CMP/PL	Rn */
#define OPC_CMP_PL(rn)						(0b0100000000010101 | (((rn)<<8)&0x0F00))
/* CMP/STR	Rm,Rn */
#define OPC_CMP_STR(rm, rn)					(0b0010000000001100 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* DIV1		Rm,Rn */
#define OPC_DIV1(rm, rn)					(0b0011000000000100 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* DIV0S	Rm,Rn */
#define OPC_DIV0S(rm, rn)					(0b0010000000000111 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* DIV0U	 */
#define OPC_DIV0U							(0b0000000000011001)
/* DMULS.L	Rm,Rn */
#define OPC_DMULSL(rm, rn)					(0b0011000000001101 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* DMULU.L	Rm,Rn */
#define OPC_DMULUL(rm, rn)					(0b0011000000000101 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* DT		Rn */
#define OPC_DT(rn)							(0b0100000000010000 | (((rn)<<8)&0x0F00))
/* EXTS.B	Rm,Rn */
#define OPC_EXTSB(rm, rn)					(0b0110000000001110 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* EXTS.W	Rm,Rn */
#define OPC_EXTSW(rm, rn)					(0b0110000000001111 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* EXTU.B	Rm,Rn */
#define OPC_EXTUB(rm, rn)					(0b0110000000001100 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* EXTU.W	Rm,Rn */
#define OPC_EXTUW(rm, rn)					(0b0110000000001101 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* MAC.L	@Rm+,@Rn+ */
#define OPC_MACL(rm, rn)					(0b0000000000001111 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* MAC.W	@Rm+,@Rn+ */
#define OPC_MACW(rm, rn)					(0b0100000000001111 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* MUL.L	Rm,Rn */
#define OPC_MULL(rm, rn)					(0b0000000000000111 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* MULU.W	Rm,Rn */
#define OPC_MULUW(rm, rn)					(0b0010000000001111 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* MULS.L	Rm,Rn */
#define OPC_MULSW(rm, rn)					(0b0010000000001110 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* NEG		Rm,Rn */
#define OPC_NEG(rm, rn)						(0b0110000000001011 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* NEGC		Rm,Rn */
#define OPC_NEGC(rm, rn)					(0b0110000000001010 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* SUB		Rm,Rn */
#define OPC_SUB(rm, rn)						(0b0011000000001000 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* SUBC		Rm,Rn */
#define OPC_SUBC(rm, rn)					(0b0011000000001010 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* SUBV		Rm,Rn */
#define OPC_SUBV(rm, rn)					(0b0011000000001011 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))

/**********************************
	Logic Operation Instructions
 **********************************/
/* AND		Rm,Rn */
#define OPC_AND(rm, rn)						(0b0010000000001001  | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* AND		#imm,R0 */
#define OPC_AND_IMM(imm)					(0b1100100100000000 | ((imm)&0x00FF))
/* AND.B	#imm,@(R0,GBR) */
#define OPC_ANDB(imm)						(0b1100110100000000 | ((imm)&0x00FF))
/* NOT		Rm,Rn */
#define OPC_NOT(rm, rn)						(0b0110000000000111 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* OR		Rm,Rn */
#define OPC_OR(rm, rn)						(0b0010000000001011 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* OR		#imm,R0 */
#define OPC_OR_IMM(imm)						(0b1100101100000000 | ((imm)&0x00FF))
/* OR.B		#imm,@(R0,GBR) */
#define OPC_ORB(imm)						(0b1100111100000000 | ((imm)&0x00FF))
/* TAS.B	@Rn */
#define OPC_TASB(rn)						(0b0100000000011011 | (((rn)<<8)&0x0F00))
/* TST		Rm,Rn */
#define OPC_TST(rm, rn)						(0b0010000000001000  | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* TST		#imm,R0 */
#define OPC_TST_IMM(imm)					(0b1100100000000000 | ((imm)&0x00FF))
/* TST.B	#imm,@(R0,GBR) */
#define OPC_TSTB(imm)						(0b1100110000000000 | ((imm)&0x00FF))
/* XOR		Rm,Rn */
#define OPC_XOR(rm, rn)						(0b0010000000001000  | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* XOR		#imm,R0 */
#define OPC_XOR_IMM(imm)					(0b1100101000000000 | ((imm)&0x00FF))
/* XOR.B	#imm,@(R0,GBR) */
#define OPC_XORB(imm)						(0b1100111000000000 | ((imm)&0x00FF))

/**********************************
		Shift Instructions
 **********************************/
/* ROTL		Rn */
#define OPC_ROTL(rn)						(0b0100000000000100 | (((rn)<<8)&0x0F00))
/* ROTR		Rn */
#define OPC_ROTR(rn)						(0b0100000000000101 | (((rn)<<8)&0x0F00))
/* ROTCL	Rn */
#define OPC_ROTCL(rn)						(0b0100000000100100 | (((rn)<<8)&0x0F00))
/* ROTCR	Rn */
#define OPC_ROTCR(rn)						(0b0100000000100101 | (((rn)<<8)&0x0F00))
/* SHAD		Rm,Rn */
#define OPC_SHAD(rm, rn)					(0b0100000000001100 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* SHAL		Rn */
#define OPC_SHAL(rn)						(0b0100000000100000 | (((rn)<<8)&0x0F00))
/* SHAR		Rn */
#define OPC_SHAR(rn)						(0b0100000000100001 | (((rn)<<8)&0x0F00))
/* SHLD		Rm,Rn */
#define OPC_SHLD(rm, rn)					(0b0100000000001101 | (((rm)<<4)&0x00F0) | (((rn)<<8)&0x0F00))
/* SHLL		Rn */
#define OPC_SHLL(rn)						(0b0100000000000000 | (((rn)<<8)&0x0F00))
/* SHLR		Rn */
#define OPC_SHLR(rn)						(0b0100000000000001 | (((rn)<<8)&0x0F00))
/* SHLL2	Rn */
#define OPC_SHLL2(rn)						(0b0100000000001000 | (((rn)<<8)&0x0F00))
/* SHLR2	Rn */
#define OPC_SHLR2(rn)						(0b0100000000001001 | (((rn)<<8)&0x0F00))
/* SHLL8	Rn */
#define OPC_SHLL8(rn)						(0b0100000000011000 | (((rn)<<8)&0x0F00))
/* SHLR8	Rn */
#define OPC_SHLR8(rn)						(0b0100000000011001 | (((rn)<<8)&0x0F00))
/* SHLL16	Rn */
#define OPC_SHLL16(rn)						(0b0100000000101000 | (((rn)<<8)&0x0F00))
/* SHLR16	Rn */
#define OPC_SHLR16(rn)						(0b0100000000101001 | (((rn)<<8)&0x0F00))

/**********************************
		Branch Instructions
 **********************************/
/* BF		label */
#define OPC_BF(disp)						(0b1000101100000000 | (((disp)>>1)&0x00FF))
/* BF/S		label */
#define OPC_BF_S(disp)						(0b1000111100000000 | (((disp)>>1)&0x00FF))
/* BT		label */
#define OPC_BT(disp)						(0b1000100100000000 | (((disp)>>1)&0x00FF))
/* BT/S		label */
#define OPC_BT_S(disp)						(0b1000110100000000 | (((disp)>>1)&0x00FF))
/* BRA		label */
#define OPC_BRA(disp)						(0b1010000000000000 | (((disp)>>1)&0x0FFF))
/* BRAF		Rn */
#define OPC_BRAF(rn)						(0b0000000000100011 | (((rn)<<8)&0x0F00))
/* BSR		label */
#define OPC_BSR(disp)						(0b1011000000000000 | (((disp)>>1)&0x0FFF))
/* BSRF		Rn */
#define OPC_BSRF(rn)						(0b0000000000000011 | (((rn)<<8)&0x0F00))
/* JMP		@Rn */
#define OPC_JMP(rn)							(0b0100000000101011 | (((rn)<<8)&0x0F00))
/* JSR		@Rn */
#define OPC_JSR(rn)							(0b0100000000001011 | (((rn)<<8)&0x0F00))
/* RTS		 	*/
#define OPC_RTS								(0b0000000000001011)

/**********************************
	System Control Instructions
 **********************************/
/* CLRMAC		 */
#define OPC_CLRMAC							(0b0000000000101000)
/* CLRS			 */
#define OPC_CLRS							(0b0000000001001000)
/* CLRT			 */
#define OPC_CLRT							(0b0000000000001000)
/* LDC		Rm,SR */
#define OPC_LDC_SR(rm)						(0b0100000000001110 | (((rm)<<8)&0x0F00))
/* LDC		Rm,GBR */
#define OPC_LDC_GBR(rm)						(0b0100000000011110 | (((rm)<<8)&0x0F00))
/* LDC		Rm,VBR */
#define OPC_LDC_VBR(rm)						(0b0100000000101110 | (((rm)<<8)&0x0F00))
/* LDC		Rm,SSR */
#define OPC_LDC_SSR(rm)						(0b0100000000111110 | (((rm)<<8)&0x0F00))
/* LDC		Rm,SPC */
#define OPC_LDC_SPC(rm)						(0b0100000001001110 | (((rm)<<8)&0x0F00))
/* LDC		Rm,R0_BANK */
#define OPC_LDC_R0_BANK(rm)					(0b0100000010001110 | (((rm)<<8)&0x0F00))
/* LDC		Rm,R1_BANK */
#define OPC_LDC_R1_BANK(rm)					(0b0100000010011110 | (((rm)<<8)&0x0F00))
/* LDC		Rm,R2_BANK */
#define OPC_LDC_R2_BANK(rm)					(0b0100000010101110 | (((rm)<<8)&0x0F00))
/* LDC		Rm,R3_BANK */
#define OPC_LDC_R3_BANK(rm)					(0b0100000010111110 | (((rm)<<8)&0x0F00))
/* LDC		Rm,R4_BANK */
#define OPC_LDC_R4_BANK(rm)					(0b0100000011001110 | (((rm)<<8)&0x0F00))
/* LDC		Rm,R5_BANK */
#define OPC_LDC_R5_BANK(rm)					(0b0100000011011110 | (((rm)<<8)&0x0F00))
/* LDC		Rm,R6_BANK */
#define OPC_LDC_R6_BANK(rm)					(0b0100000011101110 | (((rm)<<8)&0x0F00))
/* LDC		Rm,R7_BANK */
#define OPC_LDC_R7_BANK(rm)					(0b0100000011111110 | (((rm)<<8)&0x0F00))
/* LDC.L	@Rm+,SR */
#define OPC_LDCL_SR(rm)						(0b0100000000000111 | (((rm)<<8)&0x0F00))
/* LDC.L	@Rm+,GBR */
#define OPC_LDCL_GBR(rm)					(0b0100000000010111 | (((rm)<<8)&0x0F00))
/* LDC.L	@Rm+,VBR */
#define OPC_LDCL_VBR(rm)					(0b0100000000100111 | (((rm)<<8)&0x0F00))
/* LDC.L	@Rm+,SSR */
#define OPC_LDCL_SSR(rm)					(0b0100000000110111 | (((rm)<<8)&0x0F00))
/* LDC.L	@Rm+,SPC */
#define OPC_LDCL_SPC(rm)					(0b0100000001000111 | (((rm)<<8)&0x0F00))
/* LDC.L	@Rm+,R0_BANK */
#define OPC_LDCL_R0_BANK(rm)				(0b0100000010000111 | (((rm)<<8)&0x0F00))
/* LDC.L	@Rm+,R1_BANK */
#define OPC_LDCL_R1_BANK(rm)				(0b0100000010010111 | (((rm)<<8)&0x0F00))
/* LDC.L	@Rm+,R2_BANK */
#define OPC_LDCL_R2_BANK(rm)				(0b0100000010100111 | (((rm)<<8)&0x0F00))
/* LDC.L	@Rm+,R3_BANK */
#define OPC_LDCL_R3_BANK(rm)				(0b0100000010110111 | (((rm)<<8)&0x0F00))
/* LDC.L	@Rm+,R4_BANK */
#define OPC_LDCL_R4_BANK(rm)				(0b0100000011000111 | (((rm)<<8)&0x0F00))
/* LDC.L	@Rm+,R5_BANK */
#define OPC_LDCL_R5_BANK(rm)				(0b0100000011010111 | (((rm)<<8)&0x0F00))
/* LDC.L	@Rm+,R6_BANK */
#define OPC_LDCL_R6_BANK(rm)				(0b0100000011100111 | (((rm)<<8)&0x0F00))
/* LDC.L	@Rm+,R7_BANK */
#define OPC_LDCL_R7_BANK(rm)				(0b0100000011110111 | (((rm)<<8)&0x0F00))
/* LDS		Rm,MACH */
#define OPC_LDS_MACH(rm)					(0b0100000000001010 | (((rm)<<8)&0x0F00))
/* LDS		Rm,MACL */
#define OPC_LDS_MACL(rm)					(0b0100000000011010 | (((rm)<<8)&0x0F00))
/* LDS		Rm,PR */
#define OPC_LDS_PR(rm)						(0b0100000000101010 | (((rm)<<8)&0x0F00))
/* LDS.L	@Rm+,MACH */
#define OPC_LDSL_MACH(rm)					(0b0100000000000110 | (((rm)<<8)&0x0F00))
/* LDS.L	@Rm+,MACL */
#define OPC_LDSL_MACL(rm)					(0b0100000000010110 | (((rm)<<8)&0x0F00))
/* LDS.L	@Rm+,PR */
#define OPC_LDSL_PR(rm)						(0b0100000000100110 | (((rm)<<8)&0x0F00))
/* LDTLB		 */
#define OPC_LDTLB							(0b0000000000111000)
/* NOP			 */
#define OPC_NOP								(0b0000000000001001)
/////////////////////////
// 'PREF' opcode here?  /
/////////////////////////
/* RTE			 */
#define OPC_RTE								(0b0000000000101011)
/* SETS			 */
#define OPC_SETS							(0b0000000001011000)
/* SETT			 */
#define OPC_SETT							(0b0000000000011000)
/* SLEEP		 */
#define OPC_SLEEP							(0b0000000000011011)
/* STC		SR,Rn */
#define OPC_STC_SR(rn)						(0b0000000000000010 | (((rn)<<8)&0x0F00))
/* STC		GBR,Rn */
#define OPC_STC_GBR(rn)						(0b0000000000010010 | (((rn)<<8)&0x0F00))
/* STC		VBR,Rn */
#define OPC_STC_VBR(rn)						(0b0000000000100010 | (((rn)<<8)&0x0F00))
/* STC		SSR,Rn */
#define OPC_STC_SSR(rn)						(0b0000000000110010 | (((rn)<<8)&0x0F00))
/* STC		SPC,Rn */
#define OPC_STC_SPC(rn)						(0b0000000001000010 | (((rn)<<8)&0x0F00))
/* STC		R0_BANK,Rn */
#define OPC_STC_R0_BANK(rn)					(0b0000000010000010 | (((rn)<<8)&0x0F00))
/* STC		R1_BANK,Rn */
#define OPC_STC_R1_BANK(rn)					(0b0000000010010010 | (((rn)<<8)&0x0F00))
/* STC		R2_BANK,Rn */
#define OPC_STC_R2_BANK(rn)					(0b0000000010100010 | (((rn)<<8)&0x0F00))
/* STC		R3_BANK,Rn */
#define OPC_STC_R3_BANK(rn)					(0b0000000010110010 | (((rn)<<8)&0x0F00))
/* STC		R4_BANK,Rn */
#define OPC_STC_R4_BANK(rn)					(0b0000000011000010 | (((rn)<<8)&0x0F00))
/* STC		R5_BANK,Rn */
#define OPC_STC_R5_BANK(rn)					(0b0000000011010010 | (((rn)<<8)&0x0F00))
/* STC		R6_BANK,Rn */
#define OPC_STC_R6_BANK(rn)					(0b0000000011100010 | (((rn)<<8)&0x0F00))
/* STC		R7_BANK,Rn */
#define OPC_STC_R7_BANK(rn)					(0b0000000011110010 | (((rn)<<8)&0x0F00))
/* STC.L	SR,@-Rn */
#define OPC_STCL_SR(rn)						(0b0100000000000011 | (((rn)<<8)&0x0F00))
/* STC.L	GBR,@-Rn */
#define OPC_STCL_GBR(rn)					(0b0100000000010011 | (((rn)<<8)&0x0F00))
/* STC.L	VBR,@-Rn */
#define OPC_STCL_VBR(rn)					(0b0100000000100011 | (((rn)<<8)&0x0F00))
/* STC.L	SSR,@-Rn */
#define OPC_STCL_SSR(rn)					(0b0100000000110011 | (((rn)<<8)&0x0F00))
/* STC.L	SPC,@-Rn */
#define OPC_STCL_SPC(rn)					(0b0100000001000011 | (((rn)<<8)&0x0F00))
/* STC.L	R0_BANK,@-Rn */
#define OPC_STCL_R0_BANK(rn)				(0b0100000010000011 | (((rn)<<8)&0x0F00))
/* STC.L	R1_BANK,@-Rn */
#define OPC_STCL_R1_BANK(rn)				(0b0100000010010011 | (((rn)<<8)&0x0F00))
/* STC.L	R2_BANK,@-Rn */
#define OPC_STCL_R2_BANK(rn)				(0b0100000010100011 | (((rn)<<8)&0x0F00))
/* STC.L	R3_BANK,@-Rn */
#define OPC_STCL_R3_BANK(rn)				(0b0100000010110011 | (((rn)<<8)&0x0F00))
/* STC.L	R4_BANK,@-Rn */
#define OPC_STCL_R4_BANK(rn)				(0b0100000011000011 | (((rn)<<8)&0x0F00))
/* STC.L	R5_BANK,@-Rn */
#define OPC_STCL_R5_BANK(rn)				(0b0100000011010011 | (((rn)<<8)&0x0F00))
/* STC.L	R6_BANK,@-Rn */
#define OPC_STCL_R6_BANK(rn)				(0b0100000011100011 | (((rn)<<8)&0x0F00))
/* STC.L	R7_BANK,@-Rn */
#define OPC_STCL_R7_BANK(rn)				(0b0100000011110011 | (((rn)<<8)&0x0F00))
/* STS		MACH,Rn */
#define OPC_STS_MACH(rn)					(0b0000000000001010 | (((rn)<<8)&0x0F00))
/* STS		MACL,Rn */
#define OPC_STS_MACL(rn)					(0b0000000000011010 | (((rn)<<8)&0x0F00))
/* STS		PR,Rn */
#define OPC_STS_PR(rn)						(0b0000000000101010 | (((rn)<<8)&0x0F00))
/* STS.L	MACH,@-Rn */
#define OPC_STSL_MACH(rn)					(0b0100000000000010 | (((rn)<<8)&0x0F00))
/* STS.L	MACL,@-Rn */
#define OPC_STSL_MACL(rn)					(0b0100000000010010 | (((rn)<<8)&0x0F00))
/* STS.L	PR,@-Rn */
#define OPC_STSL_PR(rn)						(0b0100000000100010 | (((rn)<<8)&0x0F00))
/* TRAPA	#imm */
#define OPC_TRAPA(imm)						(0b1100001100000000 | ((imm)&0x00FF))



#endif //MCA_OPCODES_H
