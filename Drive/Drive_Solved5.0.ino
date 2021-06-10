/*******************************************************************************/
// Optical Sensor Definition
#include "SPI.h"

// these pins may be different on different boards
// this is for the uno
#define PIN_SS        10
#define PIN_MISO      12
#define PIN_MOSI      11
#define PIN_SCK       13



#define PIN_MOUSECAM_RESET     8
#define PIN_MOUSECAM_CS        7

#define ADNS3080_PIXELS_X                 30
#define ADNS3080_PIXELS_Y                 30

#define ADNS3080_PRODUCT_ID            0x00
#define ADNS3080_REVISION_ID           0x01
#define ADNS3080_MOTION                0x02
#define ADNS3080_DELTA_X               0x03
#define ADNS3080_DELTA_Y               0x04
#define ADNS3080_SQUAL                 0x05
#define ADNS3080_PIXEL_SUM             0x06
#define ADNS3080_MAXIMUM_PIXEL         0x07
#define ADNS3080_CONFIGURATION_BITS    0x0a
#define ADNS3080_EXTENDED_CONFIG       0x0b
#define ADNS3080_DATA_OUT_LOWER        0x0c
#define ADNS3080_DATA_OUT_UPPER        0x0d
#define ADNS3080_SHUTTER_LOWER         0x0e
#define ADNS3080_SHUTTER_UPPER         0x0f
#define ADNS3080_FRAME_PERIOD_LOWER    0x10
#define ADNS3080_FRAME_PERIOD_UPPER    0x11
#define ADNS3080_MOTION_CLEAR          0x12
#define ADNS3080_FRAME_CAPTURE         0x13
#define ADNS3080_SROM_ENABLE           0x14
#define ADNS3080_FRAME_PERIOD_MAX_BOUND_LOWER      0x19
#define ADNS3080_FRAME_PERIOD_MAX_BOUND_UPPER      0x1a
#define ADNS3080_FRAME_PERIOD_MIN_BOUND_LOWER      0x1b
#define ADNS3080_FRAME_PERIOD_MIN_BOUND_UPPER      0x1c
#define ADNS3080_SHUTTER_MAX_BOUND_LOWER           0x1e
#define ADNS3080_SHUTTER_MAX_BOUND_UPPER           0x1e
#define ADNS3080_SROM_ID               0x1f
#define ADNS3080_OBSERVATION           0x3d
#define ADNS3080_INVERSE_PRODUCT_ID    0x3f
#define ADNS3080_PIXEL_BURST           0x40
#define ADNS3080_MOTION_BURST          0x50
#define ADNS3080_SROM_LOAD             0x60

#define ADNS3080_PRODUCT_ID_VAL        0x17


long instructionStartTime = 0;
bool instructionCompleted = 1;
bool emergencyStop = 0;

float total_x = 0;

float total_y = 0;


float total_x1 = 0;

float total_y1 = 0;


float x = 0;
float y = 0;

int a = 0;
int b = 0;




float distance_x = 0;
float distance_y = 0;




volatile byte movementflag = 0;
volatile int xydat[2];



int convTwosComp(int b) {
  //Convert from 2's complement
  if (b & 0x80) {
    b = -1 * ((b ^ 0xff) + 1);
  }
  return b;
}



int tdistance = 0;


void mousecam_reset()
{
  digitalWrite(PIN_MOUSECAM_RESET, HIGH);
  delay(1); // reset pulse >10us
  digitalWrite(PIN_MOUSECAM_RESET, LOW);
  delay(35); // 35ms from reset to functional
}


int mousecam_init()
{
  pinMode(PIN_MOUSECAM_RESET, OUTPUT);
  pinMode(PIN_MOUSECAM_CS, OUTPUT);

  digitalWrite(PIN_MOUSECAM_CS, HIGH);

  mousecam_reset();

  int pid = mousecam_read_reg(ADNS3080_PRODUCT_ID);
  if (pid != ADNS3080_PRODUCT_ID_VAL)
    return -1;

  // turn on sensitive mode
  mousecam_write_reg(ADNS3080_CONFIGURATION_BITS, 0x19);

  return 0;
}

