
void display_charachter(int number) {
  byte answer[8];
  switch (number) {
    case 0:{
       byte answer[8] = {
        B10001,
        B01110,
        B01110,
        B01110,
        B01110,
        B01110,
        B10001,
        B11111
       };
       lcd.createChar(4,answer);
    }
      break;
    case 1:{
      byte answer[8] = {//1
        B11011,
        B10011,
        B11011,
        B11011,
        B11011,
        B11011,
        B10001,
        B11111
      };}
      break;
    case 2:{
      byte answer[8] = {//2
        B10001,
        B01110,
        B11110,
        B11101,
        B11011,
        B10111,
        B00000,
        B11111
      };} break;
    case 3:{
      byte answer[8] = {//3
        B00000,
        B11101,
        B11011,
        B11101,
        B11110,
        B01110,
        B10001,
        B11111
      };} break;
    case 4:{
      byte answer[8] = {//4
        B11101,
        B11001,
        B10101,
        B01101,
        B00000,
        B11101,
        B11101,
        B11111
      };} break;
    case 5:{
      byte answer[8] = {//5
        B00000,
        B01111,
        B00001,
        B11110,
        B11110,
        B01110,
        B10001,
        B11111
      };} break;
    case 6:{
      byte answer[8] = {//6
        B11100,
        B11011,
        B10111,
        B00001,
        B01110,
        B01110,
        B10001,
        B11111
      };} break;
    case 7:{
      byte answer[8] = {//7
        B00000,
        B11110,
        B11101,
        B11011,
        B10111,
        B10111,
        B10111,
        B11111
      };} break;
    case 8:{
      byte answer[8] = {//8
        B10001,
        B01110,
        B01110,
        B10001,
        B01110,
        B01110,
        B10001,
        B11111
      };} break;
    case 9:{
      byte answer[8] = {//9
        B10001,
        B01110,
        B01110,
        B10000,
        B11110,
        B11101,
        B10011,
        B11111
      };}break;
  }
  lcd.createChar(4,answer);


}
