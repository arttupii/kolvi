#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <I2CIO.h>
#include <avr/wdt.h>
#include <EEPROM.h>
#include <stdarg.h>

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

#define KEY_SHORT 1
#define KEY_LONG 2

#define EEPROM_ADD_IDLE_TIMER_SELECTOR 0

int key = 0;


long currentTemperature = 360000;
long currentPotValue = 0;
long selectedTemperature = 0;

unsigned long idleTimer = 0;

  const unsigned long idleTimerConstants[] = {0,15, 30,60, 2*60,4*60, 7*60};

#define PRINTF_BUF 80 // define the tmp buffer size (change if desired)
   void dbg(const char *format, ...)
   {
   char buf[PRINTF_BUF];
   va_list ap;
        va_start(ap, format);
        vsnprintf(buf, sizeof(buf), format, ap);
        for(char *p = &buf[0]; *p; p++) // emulate cooked mode for newlines
        {
                if(*p == '\n')
                        Serial.print('\r');
                Serial.print(*p);
        }
        va_end(ap);
   }

void reset() {
  void (*doReset)(void) = 0;
    lcd.print("Resetting...    ");  
    doReset();
  while(1);
}

void setSelectedTemperature() {
  selectedTemperature =  currentPotValue; 
  Serial.print("Selected temperature");
  Serial.println(selectedTemperature);
}

void readTemperatureTask() {
     static long taskTick = millis();
   if(taskTick+200>millis()) return;
   taskTick = millis();
   
  currentTemperature += 100;
}

void potValueTask() {
  currentPotValue = 0;
  for(int i=0;i<10;i++) {
   long v = analogRead(A1);
     v = (v * (MAX_TEMP-MIN_TEMP) )/1024 + MIN_TEMP;
     v *= 1000;
     currentPotValue += v;
   }
   currentPotValue = currentPotValue/10;
}

void setup()
{
  wdt_disable();
  ic.begin(I2C_ADDR);   
  lcd.begin(16,2);               // initialize the lcd 
   Serial.begin(57600);

  lcd.backlight();
  lcd.home ();                   // go home
  lcd.print("Booting...");  
  analogReference(EXTERNAL);

  
  delay (500 );
  beep(1, 100);
  resetIdleTimer();
  potValueTask();
  setSelectedTemperature();
}


void printTime(long time) {
  long uptime = time/1000;
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
   
   if(abs(potValue-potValuePrevious)>1) {
     t = millis() + 3000;
     ready = false;
   }
   potValuePrevious = potValue;

   lcd.home ();
   lcd.setCursor ( 0, 0 ); 
   
   if(abs(selectedTemperature-currentTemperature)<MAX_TEMPERATURE_DIFFERENCE) {
     lcd.print("READY       ");
   } else {
     if(currentTemperature/1000<potValue) {
        lcd.print("Heating    ");
     } else {
        lcd.print("Cooling    ");
     }
   }
   lcd.print(selectedTemperature/1000);
   lcd.print((char)223);
   lcd.print("C");
   lcd.setCursor ( 0, 1 ); 
   
   if(t<millis()) {
      if(ready==false) {
       if(abs(currentPotValue-currentTemperature)<MAX_TEMPERATURE_DIFFERENCE) {
         ready = true;
         beep(1,200);
       }
     }
     if(idleTimer!=0) {
         printTime(idleTimer-millis());
     } else {
         printTime(millis()); 
     }
     lcd.print("   ");
     lcd.print(currentTemperature/1000);
     lcd.print((char)223); 
     lcd.print("C");   
   } else {
     int percent = ((potValue-MIN_TEMP)*100)/(MAX_TEMP-MIN_TEMP);
     int c = (16*percent)/100;
     setSelectedTemperature();
     for(int i=0;i<=c;i++)  lcd.print((char)255);
     for(int i=0;i<16-c;i++)  lcd.print(" ");
   }
}

void heaterOff() {
}
void heaterOn() {
}
void heatingTask() {
  if(currentTemperature<selectedTemperature) {
     //heating is needed 
  }
}