void mousecam_write_reg(int reg, int val)
{
  digitalWrite(PIN_MOUSECAM_CS, LOW);
  SPI.transfer(reg | 0x80);
  SPI.transfer(val);
  digitalWrite(PIN_MOUSECAM_CS, HIGH);
  delayMicroseconds(50);
}

int mousecam_read_reg(int reg)
{
  digitalWrite(PIN_MOUSECAM_CS, LOW);
  SPI.transfer(reg);
  delayMicroseconds(75);
  int ret = SPI.transfer(0xff);
  digitalWrite(PIN_MOUSECAM_CS, HIGH);
  delayMicroseconds(1);
  return ret;
}

struct MD
{
  byte motion;
  char dx, dy;
  byte squal;
  word shutter;
  byte max_pix;
};


void mousecam_read_motion(struct MD *p)
{
  digitalWrite(PIN_MOUSECAM_CS, LOW);
  SPI.transfer(ADNS3080_MOTION_BURST);
  delayMicroseconds(75);
  p->motion =  SPI.transfer(0xff);
  p->dx =  SPI.transfer(0xff);
  p->dy =  SPI.transfer(0xff);
  p->squal =  SPI.transfer(0xff);
  p->shutter =  SPI.transfer(0xff) << 8;
  p->shutter |=  SPI.transfer(0xff);
  p->max_pix =  SPI.transfer(0xff);
  digitalWrite(PIN_MOUSECAM_CS, HIGH);
  delayMicroseconds(5);
}

// pdata must point to an array of size ADNS3080_PIXELS_X x ADNS3080_PIXELS_Y
// you must call mousecam_reset() after this if you want to go back to normal operation
int mousecam_frame_capture(byte *pdata)
{
  mousecam_write_reg(ADNS3080_FRAME_CAPTURE, 0x83);

  digitalWrite(PIN_MOUSECAM_CS, LOW);

  SPI.transfer(ADNS3080_PIXEL_BURST);
  delayMicroseconds(50);

  int pix;
  byte started = 0;
  int count;
  int timeout = 0;
  int ret = 0;
  for (count = 0; count < ADNS3080_PIXELS_X * ADNS3080_PIXELS_Y; )
  {
    pix = SPI.transfer(0xff);
    delayMicroseconds(10);
    if (started == 0)
    {
      if (pix & 0x40)
        started = 1;
      else
      {
        timeout++;
        if (timeout == 100)
        {
          ret = -1;
          break;
        }
      }
    }
    if (started == 1)
    {
      pdata[count++] = (pix & 0x3f) << 2; // scale to normal grayscale byte range
    }
  }

  digitalWrite(PIN_MOUSECAM_CS, HIGH);
  delayMicroseconds(14);

  return ret;
}

/*******************************************************************************/

//Motor Section Definition
#include <Wire.h>
#include <INA219_WE.h>

INA219_WE ina219; // this is the instantiation of the library for the current sensor

float open_loop, closed_loop; // Duty Cycles
float vpd, vb, vref, iL, dutyref, current_mA; // Measurement Variables
unsigned int sensorValue0, sensorValue1, sensorValue2, sensorValue3; // ADC sample values declaration
float ev = 0, cv = 0, ei = 0, oc = 0; //internal signals
float Ts = 0.0008; //1.25 kHz control frequency. It's better to design the control period as integral multiple of switching period.
float kpv = 0.05024, kiv = 15.78, kdv = 0; // voltage pid.
float u0v, u1v, delta_uv, e0v, e1v, e2v; // u->output; e->error; 0->this time; 1->last time; 2->last last time
float kpi = 0.02512, kii = 39.4, kdi = 0; // current pid.
float u0i, u1i, delta_ui, e0i, e1i, e2i; // Internal values for the current controller
float uv_max = 4, uv_min = 0; //anti-windup limitation
float ui_max = 1, ui_min = 0; //anti-windup limitation
float current_limit = 1.0;
boolean Boost_mode = 0;
boolean CL_mode = 0;


unsigned int loopTrigger;
unsigned int com_count = 0; // a variables to count the interrupts. Used for program debugging.


