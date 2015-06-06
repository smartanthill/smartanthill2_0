
#include <stdio.h>
#include <boost/multiprecision/cpp_int.hpp>
using namespace boost::multiprecision;
typedef number<cpp_int_backend<4096, 4096, unsigned_magnitude, unchecked, void> > uint4096_t;
typedef number<cpp_int_backend<4096, 4096, signed_magnitude, unchecked, void> >  int4096_t;

void exponentiate( uint4096_t x, uint4096_t mod, uint4096_t exp, uint4096_t& res )
{
	res = 1;
	for ( int i=0; i<1024; i++ )
	{
		if ( exp & 1 )
		{
			res = res * x;
			res = res % mod;
			if ( res == 0 )
			{
				printf( "res=0 at i=%d\n", i );
				break;
			}
		}

		exp = exp >> 1;

		x = x * x;
		x = x % mod;
	}
}

void calc_r_inv_neg_mod_inv( uint4096_t R, uint4096_t mod, uint4096_t& R_inv, uint4096_t& mod_inv )
{
	uint4096_t q, r, r_prev, tmpu; 
	int4096_t s, s_prev, t, t_prev, tmp;
	ZEPTO_DEBUG_ASSERT( R > mod );
	r = mod;
	r_prev = R;
	s_prev = 1;
	s = 0;
	t_prev = 0;
	t = 1;

	do
	{
		q = r_prev / r; // integer division
		tmpu = r_prev - q * r;
		r_prev = r;
		r = tmpu;

		tmp = s_prev - q * s;
		s_prev = s;
		s = tmp;

		tmp = t_prev - q * t;
		t_prev = t;
		t = tmp;
	}
	while( r != 0 );

	if ( s_prev < 0 )
		s_prev = s_prev + mod;

	if ( t_prev > 0 )
		t_prev = R - t_prev;

	R_inv = (uint4096_t)s_prev;
	mod_inv = (uint4096_t)t_prev;
}

void calc_neg_mod_inv( uint4096_t R, uint4096_t mod, uint4096_t& mod_inv )
{
	uint4096_t q, r, r_prev, tmpu; 
	int4096_t s, s_prev, t, t_prev, tmp;
	ZEPTO_DEBUG_ASSERT( R > mod );
	r = mod;
	r_prev = R;
	s_prev = 1;
	s = 0;
	t_prev = 0;
	t = 1;

	do
	{
		q = r_prev / r; // integer division
		tmpu = r_prev - q * r;
		r_prev = r;
		r = tmpu;

		tmp = s_prev - q * s;
		s_prev = s;
		s = tmp;

		tmp = t_prev - q * t;
		t_prev = t;
		t = tmp;
	}
	while( r != 0 );

	if ( s_prev < 0 )
		s_prev = s_prev + mod;

	if ( t_prev > 0 )
		t_prev = R - t_prev;

	mod_inv = (uint4096_t)t_prev;
}

void calc_r_inv_neg_mod_inv_int( int R, int mod, int& R_inv, int& mod_inv )
{
	int q, r, r_prev, tmpu; 
	int s, s_prev, t, t_prev, tmp;
	ZEPTO_DEBUG_ASSERT( R > mod );
	r = mod;
	r_prev = R;
	s_prev = 1;
	s = 0;
	t_prev = 0;
	t = 1;

	do
	{
		q = r_prev / r; // integer division
		tmpu = r_prev - q * r;
		r_prev = r;
		r = tmpu;

		tmp = s_prev - q * s;
		s_prev = s;
		s = tmp;

		tmp = t_prev - q * t;
		t_prev = t;
		t = tmp;
	}
	while( r != 0 );

	if ( s_prev < 0 )
		s_prev = s_prev + mod;

/*	if ( t_prev < 0 )
		t_prev = t_prev + R;*/

	if ( t_prev > 0 )
		t_prev = R - t_prev;

	R_inv = (int)s_prev;
	mod_inv = (int)t_prev;
}

void calc_num_bar( uint4096_t R, uint4096_t mod, uint4096_t num, uint4096_t& num_bar )
{
	num_bar = ( num * R ) % mod;
}

void calc_num_bar_int( int R, int mod, int num, int& num_bar )
{
	num_bar = ( num * R ) % mod;
}

void mul_modinv_mod_R( uint4096_t R, uint4096_t mod_inv, uint4096_t num, uint4096_t& res )
{ 
	res = ( num * mod_inv ) % R;
}

void multiply_m( uint4096_t R, uint4096_t mod, uint4096_t mod_inv, uint4096_t a, uint4096_t b, uint4096_t& res )
{ 
	uint4096_t p = a * b;
	uint4096_t p1;
	mul_modinv_mod_R( R, mod_inv, p, p1 );
	res = ( p + p1 * mod ) / R;
}

void multiply_m_int( int R, int mod, int mod_inv, int a, int b, int& res )
{ 
	int p = a * b;
	res = ( p + ( ( p *mod_inv ) % R ) * mod ) / R;
}

void exponentiate_m( uint4096_t mod, uint4096_t mod_inv, uint4096_t R, uint4096_t x, uint4096_t exp, uint4096_t& res )
{
	for ( int i=0; i<1024; i++ )
	{
		if ( exp & 1 )
		{
			multiply_m( R, mod, mod_inv, res, x, res );
//			res = res * x;
//			res = res % mod;
		}

		exp = exp >> 1;

//		x = x * x;
//		x = x % mod;
		multiply_m( R, mod, mod_inv, x, x, x );
	}
}

void exponentiate_m_int( int mod, int mod_inv, int R, int x, int exp, int& res )
{
	for ( int i=0; i<1024; i++ )
	{
		if ( exp & 1 )
		{
			multiply_m_int( R, mod, mod_inv, res, x, res );
//			res = res * x;
//			res = res % mod;
			if ( res == 0 )
			{
				printf( "res=0 at i=%d\n", i );
				break;
			}
		}

		exp = exp >> 1;

//		x = x * x;
//		x = x % mod;
		multiply_m_int( R, mod, mod_inv, x, x, x );
	}
}

