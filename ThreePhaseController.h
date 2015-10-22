/* 
 * File:   ThreePhaseController.h
 * Author: Cameron
 *
 * Created on October 22, 2015, 2:21 AM
 */

#ifndef THREEPHASECONTROLLER_H
#define	THREEPHASECONTROLLER_H

#include <AVR++/basicTypes.h>

class ThreePhaseController {
 static s2 position;
 static s2 velocity;
 
public:
 static void init();
 
 static void setTorque(s2 t);
 
 inline static u2 getPosition() {return position;}
 inline static s2 getVelocity() {return velocity;}

};

#endif	/* THREEPHASECONTROLLER_H */