//************************** Motor Constants **************************//
unsigned long previousMillis = 0; //initializing time counter
const long f_i = 10000;           //time to move in forward direction, please calculate the precision and conversion factor
const long r_i = 20000;           //time to rotate clockwise
const long b_i = 30000;           //time to move backwards
const long l_i = 40000;           //time to move anticlockwise
const long s_i = 50000;
int DIRRstate;              //initializing direction states
int DIRLstate;

int DIRL = 20;                    //defining left direction pin
int DIRR = 21;                    //defining right direction pin

int pwmr = 5;                     //pin to control right wheel speed using pwm
int pwml = 9;                     //pin to control left wheel speed using pwm

//Define Commands and Constants.
String instr;
bool F;
bool B;
bool L;
bool R;
bool S;
unsigned long instructionCompleteTime=0;  //the instruction completed time
bool firstTime=1; //first time error lies in the range given
  
float target_x; //target dist in x
float target_y; //target dist in y
float err_x; //error in x
float err_y; //error in y
float tmp_x = 0; //x-direc accumulation
float tmp_y = 0; //y-direc accumulation

float kpp = 1, kip = 1;   //position controller coefficient (Kpp / Kip = 10) 1,0.1
float u0p, u1p, delta_up, e0p, e1p, e2p; // u->output; e->error; 0->this time; 1->last time; 2->last last time
float cp; //result from PI controller

/*******************************************************************************/
void setup() {

  // Motor Part //

  //************************** Motor Pins Defining **************************//
  pinMode(DIRR, OUTPUT);
  pinMode(DIRL, OUTPUT);
  pinMode(pwmr, OUTPUT);
  pinMode(pwml, OUTPUT);
  digitalWrite(pwmr, LOW);       //setting right motor speed at maximum
  digitalWrite(pwml, LOW);       //setting left motor speed at maximum
  //*******************************************************************//

  //Basic pin setups

  noInterrupts(); //disable all interrupts
  pinMode(13, OUTPUT);  //Pin13 is used to time the loops of the controller
  pinMode(3, INPUT_PULLUP); //Pin3 is the input from the Buck/Boost switch
  pinMode(2, INPUT_PULLUP); // Pin 2 is the input from the CL/OL switch
  analogReference(EXTERNAL); // We are using an external analogue reference for the ADC

  // TimerA0 initialization for control-loop interrupt.

  TCA0.SINGLE.PER = 999; //
  TCA0.SINGLE.CMP1 = 999; //
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm; //16 prescaler, 1M.
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm;

  // TimerB0 initialization for PWM output

  pinMode(6, OUTPUT);
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; //62.5kHz
  analogWrite(6, 120);

  interrupts();  //enable interrupts.
  Wire.begin(); // We need this for the i2c comms for the current sensor
  ina219.init(); // this initiates the current sensor
  Wire.setClock(700000); // set the comms speed for i2c

  // End of Motor Part //

  // Optical Sensor Part //
  pinMode(PIN_SS, OUTPUT);
  pinMode(PIN_MISO, INPUT);
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_SCK, OUTPUT);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV32);
  SPI.setDataMode(SPI_MODE3);
  SPI.setBitOrder(MSBFIRST);

  Serial.begin(38400);
  Serial1.begin(115200);

  if (mousecam_init() == -1)
  {
    Serial.println("Mouse cam failed to init");
    while (1);
  }

  // End of Optical Sensor Part //


}

/*******************************************************************************/
//This function will send the distance/angle travelled by the rover and a instruction completed signal to indicate whether the instr executed successfully
void Send_Instruction_Completed(float distance, bool Completed, char instruction){
    //Stop the rover and reset the instructions
    Stop();
    total_x1 = 0;
    total_y1 = 0;
    F = false;
    B = false;
    L = false;
    R = false;
    S = false;
    //Send message back to command
    String message = (String)instruction + (String)Completed + ',' + (String)distance;
    Serial.println("#######################################################################################################################################################");
    Serial1.print(message);
    Serial.println("############################################################################################################################################################");
  }
//This function will reset the variable used in PI controller function to 0 (need to be called every time when a new instr coming)
void Reset_PI(){
  u0p = 0;
  u1p = 0;
  delta_up = 0;
  e0p = 0;
  e1p = 0;
  e2p = 0;
  }
/*******************************************************************************/

