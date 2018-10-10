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
        lcd.write(byte(0));
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

    }
    else {
      if (bokstav == 103) {
        lcd.write(4);
      } else if (bokstav == 112) {
        lcd.write(5);
      } else {
        lcd.write(bokstav);
      }
    }
  }
  lcd_rad = min(lcd_rad++, 3);
}


void test_char() {
  lcd.setCursor ( 0, 1 );

  lcd.write(byte(0));

  lcd.write(1);
  lcd.write(2);
  lcd.write(3);
  lcd.write(4);
  lcd.write(5);
  lcd.write(6);
  lcd.write(7);

  skriv("ÅÄÖåäöpg");
}

void create_custom_char() {
  byte a0[8] = {//å
    B00100,
    B00000,
    B01110,
    B00001,
    B01111,
    B10001,
    B01111,
    B00000
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
  byte a4[8] = {//g
    B00000,
    B00000,
    B01111,
    B10001,
    B10001,
    B01111,
    B00001,
    B01110
  };
  byte a5[8] = {//p
    B00000,
    B00000,
    B11110,
    B10001,
    B10001,
    B11110,
    B10000,
    B10000
  };


  lcd.createChar(0, a0);
  lcd.createChar(1, a1);
  lcd.createChar(2, a2);
  lcd.createChar(3, a3); //skapar åÅÄÖ
  lcd.createChar(4, a4);
  lcd.createChar(5, a5);
 // lcd.createChar(6, a5);
  // lcd.createChar(7, a5);

 
 

}
