// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable serial gateway
#define MY_GATEWAY_SERIAL

#include <SPI.h>
#include <MySensors.h>  
#include <Bounce2.h>
#include "DHT.h"
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

#include "LandBoards_Digio128V2.h"
LandBoards_Digio128V2 Dio128;


#define RELAY_1st  14  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define BUTTON_PIN_1st 15
#define NUMBER_OF_RELAYS 8 // Total number of attached relays
#define RELAY_ON HIGH // GPIO value to write to turn on attached relay
#define RELAY_OFF LOW // GPIO value to write to turn off attached relay

#define COVER_PIN_1st 0
#define COVER_COUNT 12

#define COVER_PRESS_TIME 100

#define GARAGE_GATE_PIN 53
#define GATE_CHILD_ID 0
#define MAIN_DOOR_PIN 52
#define MAIN_DOOR_CHILD_ID 1

#define GARAGE_TEMP_PIN 51
#define CHILD_ID_HUM 70
#define CHILD_ID_TEMP 71
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
DHT dht(GARAGE_TEMP_PIN, DHTTYPE);

MyMessage garageGate(GATE_CHILD_ID,V_TRIPPED);

Bounce mainDoorBounce;
MyMessage mainDoor(MAIN_DOOR_CHILD_ID,V_TRIPPED);

const long temp_interval = 1000; 
unsigned long previousMillis = 0;

Bounce deBouncers[NUMBER_OF_RELAYS];
MyMessage myMessages[NUMBER_OF_RELAYS];

int pinSensorRelay [COVER_COUNT][2]; 
Bounce deBouncersCovers[COVER_COUNT + COVER_COUNT];
MyMessage myMessagesCovers[COVER_COUNT + COVER_COUNT];

static const uint8_t FORCE_UPDATE_N_READS = 10;


void before() {

  for(int i=0, pin=RELAY_1st; i<NUMBER_OF_RELAYS; i++, pin+=2) {
    // Then set relay pins in output mode
    pinMode(pin, OUTPUT);   
    // Set relay to last known state (using eeprom storage) 
    digitalWrite(pin, loadState(pin)?RELAY_ON:RELAY_OFF);
  }

}

void setup() { 
  // Setup locally attached sensors
  delay(2000);
  Dio128.begin();
  dht.begin();

  Dio128.pinMode(127, OUTPUT); 

    // button for COVERers
    for (int i=0, pin=COVER_PIN_1st; i < COVER_COUNT * 2 ; i++, pin+=2) {
      // Then set relay pins in output mode
      Dio128.pinMode(pin, OUTPUT);
      Dio128.pinMode(pin+1, OUTPUT);

      Dio128.digitalWrite(pin, RELAY_OFF);
      Dio128.digitalWrite(pin+1, RELAY_OFF);
      // fill pin to sensor mapping
      pinSensorRelay[i][0] = pin; // reley pin UP
      pinSensorRelay[i][1] = pin+1; // relay pin DOWN
    }

    pinMode(GARAGE_GATE_PIN, INPUT);
    digitalWrite(GARAGE_GATE_PIN, HIGH);

  //create table of debucers and attach to pins, create table of messaages, use relay pin number as sensor id
    for(int i=0, pin_button=BUTTON_PIN_1st, pin_relay=RELAY_1st; i < NUMBER_OF_RELAYS; i++, pin_button+=2, pin_relay+=2){
      deBouncers[i] = Bounce();
      deBouncers[i].attach(pin_button);
      deBouncers[i].interval(5);
      pinMode(pin_button, INPUT_PULLUP);
      myMessages[i] = MyMessage(pin_relay,V_LIGHT);
    }

    mainDoorBounce = Bounce();
    mainDoorBounce.attach(MAIN_DOOR_PIN);
    mainDoorBounce.interval(5);
    pinMode(MAIN_DOOR_PIN, INPUT);
    digitalWrite(MAIN_DOOR_PIN, HIGH);

}

