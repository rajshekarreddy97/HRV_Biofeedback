#define USE_ARDUINO_INTERRUPTS false
#include <PulseSensorPlayground.h>

const int OUTPUT_TYPE = SERIAL_PLOTTER;

const int PULSE_INPUT = A0; // Analog input
const int PIN_SPEAKER = D6;
const int THRESHOLD = 550;   // Adjust this number to avoid noise when idle

int ibi[5];    // Array that stores IBI values
int current_ibi;
int count = 0;
int hrv = 0;

/*
   samplesUntilReport = the number of samples remaining to read
   until we want to report a sample over the serial connection.

   We want to report a sample value over the serial port
   only once every 20 milliseconds (10 samples) to avoid
   doing Serial output faster than the Arduino can send.
*/

byte samplesUntilReport;
const byte SAMPLES_PER_SERIAL_SAMPLE = 10;


PulseSensorPlayground pulseSensor;


void setup() 
{
  
  /*
     If we used a slower baud rate, we'd likely write bytes faster than
     they can be transmitted, which would mess up the timing
     of readSensor() calls, which would make the pulse measurement
     not work properly.
  */
  Serial.begin(115200);

  // Configure the PulseSensor manager.
  pulseSensor.analogInput(PULSE_INPUT);
  pulseSensor.setSerial(Serial);
  pulseSensor.setOutputType(OUTPUT_TYPE);
  pulseSensor.setThreshold(THRESHOLD);

  // Skip the first SAMPLES_PER_SERIAL_SAMPLE in the loop().
  samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;

  // Now that everything is ready, start reading the PulseSensor signal.
  if (!pulseSensor.begin()) 
  {
    /*
       PulseSensor initialization failed,
       likely because our Arduino platform interrupts
       aren't supported yet.

       If your Sketch hangs here, try changing USE_PS_INTERRUPT to false.
    */
    for(;;) 
    {
      Serial.print("Not working...");
    }
  }
}


void loop() 
{

  /*
     See if a sample is ready from the PulseSensor.

     If USE_INTERRUPTS is true, the PulseSensor Playground
     will automatically read and process samples from
     the PulseSensor.

     If USE_INTERRUPTS is false, this call to sawNewSample()
     will, if enough time has passed, read and process a
     sample (analog voltage) from the PulseSensor.
  */
  
  if (pulseSensor.sawNewSample()) 
  {
    /*
       Every so often, send the latest Sample.
       We don't print every sample, because our baud rate
       won't support that much I/O.
    */
    
    if (--samplesUntilReport == (byte) 0) 
    {
      samplesUntilReport = SAMPLES_PER_SERIAL_SAMPLE;

      /*
         If a beat has happened since we last checked,
         write the per-beat information to Serial.
         write a frequency to the PIN_SPEAKER
         NOTE: Do not set the optional duration of tone! That is blocking!
     */
     
      if (pulseSensor.sawStartOfBeat()) 
      { 
        // Calculate RMSSD from the IBI

        current_ibi = pulseSensor.getInterBeatIntervalMs();
        Serial.print("IBI: ");
        Serial.print(current_ibi);
        Serial.print("\n");

        if (count < 5)
        {
          ibi[count] = current_ibi; // Add the value to the array
          count++;
        }
        else
        {
          count = 0;
          int sum = 0;
          for (int i = 1; i < 5; i++)
          {
            sum += pow(abs(ibi[i-1] - ibi[i]), 2);
          }

          int average = sum / 5;

          hrv = sqrt (average);
          
          Serial.print("HRV: ");
          Serial.print(hrv);
          Serial.print("\n");
         }
       }

       if (hrv < 100) // Give feedback to the user to breathe slower and deeper
       {
        delay(500);
        tone(PIN_SPEAKER,1047); // tone(pin,frequency)
        delay(250);
        noTone(PIN_SPEAKER);
       }
       else
       {
         noTone(PIN_SPEAKER);
       }
                     
      }

      /*
        The Pulse variable is true only for a short time after the heartbeat is detected
        Use this to time the duration of the beep
      */
    }

    /*******
      Here is a good place to add code that could take up
      to a millisecond or so to run.
    *******/
}

  /******
     Don't add code here, because it could slow the sampling
     from the PulseSensor.
  ******/
