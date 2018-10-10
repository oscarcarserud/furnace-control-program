
int num_field_exit_key = 0;
int exit_key() {
  return (num_field_exit_key);
}

int num_field(int number, int max_number) {
  //skriver ut nummer med slutet placerat på lcd_plats och lcd_rad
  //max 4 siffror
  //returnerar nummer när annan tangent än 1-9 trycks
  String slask = "0000";
  slask = slask + number;
  int trim_size = 0;
  int key = 0;


  Serial.println("-----------");
  Serial.println(slask);
  Serial.println(number);

  do {
    trim_size = (slask.length()) - 4;
    slask.remove(0, trim_size);
    cursor_char = slask.charAt(3);
Serial.print("cursor_char     : ");
  Serial.print(cursor_char);
  Serial.print(" ");
  //Serial.println(cursor_char.toInt());
  
    lcd.setCursor ( lcd_plats - 3, lcd_rad);
    lcd.print(slask);
    key = key_in();

    if (key < 10) {
      slask = slask + key;//lägger till en siffra
      cursor_char = key; //global till wait_key()
    }
  } while (key < 10);
  num_field_exit_key = key;
  number = slask.toInt();
  lcd.setCursor ( lcd_plats - 3, lcd_rad);
  num_print(number);
  number = min(number, max_number);
  return number;
}