void loop() {
  delay(30); 
  // Send locally attached sensor data here, use relay pin number as sensor id
  for(int i=0, pin=RELAY_1st; i<NUMBER_OF_RELAYS; i++, pin+=2) {
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

  readGarageAndMainDoorSensors();


//    readTempAndHum();

  
}  
  
    

void presentation()  {   
  Serial.println("presentation");  
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Relay and Cover and temp", "2.0");
  
  for (int i=0, sensor=RELAY_1st; i < NUMBER_OF_RELAYS; i++, sensor+=2) {
    // Register all sensors to gw (they will be created as child devices) use relay pin number as sensor id
    present(sensor, S_LIGHT);
    delay(50); 
  }

  for (int j=1, sensor=COVER_PIN_1st; j <= COVER_COUNT; j++, sensor++) {
      // Register all sensors to gw (they will be created as child devices) use relay pin number as sensor id
    Serial.println("presentation S_COVER "); 
    Serial.println(j);   
    present(j, S_COVER);
    delay(50);
  }
 
  present(GATE_CHILD_ID, S_DOOR);
  delay(50);
  present(MAIN_DOOR_CHILD_ID, S_DOOR);
  delay(50);

  present(CHILD_ID_HUM, S_HUM, "HUM_GARAGE");
  delay(50);
  present(CHILD_ID_TEMP, S_TEMP, "TEMP_GARAGE");
  delay(50);

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
     Serial.print("Pin: ");
     Serial.print(message.sensor);

  }  

  if (message.type == V_UP) {
     // Change relay state
     //off down relay
     Dio128.digitalWrite(getRelayDownPin(message.sensor), RELAY_OFF);
     
     delay(COVER_PRESS_TIME); //opóźnienie 1s powoduje, że roleta najpierw się zatrzyma a potem przełączy się na podnoszenie
     Dio128.digitalWrite(getRelayUpPin(message.sensor), RELAY_ON);
     delay(COVER_PRESS_TIME); //opóźnienie 1s powoduje, że roleta najpierw się zatrzyma a potem przełączy się na podnoszenie
     Dio128.digitalWrite(getRelayUpPin(message.sensor), RELAY_OFF);

  } 

  if (message.type == V_DOWN) {
     // Change relay state
     Dio128.digitalWrite(getRelayUpPin(message.sensor), RELAY_OFF);
     delay(COVER_PRESS_TIME); //opóźnienie 1s powoduje, że roleta najpierw się zatrzyma a potem przełączy się na podnoszenie
     Dio128.digitalWrite(getRelayDownPin(message.sensor), RELAY_ON);
     delay(COVER_PRESS_TIME); //opóźnienie 1s powoduje, że roleta najpierw się zatrzyma a potem przełączy się na podnoszenie
     Dio128.digitalWrite(getRelayDownPin(message.sensor), RELAY_OFF);
  }

    if (message.type == V_STOP) {
     // Change relay state
     Dio128.digitalWrite(getRelayUpPin(message.sensor), RELAY_OFF);
     //delay(COVER_PRESS_TIME); //opóźnienie 1s powoduje, że roleta najpierw się zatrzyma a potem przełączy się na podnoszenie
     Dio128.digitalWrite(getRelayDownPin(message.sensor), RELAY_OFF);
     
  }

  
}

void readGarageAndMainDoorSensors(){
    uint8_t value;
    static uint8_t sentValue=2;
 
    // Short delay to allow buttons to properly settle
    value = digitalRead(GARAGE_GATE_PIN);

    if (value != sentValue) {
       Serial.println("kontracton value changed!!!!!: " + value);
       // Value has changed from last transmission, send the updated value
       send(garageGate.set(value==HIGH ? 1 : 0));
       sentValue = value;
    }
    
    if(mainDoorBounce.update()) {
      int mainDoorValue = mainDoorBounce.read();
      send(mainDoor.set(mainDoorValue==HIGH ? 1 : 0));
    }
}


int getRelayUpPin(int sensor) {
  return pinSensorRelay[sensor-1][0];
}

int getRelayDownPin(int sensor) {
  return pinSensorRelay[sensor-1][1];
}


float last_h = -1;
float last_t = -1;
float last_f = -1;

void readTempAndHum(){
    // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  } else if (last_h == h && last_t == t && last_f == f) {
    return;
  }

  last_h = h; last_t = t; last_f = f;

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hic);
  Serial.print(F("°C "));
  Serial.print(hif);
  Serial.println(F("°F"));

  send(msgTemp.set(hic,1));
  
  send(msgHum.set(h, 1));
  
}