void unbar( uint4096_t mod, uint4096_t mod_inv, uint4096_t R, uint4096_t x, uint4096_t& res )
{
	res = ( x + ( ( x * mod_inv ) % R ) * mod ) / R;
}

void unbar( int mod, int mod_inv, int R, int x, int& res )
{
	res = ( x + ( ( x * mod_inv ) % R ) * mod ) / R;
}

void calc_r_sq_bar( uint4096_t mod, uint4096_t R, uint4096_t& R2Bar )
{
	R2Bar = ( R * R ) % mod;
}

void calc_r_sq_bar_int( int mod, int R, int& R2Bar )
{
	R2Bar = ( R * R ) % mod;
}

void calc_num_bar_alt( uint4096_t R, uint4096_t R2Bar, uint4096_t mod, uint4096_t mod_inv, uint4096_t x, uint4096_t& x_bar )
{
	uint4096_t num = x * R2Bar;
	x_bar = ( num + ( ( num * mod_inv ) % R ) * mod ) / R;
}

void calc_num_bar_alt_int( int R, int R2Bar, int mod, int mod_inv, int x, int& x_bar )
{
	int num = x * R2Bar;
	x_bar = ( num + ( ( num * mod_inv ) % R ) * mod ) / R;
}

int int_test()
{
	int a, b, a_bar, b_bar, c, c_bar, x, mod, exp, res2, R, R_inv, R2Bar, mod_inv, x_bar, res2_bar;
	a = 43;
	b = 56;
	R = 100;
	mod = 97;
	res2 = 1;

	calc_r_inv_neg_mod_inv_int( R, mod, R_inv, mod_inv );
	calc_num_bar_int( R, mod, a, a_bar );
	calc_num_bar_int( R, mod, b, b_bar );
	multiply_m_int( R, mod, mod_inv, a_bar, b_bar, c_bar );
	unbar( mod, mod_inv, R, c_bar, c );
	printf( "c = %d (%d)\n", c, (a*b)%mod );
/*	calc_num_bar( R, mod, x, x_bar );
	calc_num_bar( R, mod, res2, res2_bar );
	calc_r_inv_neg_mod_inv( R, mod, R_inv, mod_inv );
	exponentiate_m( mod, mod_inv, R, x, exp, res2 );
	unbar( mod, mod_inv, R, res2, res2 );
	same = res2 == 1 || res2 == mod - 1;*/

	x = 5;
	exp = 7;
	res2 = 1;
	calc_num_bar_int( R, mod, x, x_bar );
	calc_num_bar_int( R, mod, res2, res2_bar );
	exponentiate_m_int( mod, mod_inv, R, x_bar, exp, res2_bar );
	unbar( mod, mod_inv, R, res2_bar, res2 );
	printf( "res2 = %d (%d)\n", res2, (x*x*x*x*x*x*x)%mod );

	calc_r_sq_bar_int( mod, R, R2Bar );
	calc_r_inv_neg_mod_inv_int( R, mod, R_inv, mod_inv );
	calc_num_bar_alt_int( R, R2Bar, mod, mod_inv, a, a_bar );
	calc_num_bar_alt_int( R, R2Bar, mod, mod_inv, b, b_bar );
	multiply_m_int( R, mod, mod_inv, a_bar, b_bar, c_bar );
	unbar( mod, mod_inv, R, c_bar, c );
	printf( "c = %d (%d)\n", c, (a*b)%mod );
	return 0;
}

int int1024_test()
{
	uint4096_t a, b, a_bar, b_bar, c, c_bar, x, mod, exp, res, res2, R, R_inv, R2Bar, mod_inv, x_bar, res2_bar;
	a = 43;
	b = 56;
	R = 100;
	mod = 97;
	res2 = 1;

	calc_r_inv_neg_mod_inv( R, mod, R_inv, mod_inv );
	calc_num_bar( R, mod, a, a_bar );
	calc_num_bar( R, mod, b, b_bar );
	multiply_m( R, mod, mod_inv, a_bar, b_bar, c_bar );
	unbar( mod, mod_inv, R, c_bar, c );

	x = 5;
	exp = 7;
	res2 = 1;
	calc_num_bar( R, mod, x, x_bar );
	calc_num_bar( R, mod, res2, res2_bar );
	exponentiate_m( mod, mod_inv, R, x_bar, exp, res2_bar );
	unbar( mod, mod_inv, R, res2_bar, res2 );
	uint4096_t int_res2 = (int)res2, int_res2_chk =  (int)(x*x*x*x*x*x*x)%mod;
//	printf( "res2 = %d (%d)\n", int_res2, int_res2_chk );
/*	printf( "c = %d (%d)\n", c, (a*b)%mod );
	calc_num_bar( R, mod, x, x_bar );
	calc_num_bar( R, mod, res2, res2_bar );
	calc_r_inv_neg_mod_inv( R, mod, R_inv, mod_inv );
	exponentiate_m( mod, mod_inv, R, x, exp, res2 );
	unbar( mod, mod_inv, R, res2, res2 );
	same = res2 == 1 || res2 == mod - 1;*/
	return 0;
}

int clean_test()
{
	uint4096_t x; 
	uint4096_t mod, exp, res2, R, R2Bar, mod_inv, x_bar, res2_bar;

	// set values
	R = 1;
	R = R << 256;

	mod = 3;
	for ( int i=0; i<120; i++ )
		mod = mod * 3;

	exp = mod;

	x = 7;

	// do precalculations
	calc_r_sq_bar( mod, R, R2Bar );
	calc_neg_mod_inv( R, mod, mod_inv );

	// prepare inputs
	res2 = 1;
	calc_num_bar_alt( R, R2Bar, mod, mod_inv, x, x_bar );
	calc_num_bar_alt( R, R2Bar, mod, mod_inv, res2, res2_bar );

	// exponentiation
	exponentiate_m( mod, mod_inv, R, x_bar, exp, res2_bar );
	// recovery
	unbar( mod, mod_inv, R, res2_bar, res2 );
	bool same = res2 == 1 || res2 == mod - 1;
	printf( same ? "OK\n" : "FAILED\n" );

	return 0;
}





