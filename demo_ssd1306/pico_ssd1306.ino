
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

// Need to comment out line "#include <pgmspace.h>" in Adafruit_ssd1306.cpp
// https://randomnerdtutorials.com/raspberry-pi-pico-ssd1306-oled-arduino/
// https://forum.arduino.cc/t/pi-pico-and-ssd1306-oled-display/929498/8
// https://iotdesignpro.com/articles/interfacing-oled-display-with-raspberry-pi-pico-w-using-arduino-ide

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

//MbedI2C mywire(4, 5); // SDA=GP4, SCL=GP5 default
MbedI2C mywire(20, 21); // SDA=GP20, SCL=GP21

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &mywire, -1);

void setup() {
  Serial.begin(115200);

  mywire.begin(); 
 
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C, true, false)) { 
    Serial.println("SSD1306 allocation failed");
    for(;;); // Don't proceed, loop forever
  }

  delay(2000);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Hello I2C!");
  display.display();

  // put your setup code here, to run once:
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);    
}

void loop() {
  // put your main code here, to run repeatedly:
  // put your main code here, to run repeatedly:
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(100);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(100);                      // wait for a second
}
