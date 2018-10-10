
//nya arduino, ej från lubuntu resiporitory





//EFTERSOM ÅÄÖ inte riktigt går att använda i variabelnamn, så försöker jag hålla alla variabler på engelska
//OBSERVERA SSR reläer är tvärt om
#define  RELAY_ON 0
#define  RELAY_OFF 1
//Relästyrning från digital pin 2
#define RELAY_1 2
//#define keyboard   analogRead(6)
#define numpad   analogRead(0)
#define keyboard_reference 440 // programmen sträcker sig till 410, så detta borde vara säkert
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
volatile char cursor_char = 0; //ett blinkande inverterad tecken på

int Serial_i = 0;
char Serial_buf[4];



volatile int segment = 0; //0-9 vilket segment i time och temp point vi är på
int segment_run_time = 0; // Hur många minuter vi kört på aktivt segment
int last_temperatur = 20; //vi utgår i från att ugnen är 20 grader när vi börjar


int key_ref[20];//håller info om analoga tangent värden, det finns två kanaler! 6st+12
int thermo_ref;//vilken typ av thermocouple har vi valt

volatile int lcd_rad = 0;
volatile int lcd_plats = 0;

void  program_from_eprom(int program) {
  EEPROM.update(senaste_program, program);
  int r = 0;
  //om första adressen inte är 200, så är eeprom inte initsialiserat
  //de första 10 adresserna är reserverade, till vad? thermocouple_reference, senaste_program, bla,
  int offset = 10 + (program * (4 * 10)); // tio programsteg med 2 integers i varje steg totalt (10+(10*4*10*2)
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
  delay(300);
  lcd.begin(20, 4);
  lcd.home ();                   // go home
  lcd.print("UGNSAUTOMATIK v3.0");
  lcd.setCursor ( 0, 1 );        // go to the next line

  delay(300);


  Serial.begin(9600); //ta bort sedan
  Serial.println("lever");
  //väntar tills strömmen har stabiliserats, lyssnar på tangentbords kanalen

  int wait_time = millis() + 1000; //(1 sekund)
  do {
    delay(40);
  } while (numpad > 1 && wait_time > millis());

  do {
    delay(40);
  } while (numpad > 1 && wait_time > millis());
  //måste vara under 2 minst två mätningar, samt max en sekund, efter detta måste tangentbordet vara nertryckt från start
  //
  // är eeprom initsialiserat?
  //om första adressen inte är 200, så är eeprom inte initsialiserat
  int r = 0;
  if (EEPROM.read(0) != 200) {
    Serial.println("inte initsialiserat");
    for (r = 1; r < 512; r++) {
      EEPROM.update(r, 0);
    }
    EEPROM.update(0, 200); //minne rensat
    EEPROM.update(thermocouple_reference, 3); //default till termopcouple typ "k" (b,e,j,"K",n,r,s,t)
    //begär att få veta tangenterna
    create_keyboard();
  }
  //







  burning = false;



  Serial.print("klar med init");
  //minne i eeprom
  if (E2END < 1023) { //för lite eeprom, men alla skall ha 1023 (1kb) minst
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

  create_custom_char();

  Serial.print("startar");
  // om en knapp trycks in, så kommer vi direkt till inställningsmenyn
  //läs in analoga tangent refferenser
  for (int r = 0; r < 20; r++) {
    key_ref[r] = (EEPROM.read(keyboard_reference + (r * 2)) * 255) + EEPROM.read(keyboard_reference + (r * 2) + 1);
  }
  thermo_ref = EEPROM.read(thermocouple_reference);
  max.begin();//startar termocouple adc
}




void create_keyboard() {
  lcd.clear();
  skriv("Släpp knapparna..");
  do {
  } while (raw_key() > 0);
  int r = 0;
  for (r = 0; r < 10; r++) {
    lcd_rad = 0;
    lcd.clear();
    skriv("Tryck nummer " + String(r));

    key_ref[r] = wait_key();
    Serial.print(r);
    Serial.print(" = ");
    Serial.println(String(key_ref[r]));
    EEPROM.update(keyboard_reference + (r * 2), highByte(key_ref[r]));
    EEPROM.update(keyboard_reference + (r * 2) + 1, lowByte(key_ref[r]));
  }

  lcd_rad = 0;
  lcd.clear();
  skriv("Tryck *");
  r = 10;
  key_ref[r] = wait_key();
  Serial.println(String(key_ref[r]));
  EEPROM.update(keyboard_reference + (r * 2), highByte(key_ref[r]));
  EEPROM.update(keyboard_reference + (r * 2) + 1, lowByte(key_ref[r]));

  lcd_rad = 0;
  lcd.clear();
  skriv("Tryck #");
  r = 11;
  key_ref[r] = wait_key();
  Serial.println(String(key_ref[r]));
  EEPROM.update(keyboard_reference + (r * 2), highByte(key_ref[r]));
  EEPROM.update(keyboard_reference + (r * 2) + 1, lowByte(key_ref[r]));


  /*
    for (r = 12; r < 18; r++) {
      lcd_rad = 0;
      lcd.clear();
      skriv("Tryck knapp " + String(r - 11));

      key_ref[r] = wait_key();
      Serial.println(String(key_ref[r]));
      EEPROM.update(keyboard_reference + (r * 2), highByte(key_ref[r]));
      EEPROM.update(keyboard_reference + (r * 2) + 1, lowByte(key_ref[r]));
    }

  */
  lcd_rad = 0;
  lcd.clear();
  skriv("Knappar inställda");
  skriv("Tryck #");


  while (key_in() != 11);
  lcd.clear();
}
/////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
void loop() {
  int r = 0;
  int key = 0;
  if (raw_key() > 0 ) {
    //man håller ner tangentbordet när datorn startas,

    create_keyboard();

  }
  if (burning) {
    //räknar ut vilken temperatur som det borde vara
    //gör bara en gång i minuten
    if (ask_effect) {
      //räknar ut effekten
      //ought_temperatur
      //hämtar vilken temperatur vi har
    }
  }
  else {

    //här ligger användargränssnitet för att lägga in nya program
    do {
      lcd.clear();                   // go home
      lcd_rad = 0;
      lcd_plats = 0;



      skriv("1. Starta bränning");
      skriv("2. Ändra program");
      skriv("3. Inställningar");

      lcd_rad = 3;
      lcd_plats = 19;
      cursor_char = '?';

      do {
        key = key_in();
        Serial.println(String(key));
        if (key == 0) {}
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

      }
      while (true);
    }
    while (true);
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
  }
  while (true);
}

void change_program() {
  int key;
  int program = EEPROM.read(senaste_program);

  //ställer in vilken program vi startar
  do {
    lcd.clear();
    lcd_rad = 0;
    lcd_plats = 0;
    skriv("Ändra program 0-9");
    lcd_rad = 2;
    skriv("Välj program:   ");
    skriv("*=Avbryt");

    cursor_char = program;
    lcd_rad = 2;
    lcd_plats = 14;

    key = key_in();
    if (key == 10) {
      return; // *
    }
    if (key < 10) {
      program = key;
    }
    change_segment(program);
  } while (true);


}

void change_segment(int program) {
  int key = 0;

  program_from_eprom(program);
  //lista program och tillåt ändringar

  int segment = 0;
  do {
    Serial.print("segment: ");
    Serial.println(segment);
    lcd.clear();
    lcd_rad = 0;
    lcd_plats = 0;
    String slask = "";
    slask = ("Prg.(" + String(program) + ") Segment: " + String(segment));
    skriv(slask);
    //lcd.print(segment);
    skriv("Tid_______: " + num_format(time_point[segment]) + " min");
    // num_print(time_point[segment]);
    skriv("Temperatur: " + num_format(temp_point[segment]) + " C" + char(223)); //lcd.print(char(223));
    // num_print(temp_point[segment]);
    skriv("*=Avbryt : Klar=#");

    //fält 1 (segment)
    lcd_rad = 0;
    lcd_plats = 17;
    cursor_char = segment;
    key = key_in();
    if (key == 10) {
      break; // * (går från "välj segment" till "välj program")
    }

    if (key != 11) {
      segment = key;
      continue;
    }

    Serial.println("tid");
    lcd_rad = 1;
    lcd_plats = 15;
    time_point[segment] = num_field(time_point[segment], 1440); //24 timmar max för ett segment!

    if (exit_key() == 10) {
      break;
    }
    Serial.println("temp");
    //fält 3 (temperatur)
    lcd_rad = 2;
    lcd_plats = 15;
    temp_point[segment] = num_field(temp_point[segment], 1300);
    if (exit_key() == 10) {
      break;
    }
  } while (true);

  lcd.clear();
  lcd_rad = 0;
  lcd_plats = 0;
  cursor_char = '?';
  skriv("Program (" + String(program) + ")");
  skriv("Spara ändringar:");
  skriv("*=ja : nej=#");
  lcd_rad = 1;
  lcd_plats = 16;
  while (true) {
    key = key_in();
    if (key == 10) {
      program_to_eprom(program);
      return; // *
    }
    if (key == 11) {
      return; //#
    }
  }
}

void installningar() {
  //Här förväntas vi visa temperatur också
  int key;
  do {
    lcd.clear();
    lcd_rad = 0;
    lcd_plats = 0;
    skriv("Temperatur:");
    Serial.println(max.readCJTemperature());
    skriv("Välj pyrometer typ:");
    skriv(" B E J K N R S T");
    skriv(" 1 2 3 4 5 6 7 8");



    //tar reda på vilken typ av termocouple vi har


    //ställer in vilken pyrometer vi använder

String Pyrometrar="BEJKNRST";
cursor_char=Pyrometrar.charAt(thermo_ref);//en bokstav som skall blinka
  //  display_charachter(cursor_char);

    lcd_plats = ((thermo_ref * 2)+1);
    lcd.setCursor ( lcd_plats, 2);

   
    lcd.write(6);
 

    lcd_rad = 1;
    lcd_plats = 19;
    //väntar på input
    key = key_in();
    if (key > 0 && key < 9) {
      thermo_ref = (key - 1);
      set_termo_typ(thermo_ref);
    }


    if (key == 10) {
      return;
    }
    if (key == 11) {
      return;
    }


  }
  while (true);

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
            Serial.println("Bränning klar.....................");

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
/* vi använder bara numpaden!
  int raw_key_sub() {
  //lägg i hop de båda kanalerna
  // privat till raw_key
  int key_in;
  key_in = numpad;
  if (key_in > 0) {
    key_in = key_in + 1000;//om input kommer från tangentbord 2, så lägg på 1000, för att enkelt kunna skilja dem åt.
  }
  return (key_in + keyboard);// nu har vi lagt i hop båda kanalerna
  }
*/
int raw_key() {
  //returnera ett stabilt värde från analog input
  //varje kanal osilicerar med +-1
  // bör inte anropas!!
  int key_in;
  int key_in_2;
  do {

    key_in = numpad;//raw_key_sub();
    delay(40);//för att motverka knapptryckningsbrus
    key_in_2 = numpad; //raw_key_sub();
    // +-1

    if ((key_in - 1) <= key_in_2 && key_in_2 <= (key_in + 1)) {
      return key_in;
    }

  }
  while (true);
}

int wait_key() {
  // returnerar ett analogt värde när någon tangent är tryckt
  int key = 0;
  boolean last_toggle = true;
  toggle = false;
  if (cursor_char < 10) {
    cursor_char += 48;//skall vara aschii!!
  }
  do {
    key = raw_key();

    //här måste vi också lyssna på toggle
    //eftersom man inte kan använda en interupt för att skriva till lcd
    if (toggle != last_toggle) {
      last_toggle = toggle;
      lcd.setCursor ( lcd_plats, lcd_rad);
      if (toggle) {

        lcd.write(cursor_char);//bokstäver och tecken skall skrivas som de är


      } else {
        display_charachter(int(cursor_char));
        lcd.write(6);
      }
    }
  } while (key < 1);//väntar på tangenttryckning
  //en tangenttryckning har kommit,
  do {
  } while (raw_key() > 1);//väntar på att tangenten skall släppas

  //ser till att en eventuellt markerad tangent är vanlig!
  lcd.setCursor ( lcd_plats, lcd_rad);
  if (key < 10) {
    lcd.write(key);
  } else {
    lcd.write(cursor_char);
  }
  return key;
}

int key_in() {
  //översätter tangenttryck till tangent
  // skall användas
  int key = wait_key();
  for (int r = 0; r < 12; r++) {//11 är antalet tangenter
    if ((key_ref[r] - 7) <= key && key <= (key_ref[r] + 7)) {
      Serial.print("tangent nr :" );
      Serial.println(r );
      return r  ;
    }
  }
  Serial.print("!okänd tangent! " );
  //här kan det vara bra att ställa in tangenterna
  Serial.println(key );
  for (int r = 0; r < 19; r++) {//18 är antalet tangenter

    Serial.print(r );
    Serial.print(" = " );
    Serial.println(key_ref[r] );

  }

}

/*
  int num_field(int number, int max_number) {
  //skriver ut nummer med slutet placerat på lcd_plats och lcd_rad
  //max 4 siffror
  //returnerar nummer när annan tangent än 1-9 trycks
  String slask = "0000";
  slask = slask + number;
  int trim_size = 0;
  int key = 0;
  cursor_char = slask.charAt(slask.length());

  Serial.print("freeram     : ");
  Serial.println(freeRam());

  do {
    trim_size = (slask.length()) - 4;


    slask.remove(0, trim_size);
    lcd.setCursor ( lcd_plats - 3, lcd_rad);
    lcd.print(slask);
    key = key_in();

    if (key < 10) {
      Serial.println("förr     : " + slask);
      slask = slask + key;//lägger till en siffra

      Serial.println("nuvarande: " + slask);

      cursor_char = key; //global till wait_key()
    }
  } while (key != 10);
  number = slask.toInt();
  number = min(number, max_number);
  return number;
  }
*/
int num_print(int number) {
  if (number < 1000) lcd.print(' ');
  if (number < 100) lcd.print(' ');
  if (number < 10)  lcd.print(' ');
  lcd.print(number);
}

String num_format(int number) {
  if (number < 10)   return ("   " + String(number));
  if (number < 100)  return ("  " + String(number));
  if (number < 1000) return (" " + String(number));


}

void set_termo_typ(int typ) {
  //det finns 8 olika typer som stöds, typerna är enumererade, men det tycker jag var onödigt komplicerat för mig.
  //jag numererar dem i stället
  EEPROM.update(thermocouple_reference, typ); //termopcouple typ b,e,j,k,n,r,s,t
  switch (typ) {
    case 0:
      max.setThermocoupleType(MAX31856_TCTYPE_B);
      break;
    case 1:
      max.setThermocoupleType(MAX31856_TCTYPE_E);
      break;
    case 2:
      max.setThermocoupleType(MAX31856_TCTYPE_J);
      break;
    case 3:
      max.setThermocoupleType(MAX31856_TCTYPE_K);
      break;
    case 4:
      max.setThermocoupleType(MAX31856_TCTYPE_N);
      break;
    case 5:
      max.setThermocoupleType(MAX31856_TCTYPE_R);
      break;
    case 6:
      max.setThermocoupleType(MAX31856_TCTYPE_S);
      break;
    case 7:
      max.setThermocoupleType(MAX31856_TCTYPE_T);
      break;

    default:
      max.setThermocoupleType(MAX31856_TCTYPE_K);
      break;
  }
}





