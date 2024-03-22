#include <Arduino.h>
#include <MppServer.h>
#include <MppDevices.h>
#include <ESP8266WiFi.h>

const char *DeviceVersion = "MppSR201 1.2.0"; // Based on 3waySwitch

const char *P_RESTART_MESSAGE = "RestartMessage";
const char *P_WIFI_MESSAGE = "WifiMessage";
const char *P_Controller_IP = "ControllerIP";
const char *P_Controller_Port = "ControllerPort";


// unique properties handled by MppMaker
static const char *properties[] = { //

        P_INITIAL, // relay start state
        P_CYCLE_RECOVERY, // restarts for wifi recovery mode (0 to disable)
        P_WIFI_RESTART, // restart if wifi not connected (minutes)
        P_Controller_IP,P_Controller_Port, // IP and POrt SR201
        NULL };

MppServer mppServer(DeviceVersion, properties);
class MppRelay *relays;

#define checkin 10000
unsigned long next = millis();
size_t sizeBuffer = 8; // 8 bytes size of reply buffer SR201
uint8_t bufferRx[8];
char command[2];

  WiFiClient client1;

bool refreshSR201state(void)  {
if (isWifiReady()) {
  if (client1.connect(mppServer.getProperty(P_Controller_IP), mppServer.getUnsignedProperty(P_Controller_Port))) //Try to connect to TCP Controller
     {
        Serial.println("Connected to Controller... ");
        command[0]=0x30; command[1]=0x30; // Send 00h or 30 30 in ASCII to controller for getting state
        client1.write((uint8_t *)command, sizeof(command)); // command for updating the status
        client1.flush();
    while(client1.available()){
      unsigned a =client1.read(bufferRx,sizeBuffer); // reading register status 
         Serial.printf("Bytes from controller:%c %c %c %c %c %c %c %c, bytes read total:%d\n",bufferRx[0],bufferRx[1],bufferRx[2],bufferRx[3],bufferRx[4],bufferRx[5],bufferRx[6],bufferRx[7], a);
        client1.stop();  
      }
      return true;
     }
     else   {
         Serial.println("connection to controller failed ... "); 
         return false;
     }
    }
     else   {
         Serial.println("No network connection... "); 
         return false;
     }
 }   
  
class SR201: public MppDevice {
public:
  
  SR201(unsigned relayPin,const char *arelayName) {  
    this->relayPin= relayPin;
     relayName= arelayName;
     Serial.printf("Added MppRelay on pin %d\n", relayPin);
 //    Serial.printf(" Is set to : %s\n",mppServer.isProperty(relayName.c_str())? "true" : "false");
               sRelay(mppServer.isProperty(relayName.c_str()));
  }

void sRelay(bool state) {
  refreshSR201state();
     mppServer.putProperty(relayName.c_str(),state ? "true" : "false");
     digitalWrite(relayPin, state ? HIGH : LOW);
      pinMode(relayPin, OUTPUT); // defer setting to output until state ready
      put(STATE, state ? "on" : "off");
     String rel=relayName.substring(relayName.lastIndexOf("_")+1);  
//     Serial.printf("MppRelay on pin %d is :%s Relay name:%s Relay number:%d bufferRx[0]:%d\n",relayPin, state ? "true" : "false", relayName.c_str(),rel.toInt(),bufferRx[0]);
    changeSR201state(rel.toInt(),state);
}

bool getState() {
  Serial.printf("Relay Pin:%d  \n",relayPin);
  bool state = digitalRead(relayPin);
    return state;
}

void toggleRelay() {
  bool state = digitalRead(relayPin);
  state = !state;
  sRelay(state);
}

 
private:
 unsigned relayPin;
 String relayName;

bool handleAction(String action, MppParameters parms) {
                boolean handled = false;
                if (action == "state") {
                              if (parms.hasParameter("state")) {
                                             sRelay(parms.getBoolParameter("state"));
                                             Serial.printf("Action state handled state: %s\n", parms.getBoolParameter("state") ? "true" : "false");
                                             handled = true;
                              } else if (parms.hasParameter("toggle")) {
                                                toggleRelay();
                                               Serial.printf("Action toggle handled state: %s\n", getState() ? "true" : "false");
                                             handled = true;
                              }
               }
                return handled ? true : MppDevice::handleAction(action, parms);
}
/*
  void handleDevice(unsigned long now) {
    // check and handle any interrupt

    MppDevice::handleDevice(now);
    }*/

