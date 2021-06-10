#include <Wire.h>
#include <INA219_WE.h>
#include <SPI.h>
#include <SD.h>

INA219_WE ina219; // this is the instantiation of the library for the current sensor

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

const int chipSelect = 10;
unsigned int rest_timer;
unsigned int loop_trigger;
unsigned int int_count = 0; // a variables to count the interrupts. Used for program debugging.
float u0i, u1i, delta_ui, e0i, e1i, e2i; // Internal values for the current controller
float u0v,u1v,delta_uv,e0v,e1v,e2v; // u->output; e->error; 0->this time; 1->last time; 2->last last time
float uv_max=4, uv_min=0; //anti-windup limitation
float ui_max = 1, ui_min = 0; //anti-windup limitation
float kpi = 0.02512, kii = 39.4, kdi = 0; // current pid.
float kpv=0.05024,kiv=15.78,kdv=0; // voltage pid.
float Ts = 0.001; //1 kHz control frequency.
float current_measure, current_ref = 0, error_amps,ei,current_limit = 0; // Current Control
float voltage_ref = 0, ev; //Constant Voltage
float pwm_out,cv;
float V_Bat, vb;
float Pk_1, Pk;
float V_meas1,V_meas2,V_meas3;
float V_mean_total;
float V_total1,V_total2,V_total3;
float V_mean1,V_mean2,V_mean3;
int cc;
bool finish;
float soc = 0; //State of Charge
boolean input_switch;
int state_num=0,next_state;
String dataString;

void setup() {
  //Some General Setup Stuff

  Wire.begin(); // We need this for the i2c comms for the current sensor
  Wire.setClock(700000); // set the comms speed for i2c
  ina219.init(); // this initiates the current sensor
  Serial.begin(9600); // USB Communications
 // Serial1.begin(115200);//communicate with control


  //Check for the SD Card
  Serial.println("\nInitializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("* is a card inserted?");
    while (true) {} //It will stick here FOREVER if no SD is in on boot
  } else {
    Serial.println("Wiring is correct and a card is present.");
  }

  if (SD.exists("BatCycle.csv")) { // Wipe the datalog when starting
    SD.remove("BatCycle.csv");
  }

  
  noInterrupts(); //disable all interrupts
  analogReference(EXTERNAL); // We are using an external analogue reference for the ADC

  //SMPS Pins
  pinMode(13, OUTPUT); // Using the LED on Pin D13 to indicate status
  pinMode(2, INPUT_PULLUP); // Pin 2 is the input from the CL/OL switch
  pinMode(6, OUTPUT); // This is the PWM Pin

  //Cell 1 Arrangement
  pinMode(4, OUTPUT); //Relay
  pinMode(5, OUTPUT); //Discharge
  pinMode(A3, INPUT); //Measurement

  //Cell 2 Arrangement
  pinMode(9, OUTPUT); //Relay
  pinMode(10, OUTPUT); //Discharge
  pinMode(A2, INPUT); //Measurement

  //Cell 3 Arrangement
  pinMode(7, OUTPUT); //Relay
  pinMode(8, OUTPUT); //Discharge
  pinMode(A1, INPUT); //Measurement
  
  //LEDs on pin 7 and 8
//  pinMode(7, OUTPUT);
//  pinMode(8, OUTPUT);

  //Analogue input, the battery voltage (also port B voltage)
  pinMode(A0, INPUT);

  // TimerA0 initialization for 1kHz control-loop interrupt.
  TCA0.SINGLE.PER = 999; //
  TCA0.SINGLE.CMP1 = 999; //
  TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV16_gc | TCA_SINGLE_ENABLE_bm; //16 prescaler, 1M.
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_CMP1_bm;

  // TimerB0 initialization for PWM output
  TCB0.CTRLA = TCB_CLKSEL_CLKDIV1_gc | TCB_ENABLE_bm; //62.5kHz

  interrupts();  //enable interrupts.
  analogWrite(6, 120); //just a default state to start with

}