uint8_t calc_selector_factor_int( int mod )
{
	int low_bt = mod & 0x100;
	ZEPTO_DEBUG_ASSERT( low_bt != 0 );
	for ( int i=1; i<256; i++ )
		if ( ( ( low_bt * i ) & 0x100 ) == 1 )
			return 256 - i;
	ZEPTO_DEBUG_ASSERT( 0 );
}

void m_step_int( uint8_t bt_cnt, int mod, uint8_t sel_bt, int a, int b, int& res )
{
	int accum = 0;
	int temp;
	uint8_t i;
	uint8_t low_bt, factor;
	for ( i = 0; i<bt_cnt; i++ )
	{
		low_bt = b && 0x100;
		b = b >> 8;
		temp = a * low_bt;
		accum = accum + temp;
		low_bt = accum && 0x100;
		factor = low_bt * sel_bt;
		temp = mod * factor;
		accum = accum + temp;
		ZEPTO_DEBUG_ASSERT( (accum &0x100) == 0 );
		accum = accum >> 8;
	}
}

void precalc_pow2_2n_mod_int( uint8_t bt_cnt, int mod, int& res )
{
	int t = 1;
	t = t << ( bt_cnt * 8 * 2 );
	res = t % mod;
}

void calc_num_prim_int( uint8_t bt_cnt, int mod, int pow2_2n_mod, uint8_t sel_bt, int a, int b, int& res )
{
	m_step_int( bt_cnt, mod, sel_bt, a, pow2_2n_mod, res );
}



uint8_t m_calc_selector_factor( uint4096_t mod )
{
	uint4096_t low_bt = mod & 0xff;
	ZEPTO_DEBUG_ASSERT( low_bt != 0 );
	for ( int i=1; i<256; i++ )
		if ( ( ( low_bt * i ) & 0xff ) == 1 )
			return 256 - i;
//			return i;
	ZEPTO_DEBUG_ASSERT( 0 );
}

void m_step( uint8_t bt_cnt, uint4096_t mod, uint8_t sel_bt, uint4096_t a, uint4096_t b, uint4096_t& res )
{
	uint4096_t accum = 0;
	uint4096_t temp;
	uint8_t i;
	uint4096_t low_bt, factor;
	ZEPTO_DEBUG_ASSERT( ( a >> ( bt_cnt * 8 ) ) == 0 );
	ZEPTO_DEBUG_ASSERT( ( b >> ( bt_cnt * 8 ) ) == 0 );
	for ( i = 0; i<bt_cnt; i++ )
	{
		low_bt = b & 0xff;
		b = b >> 8;
		temp = a * low_bt;
		accum = accum + temp;
		low_bt = accum & 0xff;
		factor = (low_bt * sel_bt) & 0xff;
		temp = mod * factor;
		accum = accum + temp;
		ZEPTO_DEBUG_ASSERT( (accum & 0xff) == 0 );
		accum = accum >> 8;
	}

	res = accum;

	if ( res > mod ) res = res - mod;
	ZEPTO_DEBUG_ASSERT( ( res >> ( bt_cnt * 8 ) ) == 0 );
}






















//#define M_ALLOW_NEGATIVES

/*
uint8_t m_accumulator_calc_selector_factor( uint8_t low_bt )
{
	ZEPTO_DEBUG_ASSERT( low_bt != 0 );
	for ( int i=1; i<256; i++ )
		if ( ( ( low_bt * i ) & 0xff ) == 1 )
			return 256 - i;
	ZEPTO_DEBUG_ASSERT( 0 );
}
*/


#define M_BYTE_SIZE 32 // must be a power of 2
//#define M_BYTE_SIZE_EXTRA (M_BYTE_SIZE + 3)

typedef struct _m_accumulator
{
#ifdef M_ALLOW_NEGATIVES
	int16_t cells[ M_BYTE_SIZE ];
	int16_t cell_ms;
#else // M_ALLOW_NEGATIVES
	uint16_t cells[ M_BYTE_SIZE ];
	uint16_t cell_ms;
#endif // M_ALLOW_NEGATIVES
	uint8_t offset;
} m_accumulator;

#define M_INCREMENT_OFFSET( offset ) (offset) = ( (offset) + 1 ) & ( M_BYTE_SIZE - 1 )

// TESTING HELPERS	//////////////////////////////////////////////////////////////////////////////

void m_accumulator_to_BOOST( const uint8_t* buff, uint4096_t* boost_val )
{
	*boost_val = 0;
	for ( int i=M_BYTE_SIZE-1; i>=0; i-- )
	{
		*boost_val <<= 8;
		*boost_val += buff[i];
	}
}

void m_accumulator_from_BOOST( const uint4096_t* _boost_val, uint8_t* buff )
{
	uint4096_t boost_val = *_boost_val;
	for ( int i=0; i<M_BYTE_SIZE; i++ )
	{
		buff[i] = (uint8_t)(boost_val);
		boost_val >>= 8;
	}
}

void m_accumulator_ACCUMULATOR_to_BOOST( const m_accumulator* accum, uint4096_t* boost_val )
{
	*boost_val = 0;
	uint4096_t tmp;
	for ( int i=0; i<M_BYTE_SIZE; i++ )
	{
		tmp = accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ];
		tmp = tmp << ( i*8);
		*boost_val += tmp;
	}
	tmp = accum->cell_ms;
	tmp = tmp << ( M_BYTE_SIZE*8);
	*boost_val += tmp;
}

// END OF TESTING HELPERS	///////////////////////////////////////////////////////////////////////


void m_accumulator_init_clear( m_accumulator* accum )
{
	memset( accum, 0, sizeof( m_accumulator ) );
}

