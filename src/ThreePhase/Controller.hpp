/*
 * File:   ThreePhaseController.h
 * Author: Cameron
 *
 * Created on October 22, 2015, 2:21 AM
 *
 * Takes in an amplitude to be commanded
 * handles input from predictor
 * outputs to the driver (hardware)
 */

#ifndef THREEPHASECONTROLLER_H
#define THREEPHASECONTROLLER_H

#include <AVR++/basicTypes.hpp>

#include "Driver.hpp"
#include "LookupTable.hpp"
#include "PositionEstimator.hpp"
#include <AVR++/Atomic.hpp>
#include <avr/interrupt.h>

ISR(TIMER4_OVF_vect);

namespace ThreePhaseControllerNamespace {

inline constexpr u4 ulimit(u4 in, u4 limit) { return in > limit ? limit : in; }

inline constexpr u2 ulimit(u2 in, u2 limit) { return in > limit ? limit : in; }

using namespace Basic;

/**
 * This static class wraps around the ThreePhaseDriver and some position estimation
 * to continuously tell the Driver what phase to drive at for some constant amplitude drive.
 *
 * This presents the minimal interface that all "motors" should have:
 *  - Amplitude control of PWM
 *  - Position (and velocity) feedback estimations
 *
 * "Servos" for position, velocity, current, power, whatever are outside the scope
 * of this class. See ServoController for more.
 */
class ThreePhaseController {
  /**
   * Called periodically (nominally 31.25kHz) by TIMER 4 ISR
   */
  static inline void controlLoop() __attribute__((hot));
  friend void ::TIMER4_OVF_vect();

  volatile static bool enabled;

  /**
   * Are we trying to go forward
   */
  static volatile bool isForwardTorque;

  /**
   * 90 degree phase shift
   */
  static constexpr u1 output90DegPhaseShift = ThreePhaseDriver::StepsPerCycle / 4;

  volatile static AVR::Atomic<u2> loopCount;

public:
  inline static u2 getLoopCount() {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      const auto count = loopCount.getUnsafe();
      loopCount.setUnsafe(0);
      return count;
    }
  }
  /**
   * Initialize the controller
   */
  static void init();

  /**
   * Start the controller interrupt
   */
  inline static void start() {
    // controlLoop() is called by TIMER4_OVF_vect
    TIMSK4 = 1 << TOIE4;
  }

  /**
   * Stop the controller interrupt
   */
  inline static void stop() { TIMSK4 = 0; }

  /**
   * Enable the controller
   */
  inline static void enable() {
    // Do not enable if not calibrated
    if (!Lookup::isValid)
      return;

    enabled = true;
  }

  /**
   * Disable the controller
   */
  inline static void disable() { enabled = false; }

  /**
   * Abstraction around possible amplitude values.
   *
   * Amplitude is an 8-bit number + a direction.
   *
   * We also store zero for speed. (Maybe this should not be done)
   */
  class Amplitude {
    bool forward;
    u1 amplitude;
    friend class ThreePhaseController;

  public:
    inline Amplitude(s2 const t) : forward(t >= 0), amplitude(ulimit(u2(forward ? t : -t), 255)){};
    inline Amplitude(s4 const t) : forward(t >= 0), amplitude(ulimit(u4(forward ? t : -t), 255)){};

    inline Amplitude(const bool fwd, u1 const ampl) : forward(fwd), amplitude(ampl){};

    inline Amplitude(volatile const Amplitude &that) : forward(that.forward), amplitude(that.amplitude) {}

    inline s4 operator+(s2 const d) volatile {
      s4 ampl = forward ? amplitude : -amplitude;
      ampl += d;
      return ampl;
    }
  };

private:
  /**
   * Target "push" amount
   */
  static volatile Amplitude targetAmplitude;

public:
  /**
   * Set the desired drive amplitude
   */
  inline static void setAmplitudeTarget(const Amplitude &t) {
    ATOMIC_BLOCK(ATOMIC_FORCEON) {
      targetAmplitude.amplitude = t.amplitude;
      targetAmplitude.forward = t.forward;
    }
  }

  /**
   * Get the current amplitude
   *
   * @return s2 (range [-255, 255])
   */
  static inline Amplitude getAmplitudeTarget() { return targetAmplitude; };

  static void handleNewVelocityEstimate(s2 const v);

private:
  static volatile u1 dampingVelocityGain;

public:
  inline static void setAntiDampingVelocityGain(u1 const g) { dampingVelocityGain = g; }
};

}; // namespace ThreePhaseControllerNamespace

#endif /* THREEPHASECONTROLLER_H */