void loop() {


  // Motor Part //
  unsigned long currentMillis = millis();
  if (loopTrigger) { // This loop is triggered, it wont run unless there is an interrupt

    digitalWrite(13, HIGH);   // set pin 13. Pin13 shows the time consumed by each control cycle. It's used for debugging.

    // Sample all of the measurements and check which control mode we are in
    sampling();
    CL_mode = digitalRead(3); // input from the OL_CL switch
    Boost_mode = digitalRead(2); // input from the Buck_Boost switch

    if (Boost_mode) {
      if (CL_mode) { //Closed Loop Boost
        pwm_modulate(1); // This disables the Boost as we are not using this mode
      } else { // Open Loop Boost
        pwm_modulate(1); // This disables the Boost as we are not using this mode
      }
    } else {
      if (CL_mode) { // Closed Loop Buck
        // The closed loop path has a voltage controller cascaded with a current controller. The voltage controller
        // creates a current demand based upon the voltage error. This demand is saturated to give current limiting.
        // The current loop then gives a duty cycle demand based upon the error between demanded current and measured
        // current
        current_limit = 3; // Buck has a higher current limit
        ev = vref - vb;  //voltage error at this time
        cv = pidv(ev); //voltage pid
        cv = saturation(cv, current_limit, 0); //current demand saturation
        ei = cv - iL; //current error
        closed_loop = pidi(ei); //current pid
        closed_loop = saturation(closed_loop, 0.99, 0.01); //duty_cycle saturation
        pwm_modulate(closed_loop); //pwm modulation
      } else { // Open Loop Buck
        current_limit = 3; // Buck has a higher current limit
        oc = iL - current_limit; // Calculate the difference between current measurement and current limit
        if ( oc > 0) {
          open_loop = open_loop - 0.001; // We are above the current limit so less duty cycle
        } else {
          open_loop = open_loop + 0.001; // We are below the current limit so more duty cycle
        }
        open_loop = saturation(open_loop, dutyref, 0.02); // saturate the duty cycle at the reference or a min of 0.01
        pwm_modulate(open_loop); // and send it out
      }
    }
    // closed loop control path

    digitalWrite(13, LOW);   // reset pin13.
    loopTrigger = 0;
  }


  //************************** Motor Testing **************************//
  //Create some input instructions to operate the rover (Command)
    Serial.println("Enter the instruction: ");  
    while(Serial1.available()!=0){
     instr = Serial1.readString(); //Reading the Input string from Serial port 1
     Instr_Decode(instr);
    }
    
    //If the instr execution time is larger than 20sec and the instr still not finished || Emergency stop is called, error message will be returned
    if((millis()-instructionStartTime > 70000 && !instructionCompleted) || emergencyStop){
      Serial.println("Failed to execute instruction within 20 seconds or emergency stop");
      emergencyStop=0;
      if(F || B){
        Send_Instruction_Completed(total_y,0, (F ? 'F':'B'));
        }

      if(L || R){
        Send_Instruction_Completed((-total_x/28)*90,0,(L ? 'L':'R'));
        }
      
      Stop(); //Stop the rover and reset all instr
      F = false;
      B = false;
      L = false;
      R = false;
      S = false;
      instructionCompleted = 1;
    }
    
   //Actions:
   if(F){
      Serial.println("I'm at F");
      err_y = total_y - tmp_y; //Calculate error
      cp = pi_d(err_y);
      Speed_Control(abs(cp)*0.01+0.4); //Using PI controller result to control speed

      //Position Control
      if(err_y < -0.5){
        MovingForward();
        }
      if(err_y > 0.5){
        MovingBackward();
        }
      if(err_y > -0.5 && err_y < 0.5){
        //firstTime the err lies in the given range
         if (firstTime){
            instructionCompleteTime=millis();
            firstTime=0; 
            Serial.println("########################### First Time #################################");
         } 
         //Check the motion after 10sec for position correction
         if(millis()-instructionCompleteTime > 20000 && !firstTime && !instructionCompleted){
           Stop();
           instructionCompleted=1;
           Send_Instruction_Completed(total_y,1,'F');        
         }
         Stop();
        
        Serial.println("Position Correct [Block 1]");
        Serial.println("Instr Complete Status(Forward) = " + String(instructionCompleted));
      }       
      Serial.println("err = " + String(err_y));
      Serial.println("err from PID:" + String(cp));
      //Serial.println("pwm duty = " + String(abs(cp)*0.01+0.3));
     }

  if(B){
      Serial.println("I'm at B");
      err_y = total_y - tmp_y;
      cp = pi_d(err_y);
      Speed_Control(abs(cp)*0.01+0.4);
      //Speed_Control(0.39);
      
      if(err_y < -0.5){
        MovingForward();
        }
      if(err_y > 0.5){
        MovingBackward();
        }
      if(err_y > -0.5 && err_y < 0.5){
         if (firstTime) {instructionCompleteTime=millis();firstTime=0; Serial.println("########################### First Time #################################");} //firstTime the err lies in the given range
         if(millis()-instructionCompleteTime > 10000 && !firstTime && !instructionCompleted){
           instructionCompleted=1;
           Send_Instruction_Completed(total_y,1,'B');
         }
         Stop();

        Serial.println("Position Correct [Block 2]");
        Serial.println("Instr Complete Status(Forward) = " + String(instructionCompleted));
      }       
      Serial.println("err = " + String(err_y));
      Serial.println("err from PID:" + String(cp));
      Serial.println("pwm duty = " + String(abs(cp)*0.01+0.3));
     }

   if(L){
      Serial.println("I'm at L");
      err_x = total_x - tmp_x;
      cp = pi_d(err_x);
      Speed_Control(abs(cp)*0.01+0.385);
      
      if(err_x < -0.2){
        Left90();
        }
      if(err_x > 0.2){
        Right90();
        }
      if(err_x >= -0.2 && err_x <= 0.2){
         if (firstTime) {instructionCompleteTime=millis();firstTime=0; Serial.println("########################### First Time #################################");} //firstTime the err lies in the given range
         if(millis()-instructionCompleteTime > 10000 && !firstTime && !instructionCompleted){
           instructionCompleted=1;
           Send_Instruction_Completed((-total_x/28)*90,1,'L');
         }
         Stop();

        //Serial.println("Position Correct [Block 3]");
        //Serial.println("Instr Complete Status(Forward) = " + String(instructionCompleted));
      }       
      Serial.println("err = " + String(err_x));
      //Serial.println("err from PID:" + String(cp));
      //Serial.println("pwm duty = " + String(abs(cp)*0.01+0.3));
    }

   if(R){
      //Serial.println("I'm at L");
      err_x = total_x - tmp_x;
      cp = pi_d(err_x);
      Speed_Control(abs(cp)*0.01+0.385);
      
      if(err_x < -0.2){
        Left90();
        }
      if(err_x > 0.2){
        Right90();
        }
      if(err_x >= -0.2 && err_x <= 0.2){
         if (firstTime) {instructionCompleteTime=millis();firstTime=0; Serial.println("########################### First Time #################################");} //firstTime the err lies in the given range
         if(millis()-instructionCompleteTime > 10000 && !firstTime && !instructionCompleted){
           instructionCompleted=1;
           Send_Instruction_Completed((-total_x/28)*90,1,'R'); //how far the rover moves
         }
         Stop();

        //Serial.println("Position Correct [Block 3]");
        //Serial.println("Instr Complete Status(Forward) = " + String(instructionCompleted));
      }       
      //Serial.println("err = " + String(err_x));
      //Serial.println("err from PID:" + String(cp));
      //Serial.println("pwm duty = " + String(abs(cp)*0.01+0.3));
    }
    

   if(S){
        Stop();
        instructionCompleted = 1;
    }

   Serial.println("tmp_x = " + String(tmp_x));
   Serial.println("tmp_y = " + String(tmp_y));  

  //*******************************************************************//

  // End of Motor Part //

  // Optical Sensor Part //
#if 0

  /*
      if(movementflag){

      tdistance = tdistance + convTwosComp(xydat[0]);
      Serial.println("Distance = " + String(tdistance));
      movementflag=0;
      delay(3);
      }

  */
  // if enabled this section grabs frames and outputs them as ascii art

  if (mousecam_frame_capture(frame) == 0)
  {
    int i, j, k;
    for (i = 0, k = 0; i < ADNS3080_PIXELS_Y; i++)
    {
      for (j = 0; j < ADNS3080_PIXELS_X; j++, k++)
      {
        Serial.print(asciiart(frame[k]));
        Serial.print(' ');
      }
      Serial.println();
    }
  }
  Serial.println();
  delay(250);

#else

  // if enabled this section produces a bar graph of the surface quality that can be used to focus the camera
  // also drawn is the average pixel value 0-63 and the shutter speed and the motion dx,dy.

  int val = mousecam_read_reg(ADNS3080_PIXEL_SUM);
  MD md;
  mousecam_read_motion(&md);
  for (int i = 0; i < md.squal / 4; i++)
  Serial.print('*');
  Serial.print(' ');
  Serial.print((val * 100) / 351);
  Serial.print(' ');
  Serial.print(md.shutter); Serial.print(" (");
  Serial.print((int)md.dx); Serial.print(',');
  Serial.print((int)md.dy); Serial.println(')');

  // Serial.println(md.max_pix);
  delay(100);


  distance_x = md.dx; //convTwosComp(md.dx);
  distance_y = md.dy; //convTwosComp(md.dy);

  total_x1 = (total_x1 + distance_x);
  total_y1 = (total_y1 + distance_y);

  total_x = total_x1 / 157; //Conversion from counts per inch to cm (400 counts per inch)
  total_y = total_y1 / 157; //Conversion from counts per inch to cm (400 counts per inch)


  Serial.print('\n');


  Serial.println("Distance_x = " + String(total_x));
  
  Serial.println("Distance_y = " + String(total_y));
  Serial.print('\n');

  delay(100);

#endif

  // End of Optical Sensor Part //
}
/*******************************************************************************/
char asciiart(int k)
{
  static char foo[] = "WX86*3I>!;~:,`. ";
  return foo[k >> 4];
}

