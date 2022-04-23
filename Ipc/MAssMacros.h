#pragma once

#include "MAssIpcAssert.h"

//-------------------------------------------------------

#define mass_assert_msg(assert_msg) {MAssIpcAssert::Raise(__FILE__,__LINE__,__FUNCTION__, assert_msg);}
#define mass_assert()	{mass_assert_msg(MASS_ASSERT_STR(""));}

//-------------------------------------------------------

#define mass_assert_if_equal_msg(x1,x2,assert_msg)							{ if( (x1) == (x2) ) { mass_assert_msg(assert_msg); } }
#define mass_assert_if_not_equal_msg(x1,x2,assert_msg)						{ if( (x1) != (x2) ) { mass_assert_msg(assert_msg); } }

#define mass_assert_if_equal(x1,x2)											{ if( (x1) == (x2) ) { mass_assert_msg(MASS_ASSERT_STR(#x1" EQUAL "#x2) ); } }
#define mass_assert_if_not_equal(x1,x2)										{ if( (x1) != (x2) ) { mass_assert_msg(MASS_ASSERT_STR(#x1" NOT EQUAL "#x2) ); } }

#define mass_return_if_equal(x1,x2)											{ if( (x1) == (x2) ) { mass_assert_msg(MASS_ASSERT_STR(#x1" EQUAL "#x2) );			return; } }
#define mass_return_if_not_equal(x1,x2)										{ if( (x1) != (x2) ) { mass_assert_msg(MASS_ASSERT_STR(#x1" NOT EQUAL "#x2) );		return; } }
#define mass_return_if_not_equal_msg(x1,x2,assert_msg)						{ if( (x1) != (x2) ) { mass_assert_msg(assert_msg); 								return; } }
#define mass_return_if_equal_msg(x1,x2,assert_msg)							{ if( (x1) == (x2) ) { mass_assert_msg(assert_msg); 								return; } }
#define mass_return_x_if_not_equal(x1,x2,x)									{ if( (x1) != (x2) ) { mass_assert_msg(MASS_ASSERT_STR(#x1" NOT EQUAL "#x2) );		return x; } }
#define mass_return_x_if_equal(x1,x2,x)										{ if( (x1) == (x2) ) { mass_assert_msg(MASS_ASSERT_STR(#x1" EQUAL "#x2) );			return x; } }
#define mass_return_x_if_not_equal_msg(x1,x2,x,assert_msg)					{ if( (x1) != (x2) ) { mass_assert_msg(assert_msg); 								return x; } }
#define mass_return_x_if_equal_msg(x1,x2,x,assert_msg)						{ if( (x1) == (x2) ) { mass_assert_msg(assert_msg); 								return x; } }


#define mass_return_if_equal_final(x1,x2,finalize)							{ if( (x1) == (x2) ) { mass_assert_msg(MASS_ASSERT_STR(#x1" EQUAL "#x2) );			{finalize} return; } }
#define mass_return_if_not_equal_final(x1,x2,finalize)						{ if( (x1) != (x2) ) { mass_assert_msg(MASS_ASSERT_STR(#x1" NOT EQUAL "#x2) );		{finalize} return; } }
#define mass_return_if_not_equal_msg_final(x1,x2,assert_msg,finalize)		{ if( (x1) != (x2) ) { mass_assert_msg(assert_msg ); 								{finalize} return; } }
#define mass_return_if_equal_msg_final(x1,x2,assert_msg,finalize)	 		{ if( (x1) == (x2) ) { mass_assert_msg(assert_msg ); 								{finalize} return; } }
#define mass_return_x_if_not_equal_final(x1,x2,x,finalize)					{ if( (x1) != (x2) ) { mass_assert_msg(MASS_ASSERT_STR(#x1" NOT EQUAL "#x2) );		{finalize} return x; } }
#define mass_return_x_if_equal_final(x1,x2,x,finalize)						{ if( (x1) == (x2) ) { mass_assert_msg(MASS_ASSERT_STR(#x1" EQUAL "#x2) );			{finalize} return x; } }
#define mass_return_x_if_not_equal_msg_final(x1,x2,x,assert_msg,finalize)	{ if( (x1) != (x2) ) { mass_assert_msg(assert_msg ); 								{finalize} return x; } }
#define mass_return_x_if_equal_msg_final(x1,x2,x,assert_msg,finalize)		{ if( (x1) == (x2) ) { mass_assert_msg(assert_msg ); 								{finalize} return x; } }


#define mass_return_msg(assert_msg)			{ mass_assert_msg( assert_msg ); return; }
#define mass_return()					{ mass_assert(); return; }
#define mass_return_x( x )				{ mass_assert(); return x; }
#define mass_return_x_msg( x, assert_msg )	{ mass_assert_msg( assert_msg ); return x; }

//-------------------------------------------------------