void m_accumulator_add_product( m_accumulator* accum, const uint8_t* bignum, uint8_t bytefactor )
{
	uint8_t i;
	uint16_t pr;
	uint16_t factor = bytefactor;
	ZEPTO_DEBUG_ASSERT( accum->cell_ms <= 1 );
	for ( i=0; i<M_BYTE_SIZE-1; i++ )
	{
		pr = factor * bignum[i]; 
		accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ] += (uint8_t)pr;
		accum->cells[ ( accum->offset + i + 1 ) & ( M_BYTE_SIZE - 1 ) ] += (uint8_t)(pr >> 8);
	}
	pr = factor * bignum[M_BYTE_SIZE-1]; 
	accum->cells[ ( accum->offset + M_BYTE_SIZE - 1 ) & ( M_BYTE_SIZE - 1 ) ] += (uint8_t)pr;
	accum->cell_ms += (uint8_t)(pr >> 8);
}
/*
void m_accumulator_prenormalize_lsb( m_accumulator* accum, const uint8_t* bignum, uint8_t bytefactor )
{
	accum->cells[ ( accum->offset + 1 ) & ( M_BYTE_SIZE - 1 ) ] += (uint8_t)( accum->cells[ accum->offset ] >> 8 );
}
*/
void m_accumulator_do_modular_part( m_accumulator* accum, const uint8_t* mod, uint8_t inverter )
{
	uint8_t i;
	uint16_t pr;
	uint16_t factor = inverter;

	accum->cells[ ( accum->offset + 1 ) & ( M_BYTE_SIZE - 1 ) ] += (uint8_t)( accum->cells[ accum->offset ] >> 8 ); // normalization of a least significant cell
	if ( (uint8_t)( accum->cells[ accum->offset ] ) == 0 )
	{
		accum->cells[ accum->offset ] = accum->cell_ms;
		accum->cell_ms = 0;
		M_INCREMENT_OFFSET( accum->offset );
		return;
	}

	factor *= (uint8_t)( accum->cells[ accum->offset ] );
	factor = (uint8_t)factor;

	ZEPTO_DEBUG_ASSERT( ( ( ( factor * mod[0] ) & 0xff ) + (uint8_t)( accum->cells[ accum->offset ] ) & 0xff ) == 0 );

	accum->cells[ accum->offset ] = accum->cell_ms;
	accum->cell_ms = 0;
	M_INCREMENT_OFFSET( accum->offset );

	pr = factor * mod[0]; 
	accum->cells[ accum->offset ] += ((uint8_t)(pr >> 8)) + 1;

	for ( i=0; i<M_BYTE_SIZE-1; i++ )
	{
		pr = factor * mod[i+1]; 
		accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ] += (uint8_t)pr;
		accum->cells[ ( accum->offset + i + 1 ) & ( M_BYTE_SIZE - 1 ) ] += (uint8_t)(pr >> 8);
	}

//	ZEPTO_DEBUG_ASSERT( accum->cell_ms <= 1 );
	ZEPTO_DEBUG_ASSERT( accum->cell_ms == 0 );
}

#ifdef M_ALLOW_NEGATIVES
void m_accumulator_normalize_assumed_positive( m_accumulator* accum )
#else
void m_accumulator_normalize( m_accumulator* accum )
#endif
{
	uint8_t i;
	uint16_t tmp = 0, tmp1 = 0;
	ZEPTO_DEBUG_ASSERT( accum->cell_ms <= 1 );
	for ( i=0; i<M_BYTE_SIZE; i++ )
	{
//		ZEPTO_DEBUG_ASSERT( accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ] >= 0 && accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ] <= (int16_t)0x7F00 );
		tmp += (uint8_t)(accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ]);
		tmp1 = (accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ]) >> 8;
		accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ] = (uint8_t)tmp;
		tmp >>= 8;
		tmp += tmp1;
	}
/*	tmp += accum->cell_ms;
	accum->cell_ms = (uint8_t)tmp;
	ZEPTO_DEBUG_ASSERT( accum->cell_ms == 0 || accum->cell_ms == 1 );
	ZEPTO_DEBUG_ASSERT( ( tmp >>= 8 ) == 0 );
	ZEPTO_DEBUG_ASSERT( accum->cell_ms <= 1 );*/
	tmp += (uint8_t)(accum->cell_ms);
	tmp1 = (accum->cell_ms) >> 8;
	accum->cell_ms = (uint8_t)tmp;
	tmp >>= 8;
	tmp += tmp1;
	ZEPTO_DEBUG_ASSERT( tmp == 0 );
	ZEPTO_DEBUG_ASSERT( accum->cell_ms <= 1 );
}

#ifdef M_ALLOW_NEGATIVES
void m_accumulator_normalize_expect_negative( m_accumulator* accum )
{
	uint8_t i;
	uint16_t tmp = 0;
	for ( i=0; i<M_BYTE_SIZE; i++ )
	{
		ZEPTO_DEBUG_ASSERT( accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ] >= -(int16_t)0x7F00 && accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ] <= (int16_t)0x7F00 );
		tmp += accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ];
		accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ] = (uint8_t)tmp;
		tmp >>= 8;
	}
	tmp += accum->cell_ms;
	accum->cell_ms = (uint8_t)tmp;
	ZEPTO_DEBUG_ASSERT( ( tmp >>= 8 ) == 0 );
}
#endif

void m_accumulator_make_compact( const m_accumulator* accum, uint8_t* buff )
{
	uint8_t i;
	uint16_t tmp = 0;
	ZEPTO_DEBUG_ASSERT( accum->cell_ms == 0 );
	ZEPTO_DEBUG_ASSERT( ( accum->cells[ ( accum->offset + M_BYTE_SIZE - 1 ) & ( M_BYTE_SIZE - 1 ) ] >> 8 ) == 0 );
	for ( i=0; i<M_BYTE_SIZE; i++ )
	{
		tmp += (uint8_t)( accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ] );
		buff[i] = (uint8_t)tmp; 
		tmp >>= 8;
		tmp += ( accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ] ) >> 8;
	}

