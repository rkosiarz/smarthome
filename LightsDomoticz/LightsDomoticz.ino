// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable serial gateway
#define MY_GATEWAY_SERIAL

#include <SPI.h>
#include <MySensors.h>  
#include <Bounce2.h>

#define RELAY_1  22  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define BUTTON_PIN_1 23
#define NUMBER_OF_RELAYS 16 // Total number of attached relays
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

Bounce deBouncers[NUMBER_OF_RELAYS];
MyMessage myMessages[NUMBER_OF_RELAYS];

void before() {
  //use even numbers for relay pins and use it as key to load state
  for (int i=1, pin=RELAY_1; i<=NUMBER_OF_RELAYS; i++, pin+=2) {
    // Then set relay pins in output mode
    pinMode(pin, OUTPUT);   
    // Set relay to last known state (using eeprom storage) 
    digitalWrite(pin, loadState(pin)?RELAY_ON:RELAY_OFF);
  }
}

void setup() { 
  // Setup locally attached sensors
  delay(5000);

//create table of debucers and attach to pin - piny po jednej stronie podwojnego slotu 
    for(int i=0, pin=BUTTON_PIN_1; i<NUMBER_OF_RELAYS; i++, pin+=2){
      deBouncers[i] = Bounce();
      deBouncers[i].attach(pin);
      deBouncers[i].interval(5);
      pinMode(pin, INPUT_PULLUP);
      myMessages[i] = MyMessage(i+1,V_LIGHT);
    }
  presentation();
}

void loop() { 
  // Send locally attached sensor data here, use relay pin number as sensor id
  for(int i=0, pin=RELAY_1; i<NUMBER_OF_RELAYS; i++, pin+=2){
    if (deBouncers[i].update()) {
      // Get the update value.
      int value = deBouncers[i].read();
      // Send in the new value.
      if(value == LOW){
           saveState(pin, !loadState(pin));
           digitalWrite(pin, loadState(pin)?RELAY_ON:RELAY_OFF);
           send(myMessages[i].set(loadState(pin)));
      }
    }
  }
}

void presentation()  
{   
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Relay", "1.0");
  for (int i=1, pin=RELAY_1; i<=NUMBER_OF_RELAYS; i++, pin+=2) {
    // Register all sensors to gw (they will be created as child devices) use relay pin number as sensor id
    present(pin, S_LIGHT);
  }
}

void receive(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type==V_LIGHT) {
     // Change relay state
     digitalWrite(message.sensor, message.getBool()?RELAY_ON:RELAY_OFF);
     // Store state in eeprom
     saveState(message.sensor, message.getBool());
     // Write some debug info
     Serial.print("Incoming change for sensor:");
     Serial.print(message.sensor);
     Serial.print(", New status: ");
     Serial.println(message.getBool());
   } 
}




