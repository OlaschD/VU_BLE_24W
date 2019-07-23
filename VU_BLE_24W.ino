/*
 Beschreibung : VU-Meter mit einem 1m LED-Strip 60 nach dem Video von
                https://www.y_outube.com/watch?v=bZQZlyhVY0k
 Versionen 2  : Taster an D2 High-aktiv
              : D5 - Digital Ausgang für 60 LED-Strip rechts
              : D6 - Digital Ausgang für 60 LED-Strip links
              : A5 - Mic Eingang
              : A6 - Mic Eingang
              : A4 - PotPin
              : mit Display zur Programmablaufanzeige
              : zur Steuerung kann auch ein HC-05 Modul benutzt werden
              : hierzu gibt es die App "Cinelightsvu.apk"
 Arduino      : ATMEGA2560
 Modul ID     : auf grüner Platte im Wohnzimmer
 Datum        : 22.07.2019 8:00
 Schaltung in : wird noch erstellt
 Hardwareinfo : Nur mit MEGA2560 (Speicherprobleme mit UNO)
 Status       : Ok in Arduino
 Einsatz      : V 2.0 : einbau eines Tasters zum Pro. fortschalten
              : V 2.1 : einbau eines Potis (Test) ,
              : V 2.2 : anpassen der Displayanzeige
              : V 2.3 : hinzufügen eines BLE-Modul
              : V 2.4W : Umsetzung in Eclipse und Display 1
              :        : Einbau eines Poti zur Helligkeitssteuerung
              : Wahlweise mit Display mit digital Ausgänge oder IC2 Display
 // ToDo       : Stereo-Verstärker
              :
 Hinweise     :
              :
*/
// TODO       : Stereo-Verstärker zum regeln des Audio-Eingang
    #include "Arduino.h"
    #define FASTLED_ALLOW_INTERRUPTS 1
    #define FASTLED_INTERRUPT_RETRY_COUNT 1
    #include <Adafruit_NeoPixel.h>
    #include <FastLED.h>
    #include <math.h>
    #include <Wire.h>
    #include <SoftwareSerial.h>
  //#ifdef IC2_Display          //@OS
    #include <LiquidCrystal.h>
//    #include <vu15.ino>
  //#else
  //  #include <LiquidCrystal_PCF8574.h>
  //#endif