//	ZEPTO_DEBUG_ASSERT( tmp == 0 );




/*	tmp += (uint8_t)(accum->cell_ms);
	buff[M_BYTE_SIZE] = (uint8_t)tmp; 
	tmp >>= 8;
	tmp += (accum->cell_ms) >> 8;
	buff[M_BYTE_SIZE+1] = (uint8_t)tmp; 
	buff[M_BYTE_SIZE+2] = (uint8_t)(tmp>>8); 
	ZEPTO_DEBUG_ASSERT( M_BYTE_SIZE_EXTRA == M_BYTE_SIZE + 3 ); // otherwise set the rest to 0*/
}

void m_accumulator_subtract_mod_unconditionally_and_make_compact( const m_accumulator* accum, const uint8_t* mod, uint8_t* buff )
{
	uint8_t i;
	uint16_t tmp;
//	uint8_t t1;

//	ZEPTO_DEBUG_ASSERT( accum->cell_ms <= 1 );
	ZEPTO_DEBUG_ASSERT( accum->cell_ms == 0 );
	ZEPTO_DEBUG_ASSERT( mod[0] != 0 );

	tmp = (uint16_t)(uint8_t)( accum->cells[ ( accum->offset ) & ( M_BYTE_SIZE - 1 ) ] ) + ( 0x100 - (uint16_t)(mod[0]) );
//	t1 = (uint8_t)( accum->cells[ ( accum->offset ) & ( M_BYTE_SIZE - 1 ) ] >> 8 );
	buff[0] = (uint8_t)(tmp);
	tmp >>= 8;
	ZEPTO_DEBUG_ASSERT( tmp <= 1 );

	for ( i=1; i<M_BYTE_SIZE; i++ )
	{
		tmp += ( accum->cells[ ( accum->offset + i - 1 ) & ( M_BYTE_SIZE - 1 ) ] ) >> 8;
		tmp += (uint8_t)( accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ] );
		tmp += 0xFF - (mod[i]);
		buff[i] = (uint8_t)tmp; 
		tmp >>= 8;
	}
//	ZEPTO_DEBUG_ASSERT( tmp == 1 );
	ZEPTO_DEBUG_ASSERT( tmp <= 1 );
}

void m_accumulator_subtract_mod_if_greater_eq_than_mod( const uint8_t* mod, uint8_t* buff )
{
	int8_t i;
	uint16_t tmp = 0;
	bool needs_subtr = true;
	for ( i=M_BYTE_SIZE-1; i>=0; i-- )
	{
		if ( buff[i] > mod[i] ) 
			break;
		if ( buff[i] < mod[i] ) return;
	}

	tmp = buff[0] + ( 0x100 - (uint16_t)(mod[0]) );
	buff[0] = (uint8_t)(tmp);
	tmp >>= 8;

	for ( i=1; i<M_BYTE_SIZE; i++ )
	{
		tmp += buff[i] + ( 0xFF - (uint16_t)(mod[i]) );
		buff[i] = (uint8_t)(tmp);
		tmp >>= 8;
	}
//	ZEPTO_DEBUG_ASSERT( tmp == 1 );
	ZEPTO_DEBUG_ASSERT( tmp <= 1 );
}

void m_accumulator_subtract_mod_if_necessary_and_make_compact( const m_accumulator* accum, const uint8_t* mod, uint8_t* buff )
{
	uint8_t i;
	uint16_t tmp = 0;
	ZEPTO_DEBUG_ASSERT( accum->cell_ms == 0 );
	ZEPTO_DEBUG_ASSERT( ( accum->cells[ ( accum->offset + M_BYTE_SIZE - 1 ) & ( M_BYTE_SIZE - 1 ) ] >> 8 ) == 0 );
	for ( i=0; i<M_BYTE_SIZE; i++ )
	{
		tmp += (uint8_t)( accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ] );
		buff[i] = (uint8_t)tmp; 
		tmp >>= 8;
		tmp += ( accum->cells[ ( accum->offset + i ) & ( M_BYTE_SIZE - 1 ) ] ) >> 8;
	}

	bool needs_subtr;
	if ( tmp )
	{
		needs_subtr = true;
	}
	else
	{
		for ( i=M_BYTE_SIZE-1; i>=0; i-- )
		{
			if ( buff[i] > mod[i] ) 
				break;
			if ( buff[i] < mod[i] ) return;
		}
	}

	tmp = buff[0] + ( 0x100 - (uint16_t)(mod[0]) );
	buff[0] = (uint8_t)(tmp);
	tmp >>= 8;

	for ( i=1; i<M_BYTE_SIZE; i++ )
	{
		tmp += buff[i] + ( 0xFF - (uint16_t)(mod[i]) );
		buff[i] = (uint8_t)(tmp);
		tmp >>= 8;
	}
//	ZEPTO_DEBUG_ASSERT( tmp == 1 );
	ZEPTO_DEBUG_ASSERT( tmp <= 1 );
}

bool m_accumulator_is_greater_eq_than_mod( const uint8_t* mod, const uint8_t* buff )
{
	int8_t i;
	uint16_t tmp = 0;
	for ( i=M_BYTE_SIZE-1; i>=0; i-- )
	{
		if ( buff[i] > mod[i] ) 
			return true;
		if ( buff[i] < mod[i] ) 
			return false;
	}
	return true;
}

#define M_ACCUMULATOR_CHECK_BLOCK

