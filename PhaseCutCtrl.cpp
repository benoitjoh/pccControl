#include "PhaseCutCtrl.h"
#include "Arduino.h"

//#define DEBUG_PCC 1

#define SAMPLES_AMOUNT 20
#define MAX_POWER 2048


PhaseCutCtrl PCCtrl; // pre-instatiate the class here

void ISR_acIsZero()
{
    // method for attached external interrupt for the zero phase signal
    PCCtrl.isrZeroCallback();
}


ISR(TIMER1_COMPA_vect)
{
	// this is the prepared interrupt method for the OCR1
    PCCtrl.isrOciCallback();
}

// Mapping function (costs about 8 mySeconds)

// <START OF GENERATED CODE> by map_generator.py --- 2019-01-16 14:14:28 -----
// Interpolation for 8 steps between x=0 and x=2048 with 256 x-values between each step
// Delta values are shiftet by 10  bits left for better accuracacy.
// ---------------------------------------------------------------------------

// arraydefinition for point_array ...
int point_array[8] = {17300, 14602, 12314, 10786, 10250, 9713, 8185, 5897};

// arraydefinition for delta_array ...
int delta_array[8] = {10791, 9148, 6109, 2144, 2146, 6111, 9148, 10787};
// ---------------------------------------------------------------------------


int xPartMask = (1 << 8) -1;


int mapFunction(int x)
{
    byte segment = x >> 8;
    int x_part = x & xPartMask;
    int delta = ((long)delta_array[segment] * x_part)  >> 10;
    int y = point_array[segment] - delta;
    return y;
}



// <END OF GENERATED CODE> ----------------------------------------------//


void PhaseCutCtrl::initialize(byte signal_pin, byte output_pin)
{
    this->output_pin = output_pin;
    this->signal_pin = signal_pin;
    hz_factor = 50000000 * SAMPLES_AMOUNT;
    netFreqCnt = 0;

    noInterrupts();
    pinMode(signal_pin, INPUT);
    attachInterrupt(digitalPinToInterrupt(signal_pin), ISR_acIsZero, RISING);

    pinMode(output_pin, OUTPUT);

    pcc_is_on = false;

    // set up timer interrupt for Timer1
    TCCR1A = 0;
    TCCR1B = 0;

    bitSet(TCCR1B, CS11);    // specify 8 als Prescaler for timer 1
    bitSet(TIMSK1, OCIE1A);  // activate Timer output compare Interrupt A at timer 1

    interrupts();             // activate all interrupts
    OCR1A = 0;
#ifdef DEBUG_PCC
    pinMode(13, OUTPUT); // can use pin 13 for timing mesurement with an osziloscope
#endif // DEBUG_SPEEDCONTROL

}

void PhaseCutCtrl::isrZeroCallback()
{
    // at zero phase _always_ reset timercounter. Other function waits till
    // OCR1A is reached.
    TCNT1 = 0;
    lastAcZeroMillis = millis();
    //optional code if measurement of net frequency is activated (costs 8 or 12 mySecs)

	if (++netFreqCnt == SAMPLES_AMOUNT)
	{
	//Serial.println(netFreqCnt);
		netFreqCntLast = netFreqCnt;
		netFreqCnt = 0;
		netFreqMicrosOld = netFreqMicros;
		netFreqMicros = micros();
	}

}

void PhaseCutCtrl::isrOciCallback()
{
    // fires the pcm output pin if the OCR1A delay after phase zero is reached
    if (pcc_is_on)
    {
        digitalWrite(output_pin, 1); // pin on
        digitalWrite(output_pin, 0); // pin off
    }

}

void PhaseCutCtrl::set_pcc(int pccPower)
{
    // this calculates  and sets the delta for OCR1A register from timer interrupt

    if (pccPower == 0)
    {
        // switched off
        pcc_is_on = false;
        digitalWrite(output_pin, LOW); // pin permanently LOW
    }
    else
    {
        if (pccPower == MAX_POWER)
        {
            // swiched on to 100% so no interrupt control but straight on
            pcc_is_on = false;
            digitalWrite(output_pin, HIGH); // pin permanently HIGH
        }
        else
        {
            // Let the speed be controlled by the interrupt methods
            pcc_is_on = true;

        }
    }

#ifdef DEBUG_PCC
    PORTB &= ~B00100000; //set pin13 to LOW for timemeasurement
#endif // DEBUG_PCC

    // set the Output Compare Register 1 A
    int oc_value = mapFunction(pccPower);
    OCR1A = oc_value;

#ifdef DEBUG_PCC
    PORTB |=  B00100000; //set pin13 back to HIGH for timemeasurement
    Serial.print("PCC:\t" + String(pccPower) + "\t" + String(oc_value) + "\n");
#endif // DEBUG_PCC

}

bool PhaseCutCtrl::netIsAllive()
// return false if the last acZero event passed longer than 300ms
{
    return (millis() - lastAcZeroMillis < 300);

}

unsigned int PhaseCutCtrl::getNetFrequency()
// measures micros of 100 zero interrupt and calculates then
// the average netFrequency in centiHz (100 --> 1Hz )
// prescaler of timer1 is 8, cpu works on 16MHz
{
    unsigned int result;

    if (millis() - lastAcZeroMillis < 100)
    // if the last call of acZero event was more than 100ms past, return a 0.0!
    {
        unsigned long diff = netFreqMicros - netFreqMicrosOld; // 8mySec

        if (diff == 0)
        {
            result = 0;
        }
        else
        {
            result = hz_factor / diff;

        }

    }
    else
    {
        result =  0;
    }
    return result;

}

