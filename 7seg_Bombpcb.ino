/*  Test zur Multiplex-Ansteuerung mehrerer 7-Segmentanzeigen.

     Segmente a-f + dot auf digitalPins angeschlossen.
     Massen werden vom µC durch LOW-pegel geschalten

    LP 2019

   Einfache Stoppuhr

    Jegliche Teile des Codes wurden selbst geschrieben.


*/

#define BLINKER 1    //Interne Led

//**Pinbelegung der Segmente**//

int pinConfig[]{    //enthält die Belegung der Anschlüsse, kann also von a-g auf alle Pins gepatcht werden

4,  //a
6,  //b
7,  //c
9,  //d
8,  //e
5,  //f
2,  //g 
3   //°
  
};

//**Pinbelegung der Massen der Anzeigen**//

int _Agnd = 0;
int _Bgnd = 10;
int _Cgnd = 11;
int _Dgnd = 12;

//**Einstellungen**//

const int  numOfPanels = 4;   //Zahl der Anzeigesegmente -> Später 10.
const int  startPin = 0;      //Erster Pin der Reihe am µC
const int  displayTime = 5;   //Anzeige der Zahl in millisekunden. Default 5 

//**Systemeinstellungen**//

volatile int timer = 0;   //counter für mux in der ISR
volatile int timeTick = 0;    //counter für Zeit
int endTick = 500;    //default für blinken der LEDS

//**Array für Position der Zahlen auf dem Display a-g**//

byte digitNumbers[] {
  0b10000001,  //"0"
  0b11110011,  //"1"
  0b01001001,  //"2"
  0b01100001,  //"3"
  0b00110011,  //"4"
  0b00100101,  //"5"
  0b00000101,  //"6"
  0b11110001,  //"7"
  0b00000001,  //"8"
  0b00100001,  //"9"
  0b00000000,   //alle Digits an 0x0A
  0b11111111,   //alle Digits aus 0x0B
  
};




bool displayNumbers(int pos1, int pos2, int pos3, int pos4 ) {    // Funktion zur Anzeige auf den Displays. Nimmt alle digits einzeln als Argumente

 static int y = 0;    // Aktuelles Digit

  if (timer >= displayTime) {   // Wenn der Timer-Interrupt-Zähler über einem treshold ist, wird die Anzeige aktualisiert. 
    
    int val;    //Enthält den Wert der im aktuellen Durchlauf anzuzeigenden Zahl

    if (y == 0) val = pos1;   // Ordnet val je nach Durchlauf einem Digit zu
    if (y == 1) val = pos2;
    if (y == 2) val = pos3;
    if (y == 3) val = pos4;

    digitalWrite(_Agnd, y == 0);    // Schaltet die Massen für das aktuelle Digit auf LOW
    digitalWrite(_Bgnd, y == 1);    // Schaltet später die Transistoren
    digitalWrite(_Cgnd, y == 2);
    digitalWrite(_Dgnd, y == 3);

    for (int i = 0; i <= 6; i++) {
      digitalWrite(pinConfig[i], digitNumbers[val] & 1 << i + 1 ); //Schreibt einen HIGH-Pegel, wenn das Zahlenbild der Abfragematrix entspricht
    }

     y++;   //Erhöhe Zähler für aktuelles Digit

    if(y >= numOfPanels ){    //Setze zurückt wenn alle Digits nacheinander angezeigt wurden
      y = 0;
      }
      
    timer = 0;    // Setze timer für Zeitsteuerung auf 0

  }
}

ISR(TIMER0_COMPA_vect) {   // Bei Überlaufen des CTC wird diese Funktion aufgerufen (Jede ms um 1 erhöht), Timer/Counter0 compare match A vector
  
  timer++;    // Erhöhe timer um 1
  timeTick++;  //Zähler für Stoppuhr
  
}


void setup() {

  for (int i = startPin; i <= 8 + numOfPanels; i++) {   //Deklaration der Outputs für 8 Pins + Massen der Segmente
    pinMode(i, OUTPUT);   // Ausgang als Output hernehmen
  }

  pinMode(BLINKER, OUTPUT);   // Blinke led soll auch definiert werden

  //Serial.begin(9600);   //Serielle Kommunikation

  TCCR0A = (1 << WGM01);    // Modus auf CTC (Clear Timer on Compare)
  OCR0A = 0xF9;             //ORC0A auf 1ms -> Wenn TCNT0 == OCR0A -> Löse interrupt aus. Default: 0xF9 

  TIMSK0 |= (1 << OCIE0A);  // Setze Flag für Interrupt 
  sei();                    // Interrupts an

  TCCR0B |= (1 << CS01);    //Prescaleregister auf 1/64 der Clock setzen
  TCCR0B |= (1 << CS00);    //Bit 2 dito

  while(timeTick < 1000){   //kleine Sequenz zum testen der Digits

    displayNumbers(0x0A, 0x0A, 0x0A, 0x0A);
  }

  timeTick = 0;

   while(timeTick < 1000){

    displayNumbers(0x0B, 0x0B, 0x0B, 0x0B);
  }

  timeTick = 0;

}   //Sequenz zuende

void loop() {   //Hauptcode, jetzt nur ein Testprogramm zum testen 

static int seconds = 00; //aktuelle Sekunden
static int minutes = 3; //aktuelle Minuten

int val1, val2, val3, val4; //Anzuzeigende Werte

(timeTick/endTick) % 2  ?  digitalWrite(BLINKER, HIGH) : digitalWrite(BLINKER, LOW) ; //Blinke alle 10ms

if(timeTick >= 1000){   //Wenn der Timer eine Sekunde erreicht, zähle Seconds runter
  seconds--;
  timeTick = 0;   //und resette dann
}

if(seconds == -1){    //Wenn seconds zuende, ziehe minute ab und starte nächste seconds
  minutes--;
  seconds = 59;
}

if( seconds < 10){    //solange seconds kleiner 10 zeige im Format "00" an
val1 = seconds;
val2 = 0;
}

if( seconds >= 10){   //ansten teile in dezimalstellen
val2 = seconds / 10;
val1 = seconds % 10;
}

if( minutes < 10){    //dito minuten
val3 = minutes;
val4 = 0;
}

if( minutes >= 10){   //
val4 = minutes / 10;
val3 = minutes % 10;
}

if(minutes <= 1 && seconds == 0){   //wenn letzte Minute anbricht, lass die leds schneller blinken
endTick = 250;
}

if(minutes <= 0 && seconds == 10){  //dito letzten 10 sec 
endTick = 125;
}

if(minutes == 0 && seconds == 0){   //Wenn abgelaufen tue gar nix mehr 

digitalWrite(BLINKER, HIGH);

while(1){
  
  displayNumbers(0,0,0,0);
  
}
  
}
 
  displayNumbers(val4, val3, val2, val1);   //Zeige errechnete Zeit an

}
