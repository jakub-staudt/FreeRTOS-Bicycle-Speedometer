extern "C" {
  #include "hall_sensor.h"  // tell Arduino to treat as C
}

void setup() {
  Serial.begin(115200);
}

void loop() {
  int val = read_hall_sensor();  // works because it’s declared in the header
  Serial.println(val);
  delay(200);
}
