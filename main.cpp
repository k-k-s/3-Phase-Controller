/* 
 * File:   main.cpp
 * Author: btacklind
 *
 * Created on Nov 28, 2014, 2:02 PM
 */

#include <util/delay.h>
#include <avr/wdt.h>
#include "Board.h"

#include "ThreePhaseController.h"
#include "MLX90363.h"
#include "TwillBotInterface.h"
#include "Debug.h"
#include "Clock.h"
#include "Interpreter.h"

/**
 * All the init functions should go in here.
 * gcc will automatically call it for us because of the constructor attribute.
 */
void init() __attribute__((constructor));

void init() {
 wdt_reset();
 wdt_disable();
 // Debug::init();

 sei();
 
 ::Clock::init();
 
 TwillBotInterface::init();
 
 ThreePhaseController::init();
 // ThreePhaseDriver::init();
 // ThreePhaseDriver::setAmplitude(0);
 
 // End of init
 Board::LED.output();
 Board::LED.off();
}

/*
 * 
 */
void main() {
 
 u2 pos = 0;
 
 Clock::MicroTime t(0);
 Clock::MicroTime delta = 25_ms;
 Clock::MicroTime now;
 
 Board::LED.on();
 
 
 ThreePhaseController::setTorque(0);
 
 while(1){
  ThreePhaseController::updateDriver();

  if(TwillBotInterface::hasReceivedBlock()){
    TwillBotInterface::reserveNextReadBuffer();

    Interpreter::interpretFromMaster(TwillBotInterface::getIncomingReadBuffer());
  }
  
  Interpreter::sendNormalDataToMaster();
 }

 while(1);
}

