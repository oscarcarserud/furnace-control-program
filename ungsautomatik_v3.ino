
//EFTERSOM ÅÄÖ inte riktigt går att använda i variabelnamn, så försöker jag hålla alla variabler på engelska
//OBSERVERA SSR reläer är tvärt om
#define  RELAY_ON 0
#define  RELAY_OFF 1
//Relästyrning från digital pin 2
#define RELAY_1 2
#define keyboard   analogRead(6)
#define keyboard_reference 240
#define thermocouple_reference 1
#define senaste_program 2
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_MAX31856.h>//thermocouple library

Adafruit_MAX31856 max = Adafruit_MAX31856(10);//sätter konstrukt

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
#include <EEPROM.h>

//volatile är för globala variabler som kan ändras av en interupt
volatile boolean toggle = false;
volatile boolean burning = false; //har bränningsprogrammet startat?
volatile bool ask_effect = false; //ber vänligen om att effekten skall räknas ut av main loop
volatile int seconds_of_effect_loop = 0; //räknar upp sekunderna till en minut
volatile int effect = 0; //0-60, antal sekunder som är på

volatile unsigned int time_point[10];
volatile unsigned int temp_point[10];
volatile float ought_temperatur = 0.00000;
volatile float is_temperatur = 0.00000;//skall kunna läsas av interrupten så den kan stänga av ugnen om vi kommer för högt upp i temperatur

int Serial_i = 0;
char Serial_buf[4];



volatile int segment = 0; //0-9 vilket segment i time och temp point vi är på
int segment_run_time = 0; // Hur många minuter vi kört på aktivt segment
int last_temperatur = 20; //vi utgår i från att ugnen är 20 grader när vi börjar

byte a0[8] = {//å
  B00100, B00000, B01110, B00001, B01111, B10001, B01111, B00000
};

byte a1[8] = {//Å
  B00100,
  B00000,
  B01110,
  B10001,
  B11111,
  B10001,
  B10001,
  B00000
};


byte a2[8] = {//Ä
  B01010,
  B00000,
  B01110,
  B10001,
  B11111,
  B10001,
  B10001,
  B00000
};


byte a3[8] = {//Ö
  B01010,
  B00000,
  B01110,
  B10001,
  B10001,
  B10001,
  B01110,
  B00000
};

int key_ref[6];//håller info om analoga tangent värden
int thermo_ref;//vilken typ av thermocouple har vi valt

void  program_from_eprom(int program) {
  EEPROM.update(senaste_program, program);
  int r = 0;
  //om första adressen inte är 200, så är eeprom inte initsialiserat
  if (EEPROM.read(0) != 200) {
    Serial.println("inte initsialiserat");
    for (r = 1; r < 512; r++) {
      EEPROM.update(r, 0);
    }
    EEPROM.update(0, 200); //minne rensat
    EEPROM.update(thermocouple_reference, 3); //default till termopcouple typ "k" b,e,j,"K",n,r,s,t
  }

  //de första 10 adresserna är reserverade
  int offset = 10 + (program * (4 * 10)); // tio programsteg med 2 integers i varje steg
  for (r = 0; r < 10; r++) {
    time_point[r] = (EEPROM.read(offset + (r * 4)) * 255) + EEPROM.read(offset + (r * 4) + 1);
    temp_point[r] = (EEPROM.read(offset + (r * 4) + 2) * 255) + EEPROM.read(offset + (r * 4) + 3);
  }
}

void  program_to_eprom(int program) {
  int r = 0;
  //de första 10 adresserna är reserverade
  int offset = 10 + (program * (4 * 10)); // tio programsteg med 2 integers i varje steg
  for (r = 0; r < 10; r++) {
    EEPROM.update(offset + (r * 4), highByte( time_point[r]));
    EEPROM.update(offset + (r * 4) + 1, lowByte( time_point[r]));
    EEPROM.update(offset + (r * 4) + 2, highByte( temp_point[r]));
    EEPROM.update(offset + (r * 4) + 3, lowByte( temp_point[r]));

  }
}

