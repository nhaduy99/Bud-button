#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson
#include <DoubleResetDetector.h>  //https://github.com/datacute/DoubleResetDetector

void blink_control(char interval, char repeat_time);
void blink_signal();

// Number of seconds after reset during which a 
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 10

// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
  
//ESP8266WebServer server(80);
//define your default values here, if there are different values in config.json, they are overwritten.

#define TRIGGER_PIN  4
#define SMALL_LEDS   13
#define BIG_LED     15
const char* host_read;
const char* port_read;
const char* token_read;
const char* path_read;

char token[80];
char host[120];
char port[8]; 
char path[60] ;

String x = "";

char flag =0;
char previous_button_state = 0; // 0: not push - 1: pushing

unsigned int count = 0;
//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void setup() {
  Serial.begin(115200);
  Serial.println();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SMALL_LEDS, OUTPUT);
  pinMode(BIG_LED, OUTPUT);
  pinMode(TRIGGER_PIN, INPUT_PULLUP);
  pinMode(A0, INPUT);                 //Check the battery status
 
  digitalWrite(LED_BUILTIN,1);
  digitalWrite(SMALL_LEDS, 0);
  digitalWrite(BIG_LED, 0);


   //------------------Reset when pushing the button at first-------------------------
       while(digitalRead(TRIGGER_PIN) ==0)
        {
          Serial.println("CLICKED>>>");       //when reseting, the button is pushed, it will reset all SSID & password, restaurant_ID and table_ID
          for(int i=0;i<10;i++)               // top LED blinks 10 times fast.
          {
            digitalWrite(BIG_LED,1);
            delay(100);
            digitalWrite(BIG_LED,0);
            delay(100);
          }
         
            WiFiManager wifiManager;
          //reset saved settings
             wifiManager.resetSettings();   // Clear only SSID & pass
            // SPIFFS.format();             // Clear only restaurant_ID & table ID
            // delay(500);  
        }
  //---- put your setup code here, to run once:----

    while(analogRead(A0)<820)
    {
          digitalWrite(BIG_LED,1);
          digitalWrite(SMALL_LEDS,1);
          delay(300);
          digitalWrite(BIG_LED,0);
          digitalWrite(SMALL_LEDS,0);
          delay(300);
          Serial.println(analogRead(A0));
    }


 //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
  //  SPIFFS.format();
    
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(token, json["token"]);
          strcpy(host, json["host"]);
          strcpy(port, json["port"]);
          strcpy(path, json["path"]);
          
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read
  Serial.println("Unclicked .............");
  delay (300);

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length

  
  WiFiManagerParameter  custom_token("token","token",token, 80);
  WiFiManagerParameter  custom_host("host","host",host, 120);
  WiFiManagerParameter  custom_port("port","port",port, 8);
  WiFiManagerParameter  custom_path("path","path",path, 60);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  digitalWrite(BIG_LED,1); // Signal of not access to wifi yet
//  wifiManager.autoConnect("AutoConnectAP222");
     //reset saved settings
//    wifiManager.resetSettings();

//set custom ip for portal
    wifiManager.setAPStaticIPConfig(IPAddress(10,0,0,1), IPAddress(10,0,0,1), IPAddress(255,255,255,0));

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
//  wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));
  
  //add all your parameters here

//  wifiManager.addParameter(&custom_table_ID);
//  wifiManager.addParameter(&custom_restaurant_ID);
  

  wifiManager.addParameter(&custom_host);
  wifiManager.addParameter(&custom_path);
  wifiManager.addParameter(&custom_port);
  wifiManager.addParameter(&custom_token);
  

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality(30);      // Show Wifi signal at least 30%
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);  

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("Budweiser_Button")) {
    Serial.println("failed to connect and hit timeout");
    delay(5000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(2000);

}
    
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
   for(int i=0;i<3;i++)
        {
          digitalWrite(BIG_LED,1);
          delay(300);
          digitalWrite(BIG_LED,0);
          delay(300);
        }
  digitalWrite(LED_BUILTIN,0);
  //read updated parameters
  
   
   strcpy(host, custom_host.getValue());
   strcpy(port, custom_port.getValue());
   strcpy(token, custom_token.getValue());
   strcpy(path, custom_path.getValue());
    
  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();

    json["host"] = host;
    json["port"] = port;
    json["token"] = token;
    json["path"] = path;
    
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  FS_data_read();
  
}

void loop() {
  
  if(flag==1)
  {
    blink_signal();  
  }
  if (flag==0){
    if (digitalRead(TRIGGER_PIN)==0)
    {
      delay(50);
      if (digitalRead(TRIGGER_PIN)==0){
         digitalWrite(SMALL_LEDS,1);
         digitalWrite(LED_BUILTIN,1);
         digitalWrite(BIG_LED,1);
          Data(); //sending json
          digitalWrite(SMALL_LEDS,0);
         digitalWrite(LED_BUILTIN,0);
         digitalWrite(BIG_LED,0);
      }
    
    }  
  }
  

}