  bool changeSR201state(int relnum, bool state)  {
    if (isWifiReady()) {
  if (client1.connect(mppServer.getProperty(P_Controller_IP), mppServer.getUnsignedProperty(P_Controller_Port))) //Try to connect to TCP Controller
     {
        Serial.println("Connected to Controller... ");
         Serial.printf("Current relay #%d state:%d command for controller %d%d buffer:%d \n",relnum, state, command[0], command[1],bufferRx[relnum-1]);

       if( bufferRx[relnum-1]==48 && state==1  ) {   // 48 is decimal for printable 0 and hex 30
                command[0]=0x31; command[1]=(48+relnum); //hex: 31- command 1 (short)48+relnum -relay number (1,2,3...)
          Serial.printf("+ Current relay #%d state:%d command for controller %d%d realcommand=%d\n",relnum, state, command[0], command[1],(48+relnum));
          client1.write((uint8_t *)command, sizeof(command)); // command for updating the status
          client1.flush();
     }
     if(bufferRx[relnum-1]==49 && state!=1  ) { //49 is decimal printable 1 and hex 31
                command[0]=0x32; command[1]=48+relnum; // 
          Serial.printf("Current relay #%d state:%d command for controller %d%d\n",relnum, state, command[0], command[1]);
          client1.write((uint8_t *)command, sizeof(command)); // command for updating the status
          client1.flush();
          return true;
      }

    }  else   {
         Serial.println("connection to controller failed ... "); 
         return false;
     }
     client1.stop(); 
     return true;
    } else {
      Serial.println("No network connection... "); 
      return false;
    }
}

 
  } *relay1,*relay2,*relay3,*relay4,*relay5,*relay6,*relay7,*relay8;


   
void setup() {
    String BATH_HEAT=(getDefaultUDN(MppSwitch)+"_1");
    String WATER_PUMP=(getDefaultUDN(MppSwitch)+"_2");
    String WATER_BOIL=(getDefaultUDN(MppSwitch)+"_3");
    String MAIN_HALL=(getDefaultUDN(MppSwitch)+"_4");
    String OUSIDE_LIGHTS=(getDefaultUDN(MppSwitch)+"_5");
    String BEDROOM=(getDefaultUDN(MppSwitch)+"_6");
    String RESERVED_1=(getDefaultUDN(MppSwitch)+"_7");
    String RESERVED_2=(getDefaultUDN(MppSwitch)+"_8");
   
     class SR201 *relay1 = new class SR201(5,BATH_HEAT.c_str());
    mppServer.manageDevice(relay1,BATH_HEAT);
    class SR201 *relay2 = new class SR201(4,WATER_PUMP.c_str());
    mppServer.manageDevice(relay2,WATER_PUMP);
    class SR201 *relay3 = new class SR201(0,WATER_BOIL.c_str());
    mppServer.manageDevice(relay3,WATER_BOIL);
     class SR201 *relay4 = new class SR201(2,MAIN_HALL.c_str());
    mppServer.manageDevice(relay4,MAIN_HALL);
     class SR201 *relay5 = new class SR201(14,OUSIDE_LIGHTS.c_str());
    mppServer.manageDevice(relay5,OUSIDE_LIGHTS);
     class SR201 *relay6 = new class SR201(12,BEDROOM.c_str());
    mppServer.manageDevice(relay6,BEDROOM);
     class SR201 *relay7 = new class SR201(13,RESERVED_1.c_str());
    mppServer.manageDevice(relay7,RESERVED_1);
     class SR201 *relay8 = new class SR201(15,RESERVED_2.c_str());
    mppServer.manageDevice(relay8,RESERVED_2);
    
//Relay setup
 mppServer.setPropertyDefault(P_Controller_IP, "192.168.1.100");
 mppServer.setPropertyDefault(P_Controller_Port, "6722");
  // start the web and mpp server
  mppServer.begin();

}

// The loop function is called in an endless loop
void loop() {

  
  mppServer.handleClients(); // let the server handle any incoming requests
  mppServer.handleCommand(); // optional, handle user Serial input

  
  unsigned long now = millis();

  if (now > next) {
//    refreshSR201state();
    MppSerial.printf("heap=%d at %lus\n", ESP.getFreeHeap(), now / 1000);
    next = now + checkin;
  }
}
