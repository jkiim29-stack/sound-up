/\*  
 \=====================================================  
 ARDUINO NANO FIVE-LEVEL SOUND METER \+ NAME TRIGGERS  
 \=====================================================

 Microphone: HW-484  
   \+  \-\> 5V  
   G  \-\> GND  
   A0 \-\> Arduino A0  
   D0 \-\> not connected

 OLED: SSD1306 128x64 I2C, address 0x3C  
   GND \-\> GND  
   VCC \-\> supported supply voltage  
   SCL \-\> A5  
   SDA \-\> A4

 Five LEDs (each with its own 220-330 ohm resistor):  
   Green    \-\> D2  (LED 1\)  
   Yellow 1 \-\> D3  (LED 2\)  
   Yellow 2 \-\> D5  (LED 3\)  
   Yellow 3 \-\> D6  (LED 4\)  
   Red      \-\> D4  (LED 5\)

 NAME TRIGGERS (based on estimated dB, not speech recognition):

   JOHN:   76.0 through 79.0 for 3 seconds  
           Turns on LED 1 \+ LED 3 \+ LED 5\.

   ADEN:   84.0 through 89.0 for 3 seconds  
           Turns on LED 2 \+ LED 4\.

   CARTER: 100.0 through 113.0 for 3 seconds  
           Shows CARTER and a smiley on the OLED.  
           LEDs continue to show the normal sound category.

 Each trigger must remain in its range continuously  
 for 3 seconds before activating.

 Leaving a name range for 3 seconds returns to normal.

 The dB values are estimates, not calibrated dB SPL.  
\*/

\#include \<Wire.h\>  
\#include \<Adafruit\_GFX.h\>  
\#include \<Adafruit\_SSD1306.h\>  
\#include \<math.h\>

// \=====================================================  
// 1\. OLED SETTINGS  
// \=====================================================

const int SCREEN\_WIDTH \= 128;  
const int SCREEN\_HEIGHT \= 64;

const int OLED\_RESET \= \-1;  
const int OLED\_ADDRESS \= 0x3C;

Adafruit\_SSD1306 display(  
 SCREEN\_WIDTH,  
 SCREEN\_HEIGHT,  
 &Wire,  
 OLED\_RESET  
);

// \=====================================================  
// 2\. MICROPHONE SETTINGS  
// \=====================================================

const int MIC\_PIN \= A0;

const unsigned long SAMPLE\_WINDOW\_MS \= 50;  
const unsigned long REPORT\_INTERVAL\_MS \= 50;

// \=====================================================  
// 3\. SENSITIVITY AND SMOOTHING  
// \=====================================================

const float DISPLAY\_SENSITIVITY \= 1.0;  
const float RMS\_SMOOTHING \= 0.30;

float smoothedRMS \= 0.0;

bool rmsInitialized \= false;

unsigned long lastReport \= 0;

// \=====================================================  
// 4\. LED PINS  
// \=====================================================

const int GREEN\_LED\_PIN \= 2;    // LED 1  
const int YELLOW1\_LED\_PIN \= 3;  // LED 2  
const int YELLOW2\_LED\_PIN \= 5;  // LED 3  
const int YELLOW3\_LED\_PIN \= 6;  // LED 4  
const int RED\_LED\_PIN \= 4;      // LED 5

// \=====================================================  
// 5\. SOUND CATEGORY THRESHOLDS  
// \=====================================================

const float QUIET\_MAX\_DB \= 80.0;

const float LOW\_AVERAGE\_MAX\_DB \= 90.0;

const float AVERAGE\_MAX\_DB \= 100.0;

const float HIGH\_AVERAGE\_MAX\_DB \= 110.0;

// \=====================================================  
// 6\. OLED BAR RANGE  
// \=====================================================

const float BAR\_MIN\_LEVEL \= 70.0;

const float BAR\_MAX\_LEVEL \= 120.0;

// \=====================================================  
// 7\. dB ESTIMATE SETTINGS  
// \=====================================================

