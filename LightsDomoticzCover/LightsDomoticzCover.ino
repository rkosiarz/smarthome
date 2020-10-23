// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable serial gateway
#define MY_GATEWAY_SERIAL

#include <SPI.h>
#include <MySensors.h>  
#include <Bounce2.h>


#define RELAY_1st  48  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define BUTTON_PIN_1st 49
#define NUMBER_OF_RELAYS 2 // Total number of attached relays
#define RELAY_ON HIGH // GPIO value to write to turn on attached relay
#define RELAY_OFF LOW // GPIO value to write to turn off attached relay

#define COVER_PIN_1st 22
#define COVER_COUNT 11



const long temp_interval = 1000; 
unsigned long previousMillis = 0;

Bounce deBouncers[NUMBER_OF_RELAYS];
MyMessage myMessages[NUMBER_OF_RELAYS];

//int pinSensorRelay [10][2] = { {1, 22, 23}, {2, 24, 25} ;

int pinSensorRelay [COVER_COUNT][2]; 
Bounce deBouncersCovers[COVER_COUNT + COVER_COUNT];
MyMessage myMessagesCovers[COVER_COUNT + COVER_COUNT];

static const uint8_t FORCE_UPDATE_N_READS = 10;


void before() {
  // button for COVERers
    for (int i=0, pin=COVER_PIN_1st; i < COVER_COUNT * 2 ; i++, pin+=2) {
      // Then set relay pins in output mode
      pinMode(pin, OUTPUT);
      Serial.print("pin: ");
      Serial.println(pin);
      digitalWrite(pin, RELAY_OFF);
      digitalWrite(pin+1, RELAY_OFF);
      // fill pin to sensor mapping
//      pinSensor[i] = pin;  //index is sensor id
      pinSensorRelay[i][0] = pin; // reley pin UP
      pinSensorRelay[i][1] = pin+1; // relay pin DOWN
    }

    digitalWrite(52, HIGH);
    digitalWrite(53, LOW);
}

void setup() { 
  // Setup locally attached sensors
  delay(1000);

  //create table of debucers and attach to pins, create table of messaages, use relay pin number as sensor id
    for(int i=0, pin_button=BUTTON_PIN_1st, pin_relay=RELAY_1st; i < NUMBER_OF_RELAYS; i++, pin_button+=2, pin_relay+=2){
      deBouncers[i] = Bounce();
      deBouncers[i].attach(pin_button);
      deBouncers[i].interval(5);
      pinMode(pin_button, INPUT_PULLUP);
      myMessages[i] = MyMessage(pin_relay,V_LIGHT);
    }



}

void loop() { 
  // Send locally attached sensor data here, use relay pin number as sensor id
  for(int i=0, pin=RELAY_1st; i<NUMBER_OF_RELAYS; i++, pin+=2) {
    if (deBouncers[i].update()) {
      // Get the update value.
      int value = deBouncers[i].read();
      Serial.print("loop: " + value);
      // Send in the new value.
      if(value == LOW){
           saveState(pin, !loadState(pin));
           digitalWrite(pin, loadState(pin)?RELAY_ON:RELAY_OFF);
           send(myMessages[i].set(loadState(pin)));
      }
    }
  }


    

//  for (int i=0, pin=COVER_PIN_1st; i < COVER_COUNT * 2 ; pin++, i++) {
//    if (deBouncersCovers[i].update()) {
//      // Get the update value.
//      int value = deBouncersCovers[i].read();
//      Serial.print("loop: " + value);
//      // Send in the new value.
//      if(value == LOW){
//           saveState(pin, !loadState(pin));
//           digitalWrite(pin, loadState(pin)?RELAY_ON:RELAY_OFF);
//           send(myMessagesCovers[i].set(loadState(pin)));
//      }
//    }
//  }

  
}  
  
    

void presentation()  {   
  Serial.println("presentation");  
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Relay and Cover", "1.0");
  for (int i=0, sensor=RELAY_1st; i < NUMBER_OF_RELAYS; i++, sensor+=2) {
    // Register all sensors to gw (they will be created as child devices) use relay pin number as sensor id
    present(sensor, S_LIGHT);
  }
//  present(51, S_LIGHT);

  for (int j=1, sensor=COVER_PIN_1st; j <= COVER_COUNT; j++, sensor++) {
//    wait(1000);
      // Register all sensors to gw (they will be created as child devices) use relay pin number as sensor id
    Serial.println("presentation S_COVER "); 
    Serial.println(j);   
    present(j, S_COVER);
  }
 


}

void receive(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  Serial.print("Message type: "); 
  Serial.println(message.type);
  Serial.print("Message sesnsor: ");
  Serial.println(message.sensor);
  Serial.print("Pin: ");
  Serial.println(getRelayUpPin(message.sensor));
  Serial.println(getRelayDownPin(message.sensor));
  Serial.print("Bool: ");
  Serial.println(message.getBool());
  
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

  if (message.type == V_UP) {
     // Change relay state
     //off down relay
     digitalWrite(getRelayDownPin(message.sensor), RELAY_OFF);
     
     delay(500); //opóźnienie 1s powoduje, że roleta najpierw się zatrzyma a potem przełączy się na podnoszenie
     digitalWrite(getRelayUpPin(message.sensor), RELAY_ON);
     delay(500); //opóźnienie 1s powoduje, że roleta najpierw się zatrzyma a potem przełączy się na podnoszenie
     digitalWrite(getRelayUpPin(message.sensor), RELAY_OFF);

  } 

  if (message.type == V_DOWN) {
     // Change relay state
     digitalWrite(getRelayUpPin(message.sensor), RELAY_OFF);
     delay(500); //opóźnienie 1s powoduje, że roleta najpierw się zatrzyma a potem przełączy się na podnoszenie
     digitalWrite(getRelayDownPin(message.sensor), RELAY_ON);
     delay(500); //opóźnienie 1s powoduje, że roleta najpierw się zatrzyma a potem przełączy się na podnoszenie
     digitalWrite(getRelayDownPin(message.sensor), RELAY_OFF);
  }

    if (message.type == V_STOP) {
     // Change relay state
     digitalWrite(getRelayUpPin(message.sensor), RELAY_OFF);
     //delay(500); //opóźnienie 1s powoduje, że roleta najpierw się zatrzyma a potem przełączy się na podnoszenie
     digitalWrite(getRelayDownPin(message.sensor), RELAY_OFF);
     
  }

  
}



int getRelayUpPin(int sensor) {
  return pinSensorRelay[sensor-1][0];
}

int getRelayDownPin(int sensor) {
  return pinSensorRelay[sensor-1][1];
}