#define DEBUG 									   // Auskommentieren wenn Serial.print nicht ausgeführt werden soll
    #define N_PIXELS       60                      // Number of pixels in strand
    #define N_PIXELS_2     60                      // Number of pixels in strand
    #define N_PIXELS_HALF (N_PIXELS/2)             // Mitte des Strip ermitteln
 // Pin_Definition
    #define MIC_PIN        A5                      // An diesem analogen Pin ist ein Mikrofon angebracht
    #define MIC_PIN_2      A4                      // An diesem analogen Pin ist ein Mikrofon angebracht
    #define LED_PIN        6                       // An diesen Pin ist der NeoPixel LED-Streifen angeschlossen
    #define LED_PIN_2      5                       // An diesen Pin ist der NeoPixel LED-Streifen2 angeschlossen
    #define POT_PIN        A3                      // Poti-PIN
    #define ButtonPin      2                       // Taster Low-Aktiv
    #define SAMPLE_WINDOW   10                     // Beispielfenster für Durchschnittsniveau
    #define SAMPLE_WINDOW_2 10                     // Beispielfenster für Durchschnittsniveau
    #define PEAK_HANG 40                           // Zeit der Pause, bevor der Spitzenpunkt abfällt /30
    #define PEAK_FALL 15                           // Tempo fallender Spitzenpunkte
    #define PEAK_FALL2 8                           // Tempo fallender Spitzenpunkte  //8
    #define INPUT_FLOOR 10                         // Unterer Bereich des analogen Leseeingangs  //10
    #define INPUT_CEILING 10                       // Maximaler Bereich des analogen Leseeingangs, je niedriger der Wert, desto empfindlicher (1023 = max) 300 (150)
    #define DC_OFFSET  0                           // DC-Offset im Mikrofonsignal - bei Ungewöhnlichkeit 0 lassen
    #define NOISE     10                           // Rauschen / Brummen / Störungen im Mikrofonsignal
    #define SAMPLES   64                           // Pufferlänge zur dynamischen Pegelanpassung
    #define SAMPLES2  64                           // Pufferlänge zur dynamischen Pegelanpassung
    #define TOP       (N_PIXELS + 4)               // Lassen Sie den Punkt etwas vom Maßstab abweichen
    #define SPEED .20                              // Betrag zur Erhöhung der RGB-Farbe um jeden Zyklus
    #define TOP2      (N_PIXELS + 1)               // Lassen Sie den Punkt etwas vom Maßstab abweichen
    #define LAST_PIXEL_OFFSET N_PIXELS-1
    #define PEAK_FALL_MILLIS 10                    // Fallrate des Punkt
    #define BG 0
    #if FASTLED_VERSION < 3001000
      #error "Requires FastLED 3.1 or later; check github for latest code."
    #endif
    #define BRIGHTNESS  60                         // Helligkeitswert
    #define LED_TYPE    WS2812B                    // Welche LED am LED_PIN für WS2812's benutzt werden
    #define COLOR_ORDER GRB                        // Farbeinstellung
    #define COLOR_MIN           0
    #define COLOR_MAX         255
    #define DRAW_MAX          100                  // Test @OS
    #define SEGMENTS            4                  // Anzahl der Segmente, in die der Amplitudenbalken geschnitten werden soll
    #define COLOR_WAIT_CYCLES  10                  // Loop-Zyklen zum Warten zwischen dem Fortschreiten des Pixelursprungs
    #define qsubd(x, b)  ((x>b)?b:0)
    #define qsuba(x, b)  ((x>b)?x-b:0)             // Analoges vorzeichenloses Subtraktionsmakro. Wenn Ergebnis <0, dann => 0. Von Andrew Tuline.
    #define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

    Adafruit_NeoPixel strip = Adafruit_NeoPixel(N_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
    Adafruit_NeoPixel strip1 = Adafruit_NeoPixel(N_PIXELS, LED_PIN_2, NEO_GRB + NEO_KHZ800);

    static uint16_t dist;                          // Eine Zufallszahl für den Rauschgenerator.
    uint16_t scale = 30;                           // Würde es nicht empfehlen, dies im laufenden Betrieb zu ändern, da die Animation sonst sehr blockig wird.
    uint8_t maxChanges = 48;                       // Wert für das Mischen zwischen Paletten.


    int Helligkeit;
    CRGBPalette16 currentPalette(OceanColors_p);
    CRGBPalette16 targetPalette(CloudColors_p);

    //Bluetooth
    #define BLUETOOTH_SPEED 9600                   // Geschwindigtkeit für's Bluetooth Modul
//    SoftwareSerial bluetooth(18, 19);
    int choice = 25; // bluetooth choice
//    bool IC2_Display = true;
//    #ifdef IC2_Display
    // Display
       const int PIN_RS = 22,PIN_EN = 26, PIN_D4 = 28, PIN_D5 = 30, PIN_D6 = 32, PIN_D7 = 34;  // festverbaut im Wohnzimmer                                                                                                                                                                                                                                                                                                                                                                                                                                                                      eingebautes Display
       //const int PIN_RS = 22,PIN_EN = 24, PIN_D4 = 26, PIN_D5 = 28, PIN_D6 = 30, PIN_D7 = 32;    // Display Nr. 2 Test-Modul

//    #else
//        LiquidCrystal_PCF8574 lcd(0x27);
//    #endif
    int Zeile = 2;
    int Spalte = 16;
    LiquidCrystal lcd(PIN_RS, PIN_EN, PIN_D4, PIN_D5, PIN_D6, PIN_D7);
    String myArray[15]= {"vu", "vu1","vu2" ,"vu3", "Vu4", "vu5", "vu6", "vu7", "vu8", "vu9", "vu10", "vu11", "vu12", "vu14","vu15"};
    String myArray2[7]=  {"Fire", "Fireblu", "Juggle","Ripple", "Sinelon", "Twinkle", "Balls"};

// Startanimation Auswahl
CRGB leds[N_PIXELS];
int numOfSegments;
int stripMiddle;
                                   	// Startanimation Auswahl (1 sieht gut aus für 1 Ring)  @EB
uint32_t stripColor[60];                        	// MY Variablen
int stripNumOfLeds = N_PIXELS;
int stripsOn2Pins = true;
//int startupAnimationDelay = 1;

//new ripple vu
uint8_t timeval = 20;                               // Gegenwärtiger Wert für "Verzögerung". Nein, ich benutze keine Verzögerungen, sondern EVERY_N_MILLIS_I.
uint16_t loops = 0;                                 // Unsere Schleifen pro Sekunde Zähler.
bool     samplepeak = 0;                            // Diese Stichprobe liegt weit über dem Durchschnitt und ist ein "Peak".
uint16_t oldsample = 0;                             // Die vorherige Stichprobe wird für die Spitzenwerterfassung und für On-the-Fly-Werte verwendet.
bool thisdir = 0;
//new ripple vu

// Modes
enum
{
} MODE;
bool reverse = true;
int BRIGHTNESS_MAX = 80;
int brightness = 20;


byte
peakLeft      = 0,                                  // Wird für fallende Punkte verwendet
peakRight     = 0,
dotCountLeft  = 0,                                  // Bildzähler zum Verzögern der Punktabfallgeschwindigkeit
dotCountRight  = 0,                                 // Bildzähler zum Verzögern der Punktabfallgeschwindigkeit
volCount  = 0;                                      // Bildzähler zum Speichern vergangener Volumendaten

int
  reading,
  vol[SAMPLES],                                     // Sammlung früherer Volumenproben
  lvl       = 10,                                   // Aktueller "gedämpfter" Audiopegel
  minLvlAvg = 0,                                    // Zur dynamischen Anpassung von Grafik niedrig und hoch
  maxLvlAvg = 512;

uint8_t volCountLeft = 0;                           // Bildzähler zum Speichern vergangener Volumendaten
int volLeft[SAMPLES];                               // Sammlung früherer Volumenproben
int lvlLeft = 10;                                   // Aktueller "gedämpfter" Audiopegel
int minLvlAvgLeft = 0;                              // Zur dynamischen Anpassung von Grafik niedrig und hoch
int maxLvlAvgLeft = 512;

uint8_t volCountRight = 0;                          // Bildzähler zum Speichern vergangener Volumendaten
int volRight[SAMPLES];                              // Sammlung früherer Volumenproben
int lvlRight = 10;                                  // Current "dampened" audio level
int minLvlAvgRight = 0;                             // For dynamic adjustment of graph low & high
int maxLvlAvgRight = 512;



void balls();
void fire();
void fireblu();
void juggle();
void ripple(boolean show_background);
void sinelon();
void twinkle();
void rainbow(uint8_t rate);
void vu15(bool is_centered, uint8_t channel);
//void startupAnimation();
void vu();
void vu1();
void vu2();
void vu3();
void Vu4();
void vu5();
void Vu6();
void vu7();
void vu8();
void vu9();
void vu10();
void vu11();
void vu12();
//void vu13();
void vu14();
void All2();
void All();
uint32_t Wheel(byte WheelPos);
//uint16_t auxReading(uint8_t channel);
//void dropPeak(uint8_t channel);
//void averageReadings(uint8_t channel);


CRGB ledsLeft[N_PIXELS];
CRGB ledsRight[N_PIXELS];


float
  greenOffset = 30,
  blueOffset = 150;
// cycle variables

int CYCLE_MIN_MILLIS = 2;
int CYCLE_MAX_MILLIS = 1000;
int cycleMillis = 20;
bool paused = false;
long lastTime = 0;
bool boring = true;
bool gReverseDirection = false;
int          myhue =   0;
//vu ripple

uint8_t colour;
uint8_t myfade = 255;                                         // Starthelligkeit.
#define maxsteps 16                                           // Die case-Anweisung würde keine Variable zulassen.
int peakspersec = 0;
int peakcount = 0;
uint8_t bgcol = 0;
int thisdelay = 20;
uint8_t max_bright = 60;   //255

    uint32_t red = strip.Color(255, 0, 0);
    uint32_t orange = strip.Color(255, 127, 0);
    uint32_t yellow = strip.Color(255, 255, 0);
    uint32_t green = strip.Color(0, 255, 0);
    uint32_t blue = strip.Color(0, 0, 255);
    uint32_t purple = strip.Color(75, 0, 130);
    uint32_t white = strip.Color(125, 125, 125);


  unsigned int sample;

//Samples
#define NSAMPLES 64
unsigned int samplearray[NSAMPLES];
unsigned long samplesum = 0;
unsigned int sampleavg = 0;
int samplecount = 0;
//unsigned int sample = 0;
unsigned long oldtime = 0;
unsigned long newtime = 0;

//Ripple variables
int color;
int center = 0;
int step = -1;
int maxSteps = 16;
float fadeRate = 0.80;
int diff;

//vu 8 variables
int
  origin = 0,
  color_wait_count = 0,
  scroll_color = COLOR_MIN,
  last_intensity = 0,
  intensity_max = 0,
  origin_at_flip = 0;
uint32_t
    draw[DRAW_MAX];
boolean
  growing = false,
  fall_from_left = true;



//background color
uint32_t currentBg = random(256);
uint32_t nextBg = currentBg;
TBlendType    currentBlending;


    byte peak = 16;                         // Höchststand der Säule; für fallende Punkte verwendet
//    unsigned int sample;
    byte dotCount = 0;                      // Bildzähler für Spitzenpunkte
    byte dotHangCount = 0;                  // Bildzähler zum Halten von Spitzenpunkten
// Taster
    int buttonPushCounter = 0;              // Zähler für die Anzahl der Tastendrücke
    int buttonState = 0;                    // aktueller Status des Taster
    int lastButtonState = 0;                // Letzter Status des Taster
int show = -1;



void setup() {
//    int error;
    Serial.begin(57600);                    // Geschwindigkeit der ersten Serialen Schnittstelle Uart0
    Serial1.begin(115200);                  // zweite Seriale Schnittstelle für's Bluethooth Modul
    pinMode(ButtonPin, INPUT);              // Taste zum Programm weiterschalten
    buttonState = digitalRead(ButtonPin);   // Lesen Sie den Tastereingangspin:
    digitalWrite(ButtonPin, HIGH);          // setzt den Eingang auf High
    //analogReference(EXTERNAL);            // Funktion um eine externe Referenzspannung auf dem Arduino anzulegen nur zwischen 0-5V
    Serial1.begin(BLUETOOTH_SPEED);         // Geschwindigkeit der zweiten Serialen Schnittstelle
//    Wire.begin();
//    Wire.beginTransmission(0x27);
//    error = Wire.endTransmission();
//    Serial.print("Error: ");
//    Serial.print(error);
//  if (error == 0)  {
#ifdef DEBUG
    Serial.println(" LCD gefunden");
#endif
    show = 0;
    delay(1500);
    lcd.begin(16,2);						// (Spalte, Zeile) 0 Basieren
//    lcd.setBacklight(255);
    lcd.setCursor(0,0);
    lcd.print("VU Meter V2.4-BL");
//    lcd.setCursor(0,1);
//    lcd.print("  Display - 0   ");


      strip.begin();
      strip.setBrightness(BRIGHTNESS);
      strip.show();                         // all pixels to 'off'
      strip1.begin();
      strip1.setBrightness(BRIGHTNESS);
      strip1.show();                        // all pixels to 'off'
    Serial.begin(115200);
      delay(5000);
  //  lcd.clear();
  //  lcd.setCursor(0,0);                     // ( Spalte / Zeile )
  //  lcd.print("VU Meter V2.4-BL");
  FastLED.addLeds < LED_TYPE, LED_PIN, COLOR_ORDER > (ledsLeft, N_PIXELS).setCorrection(TypicalLEDStrip);
  FastLED.addLeds < LED_TYPE, LED_PIN_2, COLOR_ORDER > (ledsRight, N_PIXELS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  dist = random16(12345);          // A semi-random number for our noise generator
  choice = 25;

  // See http://playground.arduino.cc/Main/I2cScanner how to test for a I2C device.
//  Wire.begin();
//  Wire.beginTransmission(0x27);
//  error = Wire.endTransmission();
//  Serial.print("Error: ");
//  Serial.print(error);
//  startupAnimation();
 }


float fscale( float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve){

  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;


  // condition curve parameter
  // limit range

  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invertieren und skalieren - das scheint intuitiver zu sein - positive Zahlen geben dem High-End der Ausgabe mehr Gewicht
  curve = pow(10, curve); // wandle die lineare Skala in einen lograthimischen Exponenten für andere pow-Funktionen um


  // Check for out of range inputValues
  if (inputValue < originalMin) {
    inputValue = originalMin;
  }
  if (inputValue > originalMax) {
    inputValue = originalMax;
  }

  // Null Referenzieren Sie die Werte
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin){
    NewRange = newEnd - newBegin;
  }
  else
  {
    NewRange = newBegin - newEnd;
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalisiere auf 0 - 1 float


  // Auf originalMin> originalMax prüfen - die Mathematik für alle anderen Fälle, d. H. Negative Zahlen, scheint in Ordnung zu sein
  if (originalMin > originalMax ) {
    return 0;
  }

  if (invFlag == 0){
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;

  }
  else     // invert the ranges
  {
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange);
  }

  return rangedValue;

  }

void loop() {
    //for mic
  uint8_t  i;
  uint16_t minLvlLeft, maxLvlLeft;
  uint16_t minLvlRight, maxLvlRight;
  int      n, height;
  Helligkeit = analogRead(POT_PIN);
  int H = map(Helligkeit,0,1023,0,255);
  FastLED.setBrightness(H);
//  lcd.setCursor(1,14);
  buttonState = digitalRead(ButtonPin);           // Lesen Sie den Tastereingangspin:
  if (buttonState != lastButtonState) {           // vergleiche den buttonState mit seinem vorherigen Zustand
    if (buttonState == HIGH)  {                   // Wenn sich der Zustand geändert hat, erhöhen Sie den Zähler
                                                  // Wenn der aktuelle Status HIGH ist, dann die Schaltfläche
      buttonPushCounter++;                        // Tasterzähler eins erhöhen
                                                  // ab hier die Anzeige anzeigen
      lcd.setCursor(0,1);						  // zweite Zeile 1. Zeichen
      //lcd.print("Pr-Nr. : ");                   // 8 Stellen
      lcd.print(buttonPushCounter);
      if(buttonPushCounter==25) {                 // wenn 25
        buttonPushCounter=0;}                     // wieder auf 1 setzen  @OS
      //lcd.setCursor(1,15);
      //lcd.print("B");
    }
  }
    lastButtonState = buttonState;                // speichere den aktuellen Zustand als letzten Zustand, für das nächste Mal durch die Schleife
// TODO in die Switchanweisung
    switch (buttonPushCounter){
      case 1: {
    	 lcd.setCursor(0,1);
    	 lcd.print("Prog-Nr.:       ");
    	 lcd.setCursor(10,1);
    	 lcd.print(choice);
        vu();
        break;}
      case 2: {
     	 lcd.setCursor(0,1);
     	 lcd.print("Prog-Nr.:       ");
     	 lcd.setCursor(10,1);
     	 lcd.print(choice);
    	  vu1();
        break;}
      case 3: {
     	 lcd.setCursor(0,1);
     	 lcd.print("Prog-Nr.:       ");
     	 lcd.setCursor(10,1);
     	 lcd.print(choice);
        vu3();
        break;}
      case 4: {
     	 lcd.setCursor(0,1);
     	 lcd.print("Prog-Nr.:       ");
     	 lcd.setCursor(10,1);
     	 lcd.print(choice);
        Vu4();
        break;}
      case 5: {
        vu2();
        break;}
      case 6: {
        vu10();
        break;}
      case 7: {
        Vu6();
        break;}
      case 8: {
        vu5();
        break;}
      case 9: {
        vu7();
        break;}
      case 10:{
        vu();         // doppelt
        break;}
      case 11:{
        vu8();
        break;}
      case 12:{
        vu11();
        break;}
      case 13:{
        vu12();
        break;}
      case 14:{
        vu9();
        break;}
      case 15:{
        vu15(false,0);     // classic
        vu15(false,1);     // classic
        break;}
      case 16:{
        fireblu();
        break;}
      case 17:{
        fire();
        break;}
      case 18:{
        juggle();
        break;}
      case 19:{
        ripple(true);    // mit Hintergrundbeleuchtung
        break;}
      case 20:{
        ripple(false);    // ohne Hintergrundbeleuchtung
        break;}
      case 21:{
        rainbow(5);
        break;}
      case 22:{
        balls();
        break;}
      case 23:{
        sinelon();
        break;}
      case 24:{
        twinkle();
        break;}
      case 25:{   // Auto VU
        All2();
        break;}
      case 26:{   // Auto Standby
        All();
        break;}
     }
  if (Serial1.available() > 0) {
    // read the incoming byte:
    uint8_t incomingByte = Serial1.read();
#ifdef DEBUG
    Serial.println(incomingByte);
#endif
    //lcd.clear();
    lcd.setCursor(0,1);
    lcd.print("               B");
  if (incomingByte == 1){        // VU 1  von unten  mit Peak
    lcd.setCursor(0,1);
    lcd.print("Prog: VU  1");
    lcd.setCursor(15,1);
    lcd.print(incomingByte);
    choice = 1;}
   else if (incomingByte == 2){  // VU 2  von Mitte mit Peak
   lcd.setCursor(0,1);
    lcd.print("Prog: VU  2");
    lcd.setCursor(15,1);
    lcd.print(incomingByte);
     choice = 2;}
   else if (incomingByte == 3){  // VU 3  von unten  mit Peak
   lcd.setCursor(0,1);
    lcd.print("Prog: VU  3");
    lcd.setCursor(15,1);
    lcd.print(incomingByte);
    choice = 3;}
   else if (incomingByte == 4){  // VU 4 von Mitte ohne Peak
    lcd.setCursor(0,1);
    lcd.print("Prog: VU  4");
    lcd.setCursor(15,1);
    lcd.print(incomingByte);
   choice = 4;}
   else if (incomingByte == 5){  // VU 5 von unten mit verschieden Farbigen Ball
    lcd.setCursor(0,1);
    lcd.print("Prog: VU  5");
    lcd.setCursor(15,1);
    lcd.print(incomingByte);
    choice = 5;}
   else if (incomingByte == 6){  // VU 6 von Mitte Rot mit blauen Ball
    lcd.setCursor(0,1);
    lcd.print("Prog: VU  6");
    lcd.setCursor(15,1);
    lcd.print(incomingByte);
    choice = 6;}
   else if (incomingByte == 7){  // VU 7 nur Bälle je Kanal mit verschiedenen Farben
    lcd.setCursor(0,1);
    lcd.print("Prog: VU  7");
    lcd.setCursor(15,1);
    lcd.print(incomingByte);
    choice = 7;}
   else if (incomingByte == 8){  // VU 8 von unten Farbig mit zwei weissen Bällen
    lcd.setCursor(0,1);
    lcd.print("Prog: VU  8");
    lcd.setCursor(15,1);
    lcd.print(incomingByte);
    choice = 8;}
   else if (incomingByte == 9){  // VU 9 Farbige Hintergrund mit Glitter gegenläufig
    lcd.setCursor(0,1);
    lcd.print("Prog: VU  9");
    lcd.setCursor(15,1);
    lcd.print(incomingByte);
    choice = 9;}
   else if (incomingByte == 10){  // VU 10
    lcd.setCursor(0,1);
    lcd.print("Prog: VU 10");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 10;}
   else if (incomingByte == 11){  // VU 11 4x
    lcd.setCursor(0,1);
    lcd.print("Prog: VU 11");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 11;}
   else if (incomingByte == 12){  // VU 12 aW
    lcd.setCursor(0,1);
    lcd.print("Prog: VU 12");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 12;}
   else if (incomingByte == 13){  // VU 13 AnalogWerte
    lcd.setCursor(0,1);
    lcd.print("Prog: VU 13");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 13;}
   else if (incomingByte == 14){   // VU 14
    lcd.setCursor(0,1);
    lcd.print("Prog: VU 14");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 14;}
   else if (incomingByte == 15){   // VU 15
    lcd.setCursor(0,1);
    lcd.print("Prog: VU 15");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 15;}
   else if (incomingByte == 16){   // Fire Blue
    lcd.setCursor(0,1);
    lcd.print("Fire Blue  ");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 16;}
   else if (incomingByte == 17){   // Fire
    lcd.print("Fire       ");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 17;}
   else if (incomingByte == 18){   // Juggle
    lcd.print("Juggle     ");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 18;}
   else if (incomingByte == 19){   // Ripple1
    lcd.print("Ripple     ");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 19;}
   else if (incomingByte == 20){   // Ripple2
    lcd.print("Ripple2    ");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 20;}
   else if (incomingByte == 21){   // Rainbow
    lcd.print("Rainbow    ");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 21;}
   else if (incomingByte == 22){   // Balls
    lcd.print("Balls      ");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 22;}
   else if (incomingByte == 23){  // Sinelon
    lcd.print("Silelon    ");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 23;}
   else if (incomingByte == 24){  // Twinkle
    lcd.print("Twinkle    ");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 24;}
   else if (incomingByte == 25){  // Auto VU
    lcd.print("Auto VU    ");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 25;}
   else if (incomingByte == 26){  // Auto Standby
    lcd.print("Auto Sandby");
    lcd.setCursor(14,1);
    lcd.print(incomingByte);
    choice = 26;}
  }

        switch(choice){
        case 1 : {
            vu1();
          break;}

        case 2: {
            vu2();
          break;}

        case 3: {
            vu3();
          break;}

          case 4: {
            Vu4();
          break;}

          case 5: {
            vu2();
          break;}

          case 6: {
            vu10();
          break;}

          case 7: {
            Vu6();
          break;}

          case 8: {
            vu5();
          break;}

          case 9: {
            vu7();
          break;}

           case 10: {
            vu14();
          break;}

           case 11: {
            vu8();
          break;}

          case 12: {
            vu11();
          break;}

        case 13: {
            vu12();
          break;}

          case 14: {
            vu9();
          break;}

        case 15: {
            vu15(false, 0); // classic
           vu15(false, 1); // classic
          break;}

          case 16: {
           fireblu();
           break;}

          case 17: {
           fire();
           break;}

           case 18: {
           juggle();
           break;}


        case 19: {
           ripple(true); //ripple bg
          break;}

        case 20: {
          ripple(false); // ripple
          break;}

         case 21: {
          rainbow(5); // rainbow
          break;}

          case 22: { 		// 3 springende Bälle
          balls();
          break;}

          case 23: {
          sinelon();
          break;}
          case 24: {
          twinkle();
          break;}
          case 25: {
          lcd.setCursor(0,1);
          lcd.print("- Auto VU -");
          All2();
          break;}
          case 26: {
          lcd.setCursor(0,1);
          lcd.print("-Auto Standby-");
          All();
          break;}

          }
}
void vu() {
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 0 ");
  uint8_t  i;
  uint16_t minLvlLeft, maxLvlLeft;
  uint16_t minLvlRight, maxLvlRight;
  int      n, height;

  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlLeft = ((lvlLeft * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlLeft - minLvlAvgLeft) / (long)(maxLvlAvgLeft - minLvlAvgLeft);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakLeft)     peakLeft   = height; // Keep 'peak' dot at top


  // Color pixels based on rainbow gradient
  for(i=0; i<N_PIXELS; i++) {
    if(i >= height)               strip.setPixelColor(i,   0,   0, 0);
    else strip.setPixelColor(i,Wheel(map(i,0,strip.numPixels()-1,30,150)));
  }
   	   	   	   	   	   	   	   	   	   	   	   	   // Draw peak dot
  if(peakLeft > 0 && peakLeft <= N_PIXELS-1) strip.setPixelColor(peakLeft,Wheel(map(peakLeft,0,strip.numPixels()-1,30,150)));
   strip.show(); 								// Update strip
   	   	   	   	   	   	   	   	   	   	   	   	   // Every few frames, make the peak pixel drop by 1:

    if(++dotCountLeft >= PEAK_FALL) { 				//fall rate

      if(peakLeft > 0) peakLeft--;
      dotCountLeft = 0;
    }
  volLeft[volCountLeft] = n;                      // Save sample for dynamic leveling
  if(++volCountLeft >= SAMPLES) volCountLeft = 0; // Advance/rollover sample counter


  minLvlLeft = maxLvlLeft = vol[0];					// Get volume range of prior frames
  for(i=1; i<SAMPLES; i++) {
    if(volLeft[i] < minLvlLeft)      minLvlLeft = volLeft[i];
    else if(volLeft[i] > maxLvlLeft) maxLvlLeft= volLeft[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvlLeft - minLvlLeft) < TOP) maxLvlLeft = minLvlLeft + TOP;
  minLvlAvgLeft = (minLvlAvgLeft * 63 + minLvlLeft) >> 6; 	// Dampen min/max levels
  maxLvlAvgLeft = (maxLvlAvgLeft * 63 + maxLvlLeft) >> 6; 	// (fake rolling average)
  n   = analogRead(MIC_PIN_2);                        		// Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); 							// Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             		// Remove noise/hum
  lvlRight = ((lvlRight * 7) + n) >> 3;    					// "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlRight - minLvlAvgRight) / (long)(maxLvlAvgRight - minLvlAvgRight);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakRight)     peakRight   = height; // Keep 'peak' dot at top
  for(i=0; i<N_PIXELS; i++) {								// Color pixels based on rainbow gradient
    if(i >= height)               strip1.setPixelColor(i,   0,   0, 0);
    else strip1.setPixelColor(i,Wheel(map(i,0,strip1.numPixels()-1,30,150)));
  }
  // Draw peak dot
  if(peakRight > 0 && peakRight <= N_PIXELS-1) strip1.setPixelColor(peakRight,Wheel(map(peakRight,0,strip1.numPixels()-1,30,150)));
   strip1.show(); 											// Update strip
   	   	   	   	   	   	   	   	   	   // Every few frames, make the peak pixel drop by 1:
    if(++dotCountRight >= PEAK_FALL) { //fall rate
      if(peakRight > 0) peakRight--;
      dotCountRight = 0;
      }
  volRight[volCountRight] = n;                      // Save sample for dynamic leveling
  if(++volCountRight >= SAMPLES2) volCountRight = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvlRight = maxLvlRight = vol[0];
  for(i=1; i<SAMPLES2; i++) {
    if(volRight[i] < minLvlRight)      minLvlRight = volRight[i];
    else if(volRight[i] > maxLvlRight) maxLvlRight = volRight[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvlRight - minLvlRight) < TOP) maxLvlRight = minLvlRight + TOP;
  minLvlAvgRight = (minLvlAvgRight * 63 + minLvlRight) >> 6; // Dampen min/max levels
  maxLvlAvgRight = (maxLvlAvgRight * 63 + maxLvlRight) >> 6; // (fake rolling average)
}


uint32_t Wheel(byte WheelPos) {		// Geben Sie einen Wert von 0 bis 255 ein, um einen Farbwert zu erhalten.
                              		// Die Farben sind ein Übergang von r - g - b - zurück zu r.
	if(WheelPos < 85) {
		return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
	} else if(WheelPos < 170) {
		WheelPos -= 85;
		return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
	} else {
		WheelPos -= 170;
		return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
	}
}


void vu1() {
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 1 ");
  uint8_t  i;
  uint16_t minLvlLeft, maxLvlLeft;
  uint16_t minLvlRight, maxLvlRight;
  int      n, height;

  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlLeft = ((lvlLeft * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

   // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlLeft - minLvlAvgLeft) / (long)(maxLvlAvgLeft - minLvlAvgLeft);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakLeft)     peakLeft   = height; // Keep 'peak' dot at top


  // Color pixels based on rainbow gradient
  for(i=0; i<N_PIXELS_HALF; i++) {
    if(i >= height) {
      strip.setPixelColor(N_PIXELS_HALF-i-1,   0,   0, 0);
      strip.setPixelColor(N_PIXELS_HALF+i,   0,   0, 0);
    }
    else {
      uint32_t color = Wheel(map(i,0,N_PIXELS_HALF-1,30,150));
      strip.setPixelColor(N_PIXELS_HALF-i-1,color);
      strip.setPixelColor(N_PIXELS_HALF+i,color);
   }

  }

// Draw peak dot
  if(peakLeft > 0 && peakLeft <= N_PIXELS_HALF-1) {
    uint32_t color = Wheel(map(peak,0,N_PIXELS_HALF-1,30,150));
    strip.setPixelColor(N_PIXELS_HALF-peakLeft-1,color);
    strip.setPixelColor(N_PIXELS_HALF+peakLeft,color);
  }

   strip.show(); // Update strip

// Every few frames, make the peak pixel drop by 1:

    if(++dotCountLeft >= PEAK_FALL) { //fall rate

      if(peakLeft > 0) peakLeft--;
      dotCountLeft = 0;
    }
  // Draw peak dot
  if(peakLeft > 0 && peakLeft <= N_PIXELS_HALF-1) {
    uint32_t color = Wheel(map(peakLeft,0,N_PIXELS_HALF-1,30,150));
    strip.setPixelColor(N_PIXELS_HALF-peakLeft-1,color);
    strip.setPixelColor(N_PIXELS_HALF+peakLeft,color);
  }

   strip.show(); // Update strip

// Every few frames, make the peak pixel drop by 1:

    if(++dotCountLeft >= PEAK_FALL) { //fall rate

      if(peakLeft > 0) peakLeft--;
      dotCountLeft = 0;
    }



  volLeft[volCountLeft] = n;                      // Save sample for dynamic leveling
  if(++volCountLeft >= SAMPLES) volCountLeft = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvlLeft = maxLvlLeft = vol[0];
  for(i=1; i<SAMPLES; i++) {
    if(volLeft[i] < minLvlLeft)      minLvlLeft = volLeft[i];
    else if(volLeft[i] > maxLvlLeft) maxLvlLeft= volLeft[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
   if((maxLvlLeft - minLvlLeft) < TOP) maxLvlLeft = minLvlLeft + TOP;
  minLvlAvgLeft = (minLvlAvgLeft * 63 + minLvlLeft) >> 6; // Dampen min/max levels
  maxLvlAvgLeft = (maxLvlAvgLeft * 63 + maxLvlLeft) >> 6; // (fake rolling average)


  n   = analogRead(MIC_PIN_2);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlRight = ((lvlRight * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlRight - minLvlAvgRight) / (long)(maxLvlAvgRight - minLvlAvgRight);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakRight)     peakRight   = height; // Keep 'peak' dot at top


  // Color pixels based on rainbow gradient
  for(i=0; i<N_PIXELS_HALF; i++) {
    if(i >= height) {
      strip1.setPixelColor(N_PIXELS_HALF-i-1,   0,   0, 0);
      strip1.setPixelColor(N_PIXELS_HALF+i,   0,   0, 0);
    }
    else {
      uint32_t color = Wheel(map(i,0,N_PIXELS_HALF-1,30,150));
      strip1.setPixelColor(N_PIXELS_HALF-i-1,color);
      strip1.setPixelColor(N_PIXELS_HALF+i,color);
    }

  }

// Draw peak dot
  if(peakRight > 0 && peakRight <= N_PIXELS_HALF-1) {
    uint32_t color = Wheel(map(peakRight,0,N_PIXELS_HALF-1,30,150));
    strip1.setPixelColor(N_PIXELS_HALF-peakRight-1,color);
    strip1.setPixelColor(N_PIXELS_HALF+peakRight,color);
  }

   strip1.show(); // Update strip

// Every few frames, make the peak pixel drop by 1:

    if(++dotCountRight >= PEAK_FALL) { //fall rate

      if(peakRight > 0) peakRight--;
      dotCountRight = 0;
    }
  // Draw peak dot
  if(peakRight > 0 && peakRight <= N_PIXELS_HALF-1) {
    uint32_t color = Wheel(map(peakRight,0,N_PIXELS_HALF-1,30,150));
    strip1.setPixelColor(N_PIXELS_HALF-peakRight-1,color);
    strip1.setPixelColor(N_PIXELS_HALF+peakRight,color);
  }

   strip1.show(); // Update strip

// Every few frames, make the peak pixel drop by 1:

     if(++dotCountRight >= PEAK_FALL) { //fall rate

      if(peakRight > 0) peakRight--;
      dotCountRight = 0;
      }
  volRight[volCountRight] = n;                      // Save sample for dynamic leveling
  if(++volCountRight >= SAMPLES2) volCountRight = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvlRight = maxLvlRight = vol[0];
  for(i=1; i<SAMPLES2; i++) {
    if(volRight[i] < minLvlRight)      minLvlRight = volRight[i];
    else if(volRight[i] > maxLvlRight) maxLvlRight = volRight[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvlRight - minLvlRight) < TOP) maxLvlRight = minLvlRight + TOP;
  minLvlAvgRight = (minLvlAvgRight * 63 + minLvlRight) >> 6; // Dampen min/max levels
  maxLvlAvgRight = (maxLvlAvgRight * 63 + maxLvlRight) >> 6; // (fake rolling average)

}


void vu2() {
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 2");
  uint8_t  i;
  uint16_t minLvlLeft, maxLvlLeft;
  uint16_t minLvlRight, maxLvlRight;
  int      n, height;
//  lcd.setCursor(1,0);
//  lcd.print("VU-Meter 3 ");
  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlLeft = ((lvlLeft * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlLeft - minLvlAvgLeft) / (long)(maxLvlAvgLeft - minLvlAvgLeft);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakLeft)     peakLeft   = height; // Keep 'peak' dot at top

  // Color pixels based on rainbow gradient
  for(i=0; i<N_PIXELS; i++) {
    if(i >= height)               strip.setPixelColor(i,   0,   0, 0);
                                 else strip.setPixelColor(i,0, 0, 255);

  }


  // Draw peak dot
  if(peakLeft > 0 && peakLeft <= N_PIXELS-1) strip.setPixelColor(peakLeft,(map(peakLeft,0,strip.numPixels()-1,green,green)));

   strip.show(); // Update strip

// Every few frames, make the peak pixel drop by 1:

    if(++dotCountLeft >= PEAK_FALL) { //fall rate

      if(peakLeft > 0) peakLeft--;
      dotCountLeft = 0;
    }

  volLeft[volCountLeft] = n;                      // Save sample for dynamic leveling
  if(++volCountLeft >= SAMPLES) volCountLeft = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvlLeft = maxLvlLeft = vol[0];
  for(i=1; i<SAMPLES; i++) {
    if(volLeft[i] < minLvlLeft)      minLvlLeft = volLeft[i];
    else if(volLeft[i] > maxLvlLeft) maxLvlLeft= volLeft[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvlLeft - minLvlLeft) < TOP) maxLvlLeft = minLvlLeft + TOP;
  minLvlAvgLeft = (minLvlAvgLeft * 63 + minLvlLeft) >> 6; // Dampen min/max levels
  maxLvlAvgLeft = (maxLvlAvgLeft * 63 + maxLvlLeft) >> 6; // (fake rolling average)

  n   = analogRead(MIC_PIN_2);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlRight = ((lvlRight * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlRight - minLvlAvgRight) / (long)(maxLvlAvgRight - minLvlAvgRight);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakRight)     peakRight   = height; // Keep 'peak' dot at top


  // Color pixels based on rainbow gradient
  for(i=0; i<N_PIXELS; i++) {
    if(i >= height)               strip1.setPixelColor(i,   0,   0, 0);
    else strip1.setPixelColor(i,0, 0, 255);

  }


  // Draw peak dot
  if(peakRight > 0 && peakRight <= N_PIXELS-1) strip1.setPixelColor(peakRight,(map(peakRight,0,strip1.numPixels()-1,red,red)));

   strip1.show(); // Update strip

// Every few frames, make the peak pixel drop by 1:

    if(++dotCountRight >= PEAK_FALL) { //fall rate

      if(peakRight > 0) peakRight--;
      dotCountRight = 0;
      }

  volRight[volCountRight] = n;                      // Save sample for dynamic leveling
  if(++volCountRight >= SAMPLES2) volCountRight = 0; // Advance/rollover sample counter

   // Get volume range of prior frames
  minLvlRight = maxLvlRight = vol[0];
  for(i=1; i<SAMPLES2; i++) {
    if(volRight[i] < minLvlRight)      minLvlRight = volRight[i];
    else if(volRight[i] > maxLvlRight) maxLvlRight = volRight[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvlRight - minLvlRight) < TOP) maxLvlRight = minLvlRight + TOP;
  minLvlAvgRight = (minLvlAvgRight * 63 + minLvlRight) >> 6; // Dampen min/max levels
  maxLvlAvgRight = (maxLvlAvgRight * 63 + maxLvlRight) >> 6; // (fake rolling average)
}



void vu3() {
  uint8_t  i;
  uint16_t minLvlLeft, maxLvlLeft;
  uint16_t minLvlRight, maxLvlRight;
  int      n, height;
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 3 ");
  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlLeft = ((lvlLeft * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlLeft - minLvlAvgLeft) / (long)(maxLvlAvgLeft - minLvlAvgLeft);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakLeft)     peakLeft   = height; // Keep 'peak' dot at top

  greenOffset += SPEED;
  blueOffset += SPEED;
  if (greenOffset >= 255) greenOffset = 0;
  if (blueOffset >= 255) blueOffset = 0;

  // Color pixels based on rainbow gradient
  for (i = 0; i < N_PIXELS; i++) {
    if (i >= height) {
      strip.setPixelColor(i, 0, 0, 0);
    } else {
      strip.setPixelColor(i, Wheel(
        map(i, 0, strip.numPixels() - 1, (int)greenOffset, (int)blueOffset)
      ));
    }
  }
// Draw peak dot
  if(peakLeft > 0 && peakLeft <= N_PIXELS-1) strip.setPixelColor(peakLeft,Wheel(map(peakLeft,0,strip.numPixels()-1,30,150)));

   strip.show(); // Update strip

// Every few frames, make the peak pixel drop by 1:

    if(++dotCountLeft >= PEAK_FALL) { //fall rate

      if(peakLeft > 0) peakLeft--;
      dotCountLeft = 0;
    }

   volLeft[volCountLeft] = n;                      // Save sample for dynamic leveling
  if(++volCountLeft >= SAMPLES) volCountLeft = 0; // Advance/rollover sample counter


   // Get volume range of prior frames
  minLvlLeft = maxLvlLeft = vol[0];
  for(i=1; i<SAMPLES; i++) {
    if(volLeft[i] < minLvlLeft)      minLvlLeft = volLeft[i];
    else if(volLeft[i] > maxLvlLeft) maxLvlLeft= volLeft[i];
  }

  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvlLeft - minLvlLeft) < TOP) maxLvlLeft = minLvlLeft + TOP;
  minLvlAvgLeft = (minLvlAvgLeft * 63 + minLvlLeft) >> 6; // Dampen min/max levels
  maxLvlAvgLeft = (maxLvlAvgLeft * 63 + maxLvlLeft) >> 6; // (fake rolling average)

  n   = analogRead(MIC_PIN_2);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlRight = ((lvlRight * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

   // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlRight - minLvlAvgRight) / (long)(maxLvlAvgRight - minLvlAvgRight);

 if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakRight)     peakRight   = height; // Keep 'peak' dot at top


  greenOffset += SPEED;
  blueOffset += SPEED;
  if (greenOffset >= 255) greenOffset = 0;
  if (blueOffset >= 255) blueOffset = 0;

  // Color pixels based on rainbow gradient
  for (i = 0; i < N_PIXELS; i++) {
    if (i >= height) {
      strip1.setPixelColor(i, 0, 0, 0);
    } else {
      strip1.setPixelColor(i, Wheel(
        map(i, 0, strip1.numPixels() - 1, (int)greenOffset, (int)blueOffset)
      ));
    }
  }

// Draw peak dot
  if(peakRight > 0 && peakRight <= N_PIXELS-1) strip1.setPixelColor(peakRight,Wheel(map(peakRight,0,strip1.numPixels()-1,30,150)));

   strip1.show(); // Update strip

// Every few frames, make the peak pixel drop by 1:

    if(++dotCountRight >= PEAK_FALL) { //fall rate

      if(peakRight > 0) peakRight--;
      dotCountRight = 0;
      }

 volRight[volCountRight] = n;                      // Save sample for dynamic leveling
  if(++volCountRight >= SAMPLES2) volCountRight = 0; // Advance/rollover sample counter


 // Get volume range of prior frames
  minLvlRight = maxLvlRight = vol[0];
  for(i=1; i<SAMPLES2; i++) {
    if(volRight[i] < minLvlRight)      minLvlRight = volRight[i];
    else if(volRight[i] > maxLvlRight) maxLvlRight = volRight[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
   if((maxLvlRight - minLvlRight) < TOP) maxLvlRight = minLvlRight + TOP;
  minLvlAvgRight = (minLvlAvgRight * 63 + minLvlRight) >> 6; // Dampen min/max levels
  maxLvlAvgRight = (maxLvlAvgRight * 63 + maxLvlRight) >> 6; // (fake rolling average)
}


void Vu4() {
  uint8_t  i;
  uint16_t minLvlLeft, maxLvlLeft;
  uint16_t minLvlRight, maxLvlRight;
  int      n, height;
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 2 ");
  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlLeft = ((lvlLeft * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlLeft - minLvlAvgLeft) / (long)(maxLvlAvgLeft - minLvlAvgLeft);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakLeft)     peakLeft   = height; // Keep 'peak' dot at top

  greenOffset += SPEED;
  blueOffset += SPEED;
  if (greenOffset >= 255) greenOffset = 0;
  if (blueOffset >= 255) blueOffset = 0;

  // Color pixels based on rainbow gradient
  for(i=0; i<N_PIXELS_HALF; i++) {
    if(i >= height) {
      strip.setPixelColor(N_PIXELS_HALF-i-1,   0,   0, 0);
      strip.setPixelColor(N_PIXELS_HALF+i,   0,   0, 0);
    }
    else {
      uint32_t color = Wheel(map(i,0,N_PIXELS_HALF-1,(int)greenOffset, (int)blueOffset));
      strip.setPixelColor(N_PIXELS_HALF-i-1,color);
      strip.setPixelColor(N_PIXELS_HALF+i,color);
    }

  }
// Draw peak dot
  if(peakLeft > 0 && peakLeft <= N_PIXELS_HALF-1) {
    uint32_t color = Wheel(map(peakLeft,0,N_PIXELS_HALF-1,30,150));
    strip.setPixelColor(N_PIXELS_HALF-peakLeft-1,color);
    strip.setPixelColor(N_PIXELS_HALF+peakLeft,color);
  }

   strip.show(); // Update strip

// Every few frames, make the peak pixel drop by 1:

    if(++dotCountLeft >= PEAK_FALL) { //fall rate

      if(peakLeft > 0) peakLeft--;
      dotCountLeft = 0;
    }


  volLeft[volCountLeft] = n;                      // Save sample for dynamic leveling
  if(++volCountLeft >= SAMPLES) volCountLeft = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvlLeft = maxLvlLeft = vol[0];
  for(i=1; i<SAMPLES; i++) {
    if(volLeft[i] < minLvlLeft)      minLvlLeft = volLeft[i];
    else if(volLeft[i] > maxLvlLeft) maxLvlLeft= volLeft[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
   if((maxLvlLeft - minLvlLeft) < TOP) maxLvlLeft = minLvlLeft + TOP;
  minLvlAvgLeft = (minLvlAvgLeft * 63 + minLvlLeft) >> 6; // Dampen min/max levels
  maxLvlAvgLeft = (maxLvlAvgLeft * 63 + maxLvlLeft) >> 6; // (fake rolling average)


   n   = analogRead(MIC_PIN_2);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlRight = ((lvlRight * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlRight - minLvlAvgRight) / (long)(maxLvlAvgRight - minLvlAvgRight);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakRight)     peakRight   = height; // Keep 'peak' dot at top

  greenOffset += SPEED;
  blueOffset += SPEED;
  if (greenOffset >= 255) greenOffset = 0;
  if (blueOffset >= 255) blueOffset = 0;

  // Color pixels based on rainbow gradient
  for(i=0; i<N_PIXELS_HALF; i++) {
    if(i >= height) {
      strip1.setPixelColor(N_PIXELS_HALF-i-1,   0,   0, 0);
      strip1.setPixelColor(N_PIXELS_HALF+i,   0,   0, 0);
    }
    else {
      uint32_t color = Wheel(map(i,0,N_PIXELS_HALF-1,(int)greenOffset, (int)blueOffset));
      strip1.setPixelColor(N_PIXELS_HALF-i-1,color);
      strip1.setPixelColor(N_PIXELS_HALF+i,color);
    }

  }

// Draw peak dot
  if(peakRight > 0 && peakRight <= N_PIXELS_HALF-1) {
    uint32_t color = Wheel(map(peakRight,0,N_PIXELS_HALF-1,30,150));
    strip1.setPixelColor(N_PIXELS_HALF-peakRight-1,color);
    strip1.setPixelColor(N_PIXELS_HALF+peakRight,color);
  }

   strip1.show(); // Update strip

/// Every few frames, make the peak pixel drop by 1:

    if(++dotCountRight >= PEAK_FALL) { //fall rate

      if(peakRight > 0) peakRight--;
      dotCountRight = 0;
      }
  volRight[volCountRight] = n;                      // Save sample for dynamic leveling
  if(++volCountRight >= SAMPLES2) volCountRight = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvlRight = maxLvlRight = vol[0];
  for(i=1; i<SAMPLES2; i++) {
    if(volRight[i] < minLvlRight)      minLvlRight = volRight[i];
    else if(volRight[i] > maxLvlRight) maxLvlRight = volRight[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvlRight - minLvlRight) < TOP) maxLvlRight = minLvlRight + TOP;
  minLvlAvgRight = (minLvlAvgRight * 63 + minLvlRight) >> 6; // Dampen min/max levels
  maxLvlAvgRight = (maxLvlAvgRight * 63 + maxLvlRight) >> 6; // (fake rolling average)

}

void vu5()
{
  uint8_t  i;
  uint16_t minLvlLeft, maxLvlLeft;
  uint16_t minLvlRight, maxLvlRight;
  int      n, height;
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 5 ");
  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlLeft = ((lvlLeft * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlLeft - minLvlAvgLeft) / (long)(maxLvlAvgLeft - minLvlAvgLeft);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakLeft)     peakLeft   = height; // Keep 'peak' dot at top


#ifdef CENTERED
 // Color pixels based on rainbow gradient
  for(i=0; i<(N_PIXELS/2); i++) {
    if(((N_PIXELS/2)+i) >= height)
    {
      strip.setPixelColor(((N_PIXELS/2) + i),   0,   0, 0);
      strip.setPixelColor(((N_PIXELS/2) - i),   0,   0, 0);
    }
    else
    {
      strip.setPixelColor(((N_PIXELS/2) + i),Wheel(map(((N_PIXELS/2) + i),0,strip.numPixels()-1,30,150)));
      strip.setPixelColor(((N_PIXELS/2) - i),Wheel(map(((N_PIXELS/2) - i),0,strip.numPixels()-1,30,150)));
    }
  }

  // Draw peak dot
  if(peakLeft > 0 && peakRight <= LAST_PIXEL_OFFSET)
  {
    strip.setPixelColor(((N_PIXELS/2) + peakLeft),255,255,255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
    strip.setPixelColor(((N_PIXELS/2) - peakLeft),255,255,255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
  }
#else
  // Color pixels based on rainbow gradient
  for(i=0; i<N_PIXELS; i++)
  {
    if(i >= height)
    {
      strip.setPixelColor(i,   0,   0, 0);
    }
    else
    {
      strip.setPixelColor(i,Wheel(map(i,0,strip.numPixels()-1,30,150)));
    }
  }

  // Draw peak dot
  if(peakLeft > 0 && peakLeft <= LAST_PIXEL_OFFSET)
  {
    strip.setPixelColor(peakLeft,255,255,255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
  }

#endif

  // Every few frames, make the peak pixel drop by 1:

  if (millis() - lastTime >= PEAK_FALL_MILLIS)
  {
    lastTime = millis();

    strip.show(); // Update strip

    //fall rate
    if(peakLeft > 0) peakLeft--;
    }

   volLeft[volCountLeft] = n;                      // Save sample for dynamic leveling
  if(++volCountLeft >= SAMPLES) volCountLeft = 0; // Advance/rollover sample counter

   // Get volume range of prior frames
  minLvlLeft = maxLvlLeft = vol[0];
  for(i=1; i<SAMPLES; i++) {
    if(volLeft[i] < minLvlLeft)      minLvlLeft = volLeft[i];
    else if(volLeft[i] > maxLvlLeft) maxLvlLeft= volLeft[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvlLeft - minLvlLeft) < TOP) maxLvlLeft = minLvlLeft + TOP;
  minLvlAvgLeft = (minLvlAvgLeft * 63 + minLvlLeft) >> 6; // Dampen min/max levels
  maxLvlAvgLeft = (maxLvlAvgLeft * 63 + maxLvlLeft) >> 6; // (fake rolling average)

n   = analogRead(MIC_PIN_2);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlRight = ((lvlRight * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlRight - minLvlAvgRight) / (long)(maxLvlAvgRight - minLvlAvgRight);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakRight)     peakRight   = height; // Keep 'peak' dot at top


#ifdef CENTERED
 // Color pixels based on rainbow gradient
  for(i=0; i<(N_PIXELS/2); i++) {
    if(((N_PIXELS/2)+i) >= height)
    {
      strip1.setPixelColor(((N_PIXELS/2) + i),   0,   0, 0);
      strip1.setPixelColor(((N_PIXELS/2) - i),   0,   0, 0);
    }
    else
    {
      strip1.setPixelColor(((N_PIXELS/2) + i),Wheel(map(((N_PIXELS/2) + i),0,strip1.numPixels()-1,30,150)));
      strip1.setPixelColor(((N_PIXELS/2) - i),Wheel(map(((N_PIXELS/2) - i),0,strip1.numPixels()-1,30,150)));
    }
  }

  // Draw peak dot
  if(peakRight > 0 && peakRight <= LAST_PIXEL_OFFSET)
  {
    strip1.setPixelColor(((N_PIXELS/2) + peakRight),255,255,255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
    strip1.setPixelColor(((N_PIXELS/2) - peakRight),255,255,255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
  }
#else
  // Color pixels based on rainbow gradient
  for(i=0; i<N_PIXELS; i++)
  {
    if(i >= height)
    {
      strip1.setPixelColor(i,   0,   0, 0);
    }
    else
    {
      strip1.setPixelColor(i,Wheel(map(i,0,strip1.numPixels()-1,30,150)));
    }
  }

  // Draw peak dot
  if(peakRight > 0 && peakRight <= LAST_PIXEL_OFFSET)
  {
    strip1.setPixelColor(peakRight,255,255,255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
  }

#endif

  // Every few frames, make the peak pixel drop by 1:

  if (millis() - lastTime >= PEAK_FALL_MILLIS)
  {
    lastTime = millis();

    strip1.show(); // Update strip

    //fall rate
    if(peakRight > 0) peakRight--;
    }

  volRight[volCountRight] = n;                      // Save sample for dynamic leveling
  if(++volCountRight >= SAMPLES2) volCountRight = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvlRight = maxLvlRight = vol[0];
  for(i=1; i<SAMPLES2; i++) {
    if(volRight[i] < minLvlRight)      minLvlRight = volRight[i];
    else if(volRight[i] > maxLvlRight) maxLvlRight = volRight[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
   if((maxLvlRight - minLvlRight) < TOP) maxLvlRight = minLvlRight + TOP;
  minLvlAvgRight = (minLvlAvgRight * 63 + minLvlRight) >> 6; // Dampen min/max levels
  maxLvlAvgRight = (maxLvlAvgRight * 63 + maxLvlRight) >> 6; // (fake rolling average)
}


void Vu6()
{
  uint8_t  i;
  uint16_t minLvlLeft, maxLvlLeft;
  uint16_t minLvlRight, maxLvlRight;
  int      n, height;
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 6 ");
  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlLeft = ((lvlLeft * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlLeft - minLvlAvgLeft) / (long)(maxLvlAvgLeft - minLvlAvgLeft);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakLeft)     peakLeft   = height; // Keep 'peak' dot at top


#ifdef CENTERED
  // Draw peak dot
  if(peakLeft > 0 && peakLeft <= LAST_PIXEL_OFFSET)
  {
    strip.setPixelColor(((N_PIXELS/2) + peakLeft),255,255,255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
    strip.setPixelColor(((N_PIXELS/2) - peakLeft),255,255,255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
  }
#else
  // Color pixels based on rainbow gradient
  for(i=0; i<N_PIXELS; i++)
  {
    if(i >= height)
    {
      strip.setPixelColor(i,   0,   0, 0);
    }
    else
    {
     }
  }

  // Draw peak dot
  if(peakLeft > 0 && peak <= LAST_PIXEL_OFFSET)
  {
    //strip.setPixelColor(peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
    strip.setPixelColor(peakLeft, 255,0,0); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
  }

#endif

  // Every few frames, make the peak pixel drop by 1:

  if (millis() - lastTime >= PEAK_FALL_MILLIS)
  {
    lastTime = millis();

    strip.show(); // Update strip

    //fall rate
    if(peakLeft > 0) peakLeft--;
    }

  volLeft[volCountLeft] = n;                      // Save sample for dynamic leveling
  if(++volCountLeft >= SAMPLES) volCountLeft = 0; // Advance/rollover sample counter

 // Get volume range of prior frames
  minLvlLeft = maxLvlLeft = vol[0];
  for(i=1; i<SAMPLES; i++) {
    if(volLeft[i] < minLvlLeft)      minLvlLeft = volLeft[i];
    else if(volLeft[i] > maxLvlLeft) maxLvlLeft= volLeft[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvlLeft - minLvlLeft) < TOP) maxLvlLeft = minLvlLeft + TOP;
  minLvlAvgLeft = (minLvlAvgLeft * 63 + minLvlLeft) >> 6; // Dampen min/max levels
  maxLvlAvgLeft = (maxLvlAvgLeft * 63 + maxLvlLeft) >> 6; // (fake rolling average)

  n   = analogRead(MIC_PIN_2);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlRight = ((lvlRight * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlRight - minLvlAvgRight) / (long)(maxLvlAvgRight - minLvlAvgRight);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakRight)     peakRight   = height; // Keep 'peak' dot at top


#ifdef CENTERED
  // Draw peak dot
  if(peakRight > 0 && peakRight <= LAST_PIXEL_OFFSET)
  {
    strip1.setPixelColor(((N_PIXELS/2) + peakRight),255,255,255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
    strip1.setPixelColor(((N_PIXELS/2) - peakRight),255,255,255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
  }
#else
  // Color pixels based on rainbow gradient
  for(i=0; i<N_PIXELS; i++)
  {
    if(i >= height)
    {
      strip1.setPixelColor(i,   0,   0, 0);
    }
    else
    {
     }
  }

  // Draw peak dot
  if(peakRight > 0 && peakRight <= LAST_PIXEL_OFFSET)
  {
    strip1.setPixelColor(peakRight,0,0,255); // (peak,Wheel(map(peak,0,strip.numPixels()-1,30,150)));
  }

#endif

  // Every few frames, make the peak pixel drop by 1:

  if (millis() - lastTime >= PEAK_FALL_MILLIS)
  {
    lastTime = millis();

    strip1.show(); // Update strip

    //fall rate
    if(peakRight > 0) peakRight--;
    }

   volRight[volCountRight] = n;                      // Save sample for dynamic leveling
  if(++volCountRight >= SAMPLES2) volCountRight = 0; // Advance/rollover sample counter


  // Get volume range of prior frames
  minLvlRight = maxLvlRight = vol[0];
  for(i=1; i<SAMPLES2; i++) {
    if(volRight[i] < minLvlRight)      minLvlRight = volRight[i];
    else if(volRight[i] > maxLvlRight) maxLvlRight = volRight[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvlRight - minLvlRight) < TOP) maxLvlRight = minLvlRight + TOP;
  minLvlAvgRight = (minLvlAvgRight * 63 + minLvlRight) >> 6; // Dampen min/max levels
  maxLvlAvgRight = (maxLvlAvgRight * 63 + maxLvlRight) >> 6; // (fake rolling average)
}

void vu7() {
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 7 ");
  EVERY_N_MILLISECONDS(1000) {
    peakspersec = peakcount;                                       // Count the peaks per second. This value will become the foreground hue.
    peakcount = 0;                                                 // Reset the counter every second.
  }

  soundmems();
  //soundmems_2();
  EVERY_N_MILLISECONDS(20) {
   ripple3();
  }

   show_at_max_brightness_for_power();

} // loop()

void soundmems() {                                                  // Rolling average counter - means we don't have to go through an array each time.
  newtime = millis();
  int tmp = analogRead(MIC_PIN) - 512;
  sample = abs(tmp);

  int potin = map(analogRead(POT_PIN), 0, 1023, 0, 60);

  samplesum = samplesum + sample - samplearray[samplecount];        // Add the new sample and remove the oldest sample in the array
  sampleavg = samplesum / NSAMPLES;                                 // Get an average
  samplearray[samplecount] = sample;                                // Update oldest sample in the array with new sample
  samplecount = (samplecount + 1) % NSAMPLES;                       // Update the counter for the array

  if (newtime > (oldtime + 200)) digitalWrite(13, LOW);             // Turn the LED off 200ms after the last peak.

  if ((sample > (sampleavg + potin)) && (newtime > (oldtime + 60)) ) { // Check for a peak, which is 30 > the average, but wait at least 60ms for another.
    step = -1;
    peakcount++;
    digitalWrite(13, HIGH);
    oldtime = newtime;
  }
}  // soundmems()


void ripple3() {
  for (int i = 0; i < N_PIXELS; i++) ledsLeft[i] = CHSV(bgcol, 255, sampleavg*2);  // Set the background colour.

  switch (step) {

    case -1:                                                          // Initialize ripple variables.
      center = random(N_PIXELS);
      colour = (peakspersec*10) % 255;                                             // More peaks/s = higher the hue colour.
      step = 0;
      bgcol = bgcol+8;
      break;

    case 0:
      ledsLeft[center] = CHSV(colour, 255, 255);                          // Display the first pixel of the ripple.
      step ++;
      break;

    case maxsteps:                                                    // At the end of the ripples.
      // step = -1;
      break;

    default:                                                             // Middle of the ripples.
      ledsLeft[(center + step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade/step*2);       // Simple wrap from Marc Miller.
      ledsLeft[(center - step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade/step*2);
      step ++;                                                         // Next step.
      break;

  }
   // Copy left LED array into right LED array
  for (uint8_t i = 0; i < N_PIXELS; i++) {
    ledsRight[i] = ledsLeft[i];

     //FastLED.show();
  } // switch step
} // ripple()


void vu8() {
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 8 ");
  int intensity = calculateIntensity();
  updateOrigin(intensity);
  assignDrawValues(intensity);
  writeSegmented();
  updateGlobals();
}

int calculateIntensity() {
  int      intensity;
  // int      n, height;
  reading   = analogRead(MIC_PIN);                        // Raw reading from mic
  reading   = abs(reading - 512 - DC_OFFSET); // Center on zero
  reading   = (reading <= NOISE) ? 0 : (reading - NOISE);             // Remove noise/hum
  lvlLeft = ((lvlLeft * 7) + reading) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  intensity = DRAW_MAX * (lvl - minLvlAvg) / (long)(maxLvlAvg - minLvlAvg);

  reading   = analogRead(MIC_PIN_2);                        // Raw reading from mic
  reading   = abs(reading - 512 - DC_OFFSET); // Center on zero
  reading   = (reading <= NOISE) ? 0 : (reading - NOISE);             // Remove noise/hum
  lvlRight = ((lvlRight * 7) + reading) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  intensity = DRAW_MAX * (lvlLeft - minLvlAvgLeft) / (long)(maxLvlAvgLeft - minLvlAvgLeft);

  // Calculate bar height based on dynamic min/max levels (fixed point):
  intensity = DRAW_MAX * (lvlRight - minLvlAvgRight) / (long)(maxLvlAvgRight - minLvlAvgRight);

  return constrain(intensity, 0, DRAW_MAX-1);
}

void updateOrigin(int intensity) {
  // detect peak change and save origin at curve vertex
  if (growing && intensity < last_intensity) {
    growing = false;
    intensity_max = last_intensity;
    fall_from_left = !fall_from_left;
    origin_at_flip = origin;
  } else if (intensity > last_intensity) {
    growing = true;
    origin_at_flip = origin;
  }
  last_intensity = intensity;

  // adjust origin if falling
  if (!growing) {
    if (fall_from_left) {
      origin = origin_at_flip + ((intensity_max - intensity) / 2);
    } else {
      origin = origin_at_flip - ((intensity_max - intensity) / 2);
    }
    // correct for origin out of bounds
    if (origin < 0) {
      origin = DRAW_MAX - abs(origin);
    } else if (origin > DRAW_MAX - 1) {
      origin = origin - DRAW_MAX - 1;
    }
  }
}

void assignDrawValues(int intensity) {
  // draw amplitue as 1/2 intensity both directions from origin
  int min_lit = origin - (intensity / 2);
  int max_lit = origin + (intensity / 2);
  if (min_lit < 0) {
    min_lit = min_lit + DRAW_MAX;
  }
  if (max_lit >= DRAW_MAX) {
    max_lit = max_lit - DRAW_MAX;
  }
  for (int i=0; i < DRAW_MAX; i++) {
    // if i is within origin +/- 1/2 intensity
    if (
      (min_lit < max_lit && min_lit < i && i < max_lit) // range is within bounds and i is within range
      || (min_lit > max_lit && (i > min_lit || i < max_lit)) // range wraps out of bounds and i is within that wrap
    ) {
      draw[i] = Wheel(scroll_color);
    } else {
      draw[i] = 0;
    }
  }
}

void writeSegmented() {
  int seg_len = N_PIXELS / SEGMENTS;

  for (int s = 0; s < SEGMENTS; s++) {
    for (int i = 0; i < seg_len; i++) {
      strip.setPixelColor(i + (s*seg_len), draw[map(i, 0, seg_len, 0, DRAW_MAX)]);
      strip1.setPixelColor(i + (s*seg_len), draw[map(i, 0, seg_len, 0, DRAW_MAX)]);
    }
  }
  strip.show();
  strip1.show();
}

uint32_t * segmentAndResize(uint32_t* draw) {
  int seg_len = N_PIXELS / SEGMENTS;

  uint32_t segmented[N_PIXELS];
  for (int s = 0; s < SEGMENTS; s++) {
    for (int i = 0; i < seg_len; i++) {
      segmented[i + (s * seg_len) ] = draw[map(i, 0, seg_len, 0, DRAW_MAX)];
    }
  }

  return segmented;
}

void writeToStrip(uint32_t* draw) {
  for (int i = 0; i < N_PIXELS; i++) {
    strip.setPixelColor(i, draw[i]);
    strip1.setPixelColor(i, draw[i]);
  }
  strip.show();
  strip1.show();
}

void updateGlobals() {
   uint16_t minLvlLeft, maxLvlLeft;
  uint16_t minLvlRight, maxLvlRight;

  //advance color wheel
  color_wait_count++;
  if (color_wait_count > COLOR_WAIT_CYCLES) {
    color_wait_count = 0;
    scroll_color++;
    if (scroll_color > COLOR_MAX) {
      scroll_color = COLOR_MIN;
    }
  }

  volLeft[volCountLeft] = reading;                      // Save sample for dynamic leveling
  if(++volCountLeft >= SAMPLES) volCountLeft = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvlLeft = maxLvlLeft = volLeft[0];
  for(uint8_t i=1; i<SAMPLES; i++) {

    if(volLeft[i] < minLvlLeft)      minLvlLeft = volLeft[i];
    else if(volLeft[i] > maxLvlLeft) maxLvlLeft = volLeft[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvlLeft - minLvlLeft) < N_PIXELS) maxLvlLeft = minLvlLeft + N_PIXELS;
  minLvlAvgLeft = (minLvlAvgLeft * 63 + minLvlLeft) >> 6; // Dampen min/max levels
  maxLvlAvgLeft = (maxLvlAvgLeft * 63 + maxLvlLeft) >> 6; // (fake rolling average)

  volLeft[volCountLeft] = reading;                      // Save sample for dynamic leveling
  if(++volCountLeft >= SAMPLES) volCountLeft = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvlRight = maxLvlRight = volRight[0];
  for(uint8_t i=1; i<SAMPLES2; i++) {
    if(volRight[i] < minLvlRight)      minLvlRight = volRight[i];
    else if(volRight[i] > maxLvlRight) maxLvlRight = volRight[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvlRight - minLvlRight) < N_PIXELS) maxLvlRight = minLvlRight + N_PIXELS;
  minLvlAvgRight = (minLvlAvgRight * 63 + minLvlRight) >> 6; // Dampen min/max levels
  maxLvlAvgRight = (maxLvlAvgRight * 63 + maxLvlRight) >> 6; // (fake rolling average)
}


void vu9() {
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 9 ");
  //currentBlending = LINEARBLEND;
  currentPalette = OceanColors_p;                             // Initial palette.
  currentBlending = LINEARBLEND;
  EVERY_N_SECONDS(5) {                                        // Change the palette every 5 seconds.
    for (int i = 0; i < 16; i++) {
      targetPalette[i] = CHSV(random8(), 255, 255);
    }
  }

  EVERY_N_MILLISECONDS(100) {                                 // AWESOME palette blending capability once they do change.
    uint8_t maxChanges = 24;
    nblendPaletteTowardPalette(currentPalette, targetPalette, maxChanges);
  }


  EVERY_N_MILLIS_I(thistimer,20) {                            // For fun, let's make the animation have a variable rate.
    uint8_t timeval = beatsin8(10,20,50);                     // Use a sinewave for the line below. Could also use peak/beat detection.
    thistimer.setPeriod(timeval);                             // Allows you to change how often this routine runs.
     fadeToBlackBy(ledsLeft, N_PIXELS, 16);                        // 1 = slow, 255 = fast fade. Depending on the faderate, the LED's further away will fade out.
    sndwave();
    soundble();
  }
   // Copy left LED array into right LED array
  for (uint8_t i = 0; i < N_PIXELS; i++) {
    ledsRight[i] = ledsLeft[i];
  }
  FastLED.setBrightness(max_bright);
  FastLED.show();

} // loop()



void soundble() {                                            // Quick and dirty sampling of the microphone.

  int tmp = analogRead(MIC_PIN) - 512 - DC_OFFSET;
  sample = abs(tmp);

}  // soundmems()



void sndwave() {

  ledsLeft[N_PIXELS/2] = ColorFromPalette(currentPalette, sample, sample*2, currentBlending); // Put the sample into the center
  ledsRight[N_PIXELS/2] = ColorFromPalette(currentPalette, sample, sample*2, currentBlending); // Put the sample into the center

  for (int i = N_PIXELS - 1; i > N_PIXELS/2; i--) {       //move to the left      // Copy to the left, and let the fade do the rest.
    ledsLeft[i] = ledsLeft[i - 1];
     ledsRight[i] = ledsRight[i - 1];
  }

  for (int i = 0; i < N_PIXELS/2; i++) {                  // move to the right    // Copy to the right, and let the fade to the rest.
    ledsLeft[i] = ledsLeft[i + 1];
    ledsRight[i] = ledsRight[i + 1];
  }
  addGlitter(sampleavg);
}

void vu10() {

uint8_t  i;
  uint16_t minLvlLeft, maxLvlLeft;
  uint16_t minLvlRight, maxLvlRight;
  int      n, height;
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 10");
  n   = analogRead(MIC_PIN);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlLeft = ((lvlLeft * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

   // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlLeft - minLvlAvgLeft) / (long)(maxLvlAvgLeft - minLvlAvgLeft);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakLeft)     peakLeft   = height; // Keep 'peak' dot at top


  // Color pixels based on rainbow gradient
  for(i=0; i<N_PIXELS_HALF; i++) {
    if(i >= height) {
      strip.setPixelColor(N_PIXELS_HALF-i-1,   0,   0, 0);
      strip.setPixelColor(N_PIXELS_HALF+i,   0,   0, 0);
    }
    else {
      strip.setPixelColor(N_PIXELS_HALF-i-1,255, 0, 0);
      strip.setPixelColor(N_PIXELS_HALF+i,255, 0, 0);
   }

  }

// Draw peak dot
  if(peakLeft > 0 && peakLeft <= N_PIXELS_HALF-1) {
    uint32_t color = Wheel(map(peak,0,N_PIXELS_HALF-1,30,150));
    strip.setPixelColor(N_PIXELS_HALF-peakLeft-1,0,0, 255);
    strip.setPixelColor(N_PIXELS_HALF+peakLeft,0,0, 255);
  }

   strip.show(); // Update strip

// Every few frames, make the peak pixel drop by 1:

    if(++dotCountLeft >= PEAK_FALL) { //fall rate

      if(peakLeft > 0) peakLeft--;
      dotCountLeft = 0;
    }


 volLeft[volCountLeft] = n;                      // Save sample for dynamic leveling
  if(++volCountLeft >= SAMPLES) volCountLeft = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvlLeft = maxLvlLeft = vol[0];
  for(i=1; i<SAMPLES; i++) {
    if(volLeft[i] < minLvlLeft)      minLvlLeft = volLeft[i];
    else if(volLeft[i] > maxLvlLeft) maxLvlLeft= volLeft[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
   if((maxLvlLeft - minLvlLeft) < TOP) maxLvlLeft = minLvlLeft + TOP;
  minLvlAvgLeft = (minLvlAvgLeft * 63 + minLvlLeft) >> 6; // Dampen min/max levels
  maxLvlAvgLeft = (maxLvlAvgLeft * 63 + maxLvlLeft) >> 6; // (fake rolling average)


  n   = analogRead(MIC_PIN_2);                        // Raw reading from mic
  n   = abs(n - 512 - DC_OFFSET); // Center on zero
  n   = (n <= NOISE) ? 0 : (n - NOISE);             // Remove noise/hum
  lvlRight = ((lvlRight * 7) + n) >> 3;    // "Dampened" reading (else looks twitchy)

  // Calculate bar height based on dynamic min/max levels (fixed point):
  height = TOP * (lvlRight - minLvlAvgRight) / (long)(maxLvlAvgRight - minLvlAvgRight);

  if(height < 0L)       height = 0;      // Clip output
  else if(height > TOP) height = TOP;
  if(height > peakRight)     peakRight   = height; // Keep 'peak' dot at top


  // Color pixels based on rainbow gradient
  for(i=0; i<N_PIXELS_HALF; i++) {
    if(i >= height) {
      strip1.setPixelColor(N_PIXELS_HALF-i-1,   0,   0, 0);
      strip1.setPixelColor(N_PIXELS_HALF+i,   0,   0, 0);
    }
    else {
      strip1.setPixelColor(N_PIXELS_HALF-i-1,255,0, 0);
      strip1.setPixelColor(N_PIXELS_HALF+i,255,0, 0);
    }

  }

// Draw peak dot
  if(peakRight > 0 && peakRight <= N_PIXELS_HALF-1) {

    strip1.setPixelColor(N_PIXELS_HALF-peakRight-1,0,0, 255);
    strip1.setPixelColor(N_PIXELS_HALF+peakRight,0,0, 255);


  }

   strip1.show(); // Update strip

// Every few frames, make the peak pixel drop by 1:

    if(++dotCountRight >= PEAK_FALL) { //fall rate

      if(peakRight > 0) peakRight--;
      dotCountRight = 0;

      }
  volRight[volCountRight] = n;                      // Save sample for dynamic leveling
  if(++volCountRight >= SAMPLES2) volCountRight = 0; // Advance/rollover sample counter

  // Get volume range of prior frames
  minLvlRight = maxLvlRight = vol[0];
  for(i=1; i<SAMPLES2; i++) {
    if(volRight[i] < minLvlRight)      minLvlRight = volRight[i];
    else if(volRight[i] > maxLvlRight) maxLvlRight = volRight[i];
  }
  // minLvl and maxLvl indicate the volume range over prior frames, used
  // for vertically scaling the output graph (so it looks interesting
  // regardless of volume level).  If they're too close together though
  // (e.g. at very low volume levels) the graph becomes super coarse
  // and 'jumpy'...so keep some minimum distance between them (this
  // also lets the graph go to zero when no sound is playing):
  if((maxLvlRight - minLvlRight) < TOP) maxLvlRight = minLvlRight + TOP;
  minLvlAvgRight = (minLvlAvgRight * 63 + minLvlRight) >> 6; // Dampen min/max levels
  maxLvlAvgRight = (maxLvlAvgRight * 63 + maxLvlRight) >> 6; // (fake rolling average)

}


void vu11() {
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 11");
  EVERY_N_MILLISECONDS(1000) {
    peakspersec = peakcount;                                  // Count the peaks per second. This value will become the foreground hue.
    peakcount = 0;                                            // Reset the counter every second.
  }

  soundrip();

  EVERY_N_MILLISECONDS(20) {
   rippled();
  }

   FastLED.show();

} // loop()


void soundrip() {                                            // Rolling average counter - means we don't have to go through an array each time.

  newtime = millis();
  int tmp = analogRead(MIC_PIN) - 512;
  sample = abs(tmp);

  int potin = map(analogRead(POT_PIN), 0, 1023, 0, 60);

  samplesum = samplesum + sample - samplearray[samplecount];  // Add the new sample and remove the oldest sample in the array
  sampleavg = samplesum / NSAMPLES;                           // Get an average

#ifdef DEBUG
  Serial.println(sampleavg);
#endif

  samplearray[samplecount] = sample;                          // Update oldest sample in the array with new sample
  samplecount = (samplecount + 1) % NSAMPLES;                 // Update the counter for the array

  if (newtime > (oldtime + 200)) digitalWrite(13, LOW);       // Turn the LED off 200ms after the last peak.

  if ((sample > (sampleavg + potin)) && (newtime > (oldtime + 60)) ) { // Check for a peak, which is 30 > the average, but wait at least 60ms for another.
    step = -1;
    peakcount++;
    oldtime = newtime;
  }

}  // soundmems()



void rippled() {

  fadeToBlackBy(ledsLeft, N_PIXELS, 64);                          // 8 bit, 1 = slow, 255 = fast

  switch (step) {

    case -1:                                                  // Initialize ripple variables.
      center = random(N_PIXELS);
      colour = (peakspersec*10) % 255;                        // More peaks/s = higher the hue colour.
      step = 0;
      break;

    case 0:
      ledsLeft[center] = CHSV(colour, 255, 255);                  // Display the first pixel of the ripple.
      step ++;
      break;

    case maxsteps:                                            // At the end of the ripples.
      // step = -1;
      break;

    default:                                                  // Middle of the ripples.
      ledsLeft[(center + step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade/step*2);       // Simple wrap from Marc Miller.
      ledsLeft[(center - step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade/step*2);
      step ++;                                                // Next step.
      break;


  }
    // Copy left LED array into right LED array
  for (uint8_t i = 0; i < N_PIXELS; i++) {
    ledsRight[i] = ledsLeft[i];

  } // switch step

} // ripple()


 //Used to draw a line between two points of a given color
    void drawLine(uint8_t from, uint8_t to, uint32_t c) {
      uint8_t fromTemp;
      if (from > to) {
        fromTemp = from;
        from = to;
        to = fromTemp;
      }
      for(int i=from; i<=to; i++){
        strip.setPixelColor(i, c);
      }
    }

void setPixel(int Pixel, byte red, byte green, byte blue) {
    strip.setPixelColor(Pixel, strip.Color(red, green, blue));

}


void setAll(byte red, byte green, byte blue) {

  for(int i = 0; i < N_PIXELS; i++ ) {

    setPixel(i, red, green, blue);

  }

  strip.show();

}
void vu12() {
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 12");
  EVERY_N_MILLISECONDS(1000) {
    peakspersec = peakcount;                                  // Count the peaks per second. This value will become the foreground hue.
    peakcount = 0;                                            // Reset the counter every second.
  }

  soundripped();

  EVERY_N_MILLISECONDS(20) {
  rippvu();
  }

   FastLED.show();

} // loop()


void soundripped() {                                            // Rolling average counter - means we don't have to go through an array each time.

  newtime = millis();
  int tmp = analogRead(MIC_PIN) - 512;
  sample = abs(tmp);

  int potin = map(analogRead(POT_PIN), 0, 1023, 0, 60);

  samplesum = samplesum + sample - samplearray[samplecount];  // Add the new sample and remove the oldest sample in the array
  sampleavg = samplesum / NSAMPLES;                           // Get an average
#ifdef DEBUG
  Serial.println(sampleavg);
#endif

  samplearray[samplecount] = sample;                          // Update oldest sample in the array with new sample
  samplecount = (samplecount + 1) % NSAMPLES;                 // Update the counter for the array

  if (newtime > (oldtime + 200)) digitalWrite(13, LOW);       // Turn the LED off 200ms after the last peak.

  if ((sample > (sampleavg + potin)) && (newtime > (oldtime + 60)) ) { // Check for a peak, which is 30 > the average, but wait at least 60ms for another.
    step = -1;
    peakcount++;
    oldtime = newtime;
  }

}  // soundmems()
void rippvu() {                                                                 // Display ripples triggered by peaks.

fadeToBlackBy(ledsLeft, N_PIXELS, 64);                          // 8 bit, 1 = slow, 255 = fast
fadeToBlackBy(ledsRight, N_PIXELS, 64);                          // 8 bit, 1 = slow, 255 = fast

  switch (step) {

    case -1:                                                  // Initialize ripple variables.
      center = random(N_PIXELS);
      colour = (peakspersec*10) % 255;                        // More peaks/s = higher the hue colour.
      step = 0;
      break;

    case 0:
      ledsLeft[center] = CHSV(colour, 255, 255);                  // Display the first pixel of the ripple.
      //ledsRight[center] = CHSV(colour, 255, 255);
      step ++;
      break;

    case maxsteps:                                            // At the end of the ripples.
      // step = -1;
      break;

    default:                                                  // Middle of the ripples.
      ledsLeft[(center + step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade/step*2);       // Simple wrap from Marc Miller.
      //ledsRight[(center - step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade/step*2);
      step ++;                                                // Next step.
      break;
  }
  // Copy left LED array into right LED array
  for (uint8_t i = 0; i < N_PIXELS; i++) {
    ledsRight[i] = ledsLeft[i];
  } // switch step
   addGlitter(sampleavg);
} // ripple()



void vu14() {
//  lcd.setCursor(0,0);
//  lcd.print("VU-Meter 14");
  EVERY_N_MILLISECONDS(1000) {
    peakspersec = peakcount;                                       // Count the peaks per second. This value will become the foreground hue.
    peakcount = 0;                                                 // Reset the counter every second.
  }

  soundmems2();
  //soundmems_2();
  EVERY_N_MILLISECONDS(20) {
   ripple4();
  }

   show_at_max_brightness_for_power();

} // loop()

void soundmems2() {                                                  // Rolling average counter - means we don't have to go through an array each time.
  newtime = millis();
  int tmp = analogRead(MIC_PIN) - 512;
  sample = abs(tmp);

  int potin = map(analogRead(POT_PIN), 0, 1023, 0, 60);

  samplesum = samplesum + sample - samplearray[samplecount];        // Add the new sample and remove the oldest sample in the array
  sampleavg = samplesum / NSAMPLES;                                 // Get an average
  samplearray[samplecount] = sample;                                // Update oldest sample in the array with new sample
  samplecount = (samplecount + 1) % NSAMPLES;                       // Update the counter for the array

  if (newtime > (oldtime + 200)) digitalWrite(13, LOW);             // Turn the LED off 200ms after the last peak.

  if ((sample > (sampleavg + potin)) && (newtime > (oldtime + 60)) ) { // Check for a peak, which is 30 > the average, but wait at least 60ms for another.
    step = -1;
    peakcount++;
    digitalWrite(13, HIGH);
    oldtime = newtime;
  }
}  // soundmems()


void ripple4() {
  for (int i = 0; i < N_PIXELS; i++) ledsLeft[i] = CHSV(bgcol, 255, sampleavg*2);  // Set the background colour.
  for (int i = 0; i < N_PIXELS; i++) ledsRight[i] = CHSV(bgcol, 255, sampleavg*2);  // Set the background colour.

  switch (step) {

    case -1:                                                          // Initialize ripple variables.
      center = random(N_PIXELS);
      colour = (peakspersec*10) % 255;                                             // More peaks/s = higher the hue colour.
      step = 0;
      bgcol = bgcol+8;
      break;

    case 0:
      ledsLeft[center] = CHSV(colour, 255, 255);                          // Display the first pixel of the ripple.
      ledsRight[center] = CHSV(colour, 255, 255);
      step ++;
      break;

    case maxsteps:                                                    // At the end of the ripples.
      // step = -1;
      break;

    default:                                                             // Middle of the ripples.
      ledsLeft[(center + step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade/step*2);       // Simple wrap from Marc Miller.
      ledsRight[(center - step + N_PIXELS) % N_PIXELS] += CHSV(colour, 255, myfade/step*2);
      step ++;                                                         // Next step.
      break;


  } // switch step
} // ripple()


// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos, float opacity) {

  if(WheelPos < 85) {
    return strip.Color((WheelPos * 3) * opacity, (255 - WheelPos * 3) * opacity, 0);
  }
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color((255 - WheelPos * 3) * opacity, 0, (WheelPos * 3) * opacity);
  }
  else {
    WheelPos -= 170;
    return strip.Color(0, (WheelPos * 3) * opacity, (255 - WheelPos * 3) * opacity);
  }
}


void addGlitter(fract8 chanceOfGlitter) {                                      // Let's add some glitter, thanks to Mark

  if( random8() < chanceOfGlitter) {
    ledsLeft[random16(N_PIXELS)] += CRGB::White;
    ledsRight[random16(N_PIXELS)] += CRGB::White;
  }

} // addGlitter()

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {fire, fireblu, juggle, ripple, sinelon, twinkle, balls};
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current


void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}
void All() {
  String Muster;
  // Rufen Sie die aktuelle Musterfunktion einmal auf und aktualisieren Sie das 'leds'-Array
  gPatterns[gCurrentPatternNumber]();
  EVERY_N_SECONDS( 30 ) { nextPattern(); } // change patterns periodically
#ifdef DEBUG
  Serial.print("Prog.: ");
  Serial.println(Muster[gCurrentPatternNumber]);
#endif
}
// zweite Liste
// Liste der Muster, die durchlaufen werden sollen. Jede wird im Folgenden als separate Funktion definiert.
typedef void (*SimplePatternList[])();
SimplePatternList qPatterns = {vu, vu1,vu2 ,vu3, Vu4, vu5,  Vu6, vu7, vu8, vu9, vu10, vu11, vu12, vu14};
// myArray[] = {vu, vu1,vu2 ,vu3, Vu4, vu5,  Vu6, vu7, vu8, vu9, vu10, vu11, vu12, vu14};
uint8_t qCurrentPatternNumber = 0; // Index number of which pattern is current

void nextPattern2()
{
  // add one to the current pattern number, and wrap around at the end
  qCurrentPatternNumber = (qCurrentPatternNumber + 1) % ARRAY_SIZE( qPatterns);
}
void All2()   {
  uint8_t U_Nr = 0;
  char Old_U = 0;
  String text;
  char Str1[1];                              // Rufen Sie die aktuelle Musterfunktion einmal auf und aktualisieren Sie das 'leds'-Array
  qPatterns[qCurrentPatternNumber]();
  EVERY_N_SECONDS( 60 ) { nextPattern2(); }        // Muster regelmäßig ändern
  U_Nr = qCurrentPatternNumber;

  do {
#ifdef DEBUG
    Serial.print("In der Schleife - ");
#endif
    text = (myArray[U_Nr]);
#ifdef DEBUG
    Serial.println(text);
#endif
    lcd.setCursor(14,1);
#ifdef DEBUG
    Serial.print("Pro-Nr.:  ");
#endif
    if(U_Nr <=9) {
      lcd.print(" ");
      lcd.print(U_Nr);
    }
    else  lcd.print(U_Nr);
#ifdef DEBUG
    Serial.println(U_Nr);
    Serial.println(qCurrentPatternNumber);
#endif
  }
  while ( Old_U == U_Nr +1);
  Old_U = U_Nr+1;
}

//void startupAnimation(int Typ) {
//  for (int j = 0; j < 2; j++) {                                    // äussere Schleife zum zählen
//    for (int i = 0; i <= numOfSegments; i++) {                     // innere Schleife
//      if (animType == 1)
//        strip.setPixelColor(stripMiddle - (numOfSegments - i), stripColor[i]);
//      else
//        strip.setPixelColor(stripMiddle - i, stripColor[i]);
//
//      if (stripsOn2Pins)
//        strip1.setPixelColor(i, stripColor[i]);
//      else
//        strip.setPixelColor(stripMiddle + i, stripColor[i]);
//
//      strip.show();
//      if (stripsOn2Pins)
//        strip1.show();
//
//      delay(startupAnimationDelay);
//    }
//
//    for (int i = 0; i <= numOfSegments; i++) {
//      if (animType == 1)
//        strip.setPixelColor(stripMiddle - (numOfSegments - i), 0);
//      else
//        strip.setPixelColor(stripMiddle - i, 0);
//
//      if (stripsOn2Pins)
//        strip1.setPixelColor(i, 0);
//      else
//        strip.setPixelColor(stripMiddle + i, 0);
//
//      strip.show();
//      if (stripsOn2Pins)
//        strip1.show();
//
//      delay(startupAnimationDelay);
//    }
//  }
//}
