#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <I2CIO.h>

#define I2C_ADDR 0x27
#define POSITIVE 1          
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7);  // Set the LCD I2C address

I2CIO ic;

#define BACKLIGHT_PIN     13

#define LCD_BACKLIGHT   0x08
#define LCD_NOBACKLIGHT 0x00

#define MIN_TEMP 250
#define MAX_TEMP 450

#define MAX_TEMPERATURE_DIFFERENCE  (1*1000)

long currentTemperature = 360000;
long currentPotValue = 0;

void readTemperatureTask() {
     static long taskTick = millis();
   if(taskTick+200>millis()) return;
   taskTick = millis();
   
  currentTemperature += 100;
}
void potValueTask() {
   long v = analogRead(A1);
   v = (v * (MAX_TEMP-MIN_TEMP) )/1024 + MIN_TEMP;
   if(abs(currentPotValue-v*1000)<=1) {
     return;
   }
   currentPotValue = v*1000;  
}

void setup()
{
  ic.begin(I2C_ADDR);   
  lcd.begin(16,2);               // initialize the lcd 
   Serial.begin(57600);

  lcd.backlight();
  lcd.home ();                   // go home
  lcd.print("Booting...");  
  delay ( 1000 );
  beep(1, 100);
}


void printUptime() {
  long uptime = millis()/1000;
  int seconds = (uptime % 60);
  int minutes = (uptime % 3600) / 60;
  int hours = (uptime % 86400) / 3600;
  int days = (uptime % (86400 * 30)) / 86400;
  
  char buf[10];
  sprintf(buf,"%02d:%02d:%02d", hours, minutes, seconds);
  lcd.print(buf);
}

void printInfoTask() {
   static long taskTick = millis();
   if(taskTick+100>millis()) return;
   taskTick = millis();
   
   int potValue = currentPotValue/1000;
   static int potValuePrevious = potValue;
   static long t = 0;
   static boolean ready = false;
   
   if(potValue!=potValuePrevious) {
     t = millis() + 3000;
     ready = false;
   }
   potValuePrevious = potValue;

   lcd.home ();
   lcd.setCursor ( 0, 0 ); 
   
   if(abs(currentPotValue-currentTemperature)<MAX_TEMPERATURE_DIFFERENCE) {
     lcd.print("READY       ");
   } else {
     if(currentTemperature/1000<potValue) {
        lcd.print("HEATING     ");
     } else {
        lcd.print("COOLING     ");
     }
   }
   lcd.print(potValue);
   lcd.print("C"); 
   lcd.setCursor ( 0, 1 ); 
   
   if(t<millis()) {
      if(ready==false) {
       if(abs(currentPotValue-currentTemperature)<MAX_TEMPERATURE_DIFFERENCE) {
         ready = true;
         beep(1,200);
       }
     }
     printUptime();
     lcd.print("    ");
     lcd.print(currentTemperature/1000);
     lcd.print("C         ");   
   } else {
     int percent = ((potValue-MIN_TEMP)*100)/(MAX_TEMP-MIN_TEMP);
     int c = (16*percent)/100;
     
     for(int i=0;i<=c;i++)  lcd.print((char)255);
     for(int i=0;i<16-c;i++)  lcd.print(" ");
   }
}

void heatingTask() {
  if(currentTemperature<currentPotValue) {
     //heating is needed 
  }
}

void loop()
{
  potValueTask();
  readTemperatureTask();
  runBeepTask(); 
  printInfoTask();
  heatingTask();
}
