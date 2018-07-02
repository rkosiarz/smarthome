// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable serial gateway
#define MY_GATEWAY_SERIAL

#include <SPI.h>
#include <MySensors.h>  
#include <Bounce2.h>
#include <DHT.h>

#define RELAY_1  22  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define BUTTON_PIN_1 23
#define NUMBER_OF_RELAYS 16 // Total number of attached relays
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

#define NUMBER_OF_TEMP_SENSORS 16
#define TEMP_SENSOR_PIN_1 64

#define NUMBER_OF_MOTION_SENSORS 16
#define MOTION_SENSOR_PIN_1 96


const long temp_interval = 1000; 
unsigned long previousMillis = 0;

Bounce deBouncers[NUMBER_OF_RELAYS];
MyMessage myMessages[NUMBER_OF_RELAYS];
MyMessage msgHums[NUMBER_OF_TEMP_SENSORS];
MyMessage msgTemps[NUMBER_OF_TEMP_SENSORS];
DHT dhts[NUMBER_OF_TEMP_SENSORS];
MyMessage msgMotions[NUMBER_OF_MOTION_SENSORS];
static const uint8_t FORCE_UPDATE_N_READS = 10;


void before() {
  //use even numbers for relay pins and use it as key to load state
  for (int i=1, pin=RELAY_1; i<=NUMBER_OF_RELAYS; i++, pin+=2) {
    // Then set relay pins in output mode
    pinMode(pin, OUTPUT);   
    // Set relay to last known state (using eeprom storage) 
    digitalWrite(pin, loadState(pin)?RELAY_ON:RELAY_OFF);
  }

  //setup dht sensors
  for (int i=1, pin=TEMP_SENSOR_PIN_1; i<=NUMBER_OF_TEMP_SENSORS; i++, pin++) {
    dhts[i].setup(i);
  }

//to change
  //sleep(dht.getMinimumSamplingPeriod());
}

void setup() { 
  // Setup locally attached sensors
  delay(5000);

  //create table of debucers and attach to pins, create table of messaages, use relay pin number as sensor id
    for(int i=0, pin_button=BUTTON_PIN_1, pin_relay=RELAY_1; i<NUMBER_OF_RELAYS; i++, pin_button+=2, pin_relay+=2){
      deBouncers[i] = Bounce();
      deBouncers[i].attach(pin_button);
      deBouncers[i].interval(5);
      pinMode(pin_button, INPUT_PULLUP);
      myMessages[i] = MyMessage(pin_relay,V_LIGHT);
    }

    for(int i=0, pin_motion=MOTION_SENSOR_PIN_1; i<NUMBER_OF_MOTION_SENSORS; i++, pin_motion++) {
      pinMode(pin_motion, INPUT);
      msgMotions[i] = MyMessage (pin_motion, V_TRIPPED);
    }
  presentation();
}

void loop() { 
  // Send locally attached sensor data here, use relay pin number as sensor id
  for(int i=0, pin=RELAY_1; i<NUMBER_OF_RELAYS; i++, pin+=2) {
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

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= temp_interval) {
    previousMillis = currentMillis;
    for (int i=0; i<NUMBER_OF_TEMP_SENSORS; i++) {
      // Force reading sensor, so it works also after sleep()
      dhts[i].readSensor(true);
      // Get temperature from DHT library
      float temperature = dhts[i].getTemperature();
      if (isnan(temperature)) {
        Serial.println("Failed reading temperature from DHT!");
      } else {
        send(msgTemps[i].set(temperature, 1));
        
        #ifdef MY_DEBUG
        Serial.print("T: ");
        Serial.println(temperature);
        #endif
    }
    
      // Get humidity from DHT library
      float humidity = dhts[i].getHumidity();
      if (isnan(humidity)) {
        Serial.println("Failed reading humidity from DHT");
      } else {
        send(msgHums[i].set(humidity, 1));
    
        #ifdef MY_DEBUG
        Serial.print("H: ");
        Serial.println(humidity);
        #endif
      } 
    }
  }

 
  for (int i=0, pin_motion=MOTION_SENSOR_PIN_1; i<NUMBER_OF_MOTION_SENSORS; i++, pin_motion++) {
    //read motion 
    bool tripped = digitalRead(pin_motion) == HIGH;

    //send message about movement
    send(msgMotions[i].set(tripped?"1":"0"));  // Send tripped value to gw
    
  }

  
}  
  
    

void presentation()  
{   
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Relay", "1.0");
  for (int i=1, sensor=RELAY_1; i<=NUMBER_OF_RELAYS; i++, sensor+=2) {
    // Register all sensors to gw (they will be created as child devices) use relay pin number as sensor id
    present(sensor, S_LIGHT);
  }

  //pin as sensor to know where each sensor is connected.
  for (int i=1, sensor=TEMP_SENSOR_PIN_1; i<=NUMBER_OF_TEMP_SENSORS; i++, sensor+=2) {
    present(/*CHILD_ID_HUM*/sensor, S_HUM);
    present(/*CHILD_ID_TEMP*/sensor+1, S_TEMP);
  }

  //pin as sensor to know where each sensor is connected.
  for (int i=1, sensor=MOTION_SENSOR_PIN_1; i<=NUMBER_OF_MOTION_SENSORS; i++, sensor++) {
    present(sensor, S_MOTION);
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