// Example calibration values only.  
// Actual calibration requires a reference meter.

const float REFERENCE\_SPL\_DB \= 94.0;

const float REFERENCE\_RMS\_ADC \= 10.0;

const bool CALIBRATION\_COMPLETE \= false;

const float MIN\_DB \= 70.0;

const float MAX\_DB \= 120.0;

// \=====================================================  
// 8\. NAME TRIGGER SETTINGS  
// \=====================================================

enum SpecialPerson {

 PERSON\_NONE,

 PERSON\_JOHN,

 PERSON\_ADEN,

 PERSON\_CARTER

};

// A reading must stay in its range for 3 seconds.

const unsigned long NAME\_HOLD\_TIME\_MS \= 1500;

SpecialPerson activePerson \= PERSON\_NONE;

SpecialPerson candidatePerson \= PERSON\_NONE;

unsigned long candidateSince \= 0;

// JOHN: 76-79

const float JOHN\_MIN\_DB \= 76.0;

const float JOHN\_MAX\_DB \= 79.0;

// ADEN: 84-89

const float ADEN\_MIN\_DB \= 84.0;

const float ADEN\_MAX\_DB \= 89.0;

// CARTER: 100-113

const float CARTER\_MIN\_DB \= 100.0;

const float CARTER\_MAX\_DB \= 113.0;

// \=====================================================  
// 9\. SOUND CATEGORY ENUM  
// \=====================================================

enum SoundCategory {

 SOUND\_QUIET,

 SOUND\_LOW\_AVERAGE,

 SOUND\_AVERAGE,

 SOUND\_HIGH\_AVERAGE,

 SOUND\_LOUD

};

// \=====================================================  
// 10\. READ MICROPHONE RMS  
// \=====================================================

float readMicrophoneRMS() {

 unsigned long start \= millis();

 float sum \= 0.0;

 float sumSquares \= 0.0;

 unsigned int count \= 0;

 while (millis() \- start \< SAMPLE\_WINDOW\_MS) {

   float sample \= analogRead(MIC\_PIN);

   sum \+= sample;

   sumSquares \+= sample \* sample;

   count++;  
 }

 if (count \== 0) {

   return 0.0;  
 }

 float mean \= sum / count;

 float meanSquare \= sumSquares / count;

 // Remove the microphone's DC bias.

 float variance \= meanSquare \- (mean \* mean);

 if (variance \< 0.0) {

   variance \= 0.0;  
 }

 return sqrt(variance);  
}

// \=====================================================  
// 11\. CALCULATE ESTIMATED dB  
// \=====================================================

float calculateDbEstimate(float rms) {

 if (rms \< 0.1 \|\| REFERENCE\_RMS\_ADC \<= 0.0) {

   return MIN\_DB;  
 }

 float db \= REFERENCE\_SPL\_DB \+

            20.0 \* log10(  
              rms / REFERENCE\_RMS\_ADC  
            );

 if (db \< MIN\_DB) {

   db \= MIN\_DB;  
 }

 if (db \> MAX\_DB) {

   db \= MAX\_DB;  
 }

 return db;  
}

// \=====================================================  
// 12\. IDENTIFY NORMAL SOUND CATEGORY  
// \=====================================================

SoundCategory getSoundCategory(float db) {

 if (db \< QUIET\_MAX\_DB) {

   return SOUND\_QUIET;  
 }

 else if (db \< LOW\_AVERAGE\_MAX\_DB) {

   return SOUND\_LOW\_AVERAGE;  
 }

 else if (db \< AVERAGE\_MAX\_DB) {

   return SOUND\_AVERAGE;  
 }

 else if (db \< HIGH\_AVERAGE\_MAX\_DB) {

   return SOUND\_HIGH\_AVERAGE;  
 }

 else {

   return SOUND\_LOUD;  
 }  
}

// \=====================================================  
// 13\. IDENTIFY NAME RANGE  
// \=====================================================

