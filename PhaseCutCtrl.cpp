#include "PhaseCutCtrl.h"
#include "Arduino.h"

PhaseCutCtrl PCCtrl; //preinstatiate

void ISR_acIsZero()
{
    // attached interrupt for the zero phase signal
    PCCtrl.isrZeroCallback();
}


ISR(TIMER1_COMPA_vect)
{
    PCCtrl.isrOciCallback();
}



#define SAMPLES_AMOUNT 20

void PhaseCutCtrl::initialize(byte s_pin, byte o_pin, int max_pwr)
{
    output_pin = o_pin;
    signal_pin = s_pin;
    hz_factor = 500000 * SAMPLES_AMOUNT;
    netFreqCnt = 0;
    max_power = max_pwr;

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
    // this calculates the delta for OCR1A register from timer interrupt
    // the routine is called from the speedController

    // call this method only if pccPower had changed! Because some float
    // operations are included...
    //    use something like "if (adjustPcmPower()) {PCCtrl.set_pcc(pcmPower);} ..."

    float p = 0.0;

    if (pccPower == 0)
    {
        // switched off
        pcc_is_on = false;
        digitalWrite(output_pin, 0); // pin permanently LOW
    }
    else
    {
        if (pccPower == max_power)
        {
            // swiched on to 100% so no interrupt control but straight on
            pcc_is_on = false;
            digitalWrite(output_pin, 1); // pin permanently HIGH
        }
        else
        {
            // Let the speed be controlled by the interrupt methods
            pcc_is_on = true;

        }
    }

    // formula for calculating the time shift
    // set the Output Compare Register 1 A
    p = pccPower / (float)max_power;
    OCR1A = int(-564 * p * p * p + 846 * p * p - 423 * p + 173) * 100;

}

bool PhaseCutCtrl::netIsAllive()
// return false if the last acZero event passed longer than 300ms
{
    return (millis() - lastAcZeroMillis < 300);

}

float PhaseCutCtrl::getNetFrequency()
// measures micros of 100 zero interrupt and calculates then
// the average netFrequency in float netFreq
// (the float calculation needs about 30 µSec)
// prescaler of timer1 is 8, cpu works on 16MHz
{
    float result;

    if (millis() -lastAcZeroMillis < 100)
    // if the last call of acZero event was more than 100ms past, return a 0.0!
    {
        unsigned long diff = netFreqMicros - netFreqMicrosOld; // 8mySec
        Serial.println(String(diff) + "\t" + String(netFreqCntLast));

        if (diff == 0)
        {
            result = 0.0;
        }
        else
        {
            result = hz_factor / diff; // 35mySec

        }

    }
    else
    {
        result =  0.0;
    }
    return result;

}

