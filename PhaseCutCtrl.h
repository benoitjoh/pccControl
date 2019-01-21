// **************************************************************************
// Library to make usage of a phasecut controller easier
//
// It uses timer1 with prescaler 8 and needs the OCR1A interrupt

// It needs one input (pin 2 or 3) which is attached to an external Interrupt
// to find a point near the zero passage of the 220V AC curve
// The interrupt routine sets timer1 to 0
// The output pin is used to fire the triac with a distinct delay after the
// zero point
// The delay is calculated from the value passed to set_pcc(speed)
//
// The param speed can be anything between 0 (off) and 2048 (100%)
//
// Usage:
//
//   #include <PhaseCutCtrl.h>
//   void setup()
//   {
//   PCCtrl.initialize(PIN_IN_ACZERO_SIGNAL, PIN_PCC_OUT_A);
//   }
//                       |                     |
//                       |                     |
//                       |                     |
//                       |                     \--- output pin
//                       \--- input pin
//
// To set the speed:
//   PCCtrl.set_pcc(speed);
// Note: call this method only if speed really changed, because calculation needs 200mys
//       use something like "if (adjustPcmPower()) {PCCtrl.set_pcc(speed);} ..."
//
//
//
// Measurement of net frequency (via the isrZeroCallback() method)
// every x times the micros() counter is transfered to netFreqMicros and old value shifted to
// netFreqMicrosOld.
// calling getNetFreq() calculates the time for one event and returns transfored to Hertz.
// If the interupt is never called (e.g. if there is no AC supply, 0.0 is returned)
//
//                                                       Johannes Benoit 2017
// **************************************************************************

#ifndef PHASECUTCTRL_H
#define PHASECUTCTRL_H

#include "Arduino.h"


class PhaseCutCtrl
{
    public:
        // methods
        void initialize(byte signal_pin, byte output_pin);
        void set_pcc(int power);
        unsigned int getNetFrequency();
        bool netIsAllive();

        void isrZeroCallback();
        void isrOciCallback();

     private:
       // parameters
        bool pcc_is_on;
        byte signal_pin;
        byte output_pin;
        int pcc_power_last;

        // variables for frequencymeasurement
        unsigned long netFreqMicros;
        unsigned long netFreqMicrosOld;
        unsigned long lastAcZeroMillis;

        long hz_factor;
        int netFreqCnt;
        int netFreqCntLast;

    protected:


};
extern PhaseCutCtrl PCCtrl;
#endif // PHASECUTCTRL_H