SpecialPerson getPersonForReading(float db) {

 if (  
   db \>= JOHN\_MIN\_DB &&  
   db \<= JOHN\_MAX\_DB  
 ) {

   return PERSON\_JOHN;  
 }

 if (  
   db \>= ADEN\_MIN\_DB &&  
   db \<= ADEN\_MAX\_DB  
 ) {

   return PERSON\_ADEN;  
 }

 if (  
   db \>= CARTER\_MIN\_DB &&  
   db \<= CARTER\_MAX\_DB  
 ) {

   return PERSON\_CARTER;  
 }

 return PERSON\_NONE;  
}

// \=====================================================  
// 14\. REQUIRE A STABLE RANGE FOR 3 SECONDS  
// \=====================================================

SpecialPerson updatePersonState(  
 float db,  
 unsigned long now  
) {

 SpecialPerson observedPerson \=  
   getPersonForReading(db);

 // If the reading enters a different range,  
 // begin a new 3-second timer.

 if (observedPerson \!= candidatePerson) {

   candidatePerson \= observedPerson;

   candidateSince \= now;  
 }

 // Activate the new person only after the  
 // reading stays in that range for 3 seconds.

 if (  
   observedPerson \!= activePerson &&  
   now \- candidateSince \>= NAME\_HOLD\_TIME\_MS  
 ) {

   activePerson \= observedPerson;  
 }

 return activePerson;  
}

// \=====================================================  
// 15\. CONTROL THE FIVE LEDS  
// \=====================================================

void allLEDsOff() {

 digitalWrite(GREEN\_LED\_PIN, LOW);

 digitalWrite(YELLOW1\_LED\_PIN, LOW);

 digitalWrite(YELLOW2\_LED\_PIN, LOW);

 digitalWrite(YELLOW3\_LED\_PIN, LOW);

 digitalWrite(RED\_LED\_PIN, LOW);  
}

void updateLEDs(  
 SoundCategory category,  
 SpecialPerson person  
) {

 allLEDsOff();

 // JOHN:  
 // First \+ middle \+ last LEDs.  
 // LED 1, LED 3, LED 5\.

 if (person \== PERSON\_JOHN) {

   digitalWrite(GREEN\_LED\_PIN, HIGH);

   digitalWrite(YELLOW2\_LED\_PIN, HIGH);

   digitalWrite(RED\_LED\_PIN, HIGH);

   return;  
 }

 // ADEN:  
 // Second \+ fourth LEDs.  
 // LED 2 and LED 4\.

 if (person \== PERSON\_ADEN) {

   digitalWrite(YELLOW1\_LED\_PIN, HIGH);

   digitalWrite(YELLOW3\_LED\_PIN, HIGH);

   return;  
 }

 // CARTER and normal operation:  
 // Use the normal category LED.

 if (category \== SOUND\_QUIET) {

   digitalWrite(GREEN\_LED\_PIN, HIGH);  
 }

 else if (category \== SOUND\_LOW\_AVERAGE) {

   digitalWrite(YELLOW1\_LED\_PIN, HIGH);  
 }

 else if (category \== SOUND\_AVERAGE) {

   digitalWrite(YELLOW2\_LED\_PIN, HIGH);  
 }

 else if (category \== SOUND\_HIGH\_AVERAGE) {

   digitalWrite(YELLOW3\_LED\_PIN, HIGH);  
 }

 else {

   digitalWrite(RED\_LED\_PIN, HIGH);  
 }  
}

// \=====================================================  
// 16\. DRAW CARTER'S SMILEY FACE  
// \=====================================================

