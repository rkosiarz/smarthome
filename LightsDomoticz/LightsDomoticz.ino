// Enable debug prints to serial monitor
#define MY_DEBUG 


// Enable and select radio type attached
//#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Set LOW transmit power level as default, if you have an amplified NRF-module and
// power your radio separately with a good regulator you can turn up PA level. 
//#define MY_RF24_PA_LEVEL RF24_PA_LOW

// Enable serial gateway
#define MY_GATEWAY_SERIAL

// Define a lower baud rate for Arduino's running on 8 MHz (Arduino Pro Mini 3.3V & SenseBender)
#if F_CPU == 8000000L
#define MY_BAUD_RATE 38400
#endif

// Flash leds on rx/tx/err
// #define MY_LEDS_BLINKING_FEATURE
// Set blinking period
// #define MY_DEFAULT_LED_BLINK_PERIOD 300

// Inverses the behavior of leds
// #define MY_WITH_LEDS_BLINKING_INVERSE

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
#define MY_INCLUSION_BUTTON_FEATURE

// Inverses behavior of inclusion button (if using external pullup)
//#define MY_INCLUSION_BUTTON_EXTERNAL_PULLUP

// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60 
// Digital pin used for inclusion mode button
#define MY_INCLUSION_MODE_BUTTON_PIN  3 

// Uncomment to override default HW configurations
//#define MY_DEFAULT_ERR_LED_PIN 4  // Error led pin
//#define MY_DEFAULT_RX_LED_PIN  6  // Receive led pin
//#define MY_DEFAULT_TX_LED_PIN  5  // the PCB, on board LED

#include <SPI.h>
#include <MySensors.h>  
#include <Bounce2.h>

// Enable repeater functionality for this node
#define MY_REPEATER_FEATURE

#define RELAY_1  20  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define BUTTON_PIN_1 21
#define NUMBER_OF_RELAYS 17 // Total number of attached relays

#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

Bounce debouncers[NUMBER_OF_RELAYS];
MyMessage myMessages[NUMBER_OF_RELAYS];

void before() {
  //uzywam pinow po jednej stronie
  for (int sensor=1, pin=RELAY_1; sensor<=NUMBER_OF_RELAYS; sensor++, pin+=2) {
    // Then set relay pins in output mode
    pinMode(pin, OUTPUT);   
    // Set relay to last known state (using eeprom storage) 
    digitalWrite(pin, loadState(sensor)?RELAY_ON:RELAY_OFF);
  }
}


void setup() { 
  // Setup locally attached sensors
  delay(10000);

//create table of debucers and attach to pin - piny po jednej stronie podwojnego slotu 
    for(int i=0, pin=BUTTON_PIN_1; i<NUMBER_OF_RELAYS; i++, pin+=2){
      debouncers[i] = Bounce();
      debouncers[i].attach(pin);
      debouncers[i].interval(5);
      pinMode(pin, INPUT_PULLUP);
      myMessages[i] = MyMessage(i+1,V_LIGHT);
    }
  presentation();
}
void presentation()  
{   
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Relay", "1.0");

  for (int sensor=1; sensor<=NUMBER_OF_RELAYS; sensor++) {
    // Register all sensors to gw (they will be created as child devices)
    present(sensor, S_LIGHT);
  }
}

void loop() { 
  // Send locally attached sensor data here 
  for(int i=0, pin=RELAY_1; i<NUMBER_OF_RELAYS; i++, pin+=2){
    if (debouncers[i].update()) {
      // Get the update value.
      int value = debouncers[i].read();
      // Send in the new value.
      if(value == HIGH){
           saveState(i+1, !loadState(i+1));
           digitalWrite(pin, loadState(i+1)?RELAY_ON:RELAY_OFF);
           send(myMessages[i].set(loadState(i+1)));
      }
    }
  }
}

void receive(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type==V_LIGHT) {
     int RELAY_TO_CHANGE = (message.sensor)*2-1+RELAY_1;
     // Change relay state
     digitalWrite(RELAY_TO_CHANGE, message.getBool()?RELAY_ON:RELAY_OFF);
     // Store state in eeprom
     saveState(message.sensor, message.getBool());
     // Write some debug info
     Serial.print("Incoming change for sensor:");
     Serial.print(message.sensor);
     Serial.print(", New status: ");
     Serial.println(message.getBool());
   } 
}




