/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   USBInterface.cpp
 * Author: Cameron
 *
 * Created on April 18, 2018, 12:10 PM
 */

#include "PacketFormats.h"
#include "ServoController.hpp"
#include "ThreePhase/Controller.hpp"

#include <LUFA/Drivers/Board/LEDs.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Platform/Platform.h>

using namespace ThreePhaseControllerNamespace;

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void) {
  //	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void) {
  //	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void) {
  bool ConfigSuccess = true;

  ConfigSuccess &= HID_Device_ConfigureEndpoints(&Generic_HID_Interface);

  USB_Device_EnableSOFEvents();

  //	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void) { HID_Device_ProcessControlRequest(&Generic_HID_Interface); }

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void) { HID_Device_MillisecondElapsed(&Generic_HID_Interface); }

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the
 * generated report ID \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or
 * HID_REPORT_ITEM_Feature \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent)
 *
 *  \return Boolean \c true to force the sending of the report, \c false to let the library determine if it needs to be
 * sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, uint8_t *const ReportID,
                                         const uint8_t ReportType, void *ReportData, uint16_t *const ReportSize) {
  USBDataINShape *const data = (USBDataINShape *)ReportData;

  *ReportSize = REPORT_SIZE;

  data->state = state;
  data->fault = fault;
  data->position = ThreePhasePositionEstimator::getMagnetometerPhaseEstimate();
  data->velocity = ThreePhasePositionEstimator::getMagnetometerVelocityEstimate();
  data->amplitude = ThreePhaseController::getAmplitudeTarget();
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    data->cpuTemp = ADCValues::temperature.getUnsafe();
    data->current = ADCValues::current.getUnsafe() - ADCValues::currentRef.getUnsafe();
    data->VBatt = ADCValues::battery.getUnsafe();
    data->VDD = ADCValues::drive.getUnsafe();
    data->AS = ADCValues::AS.getUnsafe();
    data->BS = ADCValues::BS.getUnsafe();
    data->CS = ADCValues::CS.getUnsafe();
  }
  data->lookupValid = Lookup::isValid;
  data->mlxDataValid = !MLX90363::isTransmitting();

  if (data->mlxDataValid) {
    memcpy(data->mlxResponse, MLX90363::getRxBuffer(), MLX90363::messageLength);
    data->mlxResponseState = MLX90363::getResponseState();
  }

  data->mlxFailedComms = MLX90363::getCRCFailures();
  data->controlLoops = ThreePhaseController::getLoopCount();

  return true;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or
 * HID_REPORT_ITEM_Feature \param[in] ReportData  Pointer to a buffer where the received report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t *const HIDInterfaceInfo, const uint8_t ReportID,
                                          const uint8_t ReportType, const void *ReportData, const uint16_t ReportSize) {
  USBDataOUTShape const *const data = (USBDataOUTShape *)ReportData;

  switch (data->mode) {
  case CommandMode::ClearFault:
    clearFault();
    return;
  case CommandMode::MLXDebug:
    if (state == State::Fault && fault != Fault::Init)
      return;
    setState(State::Manual);
    memcpy(MLX90363::getTxBuffer(), data->mlx.mlxData, MLX90363::messageLength);
    if (data->mlx.crc)
      MLX90363::fillTxBufferCRC();
    MLX90363::startTransmittingUnsafe();
    return;
  case CommandMode::ThreePhase:
    if (state == State::Fault && fault != Fault::Init)
      return;
    setState(State::Manual);
    // TODO: Implement body of this "method"
    return;
  case CommandMode::Calibration:
    if (state == State::Fault && fault != Fault::Init)
      return;
    setState(State::Manual);
    ThreePhaseDriver::setAmplitude(data->calibrate.amplitude);
    ThreePhaseDriver::advanceTo(data->calibrate.angle);
    return;
  case CommandMode::Push:
    if (state == State::Fault && fault != Fault::Init)
      return;
    setState(State::Normal);
    ServoController::setEnable(false);
    ThreePhaseController::setAmplitudeTarget(data->push.command);
    return;
  case CommandMode::Servo:
    if (state == State::Fault && fault != Fault::Init)
      return;
    setState(State::Normal);
    switch (data->servo.mode) {
    case 1:
      ServoController::setAmplitude(data->servo.command);
      return;
    case 2:
      ServoController::setPosition((u2)data->servo.command);
      return;
    case 3:
      ServoController::setVelocity(data->servo.command);
      return;

    case 11:
      ServoController::setPosition_P(data->servo.command);
      return;
    case 12:
      ServoController::setPosition_I(data->servo.command);
      return;
    case 13:
      ServoController::setPosition_D(data->servo.command);
      return;

    // not used just yet
    case 21:
      ServoController::setVelocity_P(data->servo.command);
      return;
    case 22:
      ServoController::setVelocity_I(data->servo.command);
      return;
    case 23:
      ServoController::setVelocity_D(data->servo.command);
      return;
    }

  case CommandMode::Bootloader:
    bootloaderJump();
  }
}