void drawSmileyFace(int centerX, int centerY) {

 // Outer circle.

 display.drawCircle(  
   centerX,  
   centerY,  
   7,  
   SSD1306\_WHITE  
 );

 // Eyes.

 display.drawPixel(  
   centerX \- 3,  
   centerY \- 2,  
   SSD1306\_WHITE  
 );

 display.drawPixel(  
   centerX \+ 3,  
   centerY \- 2,  
   SSD1306\_WHITE  
 );

 // Smiling mouth.

 display.drawLine(  
   centerX \- 3,  
   centerY \+ 2,  
   centerX \- 1,  
   centerY \+ 4,  
   SSD1306\_WHITE  
 );

 display.drawLine(  
   centerX \- 1,  
   centerY \+ 4,  
   centerX \+ 2,  
   centerY \+ 4,  
   SSD1306\_WHITE  
 );

 display.drawLine(  
   centerX \+ 2,  
   centerY \+ 4,  
   centerX \+ 4,  
   centerY \+ 2,  
   SSD1306\_WHITE  
 );  
}

// \=====================================================  
// 17\. UPDATE OLED DISPLAY  
// \=====================================================

void updateDisplay(  
 float db,  
 SoundCategory category,  
 SpecialPerson person  
) {

 display.clearDisplay();

 display.setTextColor(SSD1306\_WHITE);

 // \-----------------------------------------------  
 // NAME OR NORMAL SOUND CATEGORY  
 // \-----------------------------------------------

 display.setTextSize(1);

 display.setCursor(0, 0);

 if (person \== PERSON\_JOHN) {

   display.print("JOHN");  
 }

 else if (person \== PERSON\_ADEN) {

   display.print("ADEN");  
 }

 else if (person \== PERSON\_CARTER) {

   display.print("CARTER");  
 }

 else if (category \== SOUND\_QUIET) {

   display.print("QUIET");  
 }

 else if (category \== SOUND\_LOW\_AVERAGE) {

   display.print("LOW AVG");  
 }

 else if (category \== SOUND\_AVERAGE) {

   display.print("AVERAGE");  
 }

 else if (category \== SOUND\_HIGH\_AVERAGE) {

   display.print("HIGH AVG");  
 }

 else {

   display.print("LOUD");  
 }

 // \-----------------------------------------------  
 // LED INDICATOR  
 // \-----------------------------------------------

 display.setCursor(64, 0);

 display.print("LED:");

 display.setCursor(88, 0);

 if (person \== PERSON\_JOHN) {

   display.print("G+Y2+R");  
 }

 else if (person \== PERSON\_ADEN) {

   display.print("Y1+Y3");  
 }

 else if (category \== SOUND\_QUIET) {

   display.print("GREEN");  
 }

 else if (category \== SOUND\_LOW\_AVERAGE) {

   display.print("YEL 1");  
 }

 else if (category \== SOUND\_AVERAGE) {

   display.print("YEL 2");  
 }

 else if (category \== SOUND\_HIGH\_AVERAGE) {

   display.print("YEL 3");  
 }

 else {

   display.print("RED");  
 }

 // \-----------------------------------------------  
 // LARGE ESTIMATED dB READING  
 // \-----------------------------------------------

 if (person \== PERSON\_CARTER) {

   // Smaller digits leave room for the smiley.

   display.setTextSize(2);

   display.setCursor(0, 16);

   display.print(db, 1);

   // Smiley between the number and dB.

   drawSmileyFace(80, 24);

   display.setTextSize(1);

   display.setCursor(100, 25);

   display.print("dB");  
 }

 else {

   display.setTextSize(3);

   display.setCursor(0, 12);

   display.print(db, 1);

   display.setTextSize(1);

   display.setCursor(98, 22);

   display.print("dB");  
 }

 // \-----------------------------------------------  
 // CALIBRATION STATUS  
 // \-----------------------------------------------

 display.setTextSize(1);

 display.setCursor(0, 36);

 if (CALIBRATION\_COMPLETE) {

   display.print("CALIBRATED ESTIMATE");  
 }

 else {

   display.print("EST. dB (UNCAL)");  
 }

 // \-----------------------------------------------  
 // SOLID SOUND BAR: 70 TO 120  
 // \-----------------------------------------------

 float range \=  
   BAR\_MAX\_LEVEL \- BAR\_MIN\_LEVEL;

 float amount \= 0.0;

 if (range \> 0.0) {

   amount \=  
     (db \- BAR\_MIN\_LEVEL) / range;  
 }

 amount \= constrain(amount, 0.0f, 1.0f);

 int barWidth \= (int)(  
   amount \* 116.0 \+ 0.5  
 );

 // Make readings above 70 visible.

 if (db \> BAR\_MIN\_LEVEL && barWidth \< 2) {

   barWidth \= 2;  
 }

 // Draw bar outline.

 display.drawRect(  
   0, 45, 120, 10,  
   SSD1306\_WHITE  
 );

 // Draw filled bar.

 if (barWidth \> 0) {

   display.fillRect(  
     2,  
     47,  
     barWidth,  
     6,  
     SSD1306\_WHITE  
   );  
 }

 // No numbers are printed below the bar.

 display.display();  
}