void loop() {
  char command = 0;
  if (loop_trigger == 1){ // FAST LOOP (1kHZ)
      state_num = next_state; //state transition
      V_Bat = analogRead(A0)*4.096/1.03; //check the battery voltage (1.03 is a correction for measurement error, you need to check this works for you)
      if ((V_Bat > 3700 || V_Bat < 2400)) { //Checking for Error states (just battery voltage for now)
          state_num = 14; //go directly to jail
          next_state = 14; // stay in jail
          current_ref = 0; // no current
          voltage_ref = 0; // no voltage
      }else if((V_meas1 > 3700 || V_meas1 < 2400)){
          state_num = 14; //go directly to jail
          next_state = 14; // stay in jail
          current_ref = 0; // no current
          voltage_ref = 0; // no voltage
      }else if((V_meas2 > 3700 || V_meas2 < 2400)){
          state_num = 14; //go directly to jail
          next_state = 14; // stay in jail
          current_ref = 0; // no current
          voltage_ref = 0; // no voltage
      }else if((V_meas3 > 3700 || V_meas3 < 2400)){
          state_num = 14; //go directly to jail
          next_state = 14; // stay in jail
          current_ref = 0; // no current
          voltage_ref = 0; // no voltage
      }
      if(cc == 1) { //Doing Constant Current 
      current_measure = (ina219.getCurrent_mA()); // sample the inductor current (via the sensor chip)
      if(current_measure >= 200){
      error_amps = (current_ref - current_measure) / 1000; //PID error calculation
      pwm_out = pidi(error_amps); //Perform the PID controller calculation
      pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
      analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)
      }
      else{//go to do MPPT perturb & observe
        Pk_1 = 0;
        finish = false;
        while(!finish){
          Pk = V_Bat * current_measure; 
          if(abs(Pk - Pk_1) == 0){
            finish = true;
          }
          else if(Pk > Pk_1){
          pwm_out = pwm_out - 0.005;
          Pk_1 = Pk;
        }
          else{
          pwm_out = pwm_out + 0.005;
          Pk_1 = Pk;
          }
        }
       pwm_out = saturation(pwm_out, 0.99, 0.01); //duty_cycle saturation
      analogWrite(6, (int)(255 - pwm_out * 255)); // write it out (inverting for the Buck here)
      }
      }
      else{
          vb = analogRead(A0)*4.096/1.03; 
          current_measure = (ina219.getCurrent_mA()); // sample the inductor current (via the sensor chip)
          current_limit = 3; // Buck has a higher current limit
          ev = (voltage_ref - vb) / 1000;  //voltage error at this time
          cv=pidv(ev);  //voltage pid
          cv=saturation(cv, current_limit, 0); //current demand saturation
          ei=cv - current_measure / 1000; //current error
          pwm_out = pidi(ei);  //current pid
          pwm_out = saturation(pwm_out,0.99,0.01);  //duty_cycle saturation
          analogWrite(6, (int)(255 - pwm_out * 255));
      }
      int_count++; //count how many interrupts since this was last reset to zero
      loop_trigger = 0; //reset the trigger and move on with life
  }
  
  if (int_count == 1000) { // SLOW LOOP (1Hz)

    input_switch = digitalRead(2); //get the OL/CL switch status
    if(Serial.available()>0){
          command = Serial.read();
        if (command == 'c') { //go to charging state
          state_num = 0;
        }
        else if(command == 'r'){//go to read soc
           Serial.println(soc); // change to Serial1.print for communication with control sub module
        }else if(command == 's'){//stop charging, go to rest state
          state_num = 3;
        }else if(command == 'd'){//start discharge
          state_num = 15;
        }
        }
    switch (state_num) { // STATE MACHINE (see diagram)
      case 0:{ // Start state (no current, no LEDs)
        current_ref = 0;
        //soc = 100- (3600-V_Bat)/1200 * 100;
        if(V_Bat <= 2500){
          soc = 0;
        }else if(V_Bat <= 2604 && V_Bat >2500){
          soc = 5;
        }else if(V_Bat <= 2708 && V_Bat >2604){
          soc = 15;
        }else if(V_Bat <= 2812 && V_Bat >2708){
          soc = 25;
        }else if(V_Bat <= 2916 && V_Bat >2812){
          soc = 35;
        }else if(V_Bat <= 3020 && V_Bat >2916){
          soc = 45;
        }else if(V_Bat <= 3124 && V_Bat >3020){
          soc = 55;
        }else if(V_Bat <= 3228 && V_Bat >3124){
          soc = 65;
        }else if(V_Bat <= 3332 && V_Bat >3228){
          soc = 75;
        }else if(V_Bat <= 3436 && V_Bat >3332){
          soc = 85;
        }else if(V_Bat <= 3540 && V_Bat >3436){
          soc = 95;
        }else if(V_Bat >= 3540){
          soc = 100;
        }
        // Turn off the relays and Discharges
        digitalWrite(4,false); 
        digitalWrite(5,false);
        digitalWrite(9,false);
        digitalWrite(10,false);
        digitalWrite(7,false);
        digitalWrite(8,false);
        next_state = 1;
        break;
      }
      case 1:{ // CC Charge state until 3.5V (91.7%) (500mA and a yellow LED)
        cc = 1;
        current_ref = 300;
        if (V_mean_total < 3500) { 
          soc = soc + (current_measure/57600);//remember to change the correct battery capacity after testing
          if (rest_timer < 60){//If the timer<60s, keep charging
            rest_timer++;  
            next_state = 1;        
          }
          else if(rest_timer < 65 && rest_timer >=60){//Go to measure cell 1
            digitalWrite(4,true);
            V_meas1 = analogRead(A3)*4.096/1.03;
            V_total1 += V_meas1;
            rest_timer++; 
          }else if(rest_timer < 70 && rest_timer >=65){//Go to measure cell2
            digitalWrite(4,false);
            digitalWrite(9,true);
            V_meas2 = analogRead(A2)*4.096/1.03;
            V_total2 += V_meas2;
            rest_timer++;
          }else if(rest_timer < 75 && rest_timer >=70){//Go to measure cell3
            digitalWrite(9,false);
            digitalWrite(7,true);
            V_meas3 = analogRead(A1)*4.096/1.03;
            V_total3 += V_meas3;
            rest_timer++;
          }
          else if(rest_timer<76){//Calculate the mean of each cells
            digitalWrite(7,false);
            V_mean1 = V_total1 / 5;
            V_mean2 = V_total2 / 5;
            V_mean3 = V_total3 / 5;
            V_mean_total = (V_mean1+V_mean2+V_mean3)/3;
            rest_timer++;
          }
          else{//go to compare and balance state
            next_state = 4;
            //rest_timer = 0;
            V_meas1 = 0;
            V_total1 = 0;
            V_meas2 = 0;
            V_total2 = 0;
            V_meas3 = 0;
            V_total3 = 0;
            V_mean_total = 0;
      }
        
      }else{//go to cv charge state
        next_state = 2;
        rest_timer = 0;
      }
        break;
      }
  

    case 2 :{//cv 1
      cc = 0;
        voltage_ref = 3600;
        if (V_mean_total < 3540) { //Go to CV
          soc = soc + (current_measure/6246000)*100; //remember to change the battery capacity after testing
          if (rest_timer < 60){
            rest_timer++; 
            V_mean1 = 0; 
            next_state = 2;        
          }
          else if (rest_timer < 65 && rest_timer >= 60){//measure cell1
            digitalWrite(4,true);
            V_meas1 = analogRead(A3)*4.096/1.03;
            V_total1 += V_meas1;
            rest_timer++; 
          }
          else if(rest_timer < 70 && rest_timer >= 65){//measure cell2
            digitalWrite(4,false);
            digitalWrite(9,true);
            V_meas2 = analogRead(A2)*4.096/1.03;
            V_total2 += V_meas2;
            rest_timer++;
          }
          else if(rest_timer < 75 && rest_timer >= 70){//measure cell3
            digitalWrite(9,false);
            digitalWrite(7,true);
            V_meas3 = analogRead(A1)*4.096/1.03;
            V_total3 += V_meas3;
            rest_timer++;
          }
          else if(rest_timer < 76 ){
            digitalWrite(7,false);
            V_mean1 = V_total1 / 5;
            V_mean2 = V_total2 / 5;
            V_mean3 = V_total3 / 5;
            V_mean_total = (V_mean1+V_mean2+V_mean3)/3;
            rest_timer++;
          }
          else{ // Go to Comparison stage
            next_state = 4;
            //rest_timer = 0;
            V_meas1 = 0;
            V_total1 = 0;
            V_meas2 = 0;
            V_total2 = 0;
            V_meas3 = 0;
            V_total3 = 0;
            V_mean_total = 0;
          }
        } 
      else{ // otherwise go to rest state
          next_state = 3;
          rest_timer = 0;
        }
        break;   
    }
    case 3:{ // Rest state
        cc = 1;
        current_ref = 0;
        voltage_ref = 0;
         soc = soc + current_measure/(57600);
        if(V_Bat < 2900){
          next_state = 1;
        }else{
        next_state = 3;
        }

        break;
      }
    case 4:{//Compare and decide whether we have to balance
      current_ref = 0;
      rest_timer = 0;
      cc=1;
      if(abs(V_mean1-V_mean2)>500){//go to balancing state(1&2)
        
        next_state = 5;
        rest_timer = 0;
        
      }else if(abs(V_mean1-V_mean3)>500){//go to balancing state(1&3)
        
        next_state = 8;
        rest_timer = 0;
        
      }else if(abs(V_mean2-V_mean3)>500){//gp to balancing state(2&3)
        
        next_state = 11;
        rest_timer = 0;
        
      }else{//otherwise go to cc Cell 1 meas
        
        next_state = 1;
        rest_timer = 0;
      }
//      if(input_switch == 0){ // UNLESS the switch = 0, then go back to start
//        
//          next_state = 0;
//          rest_timer = 0;
//        }
        break; 
      }

      case 5:{//cell balancing state(1&2)
      cc = 1;
      current_ref = 0;
      if(V_mean1 > V_mean2){//discharge cell1
        next_state = 6;
        rest_timer = 0;
      }else{//discharge cell2
        next_state = 7;
        rest_timer = 0;
      }
      break;
     }

      case 6:{//discharge cell1
        cc = 1;
        current_ref = 0;
        if(abs(V_mean1 - V_mean2) < 400){
          next_state = 4;
        }else{
          if(rest_timer < 60){
            rest_timer++;
            next_state = 6;
            V_meas1 = 0;
          }
          else if(rest_timer < 65 && rest_timer >= 60){
            digitalWrite(5,true);
            digitalWrite(4,true);
            V_meas1 = analogRead(A3)*4.096/1.03;
            V_total1 += V_meas1;
            rest_timer++;
            next_state = 6;
          }
          else if (rest_timer < 66){
            digitalWrite(4,false);
            digitalWrite(5,false);
            V_mean1 = V_total1 / 5;
            rest_timer++;
            next_state = 6;
          }
          else{
            next_state = 6;
            rest_timer = 0;
          }
        }
        break;
      }

      case 7:{//discharge cell2
        cc = 1;
        current_ref = 0;
       if (abs((V_mean2 - V_mean1) < 400)){
        next_state = 4;
       }
       else{
        if (rest_timer < 60){
            rest_timer++;  
            next_state = 7;
            V_meas2 = 0;        
          }
          else if (rest_timer < 65 && rest_timer >= 60){
            digitalWrite(10,true);
            digitalWrite(9,true);
            V_meas2 = analogRead(A2)*4.096/1.03;
            V_total2 += V_meas2;
            rest_timer++;
            next_state = 7; 
          }else if(rest_timer <66){
            digitalWrite(9,false);
            digitalWrite(10,false);
            V_mean2 = V_total2 / 5;
            rest_timer++;
            next_state = 7; 
          }
          else{ 
            next_state = 7;
            rest_timer = 0;
          }
       }
        break;   
        }

      case 8:{//cell balancing state(1&3)
      cc = 1;
      current_ref = 0;
      if(V_mean1 > V_mean3){//discharge cell1
        next_state = 9;
        rest_timer = 0;
      }else{//discharge cell2
        next_state = 10;
        rest_timer = 0;
      }
      break;
     }

     case 9:{//discharge cell1
        cc = 1;
        current_ref = 0;
        if(abs(V_mean1 - V_mean3) < 400){
          next_state = 4;
        }else{
          if(rest_timer < 60){
            rest_timer++;
            next_state = 9;
            V_meas1 = 0;
          }
          else if(rest_timer < 65 && rest_timer >= 60){
            digitalWrite(5,true);
            digitalWrite(4,true);
            V_meas1 = analogRead(A3)*4.096/1.03;
            V_total1 += V_meas1;
            rest_timer++;
            next_state = 9;
          }
          else if (rest_timer < 66){
            digitalWrite(4,false);
            digitalWrite(5,false);
            V_mean1 = V_total1 / 5;
            rest_timer++;
            next_state = 9;
          }
          else{
            next_state = 9;
            rest_timer = 0;
          }
        }
        break;
      }

      case 10:{//discharge cell3
        cc = 1;
        current_ref = 0;
       if (abs((V_mean3 - V_mean1) < 400)){
        next_state = 4;
       }
       else{
        if (rest_timer < 60){
            rest_timer++;  
            next_state = 10;
            V_meas3 = 0;        
          }
          else if (rest_timer < 65 && rest_timer >= 60){
            digitalWrite(8,true);
            digitalWrite(7,true);
            V_meas3 = analogRead(A1)*4.096/1.03;
            V_total3 += V_meas3;
            rest_timer++;
            next_state = 10; 
          }else if(rest_timer <66){
            digitalWrite(7,false);
            digitalWrite(8,false);
            V_mean3 = V_total3 / 5;
            rest_timer++;
            next_state = 10;
          }
          else{ 
            next_state = 10;
            rest_timer = 0;
          }
       }
        break;   
        }

      case 11:{//cell balancing state(2&3)
      cc = 1;
      current_ref = 0;
      if(V_mean2 > V_mean3){//discharge cell2
        next_state = 12;
        rest_timer = 0;
      }else{//discharge cell3
        next_state = 13;
        rest_timer = 0;
      }
      break;
     }

     case 12:{//discharge cell2
        cc = 1;
        current_ref = 0;
       if (abs((V_mean2 - V_mean3) < 400)){
        next_state = 4;
       }
       else{
        if (rest_timer < 60){
            rest_timer++;  
            next_state = 12;
            V_meas3 = 0;        
          }
          else if (rest_timer < 65 && rest_timer >= 60){
            digitalWrite(8,true);
            digitalWrite(7,true);
            V_meas3 = analogRead(A1)*4.096/1.03;
            V_total3 += V_meas3;
            rest_timer++;
            next_state = 12; 
          }else if(rest_timer <66){
            digitalWrite(7,false);
            digitalWrite(8,false);
            V_mean3 = V_total3 / 5;
            rest_timer++;
            next_state = 12;
          }
          else{ 
            next_state = 12;
            rest_timer = 0;
          }
       }
        break;   
        }

      case 13:{//discharge cell3
        cc = 1;
        current_ref = 0;
       if (abs((V_mean3 - V_mean2) < 400)){
        next_state = 4;
       }
       else{
        if (rest_timer < 60){
            rest_timer++;  
            next_state = 13;
            V_meas3 = 0;        
          }
          else if (rest_timer < 65 && rest_timer >= 60){
            digitalWrite(8,true);
            digitalWrite(7,true);
            V_meas3 = analogRead(A1)*4.096/1.03;
            V_total3 += V_meas3;
            rest_timer++;
            next_state = 13; 
          }else if(rest_timer <66){
            digitalWrite(7,false);
            digitalWrite(8,false);
            V_mean3 = V_total3 / 5;
            rest_timer++;
            next_state = 13;
          }
          else{ 
            next_state = 13;
            rest_timer = 0;
          }
       }
        break;   
        }

     case 15:{//Discharging state
        cc = 1;
        current_ref = -350;
        if (V_mean_total > 2500) { 
          soc = soc + (current_measure/57600);//remember to change the correct battery capacity after testing
          if (rest_timer < 60){
            rest_timer++;  
            next_state = 15;        
          }
          else if(rest_timer < 65 && rest_timer >=60){//Go to measure cell 1
            digitalWrite(4,true);
            V_meas1 = analogRead(A3)*4.096/1.03;
            V_total1 += V_meas1;
            rest_timer++; 
          }else if(rest_timer < 70 && rest_timer >=65){//Go to measure cell2
            digitalWrite(4,false);
            digitalWrite(9,true);
            V_meas2 = analogRead(A2)*4.096/1.03;
            V_total2 += V_meas2;
            rest_timer++;
          }else if(rest_timer < 75 && rest_timer >=70){//Go to measure cell3
            digitalWrite(9,false);
            digitalWrite(7,true);
            V_meas3 = analogRead(A1)*4.096/1.03;
            V_total3 += V_meas3;
            rest_timer++;
          }
          else if(rest_timer<76){
            digitalWrite(7,false);
            V_mean1 = V_total1 / 5;
            V_mean2 = V_total2 / 5;
            V_mean3 = V_total3 / 5;
            V_mean_total = (V_mean1+V_mean2+V_mean3)/3;
            rest_timer++;
          }
          else{//go to compare and balance state
            next_state = 16;
            //rest_timer = 0;
            V_meas1 = 0;
            V_total1 = 0;
            V_meas2 = 0;
            V_total2 = 0;
            V_meas3 = 0;
            V_total3 = 0;
            V_mean_total = 0;
      }
        
      }else{//go to cv charge state
        next_state = 3;
        rest_timer = 0;
      }
        break;
     }

     case 16:{//Compare and decide whether we have to balance
      current_ref = 0;
      rest_timer = 0;
      cc=1;
      if(abs(V_mean1-V_mean2)>500){//go to balancing state(1&2)
        
        next_state = 5;
        rest_timer = 0;
        
      }else if(abs(V_mean1-V_mean3)>500){//go to balancing state(1&3)
        
        next_state = 8;
        rest_timer = 0;
        
      }else if(abs(V_mean2-V_mean3)>500){//gp to balancing state(2&3)
        
        next_state = 11;
        rest_timer = 0;
        
      }else{//otherwise go to cc Cell 1 meas
        
        next_state = 15;
        rest_timer = 0;
      }

        break; 
     }

     case 14: { // Error state
        current_ref = 0;
        next_state = 8; // Always stay here
        soc = 0;
        digitalWrite(4,false);
        digitalWrite(9,false);
        digitalWrite(7,false);

        break;
      }
      default :{ // Should not end up here ....
        Serial.println("Boop");
        current_ref = 0;
        next_state = 14; // So if we are here, we go to error
      }
      
    }
     dataString = String(state_num) + "," + String(rest_timer) + "," + String(V_Bat) + "," + String(V_meas1) + "," + String(V_mean1) + "," + String(V_meas2) + "," + String(V_mean2) + ","+ String(V_meas3) + "," + String(V_mean3)+ "," +String(current_measure) + "," + String(V_mean_total)+","+String(soc); //build a datastring for the CSV file
    Serial.println(dataString); // send it to serial as well in case a computer is connected
    File dataFile = SD.open("BatCycle.csv", FILE_WRITE); // open our CSV file
    if (dataFile){ //If we succeeded (usually this fails if the SD card is out)
      dataFile.println(dataString); // print the data
    } else {
      Serial.println("File not open"); //otherwise print an error
    }
    dataFile.close(); // close the file
    int_count = 0; // reset the interrupt count so we dont come back here for 1000ms
  }
    
 }