void m_accumulator_m_step( m_accumulator* accum, const uint8_t* mod, uint8_t inverter, const uint8_t* a, const uint8_t* b, uint8_t* res )
{
	uint8_t i;


#ifdef M_ACCUMULATOR_CHECK_BLOCK
	uint4096_t accum_b = 0, a_b, b_b, mod_b, res_b, accum_alt;
	uint4096_t temp_b;
	uint4096_t low_bt, factor;
	m_accumulator_to_BOOST( mod, &mod_b );
	m_accumulator_to_BOOST( a, &a_b );
	m_accumulator_to_BOOST( b, &b_b );
	ZEPTO_DEBUG_ASSERT( ( a_b >> ( M_BYTE_SIZE * 8 ) ) == 0 );
	ZEPTO_DEBUG_ASSERT( ( b_b >> ( M_BYTE_SIZE * 8 ) ) == 0 );
/*	for ( i = 0; i<M_BYTE_SIZE; i++ )
	{
		low_bt = b_b & 0xff;
		b_b = b_b >> 8;
		temp_b = a_b * low_bt;
		accum_b = accum_b + temp_b;
		low_bt = accum_b & 0xff;
		factor = (low_bt * inverter) & 0xff;
		temp_b = mod_b * factor;
		accum_b = accum_b + temp_b;
		ZEPTO_DEBUG_ASSERT( (accum_b & 0xff) == 0 );
		accum_b = accum_b >> 8;
	}*/
/*
	res_b = accum_b;

	if ( res_b > mod_b ) res_b = res_b - mod_b;
	ZEPTO_DEBUG_ASSERT( ( res_b >> ( M_BYTE_SIZE * 8 ) ) == 0 );
	*/
#endif




	m_accumulator_init_clear( accum );
#if M_BYTE_SIZE >= 32
	for ( i = 0; i<16; i++ )
	{
		m_accumulator_add_product( accum, a, b[i] );
#ifdef M_ACCUMULATOR_CHECK_BLOCK
		low_bt = b_b & 0xff;
		b_b = b_b >> 8;
		temp_b = a_b * low_bt;
		accum_b = accum_b + temp_b;
		m_accumulator_ACCUMULATOR_to_BOOST( accum, &accum_alt );
		ZEPTO_DEBUG_ASSERT( accum_b == accum_alt );
#endif
		m_accumulator_do_modular_part( accum, mod, inverter );
#ifdef M_ACCUMULATOR_CHECK_BLOCK
		low_bt = accum_b & 0xff;
		factor = (low_bt * inverter) & 0xff;
		temp_b = mod_b * factor;
		accum_b = accum_b + temp_b;
		ZEPTO_DEBUG_ASSERT( (accum_b & 0xff) == 0 );
		accum_b = accum_b >> 8;
		m_accumulator_ACCUMULATOR_to_BOOST( accum, &accum_alt );
		ZEPTO_DEBUG_ASSERT( accum_b == accum_alt );
#endif
	}
	m_accumulator_normalize( accum );
#ifdef M_ACCUMULATOR_CHECK_BLOCK
		ZEPTO_DEBUG_ASSERT( accum_b == accum_alt );
#endif
	for ( i = 16; i<M_BYTE_SIZE; i++ )
	{
		m_accumulator_add_product( accum, a, b[i] );
#ifdef M_ACCUMULATOR_CHECK_BLOCK
		low_bt = b_b & 0xff;
		b_b = b_b >> 8;
		temp_b = a_b * low_bt;
		accum_b = accum_b + temp_b;
		m_accumulator_ACCUMULATOR_to_BOOST( accum, &accum_alt );
		ZEPTO_DEBUG_ASSERT( accum_b == accum_alt );
#endif
		m_accumulator_do_modular_part( accum, mod, inverter );
#ifdef M_ACCUMULATOR_CHECK_BLOCK
		low_bt = accum_b & 0xff;
		factor = (low_bt * inverter) & 0xff;
		temp_b = mod_b * factor;
		accum_b = accum_b + temp_b;
		ZEPTO_DEBUG_ASSERT( (accum_b & 0xff) == 0 );
		accum_b = accum_b >> 8;
		m_accumulator_ACCUMULATOR_to_BOOST( accum, &accum_alt );
		ZEPTO_DEBUG_ASSERT( accum_b == accum_alt );
#endif
	}
#elif M_BYTE_SIZE >= 64
#error not implemented
#endif

	// now we need to make compact and subtract mod, if necessary
	bool needs_mod_subtr = accum->cell_ms != 0 || 
//		( accum->cells[ ( accum->offset + M_BYTE_SIZE - 1 ) & ( M_BYTE_SIZE - 1 ) ] ) > ( (uint16_t)(mod[M_BYTE_SIZE - 2]) + ( ((uint16_t)(mod[M_BYTE_SIZE - 1])) << 8 ) );
		( accum->cells[ ( accum->offset + M_BYTE_SIZE - 1 ) & ( M_BYTE_SIZE - 1 ) ] ) > mod[M_BYTE_SIZE - 1];
	if ( needs_mod_subtr )
	{
		m_accumulator_subtract_mod_unconditionally_and_make_compact( accum, mod, res );
#ifdef M_ACCUMULATOR_CHECK_BLOCK
		res_b = accum_b;
		ZEPTO_DEBUG_ASSERT( res_b >= mod_b );
		res_b = res_b - mod_b;
		ZEPTO_DEBUG_ASSERT( ( res_b >> ( M_BYTE_SIZE * 8 ) ) == 0 );
		m_accumulator_to_BOOST( res, &accum_alt );
		if ( accum_b != accum_alt )
			accum_b == accum_alt;
		ZEPTO_DEBUG_ASSERT( res_b == accum_alt );
#endif
	}
	else
	{
		ZEPTO_DEBUG_ASSERT( ( accum->cells[ ( accum->offset + M_BYTE_SIZE - 1 ) & ( M_BYTE_SIZE - 1 ) ] >> 8 ) == 0 );

		m_accumulator_subtract_mod_if_necessary_and_make_compact( accum, mod, res );

//		m_accumulator_make_compact( accum, res );
#ifdef M_ACCUMULATOR_CHECK_BLOCK
/*		m_accumulator_to_BOOST( res, &accum_alt );
		ZEPTO_DEBUG_ASSERT( accum_b == accum_alt );
#endif
		m_accumulator_subtract_mod_if_greater_eq_than_mod( mod, res );
#ifdef M_ACCUMULATOR_CHECK_BLOCK*/
		res_b = accum_b;
		ZEPTO_DEBUG_ASSERT( res_b != mod_b );
		if ( res_b >= mod_b )
			res_b = res_b - mod_b;
		m_accumulator_to_BOOST( res, &accum_alt );
		if ( res_b != accum_alt )
			i++;
		ZEPTO_DEBUG_ASSERT( res_b == accum_alt );
		ZEPTO_DEBUG_ASSERT( res_b < mod_b );
#endif
		ZEPTO_DEBUG_ASSERT( !m_accumulator_is_greater_eq_than_mod( mod, res ) );
/*		if ( !m_accumulator_is_greater_eq_than_mod( mod, res ) )
			m_accumulator_subtract_mod_if_greater_eq_than_mod( mod, res );
		ZEPTO_DEBUG_ASSERT( !m_accumulator_is_greater_eq_than_mod( mod, res ) );*/
	}
}