// \=====================================================  
// 18\. SETUP  
// \=====================================================

void setup() {

 Serial.begin(115200);

 // Initialize all five LEDs.

 pinMode(GREEN\_LED\_PIN, OUTPUT);

 pinMode(YELLOW1\_LED\_PIN, OUTPUT);

 pinMode(YELLOW2\_LED\_PIN, OUTPUT);

 pinMode(YELLOW3\_LED\_PIN, OUTPUT);

 pinMode(RED\_LED\_PIN, OUTPUT);

 allLEDsOff();

 // Initialize OLED.

 if (\!display.begin(  
       SSD1306\_SWITCHCAPVCC,  
       OLED\_ADDRESS  
     )) {

   Serial.println("OLED initialization failed");

   while (true) {

     // Stop if the OLED could not initialize.  
   }  
 }

 // Startup screen.

 display.clearDisplay();

 display.setTextColor(SSD1306\_WHITE);

 display.setTextSize(1);

 display.setCursor(0, 0);

 display.println("5-Level Sound Meter");

 display.println("Name triggers ready");

 display.display();

 Serial.println("Sound bench ready");  
}

// \=====================================================  
// 19\. MAIN LOOP  
// \=====================================================

void loop() {

 unsigned long now \= millis();

 if (now \- lastReport \>= REPORT\_INTERVAL\_MS) {

   lastReport \= now;

   // READ MICROPHONE

   float measuredRMS \=  
     readMicrophoneRMS();

   // \-----------------------------------------------  
   // SMOOTH MICROPHONE READING  
   // \-----------------------------------------------

   if (\!rmsInitialized) {

     smoothedRMS \= measuredRMS;

     rmsInitialized \= true;  
   }

   else {

     smoothedRMS \=  
       smoothedRMS \+  
       RMS\_SMOOTHING \*  
       (measuredRMS \- smoothedRMS);  
   }

   // \-----------------------------------------------  
   // APPLY SOFTWARE SENSITIVITY  
   // \-----------------------------------------------

   float adjustedRMS \=  
     smoothedRMS \* DISPLAY\_SENSITIVITY;

   // \-----------------------------------------------  
   // CALCULATE ESTIMATED dB  
   // \-----------------------------------------------

   float db \=  
     calculateDbEstimate(adjustedRMS);

   // \-----------------------------------------------  
   // IDENTIFY NORMAL SOUND CATEGORY  
   // \-----------------------------------------------

   SoundCategory category \=  
     getSoundCategory(db);

   // \-----------------------------------------------  
   // CHECK THE THREE NAME RANGES  
   // \-----------------------------------------------

   SpecialPerson person \=  
     updatePersonState(db, now);

   // \-----------------------------------------------  
   // UPDATE LEDS  
   // \-----------------------------------------------

   updateLEDs(category, person);

   // \-----------------------------------------------  
   // UPDATE OLED  
   // \-----------------------------------------------

   updateDisplay(  
     db,  
     category,  
     person  
   );

   // \-----------------------------------------------  
   // SEND DATA TO WEBSITE  
   // \-----------------------------------------------

   // Keeps the existing browser-compatible format.

   Serial.print("sound ");

   Serial.print(db, 1);

   Serial.print(" rms ");

   Serial.println(smoothedRMS, 2);

 }  
}  
