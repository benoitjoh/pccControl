# PhaseCutControl #

Library to make usage of a phase cut controller with a ATMEGA easier.
attached you find a circuit with a layout example for a small board for the unit.


The modules uses
* timer1 with prescaler 8 and needs the OCR1A interrupt.
* pin 2 or pin 3 as external interrupt input source
* one digital out to fire the triac



## /!\ WARNING

```
/!\ Dealing with high voltage is dangerous. Be careful and know what you can do and what you better leave. /!\
```

## Installation ##

Just copy the folder PhaseCutCtrl with c++ sources in library folder from ArduinoIDE


## Principle ##

For basic overview see: [>> phase cut control on wikipedia](https://en.wikipedia.org/wiki/Phase-fired_controllers)


![graphic](https://upload.wikimedia.org/wikipedia/commons/0/07/Regulated_rectifier.gif)

In the picture the idea is shown. The trick is to detect the start point of the curve. therefor the zerodetection unit in the circuit works out a clean enough signal with the opcocoupler 4N35 and a PNP transistor.
The signal triggers an external interrupt which sets the timer1 to zero.
Now, depending from the power value, a delay is calculated and set to outputCompareRegister1a (OCR1A). The interrupt triggers a short pulse on the output pin (blue spice in the picture) and fires the triac (red curve) via the MOC3022 optocoupler. The triac is on until the AC voltage returns to zero point, and shuts of in that moment. So the load gets only the energy of grey part under the graph.
(one mistake in the picture is, that the second wave should be negative, because its alternating current of course..)



## Usage ##

### Initialization
The class is per initialized in the cpp file during compilation. This is neccessary because the interrupt method needs to know which method of the class it must call.
So the initializt() method has to be called in setup() to pass parameters.

The last Parameter PCC_POWER_MAX ist typically 999 so you have 1000 steps, but can be any value you like. 


```c++
#include <PhaseCutCtrl.h>
#define PIN_IN_ACZERO_SIGNAL   3  // pin for AC zeropoint detection (Interrupt source, so can only be 2 or 3)
#define PIN_PCC_OUT_A          4  // pin for pulse cut modulation output
#define PCC_POWER_MAX 999  // this int represents 100% power (0 means 0%)

// NOTE: the class is preinstanciated as PCCtrl.

void setup()
{
    // Phase cut Ctrl module
    PCCtrl.initialize(PIN_IN_ACZERO_SIGNAL, PIN_PCC_OUT_A, PCC_POWER_MAX);
}


```

### set the power ###

To set the power value, call set_pcc(power). run it as seldom as possible to reduce timeconsumption for the float calculations.
In the example, a function returns true, when it changed the targetpower:
```c++
//  use a wrapper like this to swap the pin that conects to the relais

void digitalWriteAtSolidState(byte mypin, byte myvalue)
{
    PCCtrl.waitUntilAcZero();
    digitalWrite(mypin, myvalue);
}


```

## Use the waitUntilAcZero() method

This Method waits in a loop until the AC curve hits the x axis at zero Volt.
In exactly that moment you can switch power relais and you have a solid state relais :-)

```c++
//  use something like

if (someFunctionThatDefinespower())
{
    PCCtrl.set_pcc(power);
}

```


## Net Frequency Measurement

calling

```c++
float PCCtlr.getNetFreq()
```

calculates the time for one event and returns transfored to Hertz.
If the interupt is never called (e.g. if there is no AC supply, 0.0 is returned)
(It needs about 1k program memory extra.)

(c) [Johannes Benoit 2018](mailto:jbenoit@t-online.de)


