uint32_t start, eTime, dbStart;
const byte btn = 30, pumpPin = 2, dbTime = 20;
bool pinState = true,
        btnState = true,
        timing = false;
void setup()
{
  Serial.begin(9600);
  pinMode(btn, INPUT_PULLUP);
  pinMode(pumpPin, OUTPUT);
}

void loop()
{
  // debounce button ++++++++++++++++
  if (digitalRead(btn) != pinState) // get state of pin 30
  {
    dbStart = millis(); // reset db timer
    pinState ^= 1;      // now they are equal, won't enter
  }                     // here again unless pin state changes
  if (millis() - dbStart > dbTime) // db timer has elapsed
    btnState = pinState;           // button state is valid
  //+++++++++++++++++++++++++++++++++++
  if (btnState == LOW && !timing)
  {
    start = millis(); // start timing
    analogWrite(pumpPin, 255);
    timing = true;
    Serial.println(F("  Timing"));
  }
  if(btnState == HIGH && timing)
  {
    analogWrite(pumpPin, 0);
    eTime = (millis() - start);
    Serial.print(F("  Elapsed millis = "));
    Serial.println(eTime);
    timing = false;
  }

}
