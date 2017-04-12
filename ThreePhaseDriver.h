/* 
 * File:   ThreePhaseDriver.h
 * Author: Cameron
 *
 * Created on December 17, 2014, 3:20 PM
 */

#ifndef THREEPHASEDRIVER_H
#define	THREEPHASEDRIVER_H

#include <AVR++/basicTypes.h>
#include <avr/interrupt.h>
#include "DriverConstants.h"

namespace ThreePhaseControllerNamespace {

using namespace AVR;

u1 constexpr min(u1 const a, u1 const b){return a > b ? b : a;};

/**
 * This static class handles controlling the low level TIMER4 registers.
 *
 * It has one main interface, advanceTo(PhasePosition), that calculates the correct
 * PWM values for the outputs and switches them all to their new values atomically.
 */
class ThreePhaseDriver {
  /**
   * Read the specialized sine lookup table
   * @param phase
   * @return
   */
  static inline u2 getPhasePWM(u1 const phase) __attribute__((const));

  /**
   * The current PWM amplitude
   */
  static u1 amplitude;

  // static u1 cacheA;
  // static register u1 cacheB asm("r11");
  // static register u1 cacheC asm("r12");
public:

  /**
   * The outputs can be in one of 3 phases
   */
  enum class Phase : u1 {
    A = 0, B = 1, C = 2, INIT
  };

protected:
  static Phase currentPhase;

public:
  /**
   * Internal granularity of sin wave for each phase
   */
  static u2 constexpr StepsPerPhase = DriverConstants::StepsPerPhase;

  /**
   * One for each of A, B, and C.
   */
  static u1 constexpr PhasesPerCycle = DriverConstants::PhasesPerCycle;

  /**
   * One Cycle is one full commutation (aka electrical revolution) of the motor.
   * This is almost certainly not one actual revolution of the motor shaft.
   */
  static u2 constexpr StepsPerCycle = DriverConstants::StepsPerCycle;

  /**
   * Highest possible timer value
   */
  static u2 constexpr MAX = 0x7ff;

  static void init();

  /**
   * Convenience function with its own internal step counter
   */
  static void advance();

  static u2 lastPhase;
  /**
   * Version of advanceTo() that takes a single u2 between 0 and 0x2ff inclusive
   * @param step
   */
  inline static void advanceTo(u2 const step) {
    lastPhase = step;
    advanceToFullSine((Phase) (step >> 8), step);
  }

  /**
   * Advance the pwm outputs to a new commutation angle
   *
   * @param phase
   * @param step
   */
  static void advanceToFullSine(Phase const phase, u1 const step);

  /**
   * Magic number to ensure we don't miss a tick of a phase
   */
  static constexpr u1 calcMaxAmplitude = 0xff - 30;
  static constexpr u1 maxAmplitude = min(DriverConstants::MaxTorque, calcMaxAmplitude);

  static inline void setAmplitude(u1 const a) {
    amplitude = a > maxAmplitude ? maxAmplitude : a;
  }

  static inline u1 getAmplitude() {
    return amplitude;
  };

  static inline void setDeadTimes(u1 dt) {
    DT4 = dt;
  };

  static inline u1 getDeadTimes() {
    return DT4;
  };

  static void advanceToBackEMF(Phase const phase, u1 const step);
};

};

#endif	/* THREEPHASEDRIVER_H */

