//  BLE Beacon Scanner Example Sketch
//  Programming Electronics Academy
#include <BLEDevice.h>            // sets up BLE device constructs
#include <BLEUtils.h>             // various BLE utilities for processing BLE data
#include <BLEScan.h>              // contains BLE scanning functions
#include <BLEAdvertisedDevice.h>  // contains BLE device characteristic data
int scanTime = 1; //In seconds
int cooldown = 0; //Cooldown in seconds
int state;
bool layneAtDoor = false;
bool jacobAtDoor = false;
bool connorAtDoor = false;
BLEScan* pBLEScan;
#define IR_PIN 13
#define LAYNE_TAG 1
#define JACOB_TAG 2
#define CONNOR_TAG 3

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks 
{
    void onResult(BLEAdvertisedDevice advertisedDevice) 
    {
      String strName;
      strName = advertisedDevice.getName().c_str();
      //Serial.printf("Name: %s \n", advertisedDevice.getName().c_str());
      //Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());

      if (!advertisedDevice.haveManufacturerData()) return;

      String mfg = advertisedDevice.getManufacturerData();
      if (mfg.length() != 25) return;

      const uint8_t* d = (const uint8_t*)mfg.c_str();

      // iBeacon prefix: 4C 00 02 15
      if (d[0] != 0x4C || d[1] != 0x00 || d[2] != 0x02 || d[3] != 0x15) return;

      // Print UUID
      Serial.print("UUID: ");
      for (int i = 4; i < 20; i++) {
        if (i == 6 || i == 8 || i == 10 || i == 12) Serial.print("-");
        if (d[i] < 16) Serial.print("0");
        Serial.print(d[i], HEX);
      }

      Serial.println("");

      uint16_t major = (d[20] << 8) | d[21];
      uint16_t minor = (d[22] << 8) | d[23];
      int rssi = advertisedDevice.getRSSI();
      int txPower = -59;

      double exp = ((double)txPower-(double)rssi)/(20.0);

      double distance = pow(10, exp);
      

      Serial.printf("Major: %d  Minor: %d RSSI: %d  Tx Power: %d dBm  Distance: %lf m\n", major, minor, rssi, txPower, distance);

      //TODO: CHECK IF COOLDOWN WORKS AS EXPECTED. IT SHOULD SINCE THE SCAN EXECUTES FIRST BEFORE COOLDOWN IS SET?
      /*if((state == HIGH)&&(cooldown <= 0)){//Door open
        if(distance<5.0){
          if(atDoor==false){ //TODO: CHANGE FROM GLOBAL ATDOOR
            Serial.println("User is at door. Play Sound");
            if(major == 1){
              layneAtDoor = true;
            }
          }
          atDoor = true;
        }
      }else{//Door closed
        atDoor = false;
      }*/

      if((distance<5.0) && (cooldown <= 0)){
        if(major == 1){
          layneAtDoor = true;
        }
        if(major == 2){
          jacobAtDoor = true;
        }
        if(major == 3){
          connorAtDoor = true;
        }
      }else{
        if(major == 1){
          layneAtDoor = false;
        }
        if(major == 2){
          jacobAtDoor = false;
        }
        if(major == 3){
          connorAtDoor = false;
        }
      }

    }
};
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, -1, 17); // RX unused, TX = 17
  pinMode(IR_PIN, INPUT); 
  Serial.println("Scanning...");
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
}
void loop() {
  // put your main code here, to run repeatedly:
  state = digitalRead(IR_PIN);

  if (state == LOW) {
    Serial.println("Door closed");
  } else {
    Serial.println("Door open");

  }


  if ((cooldown == 1)&&(state == HIGH)) { //if door still open at end of cooldown
    cooldown = 10*(1.0/((double)scanTime)); //set cooldown in seconds
  } else if(cooldown>0){
    cooldown--;
  }

  BLEScanResults * foundDevices = pBLEScan->start(scanTime, false); //Thought: scan only when door open??? Will test feasibility later

  if (state == HIGH) {
    if(layneAtDoor == true){
      Serial2.write('1');//TODO: change activation phrase later
      cooldown = 10*(1.0/((double)scanTime)); //set cooldown in seconds
      layneAtDoor = false;
      jacobAtDoor = false;
      connorAtDoor = false;
    }else if(jacobAtDoor == true){
      Serial2.write('2');
      cooldown = 10*(1.0/((double)scanTime)); //set cooldown in seconds
      layneAtDoor = false;
      jacobAtDoor = false;
      connorAtDoor = false;
    }else if(connorAtDoor == true){
      Serial2.write('3');
      cooldown = 10*(1.0/((double)scanTime)); //set cooldown in seconds
      layneAtDoor = false;
      jacobAtDoor = false;
      connorAtDoor = false;
    }else if(cooldown <= 0){
      Serial2.write('x');
      cooldown = 10*(1.0/((double)scanTime)); //set cooldown in seconds
    }
  }


  //Serial.print("Devices found: ");
  //Serial.println(foundDevices->getCount());
  Serial.println("Scan done!");
  //Serial2.write('b');
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  delay(10);
}