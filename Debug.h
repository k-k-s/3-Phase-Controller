/* 
 * File:   Debug.h
 * Author: Cameron
 *
 * Created on January 9, 2015, 4:09 PM
 */

#ifndef DEBUG_H
#define	DEBUG_H

#include "Board.h"


namespace Debug {
 extern IOpin &LED;

 void init();
 
 void endLine();
 
 void reportByte(u1 const);
 void reportHexByte(u1 const);
 void reportPhase(u2 const);
 void reportMag(u2 const);
 void reportClock();

};

#endif	/* DEBUG_H */