void readKeyTask() {
  // initialize the pushbutton pin as an input:
  key = 0;
  pinMode(8, INPUT_PULLUP);
  static long time = 0;
  static char prevState = 0;  
  if(digitalRead(8)==0 && prevState!=1) {
     time = millis();
     prevState = 1;
  } else if(prevState==1 && digitalRead(8)==1){
     prevState = 0;
     if(millis()-time>300) {
        key = KEY_LONG;
     } else {
        key = KEY_SHORT;
     }
  }
}

void resetIdleTimer() {
  int activeIdleTimerSelector = EEPROM.read(EEPROM_ADD_IDLE_TIMER_SELECTOR);
  if(idleTimerConstants[activeIdleTimerSelector]) {
    idleTimer = idleTimerConstants[activeIdleTimerSelector]*60*1000 + millis();
  } else {
    idleTimer = 0;
  }
}

int taskIdleTimer() {
  
  static boolean idleTimeSetMenuActive = false;
  static boolean idleTimeResetMenuActive = false;
  static boolean idleTimeWarningMenuActive = false;

  static int activeIdleTimerSelector = EEPROM.read(EEPROM_ADD_IDLE_TIMER_SELECTOR);
  static long menuTimer = millis();

  if(key==KEY_LONG || idleTimeSetMenuActive) {
    if(idleTimeSetMenuActive == false || key>0 ) {
      menuTimer = millis();
      idleTimeSetMenuActive = true;
    }
    if(millis()-menuTimer>5000) {
        idleTimeSetMenuActive = false;
    }

    if(key==KEY_SHORT) {
      activeIdleTimerSelector++;
      if(activeIdleTimerSelector>=(sizeof(idleTimerConstants)/sizeof(unsigned long))) {
        activeIdleTimerSelector = 0;
      }
      EEPROM.write(EEPROM_ADD_IDLE_TIMER_SELECTOR, activeIdleTimerSelector);
      resetIdleTimer();
    }
    //if(idleTime > 
    lcd.home ();
    lcd.setCursor ( 0, 0 );   
    lcd.print("Set autoPowerOff");
    lcd.setCursor ( 0, 1 ); 
    lcd.print("           ");
    if(idleTimerConstants[activeIdleTimerSelector]==0) {
      lcd.print("Off");
    } else {
      lcd.print((double)idleTimerConstants[activeIdleTimerSelector]/60);
      lcd.print("h");
    }
    lcd.print("           ");
  } else if(key==KEY_SHORT || idleTimeResetMenuActive) {
    if(idleTimeResetMenuActive == false ) {
      menuTimer = millis();
      idleTimeResetMenuActive = true;
    }
    if(millis()-menuTimer>2000) {
        idleTimeResetMenuActive = false;
    }
    lcd.setCursor ( 0, 0 ); 
    lcd.print("Resetting idle  ");
    lcd.setCursor ( 0, 1 ); 
    lcd.print("timer...        ");
    resetIdleTimer();
    idleTimeWarningMenuActive = false;
  } else if( (idleTimer>0)&&((idleTimer-millis())<1000)) {
     heaterOff();
     beep(4, 200);
     
      lcd.setCursor ( 0, 0 );   
      lcd.print(" Heater is off  ");
      lcd.setCursor ( 0, 1 );  
      lcd.print("                ");
    do {  
      runBeepTask(); 
      readKeyTask();
     } while(key!=KEY_LONG);
     reset();
  } 
  else if( (idleTimer>0)&&((idleTimer-millis())<60000)) {
    lcd.setCursor ( 0, 0 );   
    lcd.print(" Idle Warning!! ");
    lcd.setCursor ( 0, 1 );  
    lcd.print("                ");
    if(idleTimeWarningMenuActive==false) {
        idleTimeWarningMenuActive = true;
        beep(3, 200);
    }
  }
  return idleTimeSetMenuActive || idleTimeResetMenuActive || idleTimeWarningMenuActive;
}

void loop()
{
  readKeyTask();
  potValueTask();
  readTemperatureTask();
  runBeepTask(); 
  if(!taskIdleTimer()) {
    printInfoTask();
  }
  heatingTask();
  
}