void setup() {
  burning = false;
  lcd.begin(20, 4);
  lcd.home ();                   // go home
  lcd.print("UGNSAUTOMATIK");
  lcd.setCursor ( 0, 1 );        // go to the next line
  lcd.print ("v3.0");

  Serial.begin(9600); //ta bort sedan
  //minne i eeprom
  if (E2END < 1023) { //för lite eeprom
    lcd.setCursor ( 0, 2 );        // go to the next line
    lcd.print ("För lite eeprom minne!");
    while (true) {
      pinMode(LED_BUILTIN, OUTPUT);
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      delay(200);                       // wait for a second
      digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
      delay(200);
    }
  }

  pinMode(RELAY_1, OUTPUT);

  /////////////////////////////////////////////////////////////////
  //skapar en interrupt som tickar 2 ggr i sekunden
  cli();//stop interrupts
  //set timer1 interrupt at 0.5Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = 7812;//15624 = (16*10^6) / (1*1024) - 1 (must be <65536)7812
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS12 and CS10 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);
  // enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  sei();//allow interrupts
  /////////////////////////////////////////////////////////////////
  lcd.createChar(0, a0);
  lcd.createChar(1, a1);
  lcd.createChar(2, a2);
  lcd.createChar(3, a3); //skapar åÅÄÖ
  Serial.print("startar");
  // om en knapp trycks in, så kommer vi direkt till inställningsmenyn
  //läs in analoga tangent refferenser
  for (int r = 0; r < 6; r++) {
    key_ref[r] = (EEPROM.read(keyboard_reference + (r * 2)) * 255) + EEPROM.read(keyboard_reference + (r * 2) + 1);
  }
  thermo_ref = EEPROM.read(thermocouple_reference);
  max.begin();//startar termocouple adc
}


int lcd_rad = 0;
int lcd_plats = 0;
void skriv(String text) {
  lcd.setCursor ( lcd_plats, lcd_rad);
  bool utf8 = false;
  unsigned char bokstav;
  for (int r = 0; r < (text.length()); r++) {
    bokstav = text.charAt(r);
    if (bokstav == 195) {
      //början på en utf8 sekvens
      r++;
      //bokstav = text.charAt(r);
      bokstav = text.charAt(r) ;
      if (bokstav == 165) {
        lcd.write("\01");
      }
      else if (bokstav == 164) {
        lcd.write(225);
      }
      else if (bokstav == 182) {
        lcd.write(239);
      }
      else if (bokstav == 133) {
        lcd.write(1);
      }
      else if (bokstav == 132) {
        lcd.write(2);
      }
      else if (bokstav == 150) {
        lcd.write(3);
      }
    } else {
      lcd.write(bokstav);
    }
  }
  lcd_rad = min(lcd_rad++, 3);
}


/////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
void loop() {
  int r = 0;
  int key = 0;
  if (keyboard > 0) {
    //man håller ner tangentbordet när datorn startas
    lcd.clear();
    skriv("Släpp knapparna..");
    do {} while (keyboard > 0);
    for (int r = 0; r < 6; r++) {
      lcd_rad = 0;
      lcd.clear();
      skriv("Tryck knapp " + String(r + 1));

      key_ref[r] = wait_key();
      Serial.println(String(key_ref[r]));
      EEPROM.update(keyboard_reference + (r * 2), highByte(key_ref[r]));
      EEPROM.update(keyboard_reference + (r * 2) + 1, lowByte(key_ref[r]));
    }
    lcd.clear();
  }
  if (burning) {
    //räknar ut vilken temperatur som det borde vara
    //gör bara en gång i minuten
    if (ask_effect) {
      //räknar ut effekten
      //ought_temperatur
      //hämtar vilken temperatur vi har
    }
  } else {

    //här ligger användargränssnitet för att lägga in nya program
    do {
      lcd.clear();                   // go home
      lcd_rad = 0;
      lcd_plats = 0;

      display_charachter(6);
      lcd.write(4);

      do{}while(1);
      skriv("1. Starta bränning");
      skriv("2. Ändra program");
      skriv("3. Inställningar");
      skriv("1  2 3");
      //skriv("1  2 3  4       5  6");
      do {
        key = key_in();
        Serial.println(String(key));
        if (key == 1) {
          starta_branning();
          break;
        }
        if (key == 2) {
          change_program();
          break;
        }
        if (key == 3) {
          installningar();
          break;
        }

      } while (true);
    } while (true);
    // burning = true;
    /*
      Serial.println("Startar bränning-----------");

      program_from_eprom(1);

      effect = 30;
      segment_run_time = 0;
      segment = 0;
      burning = true;
    */
  }

}