byte frame[ADNS3080_PIXELS_X * ADNS3080_PIXELS_Y];

//Instruction Decoding Function
void Instr_Decode(String instruction){
  //Check the first digit of command string to determine action
  if(instruction[0] == 'P'){ //pause
    Stop();
    while(!Serial1.available() || Serial1.peek()!='U'){ //unpause
    delay(50); //50ms
  }
    Serial1.read(); //get rid of the U
    instructionStartTime=millis();
}

 if(String(instruction[0]) == "X"){ //Emergency Stop
    emergencyStop=1;
    //F = false;
    //B = false;
    //L = false;
    //R = false;
    //S = false;
    tmp_x = total_x;
    tmp_y = total_y;
 }
  
  if(String(instruction[0]) == "F"){ //Moving Forward
    F = true;
    target_x = 0;
    target_y = 100*(String(instruction[3]).toInt()) + 10*(String(instruction[4]).toInt()) + (String(instruction[5]).toInt()); //Convert to target distance
    B = false;
    L = false;
    R = false;
    S = false;
    //Accumulate distance or angle
    tmp_x = total_x + target_x;
    tmp_y = total_y + target_y;
   }
    
  if(String(instruction[0]) == "B"){
    F = false;
    B = true;
    target_x = 0;
    target_y = -(100*(String(instruction[3]).toInt()) + 10*(String(instruction[4]).toInt()) + (String(instruction[5]).toInt())); //Convert to target distance
    L = false;
    R = false;
    S = false;
    //Accumulate distance or angle
    tmp_x = total_x + target_x;
    tmp_y = total_y + target_y;
    }
    
  if(String(instruction[0]) == "L"){
    F = false;
    B = false;
    L = true;
    target_x = Angle_Conversion(10*(String(instruction[1]).toInt()) + (String(instruction[2]).toInt())); //Convert target angle to target dist
    target_y = 0;
    R = false;
    S = false;
    //Accumulate distance or angle
    tmp_x = total_x + target_x;
    tmp_y = total_y + target_y;
    }
    
  if(String(instruction[0]) == "R"){
    F = false;
    B = false;
    L = false;
    R = true;
    target_x = -(Angle_Conversion(10*(String(instruction[1]).toInt()) + (String(instruction[2]).toInt()))); //Convert target angle to target dist
    target_y = 0;
    S = false;
    //Accumulate distance or angle
    tmp_x = total_x + target_x;
    tmp_y = total_y + target_y;
    }
    
  if(String(instruction[0]) == "S"){
    F = false;
    B = false;
    L = false;
    R = false;
    S = true;

    tmp_x = total_x;
    tmp_y = total_y;
    }
  /*  
  Serial.println("Instr = " + String(instruction[0]));
  Serial.println("Target x = " + String(target_x));
  Serial.println("Target y = " + String(target_y));  
  //Serial.println("Duty Cycle of Vm = " + String(0.01*(10*String(instruction[6]).toInt() + String(instruction[7]).toInt())));
  Serial.println("Target angle = " + String(Angle_Conversion(10*(String(instruction[1]).toInt()) + (String(instruction[2]).toInt()))));
*/
/*
  if((String(instruction[0]) == "F") || (String(instruction[0]) == "B")){
       
    //Rover head is facing front or back
    if((abs(tmp_x)/28)%2 == 0){
      if((abs(tmp_x)/28)%4 == 0){
        x_coord = x_coord;
        y_coord = y_coord + target_y;
        }
      
      if((abs(tmp_x)/28)%4 != 0){
        x_coord = x_coord;
        y_coord = y_coord - target_y;
        }
    }
    
    //Rover head is facing front or back with 90 degrees 
    if((abs(tmp_x)/28)%2 != 0){
      if((abs(tmp_x)/28)%5 == 0){
        x_coord = x_coord + target_y;
        y_coord = y_coord;
        }
      
      if((abs(tmp_x)/28)%5 != 0){
        x_coord = x_coord - target_y;
        y_coord = y_coord;
        }
      }
    }

  if((String(instruction[0]) == "L") || (String(instruction[0]) == "R")){
    x_coord = x_coord;
    y_coord = y_coord;   
    }
*/

  instructionStartTime = millis(); //Start counting execution time
  instructionCompleted = 0; //Reset completion signal
  firstTime=1; //Reset first time error lying in between the range signal
  Reset_PI(); //Reset PI controller constants
  
  }


