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


#include "sa-common.h"
#include "zepto-big-num.h"

FORCE_INLINE
void zepto_bignum_add_product_and_do_modular_part( uint8_t* accum, uint8_t* extra, const uint8_t* bignum, uint8_t bytefactor, const uint8_t* mod, uint8_t inverter )
{
	uint8_t i;
	uint16_t pr1, pr2;
	uint16_t carry;

	pr1 = ((uint16_t)bytefactor) * bignum[0]; 
	pr1 += accum[0];
	if ( (uint8_t)pr1 != 0 )
	{
		pr2 = ((uint16_t)inverter) * ((uint8_t)pr1);
		inverter = (uint8_t)pr2;
		pr2 = ((uint16_t)inverter) * ZEPTO_PROG_CONSTANT_READ_BYTE(mod);
		ZEPTO_DEBUG_ASSERT( (uint8_t)( ((uint8_t)pr1) + ((uint8_t)pr2) ) == 0 );

		carry = (pr1 >> 8) + (pr2 >> 8) + 1;

		for ( i=1; i<M_BYTE_SIZE; i++ )
		{
			pr1 = ((uint16_t)bytefactor) * bignum[i]; 
			pr2 = ((uint16_t)inverter) * ZEPTO_PROG_CONSTANT_READ_BYTE(mod+i); 
			carry += (uint8_t)pr1;
			carry += (uint8_t)pr2;
			carry += accum[i];
			accum[i-1] = carry;
			carry >>= 8;
			carry += (pr1 >> 8);
			carry += (pr2 >> 8);
		}
	}
	else
	{
		carry = pr1 >> 8;

		for ( i=1; i<M_BYTE_SIZE; i++ )
		{
			pr1 = ((uint16_t)bytefactor) * bignum[i]; 
			carry += (uint8_t)pr1;
			carry += accum[i];
			accum[i-1] = carry;
			carry >>= 8;
			carry += pr1 >> 8;
		}
	}
	carry += *extra;
	accum[M_BYTE_SIZE-1] = carry;
	carry >>= 8;
	ZEPTO_DEBUG_ASSERT( carry <= 1 );
	*extra = carry;
}

FORCE_INLINE
void zepto_bignum_subtract_mod_if_necessary( const uint8_t* extra, const uint8_t* mod, uint8_t* buff )
{
	int16_t i;
	uint16_t tmp;
	ZEPTO_DEBUG_ASSERT( M_BYTE_SIZE <= 128 ); // or otherwise i must be 16-bit or unsigned with all respective precautions

	ZEPTO_DEBUG_ASSERT( *extra <= 1 );

	if ( *extra == 0 )
	{
		for ( i=M_BYTE_SIZE-1; i>=0; i-- )
		{
			if ( buff[i] > ZEPTO_PROG_CONSTANT_READ_BYTE(mod+i) ) 
				break;
			if ( buff[i] < ZEPTO_PROG_CONSTANT_READ_BYTE(mod+i) ) 
				return;
		}
	}

	tmp = buff[0] + ( 0x100 - (uint16_t)(ZEPTO_PROG_CONSTANT_READ_BYTE(mod)) );
	buff[0] = (uint8_t)(tmp);
	tmp >>= 8;

	for ( i=1; i<M_BYTE_SIZE; i++ )
	{
		tmp += buff[i] + ( 0xFF - (uint16_t)(ZEPTO_PROG_CONSTANT_READ_BYTE(mod+i)) );
		buff[i] = (uint8_t)(tmp);
		tmp >>= 8;
	}
	ZEPTO_DEBUG_ASSERT( tmp <= 1 );
}

#ifdef _DEBUG
bool zepto_bignum_is_greater_eq_than_mod( const uint8_t* mod, const uint8_t* buff )
{
	int8_t i;
	uint16_t tmp = 0;
	for ( i=M_BYTE_SIZE-1; i>=0; i-- )
	{
		if ( buff[i] > ZEPTO_PROG_CONSTANT_READ_BYTE(mod+i) ) 
			return true;
		if ( buff[i] < ZEPTO_PROG_CONSTANT_READ_BYTE(mod+i) ) 
			return false;
	}
	return true;
}
#endif

//FORCE_INLINE
void zepto_bignum_m_step( const uint8_t* mod, uint8_t inverter, const uint8_t* a, const uint8_t* b, uint8_t* _res )
{
	uint8_t i;
	uint8_t extra = 0;
	uint8_t res[M_BYTE_SIZE];
	ZEPTO_MEMSET( res, 0, M_BYTE_SIZE );
	for ( i = 0; i<M_BYTE_SIZE; i++ )
		zepto_bignum_add_product_and_do_modular_part( res, &extra, a, b[i], mod, inverter );

	zepto_bignum_subtract_mod_if_necessary( &extra, mod, res );
#ifdef _DEBUG
	bool less = !zepto_bignum_is_greater_eq_than_mod( mod, res );
	if (!less)
		less = less;
	ZEPTO_DEBUG_ASSERT( less );
#endif
	ZEPTO_MEMCPY( _res, res, M_BYTE_SIZE );
}