void starta_branning() {
  int key;
  int program = EEPROM.read(senaste_program);
  lcd.clear();
  lcd_rad = 0;
  lcd_plats = 0;
  skriv("Starta bränning...   ");
  skriv("Välj program:   ");
  lcd_rad = 3;

  skriv("Ok               < >");
  int r;
  //ställer in vilken program vi startar
  do {    //väntar på input
    lcd.setCursor ( 14, 1);
    lcd.print(String(program + 1) + " " );

    key = key_in();
    if (key == 1) {
      program_from_eprom(program);
      //när bränningen är klar kan den gå tillbaka till huvudmenyn
      return;
    }
    if (key == 5) {
      program = max(0, program - 1);
    }
    if (key == 6) {
      program = min(9, program + 1);
    }
  } while (true);
}

void change_program() {
  int key;
  int program = EEPROM.read(senaste_program);
  lcd.clear();
  lcd_rad = 0;
  lcd_plats = 0;
  skriv("Ändra program...   ");
  skriv("Välj program:   ");
  lcd_rad = 3;

  skriv("Ok               < >");
  int r;
  //ställer in vilken program vi startar
  do {    //väntar på input
    lcd.setCursor ( 14, 1);
    lcd.print(String(program + 1) + " " );

    key = key_in();
    if (key == 1) {
      program_from_eprom(program);
      //lista program och tillåt ändringar
      do {
        lcd.clear();
        lcd_rad = 0;
        lcd_plats = 0;
        skriv("Segment___:");
        skriv("Tid_______:");
        skriv("Temperatur:");
        do {
          key = key_in();
          if (key == 1) {
          }
        } while (true);
      } while (true);

      return;
    }
    if (key == 5) {
      program = max(0, program - 1);
    }
    if (key == 6) {
      program = min(9, program + 1);
    }
  } while (true);
}


void installningar() {
  int key;
  lcd.clear();
  lcd_rad = 0;
  lcd_plats = 0;
  skriv("Temperatur:");
  skriv("Välj pyrometer typ...");
  lcd_rad = 3;

  skriv("Ok               < >");
  //tar reda på vilken typ av termocouple vi har

  int r;
  //ställer in vilken pyrometer vi använder
  do {
    lcd.setCursor (0, 2);
    lcd.print(" B E J K N R S T");


    lcd_plats = (thermo_ref * 2);
    lcd.setCursor ( lcd_plats, 2);

    lcd.write("[" );
    lcd.setCursor ( lcd_plats + 2, 2);
    lcd.write("]" );


    //väntar på input
    key = key_in();
    if (key == 1) {
      return;
    }
    if (key == 5) {
      thermo_ref = max(0, thermo_ref - 1);
      set_termo_typ(thermo_ref);
    }
    if (key == 6) {
      thermo_ref = min(7, thermo_ref + 1);
      set_termo_typ(thermo_ref);
    }
  } while (true);

}

ISR(TIMER1_COMPA_vect) { //timer1 interrupt 1Hz toggles pin 13 (LED)
  //generates pulse wave of frequency 1Hz/2 = 0.5kHz (takes two cycles for full wave- toggle)
  if (toggle) {
    toggle = false;
  }
  else {
    toggle = true;
    seconds_of_effect_loop++;
    if (seconds_of_effect_loop > effect) {
      //Slår av relä
      digitalWrite(RELAY_1, RELAY_OFF);
    }
    if (seconds_of_effect_loop > 59) {
      //en hel minut har gåt
      seconds_of_effect_loop = 0;
      //Slår på relä om bränning startat och effekten är större än 2 sec bränning per minut
      if (burning) {

        segment_run_time++; //gåt yterligare en minut
        if (segment_run_time >= time_point[segment]) {
          last_temperatur = temp_point[segment]; //denna temperatur förväntas vi ha uppnått
          //nästa segment
          segment++;
          segment_run_time = 0;

          if (time_point[segment] < 1) { //altså 0
            //programmet är klart första gången tiden är 0;
            burning = false;
            segment = 0;
            segment_run_time = 0;
            Serial.println("BråÅäÄöÖ änning klar.....................");

          }
        }
        //räknar ut effekten för denna minut
        //bör temperatur är värmeökningen mellan de båda stegen delat per minut
        //sedan vet man stegningen per minut,
        //då tar man stegningen hittils som programmet kört
        ought_temperatur = ((float)(temp_point[segment] - last_temperatur) / time_point[segment]) * segment_run_time;
        ought_temperatur += last_temperatur;
        //
        if (effect > 2) {
          digitalWrite(RELAY_1, RELAY_ON);
        }
        ask_effect = true;
      }
    }
  }
}

