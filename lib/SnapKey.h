//
//  SnapKey.h
//
//
//  Created by Kun Zhao on 2014-09-04 16:12:21.
//
//

#ifndef KUN__SNAPKEY_H_INCLUDE
#define KUN__SNAPKEY_H_INCLUDE

#include <kvs/KeyPressEventListener>

class SnapKey : public kvs::KeyPressEventListener 
{
	void update( kvs::KeyEvent* event );
};

#endif // KUN__SNAPKEY_H_INCLUDE
