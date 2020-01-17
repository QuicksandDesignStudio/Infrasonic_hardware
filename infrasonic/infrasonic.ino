/*

  This is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Copyright (c) 2015 Hristo Gochkov - Example
  Copyright (c) 2020 Thejesh GN - Added sampling, Intial Sequence, File Write, LED on and Off
  Copyright (c) 2020 Romit
  

*/
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <WiFiAP.h>
#include "FS.h"
#include <SPI.h>

#define FILESYSTEM SPIFFS
// You only need to format the filesystem once
#define FORMAT_FILESYSTEM false

#if FILESYSTEM == FFat
#include <FFat.h>
#endif

#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#endif

#if FILESYSTEM == SD
#include "SD.h"
#endif


#define DBG_OUTPUT_PORT Serial

/* PIN for Built in Blue LED */
#define LED_BUILTIN 2  
/* FOR SD CARD 
CS‎: ‎GPIO 5  
MISO‎: ‎GPIO 19
MOSI‎: ‎GPIO 23 
CLK‎: ‎GPIO 18
*/
#define SD_CS 5



/* SOUND READ ANALOG PIN - ADC1_CH6 */
const int SOUND_ANALOG_READ = 34;
float timeKeeper;
float samplingTime = 500;
float duration = 4000000;
int fileNumber = 0;


const char* ssid = "infrasonics";
const char* password = "password";
const char* host = "esp32fs";
WebServer server(80);





void setup(void) {    
  //INIT ONBOAD LED and START THE INTIAL SEQUENCE
  pinMode(LED_BUILTIN, OUTPUT);
  ledDanceSequence();
  
  DBG_OUTPUT_PORT.begin(115200);
  DBG_OUTPUT_PORT.print("\n");
  DBG_OUTPUT_PORT.setDebugOutput(true);
  
  //Unless its inrernal flash, we dont have to 
  //format here. For example SD cards can be
  //formatted using a computer format
  //if (FORMAT_FILESYSTEM) FILESYSTEM.format();
  
  if(FILESYSTEM.begin()){
    DBG_OUTPUT_PORT.printf("File system has started");
    File root = FILESYSTEM.open("/");
    File file = root.openNextFile();
    while(file){
        String fileName = file.name();
        size_t fileSize = file.size();
        DBG_OUTPUT_PORT.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());        
        String xval = splitString(fileName, '_', 1);
        String xval2 = splitString(xval, '.', 0);        
        String nextFileNumberStr = String(xval2.c_str());
        int nextFileNumber = nextFileNumberStr.toInt();
        if (nextFileNumber > fileNumber){
          fileNumber = nextFileNumber;
        }        
        file = root.openNextFile();
    }
    DBG_OUTPUT_PORT.printf("\n");  
    DBG_OUTPUT_PORT.printf("MAX FILE NUMBER =");  
    DBG_OUTPUT_PORT.println(fileNumber);  
    DBG_OUTPUT_PORT.printf("\n");      
  }else{
    DBG_OUTPUT_PORT.printf("ERROR: COULDNT START FILE SYSTEM\n");  
  }


  //SETUP WIFI AP
  DBG_OUTPUT_PORT.printf("Staring the Wifi AP to %s\n", ssid);
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);


  //START HTTP SERVER  
  //server.begin();
  Serial.println("Server started");


  //Default will be a list of xmls
  server.on("/list", HTTP_GET, handleFileList);
        
  //delete file
  server.on("/delete", HTTP_GET, handleFileDelete);
  
  //called when the url is not defined here
  //use it to load content from FILESYSTEM
  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });

  
  
  //THIS IS FOR DEBUG
  server.on("/debug", HTTP_GET, []() {
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += ", \"analog\":" + String(analogRead(A0));
    json += ", \"gpio\":" + String((uint32_t)(0));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  server.begin();
  DBG_OUTPUT_PORT.println("HTTP server started");


  //led dance for the end of setup
  ledDanceSequence();
}

void loop(void) {

  //SAMPLE SOUND EVERY FIVE MINUTES. LATER WE CAN ADD INTERRUPT BASED
  const unsigned long fiveMinutes = 5 * 60 * 1000UL;  
  static unsigned long lastSampleTime = 0 - fiveMinutes;
  unsigned long now = millis();
  if (now - lastSampleTime >= fiveMinutes)
  {
    lastSampleTime += fiveMinutes;
    sampleSound();
  }  



  //HANDLE HTTP REQUESTS
  server.handleClient();
}

/*###################################################### SAMPLE FUNCTION ############################################################*/
/*
 * This will sample the anologRead for given amount of time and then will write it to a file
 * 
 */
 void sampleSound(){
  DBG_OUTPUT_PORT.println("start sample sound");
  ledOn();

  
  String filePre = "/sample_";
  String filePost = ".js";
    
  fileNumber  = fileNumber + 1;
  String fileName = filePre + fileNumber + filePost;
  DBG_OUTPUT_PORT.println("file name:");
  DBG_OUTPUT_PORT.println(fileName);
  

  String csv_data = "";
  String csv_time = "";

  //read the value in a forlopp with the delay required by the sample rate

  float currentTime = micros();
  float timeKeeper = micros();
  int value = 0;
  int counter = 0;
  DBG_OUTPUT_PORT.println(duration/samplingTime);
  bool first = true;
  
  while(1){
    currentTime = micros();
    if((currentTime - timeKeeper) > samplingTime) {
      timeKeeper = micros();
      value = analogRead(SOUND_ANALOG_READ);
      if(first){
        csv_data = value;
        csv_time = timeKeeper;
        first = false;
      }else{
        csv_data = csv_data + "," + value;
        csv_time = csv_time+ "," + timeKeeper;        
      }
      counter = counter + 1;      
    }
    
    //DBG_OUTPUT_PORT.println(counter);
    
    if(counter >= duration/samplingTime){
      //DBG_OUTPUT_PORT.println(counter);
      break;
    }
    
  }
  

    String json = "{";
    json += "\"data\": [" + csv_data+"]";
    json += ", \"time\": [" + csv_time+"]";
    json += "}";
    
    
  //DBG_OUTPUT_PORT.println(json);
  
  createFile(fileName, json);  
  
  
  ledOff();
  DBG_OUTPUT_PORT.println("end sample sound");
 }


/*###################################################### ON BOARD LED HANDLE ############################################################*/


void ledDanceSequence(){  
  for(int i =0; i < 10; i++){
    if(i%2 == 0){
      digitalWrite(LED_BUILTIN, HIGH); 
    }else{
      digitalWrite(LED_BUILTIN, LOW); 
    }    
    delay(500);
  }
}

void ledOn(){  
    digitalWrite(LED_BUILTIN, HIGH); 
}

void ledOff(){  
    digitalWrite(LED_BUILTIN, LOW); 
}

/*###################################################### FILE FUNCTIONS ############################################################*/

//format bytes
void createFile(String path, String data) {
  if (exists(path)) {
    DBG_OUTPUT_PORT.println("File already exists");        
  }else{
    DBG_OUTPUT_PORT.println("open for writing");
    File file = FILESYSTEM.open(path, "w");
    if (file) {
      file.print(data);
      file.close();
      DBG_OUTPUT_PORT.println("Wrote the data to file");
    } else {
      DBG_OUTPUT_PORT.println("Couldn't write the file");
    }
    
  }
}



String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}