FORCE_INLINE
void zepto_bignum_m_calc_num_prim( const uint8_t* mod, const uint8_t* pow2_2n_mod, uint8_t sel_bt, const uint8_t* num, uint8_t* res )
{
#ifdef ZEPTO_BIGNUM_POW2_2N_MODN_LOCATED_IN_PROGMEM
	uint8_t pow2_2n_mod_[M_BYTE_SIZE];
	ZEPTO_MEMCPY_FROM_PROGMEM( pow2_2n_mod_, pow2_2n_mod, M_BYTE_SIZE );
	zepto_bignum_m_step( mod, sel_bt, num, pow2_2n_mod_, res );
#else // ZEPTO_BIGNUM_POW2_2N_MODN_LOCATED_IN_PROGMEM
	zepto_bignum_m_step( mod, sel_bt, num, pow2_2n_mod, res );
#endif // ZEPTO_BIGNUM_POW2_2N_MODN_LOCATED_IN_PROGMEM
}

FORCE_INLINE
void zepto_bignum_m_restore_num_inline( const uint8_t* mod, uint8_t sel_bt, uint8_t* num_prim, uint8_t* num )
{
	uint8_t one[M_BYTE_SIZE];
	ZEPTO_MEMSET( one, 0, M_BYTE_SIZE );
	one[0] = 1;
	zepto_bignum_m_step( mod, sel_bt, one, num_prim, num );
}

void zepto_bignum_m_restore_num( const uint8_t* mod, uint8_t sel_bt, uint8_t* num_prim, uint8_t* num )
{
	uint8_t one[M_BYTE_SIZE];
	ZEPTO_MEMSET( one, 0, M_BYTE_SIZE );
	one[0] = 1;
	zepto_bignum_m_step( mod, sel_bt, one, num_prim, num );
}

FORCE_INLINE
void zepto_bignum_m_exponentiate_m2m( const uint8_t* mod, uint8_t inverter, uint8_t* x, const uint8_t* exp, uint8_t* res )
{
	uint16_t i;

	for ( i=0; i<M_BYTE_SIZE*8; i++ )
	{
		if ( exp[i>>3] & (1 << (i&7)) )
		{
			zepto_bignum_m_step( mod, inverter, res, x, res );
		}

		zepto_bignum_m_step( mod, inverter, x, x, x );
	}
}

FORCE_INLINE
void zepto_bignum_m_exponentiate_m2m_no_ini_res( const uint8_t* mod, uint8_t inverter, uint8_t* x, const uint8_t* exp, uint8_t* res )
{
	bool res_set = false;
	uint16_t i;
	
	for ( i=0; i<M_BYTE_SIZE*8-1; i++ )
	{
		if ( exp[i>>3] & (1 << (i&7)) )
		{
			if ( res_set )
				zepto_bignum_m_step( mod, inverter, res, x, res );
			else
			{
				res_set = true;
				ZEPTO_MEMCPY( res, x, M_BYTE_SIZE );
			}
		}

		zepto_bignum_m_step( mod, inverter, x, x, x );
	}
	
	if ( exp[i>>3] & (1 << (i&7)) )
	{
		if ( res_set )
			zepto_bignum_m_step( mod, inverter, res, x, res );
		else
		{
			res_set = true;
			ZEPTO_MEMCPY( res, x, M_BYTE_SIZE );
		}
	}
	
	ZEPTO_RUNTIME_CHECK( res_set );
}

void zepto_bignum_m_exponentiate( const uint8_t* mod, const uint8_t* pow2_2n_modn, uint8_t inverter, const uint8_t* exp, const uint8_t* x, uint8_t* res )
{
	uint8_t x_primn[M_BYTE_SIZE];

	ZEPTO_MEMSET( res, 0, M_BYTE_SIZE );
	res[0] = 1;
	zepto_bignum_m_calc_num_prim( mod, pow2_2n_modn, inverter, x, x_primn );
	zepto_bignum_m_calc_num_prim( mod, pow2_2n_modn, inverter, res, res );
	zepto_bignum_m_exponentiate_m2m_no_ini_res( mod, inverter, x_primn, exp, res );
	zepto_bignum_m_restore_num_inline( mod, inverter, res, res );
}


void zepto_bignum_m_exponentiate_in_montgomery_space( const uint8_t* mod, uint8_t inverter, const uint8_t* exp, const uint8_t* x, uint8_t* res )
{
	uint8_t x_primn[M_BYTE_SIZE];
	ZEPTO_MEMCPY( x_primn, x, M_BYTE_SIZE );
	zepto_bignum_m_exponentiate_m2m_no_ini_res( mod, inverter, x_primn, exp, res );
}










