#pragma once

#include "MAssIpc_Assert.h"

//-------------------------------------------------------

#define assert_msg_mass_ipc(assert_msg) {MAssIpc_Assert::Raise(__FILE__,__LINE__,__FUNCTION__, assert_msg);}
#define assert_mass_ipc()	{assert_msg_mass_ipc("");}

//-------------------------------------------------------

#define assert_if_equal_msg_mass_ipc(x1,x2,assert_msg)							{ if( (x1) == (x2) ) { assert_msg_mass_ipc(assert_msg); } }
#define assert_if_not_equal_msg_mass_ipc(x1,x2,assert_msg)						{ if( (x1) != (x2) ) { assert_msg_mass_ipc(assert_msg); } }

#define assert_if_equal_mass_ipc(x1,x2)											{ if( (x1) == (x2) ) { assert_msg_mass_ipc(#x1" EQUAL "#x2 ); } }
#define assert_if_not_equal_mass_ipc(x1,x2)										{ if( (x1) != (x2) ) { assert_msg_mass_ipc(#x1" NOT EQUAL "#x2 ); } }

#define return_if_equal_mass_ipc(x1,x2)											{ if( (x1) == (x2) ) { assert_msg_mass_ipc(#x1" EQUAL "#x2 );			return; } }
#define return_if_not_equal_mass_ipc(x1,x2)										{ if( (x1) != (x2) ) { assert_msg_mass_ipc(#x1" NOT EQUAL "#x2 );		return; } }
#define return_if_not_equal_msg_mass_ipc(x1,x2,assert_msg)						{ if( (x1) != (x2) ) { assert_msg_mass_ipc(assert_msg); 				return; } }
#define return_if_equal_msg_mass_ipc(x1,x2,assert_msg)							{ if( (x1) == (x2) ) { assert_msg_mass_ipc(assert_msg); 				return; } }
#define return_x_if_not_equal_mass_ipc(x1,x2,x)									{ if( (x1) != (x2) ) { assert_msg_mass_ipc(#x1" NOT EQUAL "#x2 );		return x; } }
#define return_x_if_equal_mass_ipc(x1,x2,x)										{ if( (x1) == (x2) ) { assert_msg_mass_ipc(#x1" EQUAL "#x2 );			return x; } }
#define return_x_if_not_equal_msg_mass_ipc(x1,x2,x,assert_msg)					{ if( (x1) != (x2) ) { assert_msg_mass_ipc(assert_msg); 				return x; } }
#define return_x_if_equal_msg_mass_ipc(x1,x2,x,assert_msg)						{ if( (x1) == (x2) ) { assert_msg_mass_ipc(assert_msg); 				return x; } }


//-------------------------------------------------------