/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// rutiner för förenklingar
/////////////////////////////////////////////////////////////////
/*
  int num_field(int number, byte rad, byte plats, int max_num) {
  byte kursor_plats = 3;
  int tiotal = 1;
  int lcd_key = 0;
  //utgår från 4 sifferplatser


  do {
    lcd.setCursor(plats, rad);
    num_print(number);
    lcd.setCursor(plats + kursor_plats, rad);
    // do {} while (key_in() != 0);
    wait_release_key();
    lcd_key = read_LCD_buttons();

    switch (lcd_key) {
      case btnRIGHT: {
          kursor_plats = min(kursor_plats + 1, 3);
          break;
        }
      case btnLEFT : {
          kursor_plats = max(kursor_plats - 1, 0);
          break;
        }
      case btnUP   : {
          number = number + tiotal;
          break;
        }
      case btnDOWN : {
          number = number - tiotal;
          break;
        }
    }
    switch (kursor_plats) {
      case 0: tiotal = 1000; break;
      case 1: tiotal = 100; break;
      case 2: tiotal = 10; break;
      case 3: tiotal = 1; break;
    }
    number = min(number, max_num);
    number = max(number, 0);

  } while (lcd_key != btnSELECT);
  return number;
  }




  //******************************* Hanterare för knapptryck, blinkar!!!
  int read_LCD_buttons() {              // read the buttons
  byte svar = 0;
  bool kursor_on = false;
  int adc_key_in = 0;
  //väntar med att returnera tills det att vi återgår till otryckt knapp
  //unsigned long currentMillis = millis();
  unsigned long previousMillis = 0;


  do {

    if (millis() - previousMillis >= interval) {
      lcd.noCursor();
      previousMillis = millis();
      if (kursor_on) {
        lcd.noCursor(); kursor_on = false;
      } else {
        lcd.cursor();  kursor_on = true;
      }

    }
    svar = key_in();
  } while (svar == 0);

  svar = key_in();

  return svar;

  }
*/

int raw_key() {
  int key_in;
  do {
    key_in = keyboard;
    delay(40);//för att motverka knapptryckningsbrus
    if (key_in == keyboard) {
      return key_in;
    }
  } while (true);
}

int wait_key() {
  int key = 0;
  do {
    key = raw_key();
  } while (key < 1);
  do {
  } while (raw_key() > 1);
  return key;
}

int key_in() {
  int key = wait_key();
  for (int r = 0; r < 6; r++) {
    if (key_ref[r] < (key + 20)) {
      return r + 1 ;
    }
  }
}

int num_print(int number) {
  if (number < 1000) lcd.print(' ');
  if (number < 100) lcd.print(' ');
  if (number < 10)  lcd.print(' ');
  lcd.print(number);
}

void set_termo_typ(int typ) {
  //det finns 8 olika typer som stöds, typerna är enumererade, men det tycker jag var onödigt komplicerat för mig.
  //jag numererar dem i stället
  EEPROM.update(thermocouple_reference, typ); //termopcouple typ b,e,j,k,n,r,s,t
  switch (typ) {
    case 0:  max.setThermocoupleType(MAX31856_TCTYPE_B); break;
    case 1:  max.setThermocoupleType(MAX31856_TCTYPE_E); break;
    case 2:  max.setThermocoupleType(MAX31856_TCTYPE_J); break;
    case 3:  max.setThermocoupleType(MAX31856_TCTYPE_K); break;
    case 4:  max.setThermocoupleType(MAX31856_TCTYPE_N); break;
    case 5:  max.setThermocoupleType(MAX31856_TCTYPE_R); break;
    case 6:  max.setThermocoupleType(MAX31856_TCTYPE_S); break;
    case 7:  max.setThermocoupleType(MAX31856_TCTYPE_T); break;

    default: max.setThermocoupleType(MAX31856_TCTYPE_K); break;
  }
}