// Timer A CMP1 interrupt. Every 1000us the program enters this interrupt. This is the fast 1kHz loop
ISR(TCA0_CMP1_vect) {
  loop_trigger = 1; //trigger the loop when we are back in normal flow
  TCA0.SINGLE.INTFLAGS |= TCA_SINGLE_CMP1_bm; //clear interrupt flag
}

float saturation( float sat_input, float uplim, float lowlim) { // Saturation function
  if (sat_input > uplim) sat_input = uplim;
  else if (sat_input < lowlim ) sat_input = lowlim;
  else;
  return sat_input;
}


  
float pidi(float pid_input) { // discrete PID function
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


float pidv( float pid_input){
  float e_integration;
  e0v = pid_input;
  e_integration = e0v;
 
  //anti-windup, if last-time pid output reaches the limitation, this time there won't be any intergrations.
  if(u1v >= uv_max) {
    e_integration = 0;
  } else if (u1v <= uv_min) {
    e_integration = 0;
  }

  delta_uv = kpv*(e0v-e1v) + kiv*Ts*e_integration + kdv/Ts*(e0v-2*e1v+e2v); //incremental PID programming avoids integrations.there is another PID program called positional PID.
  u0v = u1v + delta_uv;  //this time's control output

  //output limitation
  saturation(u0v,uv_max,uv_min);
  
  u1v = u0v; //update last time's control output
  e2v = e1v; //update last last time's error
  e1v = e0v; // update last time's error
  return u0v;
}  
  
