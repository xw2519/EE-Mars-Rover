struct Ball{
  char colour;
  float x;
  float y;
};

float total_x,total_y,total_angle;

void sendBallToCommand(char colour,float x,float y){
  delay(10);
}
Ball balls[10];

String readNChars(int n){ //This code is meant to read n chars from the Serial port pointed to by s.
String result = "";
  for(int i =0;(i<n && Serial.available());i++){
    result += String((char)Serial.read());  
  }
  return result;
}
String readNChars1(int n){ //This code is meant to read n chars from the Serial1 port.
String result = "";
  for(int i =0;(i<n && Serial1.available());i++){
    result += String((char)Serial1.read());  
  }
  return result;
}

String dataFromVision;


void setup() {
  // put your setup code here, to run once:
Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
if(Serial.available()){
      dataFromVision=Serial.readString();
     Serial.println(dataFromVision);
      if(dataFromVision[0]=='s'){
        Serial.write('X'); 
      }else if(dataFromVision[0]=='p'){ //Vision tells control to pause
        Serial.write('P'); //Control tells drive to Pause
        while(!Serial.available() || Serial.readString()!="p"){ //Waits until drive has paused and confirms
          delay(1);
        }
        Serial.write('s'); //Control tells vision that it has stopped moving
        while(!Serial.available() || Serial.peek()!='c'){
        while(Serial.available() < 7){//Length of ball message (e.g. bY45+05) is 7 characters
           if (Serial.peek() == 'c') {
            //Serial1.read();
          break;
          
        }
          delay(1);
          if(Serial.available()&&(!((Serial.peek()=='b')||(Serial.peek()=='c')))){ 
          Serial.print("Discarded: ");     
          Serial.println(Serial.read());     
          }
          
          //Serial.println("149");
          //Serial.println(Serial1.peek());
        }
         if (Serial.peek() == 'c') {
          //Serial.println("151");
          Serial.read();
          break;
        }
       Serial.println("157");
        dataFromVision=readNChars(7);
        Serial.println(dataFromVision);
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
        if(!Serial.available() || (Serial.peek() != 'c')){ //I think this if statement is needed if we avoid sending 'a' if vision has sent c. Otherwise, always do Serial1.write('a');
        while(Serial.available()){Serial.read();} //Flushes the buffer, used to remove any extra ball messages that were sent by vision
        Serial.write('a');
        Serial.flush(); //Waits until 'a' was actually sent
       
        }
       /* while(!Serial1.available()){ delay(1); }
          if(Serial1.peek()=='c'){
            Serial1.read();
            break;
                }
          else{ Serial1.write('a'); } */ //Not sure why this block of code was needed -> It looks like it waits for another message from Vision before acknowledging?
        }
        delay(1);
        Serial.println("Received C from Vision");
        Serial.read();
        Serial.write('U');
        delay(50);
         while(Serial.available()){Serial.read();}
        Serial.write('m');
      }

      

      
    }
    delay(500);
}
