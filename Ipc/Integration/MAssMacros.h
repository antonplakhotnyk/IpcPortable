#pragma once


//-------------------------------------------------------
// LogMsg


// #ifdef _DEBUG
// #define check_point AVLogMsg(__FILE__, __LINE__, __FUNCTION__)
// #define check_point_manual(file,line) AVLogMsg(file, line, __FUNCTION__)
// #else					  
// #ifdef DEBUG_POINT_IN_RELEASE
// #define check_point AVLogMsg(__FILE__, __LINE__, __FUNCTION__)
// #define check_point_manual(file,line) AVLogMsg(file, line, __FUNCTION__)
// #else					  
// #define check_point 
// #define check_point_manual(file,line)
// #endif// DEBUG_POINT_IN_RELEASE
// #endif// _DEBUG

// #define mass_av_assert_msg(msg_a) { if(ErrorPool::Raise(__FILE__,__LINE__,__FUNCTION__)) {/*AssertProc(msg_a,__FILE__,__LINE__,__FUNCTION__);*/ } }
// #define av_assert()	{ if(ErrorPool::Raise(__FILE__,__LINE__,__FUNCTION__)) {/*AssertProc(msg_a,__FILE__,__LINE__,__FUNCTION__);*/ } }
#define mass_av_assert_msg(msg_a) {}
#define mass_av_assert()	{}

//-------------------------------------------------------

#define mass_assert_if_equal_msg(x1,x2,msg_a)		{ if( (x1) == (x2) ) { mass_av_assert_msg(msg_a ); } }
#define mass_assert_if_not_equal_msg(x1,x2,msg_a)	{ if( (x1) != (x2) ) { mass_av_assert_msg(msg_a ); } }

#define mass_assert_if_equal(x1,x2)						{ if( (x1) == (x2) ) { mass_av_assert_msg(#x1" EQUAL "#x2 ); } }
#define mass_assert_if_not_equal(x1,x2)					{ if( (x1) != (x2) ) { mass_av_assert_msg(#x1" NOT EQUAL "#x2 ); } }

#define mass_return_if_equal(x1,x2)						{ if( (x1) == (x2) ) { mass_av_assert_msg(#x1" EQUAL "#x2 );		return; } }
#define mass_return_if_not_equal(x1,x2)					{ if( (x1) != (x2) ) { mass_av_assert_msg(#x1" NOT EQUAL "#x2 );	return; } }
#define mass_return_if_not_equal_msg(x1,x2,msg_a)		{ if( (x1) != (x2) ) { mass_av_assert_msg(msg_a ); 				return; } }
#define mass_return_if_equal_msg(x1,x2,msg_a)			{ if( (x1) == (x2) ) { mass_av_assert_msg(msg_a ); 				return; } }
#define mass_return_x_if_not_equal(x1,x2,x)				{ if( (x1) != (x2) ) { mass_av_assert_msg(#x1" NOT EQUAL "#x2 );	return x; } }
#define mass_return_x_if_equal(x1,x2,x)					{ if( (x1) == (x2) ) { mass_av_assert_msg(#x1" EQUAL "#x2 );		return x; } }
#define mass_return_x_if_not_equal_msg(x1,x2,x,msg_a)	{ if( (x1) != (x2) ) { mass_av_assert_msg(msg_a ); 				return x; } }
#define mass_return_x_if_equal_msg(x1,x2,x,msg_a)		{ if( (x1) == (x2) ) { mass_av_assert_msg(msg_a ); 				return x; } }


#define mass_return_if_equal_final(x1,x2,finalize)					{ if( (x1) == (x2) ) { mass_av_assert_msg(#x1" EQUAL "#x2 );		{finalize} return; } }
#define mass_return_if_not_equal_final(x1,x2,finalize)				{ if( (x1) != (x2) ) { mass_av_assert_msg(#x1" NOT EQUAL "#x2 );	{finalize} return; } }
#define mass_return_if_not_equal_msg_final(x1,x2,msg_a,finalize)		{ if( (x1) != (x2) ) { mass_av_assert_msg(msg_a ); 				{finalize} return; } }
#define mass_return_if_equal_msg_final(x1,x2,msg_a,finalize)			{ if( (x1) == (x2) ) { mass_av_assert_msg(msg_a ); 				{finalize} return; } }
#define mass_return_x_if_not_equal_final(x1,x2,x,finalize)			{ if( (x1) != (x2) ) { mass_av_assert_msg(#x1" NOT EQUAL "#x2 );	{finalize} return x; } }
#define mass_return_x_if_equal_final(x1,x2,x,finalize)				{ if( (x1) == (x2) ) { mass_av_assert_msg(#x1" EQUAL "#x2 );		{finalize} return x; } }
#define mass_return_x_if_not_equal_msg_final(x1,x2,x,msg_a,finalize)	{ if( (x1) != (x2) ) { mass_av_assert_msg(msg_a ); 				{finalize} return x; } }
#define mass_return_x_if_equal_msg_final(x1,x2,x,msg_a,finalize)		{ if( (x1) == (x2) ) { mass_av_assert_msg(msg_a ); 				{finalize} return x; } }


#define mass_return_msg(msg_a)			{ mass_av_assert_msg( msg_a ); return; }
#define mass_return_x( x )				{ mass_av_assert(); return x; }
#define mass_return_x_msg( x, msg_a )	{ mass_av_assert_msg( msg_a ); return x; }

//-------------------------------------------------------
