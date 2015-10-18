
int beepCnt=0;
long beepTaskTick=0;
int beepLengthMs=200;

void beep(int count, int ms) {
  beepCnt = count*2;
  beepTaskTick = millis();
  beepLengthMs = ms;
}

void setBeepPin(int on) {
    pinMode(5, OUTPUT); 
    if(on>1) {
      digitalWrite(5, !digitalRead(5) ); //toggle
    } else {
      if(on) {
         digitalWrite(5, HIGH); 
      } else {
         digitalWrite(5 , LOW);  
      }
    }
}

void runBeepTask() {
  if(beepCnt>0) {
    if(beepTaskTick!=0 && ((beepTaskTick + beepLengthMs) < millis()) ) {
      setBeepPin(2);
      beepCnt--;
      beepTaskTick = millis();
    }
  } else {
    setBeepPin(0);
    beepTaskTick=0;
  }
}