bool exists(String path){
  bool yes = false;
  File file = FILESYSTEM.open(path, "r");
  if(!file.isDirectory()){
    yes = true;
  }
  file.close();
  return yes;
}

/*###################################################### HANDLE SERVER FUNCTIONS ############################################################*/

String getContentType(String filename) {
  if (server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}


bool handleFileRead(String path) {
  DBG_OUTPUT_PORT.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.htm";
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (exists(pathWithGz) || exists(path)) {
    if (exists(pathWithGz)) {
      path += ".gz";
    }
    File file = FILESYSTEM.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}


void handleFileDelete() {
  if (server.args() == 0) {
    return server.send(500, "text/plain", "BAD ARGS");
  }
  String path = server.arg(0);
  DBG_OUTPUT_PORT.println("handleFileDelete: " + path);
  if (path == "/") {
    return server.send(500, "text/plain", "BAD PATH");
  }
  if (!exists(path)) {
    return server.send(404, "text/plain", "FileNotFound");
  }
  FILESYSTEM.remove(path);
  server.send(200, "text/plain", "Deleted. Thank you.");
  path = String();
}


void handleFileList() {

  DBG_OUTPUT_PORT.println("handleFileList: /");


  File root = FILESYSTEM.open("/");
  DBG_OUTPUT_PORT.println(root);

  String output = "[";
  if(root.isDirectory()){
    DBG_OUTPUT_PORT.println("Root is directory");
      File file = root.openNextFile();
      DBG_OUTPUT_PORT.println("next file");
      DBG_OUTPUT_PORT.println(file);
      while(file){
          DBG_OUTPUT_PORT.println("inside while");
          if (output != "[") {
            output += ',';
          }
          output += "{\"type\":\"";
          output += (file.isDirectory()) ? "dir" : "file";
          output += "\",\"name\":\"";
          output += String(file.name()).substring(1);
          output += "\"}";
          file = root.openNextFile();
      }
      DBG_OUTPUT_PORT.println("end file while");
  }else{
    DBG_OUTPUT_PORT.println("Root is not a directory??");
  }
  output += "]";
  server.send(200, "text/json", output);
}
/*###################################################### END - HANDLE SERVER FUNCTIONS ############################################################*/


/*###################################################### STRING UTILITY FUNCTIONS ############################################################*/
String splitString(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    
    if(found > index){
      String x = data.substring(strIndex[0], strIndex[1]);
      return x;
    }else{
      return String("");  
    }    
}

/*###################################################### END STRING UTILITY FUNCTIONS ############################################################*/