//Turning Method Conversion
float Angle_Conversion(float target_angle){
  return (target_angle/90)*28;  //Converting from input angle to target dist. along x
  }

//Five possible actions
  void MovingForward(){
    digitalWrite(pwmr, HIGH);       //setting right motor speed at maximum
    digitalWrite(pwml, HIGH);       //setting left motor speed at maximum   
    DIRRstate = LOW; //Set motor state
    DIRLstate = HIGH;
    digitalWrite(DIRR, DIRRstate);
    digitalWrite(DIRL, DIRLstate);    
    }

   void MovingBackward(){
    digitalWrite(pwmr, HIGH);       //setting right motor speed at maximum
    digitalWrite(pwml, HIGH);       //setting left motor speed at maximum   
    DIRRstate = HIGH; //Set motor state
    DIRLstate = LOW;   
    digitalWrite(DIRR, DIRRstate);
    digitalWrite(DIRL, DIRLstate);
   }

   void Left90(){
    digitalWrite(pwmr, HIGH);       //setting right motor speed at maximum
    digitalWrite(pwml, HIGH);       //setting left motor speed at maximum   
    DIRRstate = LOW; //Set motor state
    DIRLstate = LOW;
    digitalWrite(DIRR, DIRRstate);
    digitalWrite(DIRL, DIRLstate);   
    }

   void Right90(){
    digitalWrite(pwmr, HIGH);       //setting right motor speed at maximum
    digitalWrite(pwml, HIGH);       //setting left motor speed at maximum   
    DIRRstate = HIGH; //Set motor state
    DIRLstate = HIGH;  
    digitalWrite(DIRR, DIRRstate);
    digitalWrite(DIRL, DIRLstate); 
    }

   void Stop(){
    digitalWrite(pwmr, LOW);       //setting right motor speed at 0
    digitalWrite(pwml, LOW);       //setting left motor speed at 0 
    Speed_Control(0); 
    }



