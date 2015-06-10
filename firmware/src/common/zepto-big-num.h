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

#if !defined __SA_BIG_NUM_H__
#define __SA_BIG_NUM_H__

#include "sa-common.h"

#define M_BYTE_SIZE 128 // must be a power of 2

#ifdef __cplusplus
extern "C" 
{
#endif


void zepto_bignum_m_exponentiate( const uint8_t* mod, const uint8_t* pow2_2n_modn, uint8_t inverter, const uint8_t* exp, const uint8_t* x, uint8_t* res );
//void zepto_bignum_m_step( const uint8_t* mod, uint8_t inverter, const uint8_t* a, const uint8_t* b, uint8_t* res );
void zepto_bignum_m_restore_num( const uint8_t* mod, uint8_t sel_bt, uint8_t* num_prim, uint8_t* num );
void zepto_bignum_m_exponentiate_in_montgomery_space( const uint8_t* mod, uint8_t inverter, const uint8_t* exp, const uint8_t* x, uint8_t* res );

#ifdef __cplusplus
}
#endif

#endif // __SA_BIG_NUM_H__