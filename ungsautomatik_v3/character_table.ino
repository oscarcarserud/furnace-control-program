
int last_number = 255;

void display_charachter(int number) {

  // vi vill ha en blink rutin som visar aktuellt tecken inverterat. För bokstäver? använd ascii
  if (number > 47 && number < 58) {
    number = number - 48; //det är jobbigt när det kommer aschii tecken i stället för siffror
  }
  //om vi kommer hit varje 0.5 sekund, (blink)
  //så kan det vara bra att inte ändra tecknet,
  //om det inte behöver ändras
  if (number == last_number) {
    return;
  }
  last_number = number;
  //default till svart




  switch (number) {
    case 0: {
        byte answer[8] {
          B10001,
          B01110,
          B01110,
          B01110,
          B01110,
          B01110,
          B10001,
          B11111
        }; lcd.createChar(6, answer); break;
      }

    case 1: {
        byte answer[8] = {
          B11011,
          B10011,
          B11011,
          B11011,
          B11011,
          B11011,
          B10001,
          B11111
        }; lcd.createChar(6, answer); break;
      }

    case 2: {
        byte answer[8] = {
          B10001,
          B01110,
          B11110,
          B11101,
          B11011,
          B10111,
          B00000,
          B11111
        }; lcd.createChar(6, answer); break;
      }
    case 3: {
        byte answer[8] = {
          B00000,
          B11101,
          B11011,
          B11101,
          B11110,
          B01110,
          B10001,
          B11111
        }; lcd.createChar(6, answer); break;
      }
    case 4: {
        byte answer[8] = {
          B11101,
          B11001,
          B10101,
          B01101,
          B00000,
          B11101,
          B11101,
          B11111
        }; lcd.createChar(6, answer); break;
      }
    case 5: {//5
        byte answer[8] = {
          B00000,
          B01111,
          B00001,
          B11110,
          B11110,
          B01110,
          B10001,
          B11111
        }; lcd.createChar(6, answer); break;
      }
    case 6: {//6
        byte answer[8] = {
          B11100,
          B11011,
          B10111,
          B00001,
          B01110,
          B01110,
          B10001,
          B11111
        }; lcd.createChar(6, answer); break;
      }
    case 7: {
        byte answer[8] = {
          B00000,
          B11110,
          B11101,
          B11011,
          B10111,
          B10111,
          B10111,
          B11111
        }; lcd.createChar(6, answer); break;
      }
    case 8: {
        byte answer[8] = {
          B10001,
          B01110,
          B01110,
          B10001,
          B01110,
          B01110,
          B10001,
          B11111
        }; lcd.createChar(6, answer); break;
      }
    case 9: {
        byte answer[8] = {
          B10001,
          B01110,
          B01110,
          B10000,
          B11110,
          B11101,
          B10011,
          B11111
        }; lcd.createChar(6, answer); break;
      }
    case 63: {
        byte answer[8] = {
          B10001,
          B01110,
          B01110,
          B11101,
          B11011,
          B11111,
          B11011,
          B11111
        }; lcd.createChar(6, answer); break;
      }



    case 163: {
        byte answer[8] = {
          B00000,
          B00000,
          B00000,
          B00000,
          B00000,
          B00000,
          B00000,
          B00000
        }; lcd.createChar(6, answer); break;
      }
    case 66: {
        byte answer[8] = {
          B00001,
          B01110,
          B01110,
          B00001,
          B01110,
          B01110,
          B00001,
          B11111
        }; lcd.createChar(6, answer); break;//B
      }
    case 69: {
        byte answer[8] = {
          B00000,
          B01111,
          B01111,
          B00001,
          B01111,
          B01111,
          B00000,
          B11111
        }; lcd.createChar(6, answer); break;//E
      }
    case 74: {
        byte answer[8] = {
          B11000,
          B11101,
          B11101,
          B11101,
          B11101,
          B01101,
          B10011,
          B11111
        }; lcd.createChar(6, answer); break;//J
      }
    case 75: {
        byte answer[8] = {
          B01110,
          B01101,
          B01011,
          B00111,
          B01011,
          B01101,
          B01110,
          B11111
        }; lcd.createChar(6, answer); break;//K
      }
    case 78: {
        byte answer[8] = {
          B01110,
          B01110,
          B00110,
          B01010,
          B01100,
          B01110,
          B01110,
          B11111
        }; lcd.createChar(6, answer); break;//N
      }
    case 82: {
        byte answer[8] = {
          B00001,
          B01110,
          B01110,
          B00001,
          B01011,
          B01101,
          B01110,
          B11111
        }; lcd.createChar(6, answer); break;//R
      }
    case 83: {
        byte answer[8] = {
          B10000,
          B01111,
          B01111,
          B10001,
          B11110,
          B11110,
          B00001,
          B11111
        }; lcd.createChar(6, answer); break;//S
      }
    case 84: {
        byte answer[8] = {
          B00000,
          B11011,
          B11011,
          B11011,
          B11011,
          B11011,
          B11011,
          B11111
        }; lcd.createChar(6, answer); break;//T
      }
    default: {
        byte answer[8] = {
          B10001,
          B11111,
          B10011,
          B11111,
          B10011,
          B11111,
          B10011,
          B00100
        }; lcd.createChar(6, answer); break;
      }

  }




}

