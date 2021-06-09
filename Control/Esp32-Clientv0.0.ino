/*
	Esp32 Websockets Client

	This sketch:
        1. Connects to a WiFi network
        2. Connects to a Websockets server
        3. Sends the websockets server a message ("Hello Server")
        4. Prints all incoming messages while the connection is open

	Hardware:
        For this sketch you only need an ESP32 board.

	Created 15/02/2019
	By Gil Maimon
	https://github.com/gilmaimon/ArduinoWebsockets

*/

#include <ArduinoWebsockets.h>
#include <WiFi.h>

const char* ssid = "AsusMesh72"; //Enter SSID
const char* password = "40572970"; //Enter Password
const char* websockets_server_host = "3.143.233.110"; //Enter server adress
const uint16_t websockets_server_port = 8000; // Enter server port
bool auto_mode = false;
using namespace websockets;

String dataFromDrive;
String dataFromVision;
String dataToCommand;
float total_x=0;
float total_y=0;
float total_angle=0; //Angle 0 means North. Going clockwise, so 90 is East.
float distance=0;

struct Ball{
  char colour;
  float x;
  float y;
};

Ball balls[10];

WebsocketsClient client;

void sendBallToCommand(char colour,float x,float y){
  //client.send("Ball!");
  Serial.println("Ball");
}
void setup() {
    Serial.begin(115200);
    Serial1.begin(115200, SERIAL_8N1,23,22); // Vision
    Serial2.begin(115200, SERIAL_8N1,16,17); //Drive
    // Connect to wifi
    WiFi.begin(ssid, password);

    // Wait some time to connect to wifi
    for(int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
        Serial.print(".");
        delay(1000);
    }

    // Check if connected to wifi
    if(WiFi.status() != WL_CONNECTED) {
        Serial.println("No Wifi!");
        return;
    }

    Serial.println("Connected to Wifi, Connecting to server.");
    // try to connect to Websockets server
    if(!client.connect(websockets_server_host, websockets_server_port, "/ws/rover")){Serial.print(".");delay(100);}
//    if(connected) {
        Serial.println("Connected!");
       // client.send("Hello Server");
  //  } else {
 //       Serial.println("Not Connected!");
 //   }
    
    // run callback when messages are received
    client.onMessage([&](WebsocketsMessage message){
        if(message.data()=="Autonomous"){
          auto_mode = true;
        }else{
        Serial2.print(message.data());
    }});}

void loop() {
    // let the websockets client check for incoming messages
    if(Serial.available()) { 
      Serial2.print(Serial.readString());
    }
    if(client.available()) {
        client.poll();
    }
    if(Serial2.available()){
      dataFromDrive = Serial2.readString();
      distance=atof(dataFromDrive.substring(3).c_str());
      if(dataFromDrive[0]=='F' || dataFromDrive[0]=='B'){
        total_y += ((dataFromDrive[0]=='F')? 1:-1)*(distance * cos(3.14159265*total_angle/180.0));
        total_x += ((dataFromDrive[0]=='F')? 1:-1)*(distance * sin(3.14159265*total_angle/180.0)); 
      }
      if(dataFromDrive[0]=='L' || dataFromDrive[0]=='R'){
        total_angle = (total_angle + distance)-(360*int(int((total_angle + distance))/360)); 
      }
      if(dataFromDrive[1]=='0'){client.send("{type:\"Terminal\",message:\"Instruction not completed\"}");}
      dataToCommand = "{type:\"Map\",x_distance:\"" + String(total_x) + "\",y_distance:\"" + String(total_y) + "\",angle:\""+String(total_angle)+"\"}";
      //client.send(dataToCommand);
      Serial.println(dataToCommand);
      }

    if(Serial1.available()){
      dataFromVision=Serial1.readString();
      if(dataFromVision=="s"){
        Serial2.write('X'); 
      }else if(dataFromVision=="p"){ //Vision tells control to pause
        Serial2.write('P'); //Control tells drive to Pause
        while(!Serial2.available() || Serial2.readString()!="p"){ //Waits until drive has paused and confirms
          delay(1);
        }
        Serial1.write('s'); //Control tells vision that it has stopped moving
        while(Serial1.available() || Serial1.peek()!='c'){
        while(!Serial1.available() || Serial1.peek()!='b'){
          delay(1);
        }
        dataFromVision=Serial1.readString();
        float ball_x=total_x;
        float ball_y=total_y;
        float perp_distance = dataFromVision.substring(2,4).toFloat();
        float other_distance= atof(dataFromVision.substring(4).c_str());
        float total_distance = sqrt(perp_distance*perp_distance + other_distance*other_distance);
        float relative_angle = total_angle+(180.0*atan(other_distance/perp_distance)/3.14159265);
        ball_x += (total_distance*sin(3.14159265*(relative_angle)/180.0));
        ball_y += (total_distance*cos(3.14159265*(relative_angle)/180.0));
        sendBallToCommand(dataFromVision[1],ball_x,ball_y);
        switch(dataFromVision[1]){
         case 'R':
          balls[0] = Ball{'R',ball_x,ball_y};
          break;
         case 'P':
          balls[1] = Ball{'P',ball_x,ball_y};
          break;
         case 'Y':
          balls[2] = Ball{'Y',ball_x,ball_y};
          break;
         case 'G':
          balls[3] = Ball{'G',ball_x,ball_y};
          break;
         case 'B':
          balls[4] = Ball{'B',ball_x,ball_y};
          break;
         case 'U':
          balls[5] = Ball{'U',ball_x,ball_y};
         break;
        }
        Serial1.write('a');
        }
        Serial1.read();
        Serial2.write('U');
        delay(50);
        Serial1.write('m');
      }

      

      
    }
    delay(500);
}