void m_accumulator_m_calc_num_prim( m_accumulator* accum, const uint8_t* mod, uint8_t* pow2_2n_mod, uint8_t sel_bt, uint8_t* num, uint8_t* res )
{
	m_accumulator_m_step( accum, mod, sel_bt, num, pow2_2n_mod, res );
}

void m_accumulator_m_restore_numm( m_accumulator* accum, const uint8_t* mod, uint8_t sel_bt, uint8_t* num_prim, uint8_t* num )
{
	uint8_t one[M_BYTE_SIZE];
	memset( one, 0, M_BYTE_SIZE );
	one[0] = 1;
	m_accumulator_m_step( accum, mod, sel_bt, one, num_prim, num );
}

void m_accumulator_m_exponentiate( m_accumulator* accum, const uint8_t* mod, uint8_t inverter, uint8_t* x, const uint8_t* exp, uint8_t* res )
{
	for ( int i=0; i<M_BYTE_SIZE*8; i++ )
	{
		if ( exp[i/8] & (1 << (i%8)) )
		{
			m_accumulator_m_step( accum, mod, inverter, res, x, res );
			if ( res == 0 )
			{
				printf( "res=0 at i=%d\n", i );
				break;
			}
		}

//		exp = exp >> 1;

		m_accumulator_m_step( accum, mod, inverter, x, x, x );

		// check how bit size of x changes
/*		for ( int j=0; j<(bt_cnt+256)*8; j++ )
		{
			if ( (res>>j) == 0 )
			{
				printf( "%d: sz = %d\n", i, j );
				break;
			}
		}*/
	}
}




















void m_precalc_pow2_2n_mod( uint8_t bt_cnt, uint4096_t mod, uint4096_t& res )
{
	uint4096_t t = 1;
	t = t << ( bt_cnt * 8 * 2 );
	res = t % mod;
}

void m_calc_num_prim( uint8_t bt_cnt, uint4096_t mod, uint4096_t pow2_2n_mod, uint8_t sel_bt, uint4096_t num, uint4096_t& res )
{
	m_step( bt_cnt, mod, sel_bt, num, pow2_2n_mod, res );
}

void m_restore_numm( uint8_t bt_cnt, uint4096_t mod, uint4096_t pow2_2n_mod, uint8_t sel_bt, uint4096_t num_prim, uint4096_t& num )
{
	m_step( bt_cnt, mod, sel_bt, 1, num_prim, num );
}

void m_exponentiate( uint8_t bt_cnt, uint4096_t mod, uint8_t sel_bt, uint4096_t x, uint4096_t exp, uint4096_t& res )
{
	for ( int i=0; i<bt_cnt*8; i++ )
	{
		if ( exp & 1 )
		{
			m_step( bt_cnt, mod, sel_bt, res, x, res );
			if ( res == 0 )
			{
				printf( "res=0 at i=%d\n", i );
				break;
			}
		}

		exp = exp >> 1;

		m_step( bt_cnt, mod, sel_bt, x, x, x );

		// check how bit size of x changes
/*		for ( int j=0; j<(bt_cnt+256)*8; j++ )
		{
			if ( (res>>j) == 0 )
			{
				printf( "%d: sz = %d\n", i, j );
				break;
			}
		}*/
	}
}

int test_approach_2()
{
	uint8_t bt_cnt = 32;
//	uint8_t bt_cnt = 1;
	uint8_t sel_bt;
	uint4096_t x, mod, exp, res, pow2_2n_mod, x_prim, res_prim, res2;

/*	mod = 3;
	for ( int i=0; i<120; i++ )
		mod = mod * 3;
	mod = 101;*/
	mod = 1;
	mod = ( mod << (bt_cnt*8) ) - 13;
//	mod = 127;
	mod = 65521;

	exp = mod;

	x = 7;
//	x = (mod>>1)+2;
	res = 1;

	sel_bt = m_calc_selector_factor( mod );
	m_precalc_pow2_2n_mod( bt_cnt, mod, pow2_2n_mod );
	m_calc_num_prim( bt_cnt, mod, pow2_2n_mod, sel_bt, x, x_prim );
	m_calc_num_prim( bt_cnt, mod, pow2_2n_mod, sel_bt, res, res_prim );
	m_exponentiate( bt_cnt, mod, sel_bt, x_prim, exp, res_prim );
	m_restore_numm( bt_cnt, mod, pow2_2n_mod, sel_bt, res_prim, res );

	exponentiate( x, mod, exp, res2 );
//	bool same = res == 1 || res == mod - 1 || res == x || res == mod-x;
	bool same = res == res2;
	printf( same ? "OK\n" : "FAILED\n" );

	return 0;
}


