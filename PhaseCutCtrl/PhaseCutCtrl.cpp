#include "PhaseCutCtrl.h"
#include "Arduino.h"

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



#define SAMPLES_AMOUNT 20

void PhaseCutCtrl::initialize(byte signal_pin, byte output_pin, int max_power)
{
    this->output_pin = output_pin;
    this->signal_pin = signal_pin;
    hz_factor = 50000000 * SAMPLES_AMOUNT;
    netFreqCnt = 0;
    this->max_power = max_power;

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

    //pinMode(13, OUTPUT); // can use pin 13 for timing mesurement with an osziloscope

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
    
    //PORTB &= ~B00100000; //set pin13 to LOW for timemeasurement
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

    // formula for calculating the time shift. it is a function that 
    // is relative easy to calculate and cares for a near linear relation 
    // between pccPower and resulting load power
    
     p = pccPower / (float)max_power;
    
    // set the Output Compare Register 1 A
    OCR1A = int(-564 * p * p * p + 846 * p * p - 423 * p + 173) * 100;
    //PORTB |=  B00100000; //set pin13 back to HIGH for timemeasurement

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