//-------------SENDING SENSOR DATA TO SERVER--------------
void FS_data_read()
{
         
 File configFile = SPIFFS.open("/config.json", "r");   
          
        WiFiManagerParameter  custom_token("token","token",token, 80);
        WiFiManagerParameter  custom_host("host","host",host, 120);
        WiFiManagerParameter  custom_port("port","port",port, 8);
        WiFiManagerParameter  custom_path("path","path",path, 60);
           if (!configFile) {
            Serial.println("failed to open config file for writing");
          }
          else{
            Serial.println("reading config file");
            String s=configFile.readStringUntil('\n');
            Serial.print("CONTENT");
            Serial.print(":");
            Serial.println(s);

            token_read = custom_token.getValue();
            Serial.print("Token: ");
            Serial.println(token_read);

            
            Serial.print("Chip_ID: ");
            Serial.println(ESP.getChipId());
            Serial.println("");
            
            host_read = custom_host.getValue();
            Serial.print("host: ");
            Serial.println(host_read);
            
            port_read = custom_port.getValue();
            Serial.print("port: ");
            Serial.println(port_read);

             
            path_read = custom_path.getValue();
            Serial.print("path: ");
            Serial.println(path_read);
         
            
          }      
}
void Data(){
   flag=1;
  Serial.print("connecting to ");
  Serial.println(host_read);
 
  const char* host_new = host_read;
   
  if(strstr(host_new,"https"))
  {
    Serial.println("there is : 'https'");
    for(int i =8 ; i<strlen(host_new);i++) 
    {
      x = x+ host_new[i];
     }
  
   Serial.print("host after cutting: ");
   Serial.println(x);
   sending_frame_2(x.c_str(),atoi(port_read),path_read, token_read,ESP.getChipId());
   x = "";
  }
  else
  {
    
      for(int i =7 ; i<strlen(host_new);i++) 
      {
        x = x+ host_new[i];
       }
       Serial.print("host after cutting: ");
      Serial.println(x);
      sending_frame_1(x.c_str(),atoi(port_read),path_read, token_read,ESP.getChipId());
      x = "";
  }

  

}
void sending_frame_2(const char* host,int port, const char* path_read, const char* token, int ID)
{
   WiFiClientSecure client;
  if (!client.connect(host, port)) {   
    Serial.println("connection failed");
       blink_control(500,3);
       flag = 0;
    return;
  }
  

  String data = "device_id=" + String(ID)+
                "&token=" + String(token);
  
  
//  Serial.println("Frame send to server: POST " + url + " HTTP/1.1");
               
  client.print("POST " + String(path_read) + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Accept: *" + "/" + "*\r\n" +
               "Authorization: " + "Token 06a1d68d-6f8f-4de4-9d8d-294c38ce543c" + "\r\n" +
               "Content-Length: " + data.length() + "\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" + "\r\n" + data);



 //--------CHECKING THE RESPONSE FROM THE SERVER---------------
      
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
//  digitalWrite(BIG_LED,0);
   Serial.println("<------------Begining of receiving frame from server------------->");  
 //---------EXPORT SERVER'S RESPONSE TO EXAMINE------------------ 
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
 //   String line = client.readString();
   Serial.print(line);
  if(line=="HTTP/1.1 200 OK")
    {
      Serial.println("OKKKK");
      flag=1;
      break;
    }
    else
    {
      blink_control(500,3);
      flag = 0;
      break;
    }
  }
  Serial.println();
  Serial.println("<------------Ending of receiving frame from server------------->");
  Serial.println("closing connection");
//  delay(1000);
  
}
void sending_frame_1(const char* host,int port, const char* path_read, const char* token, int ID)
{
  WiFiClient client;
  if (!client.connect(host, port)) {   
    Serial.println("connection failed");
    blink_control(500,3);
    flag = 0;
    return;
  }
  
  
  String data = "device_id=" + String(ID)+
                "&token=" + String(token);
  
  
  client.print("POST " + String(path_read) + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Accept: *" + "/" + "*\r\n" +
               "Authorization: " + "Token 06a1d68d-6f8f-4de4-9d8d-294c38ce543c" + "\r\n" +
               "Content-Length: " + data.length() + "\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" + "\r\n" + data);



 //--------CHECKING THE RESPONSE FROM THE SERVER---------------
      
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
//  digitalWrite(BIG_LED,0);
   Serial.println("<------------Begining of receiving frame from server------------->");  
 //---------EXPORT SERVER'S RESPONSE TO EXAMINE------------------ 
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
 //   String line = client.readString();
    Serial.print(line);
    if(line=="HTTP/1.1 200 OK")
    {
      Serial.println("OKKKK");
      flag=1;
      break;
    }
    else
    {
      blink_control(500,3);
      flag = 0;
      break;
    }
  
  }
  Serial.println();
  Serial.println("<------------Ending of receiving frame from server------------->");
  Serial.println("closing connection");
//  delay(1000);
  
}
void blink_signal()
{
  blink_control(100,1);
 
  if(digitalRead(TRIGGER_PIN)==0)
  {
   count++;
      Serial.println (count);
      if(count>10)
      {
        flag = 0;
        Serial.println("RESET THE LIGHT");
        count = 0;
        delay(1000);   
        Serial.println("Wait for next PUSH"); 
      }
  }
}
 void blink_control(char interval, char repeat_time)
{
   for (char i=0; i<repeat_time; i++)
   {
      digitalWrite(SMALL_LEDS,1);
      digitalWrite(LED_BUILTIN,1);
      digitalWrite(BIG_LED,1);
      delay(interval);
      digitalWrite(SMALL_LEDS,0);
      digitalWrite(LED_BUILTIN,0);
      digitalWrite(BIG_LED,0);
      delay(interval);
   }
}