int test_approach_3()
{
	uint8_t bt_cnt = 32;
//	uint8_t bt_cnt = 1;
	uint8_t sel_bt;
	uint4096_t x, mod, exp, res, pow2_2n_mod, x_prim, res_prim, res2;
	uint8_t xn[M_BYTE_SIZE], modn[M_BYTE_SIZE], expn[M_BYTE_SIZE], resn[M_BYTE_SIZE], pow2_2n_modn[M_BYTE_SIZE], x_primn[M_BYTE_SIZE], res_primn[M_BYTE_SIZE];
	uint4096_t resn_boost;
	m_accumulator accum;

/*	mod = 3;
	for ( int i=0; i<120; i++ )
		mod = mod * 3;
	mod = 101;*/
	mod = 1;
	mod = ( mod << (bt_cnt*8) ) - 13;
//	mod = 127;
//	mod = 65521;

	exp = mod;

	x = 7;
//	x = (mod>>1)+2;
	res = 1;

	sel_bt = m_calc_selector_factor( mod );
	m_precalc_pow2_2n_mod( bt_cnt, mod, pow2_2n_mod );


	m_accumulator_from_BOOST( &mod, modn );
	m_accumulator_from_BOOST( &exp, expn );
	m_accumulator_from_BOOST( &x, xn );
	m_accumulator_from_BOOST( &res, resn );
	m_accumulator_from_BOOST( &pow2_2n_mod, pow2_2n_modn );
	// check validity of convertions
	uint4096_t tempo;
	m_accumulator_to_BOOST( modn, &tempo );
	ZEPTO_DEBUG_ASSERT( tempo == mod );

	// block 1: using M-mul and boost
	m_calc_num_prim( bt_cnt, mod, pow2_2n_mod, sel_bt, x, x_prim );
	m_calc_num_prim( bt_cnt, mod, pow2_2n_mod, sel_bt, res, res_prim );
	m_exponentiate( bt_cnt, mod, sel_bt, x_prim, exp, res_prim );
	m_restore_numm( bt_cnt, mod, pow2_2n_mod, sel_bt, res_prim, res );

	// block 2: using M-mul with no boost
	m_accumulator_m_calc_num_prim( &accum, modn, pow2_2n_modn, sel_bt, xn, x_primn );
	m_accumulator_m_calc_num_prim( &accum, modn, pow2_2n_modn, sel_bt, resn, res_primn );
	m_accumulator_m_exponentiate( &accum, modn, sel_bt, x_primn, expn, res_primn );
	m_accumulator_m_restore_numm( &accum, modn, sel_bt, res_primn, resn );

	m_accumulator_to_BOOST( resn, &resn_boost );

	// block 3: using std mul and boost
	exponentiate( x, mod, exp, res2 );
//	bool same = res == 1 || res == mod - 1 || res == x || res == mod-x;
	bool same = res == res2;
	printf( same ? "OK\n" : "FAILED\n" );

	same = res == resn_boost;
	printf( same ? "OK\n" : "FAILED\n" );

	return 0;
}

int quick_test()
{
//	uint32_t base = 0x1c6a5;
	uint32_t base = 0xffff;
	uint32_t mod = 0xd497;
	uint32_t diff1, diff2, tmp;

	for ( mod = 1; mod<0x10000; mod++ )
	{
		if ( (mod&0xff) == 0 )
			continue;
		diff1 = 0x10000 - mod;
		ZEPTO_DEBUG_ASSERT( ( (diff1&0xff) == (0x100 - (uint8_t)mod) ) );
		ZEPTO_DEBUG_ASSERT( ( ((diff1>>8)&0xff) == (0xff - (uint8_t)(mod>>8)) ) );
	}

	for ( mod = 0xc6a6; mod<0x10000&&mod<base; mod ++ )
	{
		if ( (mod&0xff) == 0 )
			continue;

		diff1 = base - mod;

		tmp = ( base & 0xFF ) + ( 0x100 - ((uint8_t)mod) );
		diff2 = (uint8_t)tmp;
		tmp >>= 8;

		tmp += ( (base>>8) & 0xFF ) + ( 0xff - ((uint8_t)(mod>>8)) );
		diff2 += ((uint16_t)(uint8_t)tmp) << 8;

		if ( diff1 != diff2 )
			printf( "failed at %d (%x)\n", mod, mod );
		ZEPTO_DEBUG_ASSERT( diff1 == diff2 );
	}

	return 0;
}


int main(int argc, char* argv[])
{
//	return quick_test();
	return test_approach_3();
	return clean_test();
//	return int1024_test();
//	return int_test();
	uint4096_t x, mod, exp, res, res2, R, R_inv, R2Bar, mod_inv, x_bar, res2_bar;

	R = 1;
	R = R << 256;

	mod = 3;
	for ( int i=0; i<120; i++ )
		mod = mod * 3;

	exp = mod;

	x = 7;

	exponentiate( x, mod, exp, res );

	bool same = res == 1 || res == mod - 1;
	printf( same ? "OK\n" : "FAILED\n" );

	exp = mod;
	res2 = 1;
	calc_r_sq_bar( mod, R, R2Bar );
	calc_r_inv_neg_mod_inv( R, mod, R_inv, mod_inv );
	// now we can use precalculated values:
	// mod, mod_inv, R2Bar

	// initial setup
	calc_num_bar_alt( R, R2Bar, mod, mod_inv, x, x_bar );
	calc_num_bar_alt( R, R2Bar, mod, mod_inv, res2, res2_bar );
	// exponentiation
	exponentiate_m( mod, mod_inv, R, x_bar, exp, res2_bar );
	// recovery
	unbar( mod, mod_inv, R, res2_bar, res2 );
	same = res2 == 1 || res2 == mod - 1;
	printf( same ? "OK\n" : "FAILED\n" );



/*	mod = 97; R = 100;
	calc_r_inv_neg_mod_inv( R, mod, R_inv, mod_inv );*/

/*	int x, mod = 97, exp, res, R =100, R_inv, mod_inv;
//	int x, mod = 46, exp, res, R =240, R_inv, mod_inv;
	calc_r_inv_neg_mod_inv( R, mod, R_inv, mod_inv );*/
	return 0;
}