//Function used for speed control
void Speed_Control(float duty){
  analogWrite(6, (int)(255-duty*255)); //Using duty cycle as an argument to control voltage then control speed
  }
  
// Timer A CMP1 interrupt. Every 800us the program enters this interrupt.
// This, clears the incoming interrupt flag and triggers the main loop.

ISR(TCA0_CMP1_vect) {
  TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm; //clear interrupt flag
  loopTrigger = 1;
}

// This subroutine processes all of the analogue samples, creating the required values for the main loop

void sampling() {

  // Make the initial sampling operations for the circuit measurements

  sensorValue0 = analogRead(A0); //sample Vb
  sensorValue2 = analogRead(A2); //sample Vref
  sensorValue3 = analogRead(A3); //sample Vpd
  current_mA = ina219.getCurrent_mA(); // sample the inductor current (via the sensor chip)

  // Process the values so they are a bit more usable/readable
  // The analogRead process gives a value between 0 and 1023
  // representing a voltage between 0 and the analogue reference which is 4.096V

  vb = sensorValue0 * (4.096 / 1023.0); // Convert the Vb sensor reading to volts
  vref = sensorValue2 * (4.096 / 1023.0); // Convert the Vref sensor reading to volts
  vpd = sensorValue3 * (4.096 / 1023.0); // Convert the Vpd sensor reading to volts

  // The inductor current is in mA from the sensor so we need to convert to amps.
  // We want to treat it as an input current in the Boost, so its also inverted
  // For open loop control the duty cycle reference is calculated from the sensor
  // differently from the Vref, this time scaled between zero and 1.
  // The boost duty cycle needs to be saturated with a 0.33 minimum to prevent high output voltages

  if (Boost_mode == 1) {
    iL = -current_mA / 1000.0;
    dutyref = saturation(sensorValue2 * (1.0 / 1023.0), 0.99, 0.33);
  } else {
    iL = current_mA / 1000.0;
    dutyref = sensorValue2 * (1.0 / 1023.0);
  }

}

float saturation( float sat_input, float uplim, float lowlim) { // Saturatio function
  if (sat_input > uplim) sat_input = uplim;
  else if (sat_input < lowlim ) sat_input = lowlim;
  else;
  return sat_input;
}

void pwm_modulate(float pwm_input) { // PWM function
  analogWrite(6, (int)(255 - pwm_input * 255));
}

// This is a PID controller for the voltage

float pidv( float pid_input) {
  float e_integration;
  e0v = pid_input;
  e_integration = e0v;

  //anti-windup, if last-time pid output reaches the limitation, this time there won't be any intergrations.
  if (u1v >= uv_max) {
    e_integration = 0;
  } else if (u1v <= uv_min) {
    e_integration = 0;
  }

  delta_uv = kpv * (e0v - e1v) + kiv * Ts * e_integration + kdv / Ts * (e0v - 2 * e1v + e2v); //incremental PID programming avoids integrations.there is another PID program called positional PID.
  u0v = u1v + delta_uv;  //this time's control output

  //output limitation
  saturation(u0v, uv_max, uv_min);

  u1v = u0v; //update last time's control output
  e2v = e1v; //update last last time's error
  e1v = e0v; // update last time's error
  return u0v;
}

// Current controller is less crucial than the voltage controller
// This is a PID controller for the current

float pidi(float pid_input) {
  float e_integration;
  e0i = pid_input;
  e_integration = e0i;

  //anti-windup
  if (u1i >= ui_max) {
    e_integration = 0;
  } else if (u1i <= ui_min) {
    e_integration = 0;
  }

  delta_ui = kpi * (e0i - e1i) + kii * Ts * e_integration + kdi / Ts * (e0i - 2 * e1i + e2i); //incremental PID programming avoids integrations.
  u0i = u1i + delta_ui;  //this time's control output

  //output limitation
  saturation(u0i, ui_max, ui_min);

  u1i = u0i; //update last time's control output
  e2i = e1i; //update last last time's error
  e1i = e0i; // update last time's error
  return u0i;
}


//Position Controller
// This is a PI controller for Position Control
float pi_d(float pi_d_input) {
  float e_integration;
  e0p = pi_d_input;
  e_integration = e0p;

  delta_up = kpp * (e0p - e1p) + kip * Ts * e_integration; //incremental PI programming avoids integrations.
  u0p = u1p + delta_up;  //this time's control output

  u1p = u0p; //update last time's control output
  e2p = e1p; //update last last time's error
  e1p = e0p; // update last time's error
  return u0p;
}
