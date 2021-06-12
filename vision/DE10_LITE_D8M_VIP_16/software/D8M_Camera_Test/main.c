#include "system.h"
#include "altera_avalon_timer_regs.h"
#include "altera_avalon_timer.h"
#include "altera_avalon_pio_regs.h"
#include "altera_up_avalon_accelerometer_spi.h"
#include "altera_up_avalon_rs232.h"
#include "sys/alt_irq.h"
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include "I2C_core.h"
#include "terasic_includes.h"
#include "mipi_camera_config.h"
#include "mipi_bridge_config.h"

#include "auto_focus.h"

#include <fcntl.h>
#include <unistd.h>


//debug defines
#define VIEW_BALL_COUNT
#define VIEW_BALL_DATA
#define VIEW_UART_MSGS
#define VIEW_AUTO_GAIN
//#define VIEW_ACC_READS

//offsets for image processor memory mapped interface
#define EEE_IMGPROC_STATUS 0
#define EEE_IMGPROC_MSG    1
#define EEE_IMGPROC_ID     2
#define EEE_IMGPROC_BBCOL  3

#define GAIN_STEP         0x40
#define EXPOSURE_STEP    0x100
#define CALIBRATION_MAX      8

#define MIPI_REG_PHYClkCtl		0x0056
#define MIPI_REG_PHYData0Ctl	0x0058
#define MIPI_REG_PHYData1Ctl	0x005A
#define MIPI_REG_PHYData2Ctl	0x005C
#define MIPI_REG_PHYData3Ctl	0x005E
#define MIPI_REG_PHYTimDly		0x0060
#define MIPI_REG_PHYSta			  0x0062
#define MIPI_REG_CSIStatus		0x0064
#define MIPI_REG_CSIErrEn	  	0x0066
#define MIPI_REG_MDLSynErr		0x0068
#define MIPI_REG_FrmErrCnt		0x0080
#define MIPI_REG_MDLErrCnt		0x0090

//dynamic arrays, could easily scale system to detect more than 5 balls if required
typedef struct{
  alt_u8 *data;
  size_t used;
  size_t size;
} array_u8;
typedef struct{
  alt_u16 *data;
  size_t used;
  size_t size;
} array_u16;

/*
-------Comment notation--------
////////// => Section
//-------- => Subsection


------------Transmission over UART between vision and control subsystems---------------

Vision ---> Control
s - Emergency Stop            - If stopping reason unknown, send {bX00+00} after stop, else send ball causing stop
p - Pause to gather data
c - Continue after gathering data
b - Ball data                 {'b', colour, 2 digit distance, sign, 2 digit other distance} (+right, -left) (R,P,Y,G,B,U - Colours)
t - Tilt data                 {'t', 3 digit y reading, 3 digit z reading} (Readings in 10-bit 2's complement)

Control ---> Vision
m - Movement command received
s - Stop command received
*/

alt_up_rs232_dev             *ctrl_uart;             // UART connection to control
alt_up_accelerometer_spi_dev   *acc_dev;             // SPI connection to accelerometer

alt_u16 gain     =  0x7FF;             // Initial gain
alt_u32 exposure = 0x2000;             // Initial exposure

// Variables used for storing raw ball data from image processor
alt_32    word;
alt_u16   x_min, x_max;
alt_u16   filter_x_min[5], filter_x_max[5], filter_y_min[5], filter_y_max[5], ball_bounds[4];
array_u16 ball_x_min, ball_x_max;

// Used for storing processed ball data
alt_u16   distance, diameter, mid_pos, angle, colour, touching_edge;
array_u8  ball_colours, ball_distances, ball_angles, ball_sent;

// Used to read, store and process accelerometer data
alt_32    x_read, y_read, z_read, x_drift, y_drift, z_drift;

// Flags used to control flow of program
alt_u8    balls_detected, filter_id;
alt_u8    gain_calib=0, process=0, acc_calib=0;

// Flags and data used for communication with control over UART
alt_u8    prompt, parity, moving=0;
alt_u8    closest_distance=0xFF, closest_index, stop_reasoned, last_command='c', new_data, last_colour='U';

// Variables used to count time
alt_u8    ack=0, stop_ticks=0, acc_ticks=0;
alt_u16   mem_ticks=0;

// MIPI
void mipi_clear_error(void){
  MipiBridgeRegWrite(MIPI_REG_CSIStatus,0x01FF); // clear error
	MipiBridgeRegWrite(MIPI_REG_MDLSynErr,0x0000); // clear error
	MipiBridgeRegWrite(MIPI_REG_FrmErrCnt,0x0000); // clear error
	MipiBridgeRegWrite(MIPI_REG_MDLErrCnt, 0x0000); // clear error

  MipiBridgeRegWrite(0x0082,0x00);
	MipiBridgeRegWrite(0x0084,0x00);
	MipiBridgeRegWrite(0x0086,0x00);
	MipiBridgeRegWrite(0x0088,0x00);
	MipiBridgeRegWrite(0x008A,0x00);
	MipiBridgeRegWrite(0x008C,0x00);
	MipiBridgeRegWrite(0x008E,0x00);
	MipiBridgeRegWrite(0x0090,0x00);
}

void mipi_show_error_info(void){

	alt_u16 PHY_status, SCI_status, MDLSynErr, FrmErrCnt, MDLErrCnt;

	PHY_status = MipiBridgeRegRead(MIPI_REG_PHYSta);
	SCI_status = MipiBridgeRegRead(MIPI_REG_CSIStatus);
	MDLSynErr = MipiBridgeRegRead(MIPI_REG_MDLSynErr);
	FrmErrCnt = MipiBridgeRegRead(MIPI_REG_FrmErrCnt);
	MDLErrCnt = MipiBridgeRegRead(MIPI_REG_MDLErrCnt);

	printf("PHY_status=%xh, CSI_status=%xh, MDLSynErr=%xh, FrmErrCnt=%xh, MDLErrCnt=%xh\r\n", PHY_status, SCI_status, MDLSynErr,FrmErrCnt, MDLErrCnt);
}

void mipi_show_error_info_more(void){
    printf("FrmErrCnt = %d\n",MipiBridgeRegRead(0x0080));
    printf("CRCErrCnt = %d\n",MipiBridgeRegRead(0x0082));
    printf("CorErrCnt = %d\n",MipiBridgeRegRead(0x0084));
    printf("HdrErrCnt = %d\n",MipiBridgeRegRead(0x0086));
    printf("EIDErrCnt = %d\n",MipiBridgeRegRead(0x0088));
    printf("CtlErrCnt = %d\n",MipiBridgeRegRead(0x008A));
    printf("SoTErrCnt = %d\n",MipiBridgeRegRead(0x008C));
    printf("SynErrCnt = %d\n",MipiBridgeRegRead(0x008E));
    printf("MDLErrCnt = %d\n",MipiBridgeRegRead(0x0090));
    printf("FIFOSTATUS = %d\n",MipiBridgeRegRead(0x00F8));
    printf("DataType = 0x%04x\n",MipiBridgeRegRead(0x006A));
    printf("CSIPktLen = %d\n",MipiBridgeRegRead(0x006E));
}

bool MIPI_Init(void){

		bool bSuccess;

		bSuccess = oc_i2c_init_ex(I2C_OPENCORES_MIPI_BASE, 50*1000*1000,400*1000); //I2C: 400K
		if (!bSuccess)
				printf("failed to init MIPI- Bridge i2c\r\n");

    usleep(50*1000);
    MipiBridgeInit();

    usleep(500*1000);
    MipiCameraInit();
    MIPI_BIN_LEVEL(3);

 		usleep(1000);
		return bSuccess;
}

// Functions for using dynamic array data types
void initArray_u8(array_u8 *a, size_t init_size){

  a->data = malloc(init_size * sizeof(alt_u8));
  a->used = 0;
  a->size = init_size;
}

void appendArray_u8(array_u8 *a, alt_u8 element){

  if (a->used == a->size){
    a->data = realloc(a->data, a->size * sizeof(alt_u8));
    a->size *= 2;
  }
  a->data[a->used] = element;
  a->used++;
}

void freeArray_u8(array_u8 *a){
  free(a->data);
  a->data = NULL;
  a->used = 0;
  a->size = 0;
}

void initArray_u16(array_u16 *a, size_t init_size){

  a->data = malloc(init_size * sizeof(alt_u16));
  a->used = 0;
  a->size = init_size;
}

void appendArray_u16(array_u16 *a, alt_u16 element){

  if (a->used == a->size){
    a->data = realloc(a->data, a->size * sizeof(alt_u16));
    a->size *= 2;
  }
  a->data[a->used] = element;
  a->used++;
}

void freeArray_u16(array_u16 *a){
  free(a->data);
  a->data = NULL;
  a->used = 0;
  a->size = 0;
}


alt_u8 filter_index(alt_u8 filter_id){
  switch(filter_id){
    case 'R': {
      return 0;}
    case 'P': {
      return 1;}
    case 'Y': {
      return 2;}
    case 'G': {
      return 3;}
    case 'B': {
      return 4;}
    default:{
      return 0;}
  }
}

alt_u8 get_filter_id(alt_u8 index){
  switch(index){
    case 0: {
      return 'R';}
    case 1: {
      return 'P';}
    case 2: {
      return 'Y';}
    case 3: {
      return 'G';}
    case 4: {
      return 'B';}
    default:{
      return 'U';}
  }
}

void send_ball_data(alt_u8 colour, alt_u8 distance, alt_u8 angle){
  if(ctrl_uart){
    if(alt_up_rs232_get_available_space_in_write_FIFO(ctrl_uart)>7){
      alt_up_rs232_write_data(ctrl_uart, 'b');
      alt_up_rs232_write_data(ctrl_uart, colour);
      alt_up_rs232_write_data(ctrl_uart, (distance/10)+48);
      alt_up_rs232_write_data(ctrl_uart, (distance%10)+48);
      if(angle<64){
        alt_up_rs232_write_data(ctrl_uart, '+');
      }else{
        angle = -angle;
        alt_up_rs232_write_data(ctrl_uart, '-');
      }
      alt_up_rs232_write_data(ctrl_uart, (angle/10)+48);
      alt_up_rs232_write_data(ctrl_uart, (angle%10)+48);
      last_colour = colour;
      ack = 3;
    }
  }
}


void timer_init(void* isr){

    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_BASE, 0x0003);
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE, 0);
    IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_BASE, 0x0900);
    IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_BASE, 0x0000);
    alt_irq_register(TIMER_IRQ, 0, isr);
    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_BASE, 0x0007);
}

void sys_timer_isr(){

  //////////////////////////////////////////////////////////////////////// - Process button inputs(KEY0 & KEY1)

  if((IORD(KEY_BASE,0)&0x03) == 0x01){           // touch KEY1 to trigger auto focus
    Focus_Window(320,240);
  }
  if((IORD(KEY_BASE,0)&0x03) == 0x02){           // touch KEY0 to trigger gain adjustment
    gain_calib = 0;
    gain       = 0x7FF;
    exposure   = 0x2000;

    OV8865SetGain(gain);
    OV8865SetExposure(exposure);

    #ifdef VIEW_AUTO_GAIN
      printf("\nGain = %x\n", gain);
      printf("Exposure = %x\n", exposure);
    #endif
  }

  //////////////////////////////////////////////////////////////////////// - Record incoming data from image processor

  while ((IORD(EEE_IMGPROC_0_BASE,EEE_IMGPROC_STATUS)>>8) & 0xff) {    // Check if there is data in message buffer
    word      = IORD(EEE_IMGPROC_0_BASE,EEE_IMGPROC_MSG); 			       // Get next word from message buffer
    x_min     = (word>>11) & 0x7FF;
    x_max     =  word      & 0x7FF;
    filter_id = (word>>22) & 0x0FF;

    //----------------------------------------------------------------- Record filter and ball data

    // Record filter data
    if(filter_id){
      if(filter_id=='D'){
        if(word&0xC0000000){
          ball_bounds[2] = x_min;
          ball_bounds[3] = x_max;
        }else{
          ball_bounds[0] = x_min;
          ball_bounds[1] = x_max;
        }
      }else{
        if(word&0xC0000000){
          filter_y_min[filter_index(filter_id)] = x_min;
          filter_y_max[filter_index(filter_id)] = x_max;
        }else{
          filter_x_min[filter_index(filter_id)] = x_min;
          filter_x_max[filter_index(filter_id)] = x_max;
        }
      }

    // Record ball data
    }else if(((x_max-x_min)>=0x20)&&(ball_x_min.used<8)&&(gain_calib==CALIBRATION_MAX)){
      appendArray_u16(&ball_x_min, x_min);
      appendArray_u16(&ball_x_max, x_max);
    }

    //----------------------------------------------------------------- Calibrate or process data when a full batch has arrived

    if((word&0xC0000000)&&(filter_id == 'D')){

      if(gain_calib<CALIBRATION_MAX){                        // Do calibration if not calibrated

        if(!balls_detected){ gain_calib++; }

        if((balls_detected||(gain_calib==CALIBRATION_MAX)) && (exposure>EXPOSURE_STEP)){
          if(gain>GAIN_STEP){
            gain -= GAIN_STEP;
            OV8865SetGain(gain);
          }else{
            exposure -= EXPOSURE_STEP;
            OV8865SetExposure(exposure);
          }
        }

        #ifdef VIEW_AUTO_GAIN
          printf("G = %03x   ", gain);
          printf("E = %04x   ", exposure);
          printf("C = %02d\n", gain_calib);
        #endif
      }

      if(gain_calib >= CALIBRATION_MAX){ process = 1; }      // Start processing the data if calibrated
      else{ balls_detected = 0; }
    }

    balls_detected |= (!filter_id)&&((x_max-x_min)>20);
    balls_detected |= ball_bounds[3]>=448;
    for(alt_u8 i=0; i<5; i++){
      if(!(filter_x_min[i]>= 472)){ balls_detected |= 1; }
    }
  }

  //////////////////////////////////////////////////////////////////////// - Process a full batch of data

  if(process){
    ball_colours.used   = 0;
    ball_distances.used = 0;
    ball_angles.used    = 0;
    closest_distance    = 0xFF;
    new_data            = 0;

    #ifdef VIEW_BALL_COUNT
      printf("Counted %d balls.\n\n", ball_x_min.used);                                                   // Ball count
    #endif

    //----------------------------------------------------------------- Iterate over individual balls detected and process data

    for(alt_u8 i=0; i<(ball_x_min.used); i++){
      diameter = ball_x_max.data[i] - ball_x_min.data[i];                                            // Diameter of ball in pixels
      distance = 2560/diameter;                                                                           // 2) Distance to each ball

      mid_pos  = (ball_x_min.data[i]>>1) + (ball_x_max.data[i]>>1);                                  // Position of the middle point of the ball
      if(mid_pos>320){ angle = ((((mid_pos-320)<<7)/diameter)>>5); }                                      // 3) Angle to each ball
      else           { angle = -((((320-mid_pos)<<7)/diameter)>>5); }

      for(alt_u8 j=4; j<255; j--){                                                                        // 1) Colour of ball
        if(!( (filter_x_min[j] > ball_x_max.data[i]) || (filter_x_max[j] < ball_x_min.data[i]) )){
          colour = get_filter_id(j);
          break;
        }
        if(!j){ colour = 'U'; }
      }

      touching_edge = ((ball_x_min.data[i]<=8)||(ball_x_max.data[i]>=632)||(ball_bounds[3]>=472));   // Is ball touching an edge of the frame

      if(distance < closest_distance){                                                               // Records index and distance to closest ball
        closest_distance = distance;
        if(!touching_edge){ closest_index = i; }
        else              { closest_index = 0xFF; }
      }

      if(!touching_edge){                                                                            // Record data if ball is not toughing the edge

        if(ball_sent.used==0){ new_data++; }               // If ball has not been seen before, new data found
        for(alt_u8 j=0; j<(ball_sent.used); j++){
          if(ball_sent.data[j]==colour){ break; }
          if(j==(ball_sent.used-1)){ new_data++; }
        }

        appendArray_u8(&ball_colours, colour);
        appendArray_u8(&ball_distances, distance);
        appendArray_u8(&ball_angles, angle);
      }

      #ifdef VIEW_BALL_DATA
        printf("Ball %d:\n", i+1);
        printf("    Distance: %03d\n", distance);
        if(angle<64){ printf("    Angle: +%03d\n", angle); }
        else        { printf("    Angle: -%03d\n", 65536-angle); }
        printf("    Colour: %c\n\n", colour);
      #endif
    }

    //----------------------------------------------------------------- Read and process accelerometer data

    /*alt_up_accelerometer_spi_read_x_axis(acc_dev, &x_read);
    alt_up_accelerometer_spi_read_y_axis(acc_dev, &y_read);
    alt_up_accelerometer_spi_read_z_axis(acc_dev, &z_read);

    if(acc_calib<CALIBRATION_MAX){
      x_drift = x_read;
      y_drift = x_read;
      z_drift = x_read;
      acc_calib++;
    }

    x_read -= x_drift;
    y_read -= y_drift;
    z_read -= z_drift;

    x_read &= 0x3FF;
    y_read &= 0x3FF;
    z_read &= 0x3FF;

    #ifdef VIEW_ACC_READS
      printf("X = %03x   Y = %03x   Z = %03x\n", x_read, y_read, z_read);
    #endif*/

    //----------------------------------------------------------------- Send messages to UART using processed data

    // Stop the rover if a ball is very close
    if(ctrl_uart&&moving&&(!stop_ticks)&&((closest_distance<30)||(ball_bounds[3]>=472))){
      if(alt_up_rs232_get_available_space_in_write_FIFO(ctrl_uart)){
        alt_up_rs232_write_data(ctrl_uart, 's');
        stop_reasoned = 0;
        last_command = 's';
        stop_ticks = 3;
      }
    }

    // Pause the rover if new data has to be transmitted
    if(ctrl_uart&&moving&&(!stop_ticks)&&new_data){
      if(alt_up_rs232_get_available_space_in_write_FIFO(ctrl_uart)){
        alt_up_rs232_write_data(ctrl_uart, 'p');
        last_command = 'p';
        stop_ticks = 3;
      }
    }

    // Transmit the reason for stopping the rover, send 'X00+00' if data not available
    if(ctrl_uart&&(!moving)&&(last_command=='s')&&(!ack)&&(!stop_reasoned)){
      if((closest_index==0xFF)||(ball_distances.data[closest_index]>30)){ send_ball_data('X', 0, 0); }
      else{ send_ball_data(ball_colours.data[closest_index], ball_distances.data[closest_index], ball_angles.data[closest_index]); }
    }

    // Transmit ball data if there is new data available
    if(ctrl_uart&&(!moving)&&new_data&&((last_command=='s')||(last_command=='p'))&&(!ack)){

      if(ball_sent.used==0){ send_ball_data(ball_colours.data[0], ball_distances.data[0], ball_angles.data[0]); }
      for(alt_u8 i=0; (i<(ball_colours.used))&&(!ack); i++){
        for(alt_u8 j=0; j<(ball_sent.used); j++){
          if(ball_sent.data[j]==ball_colours.data[i]){ break; }
          if(j==(ball_sent.used-1)){ send_ball_data(ball_colours.data[i], ball_distances.data[i], ball_angles.data[i]); }
        }
      }
    }

    // When there is no more new data and stop reason has been transmitted, send continue signal
    if(ctrl_uart&&(!moving)&&(!new_data)&&((last_command=='s')||(last_command=='p'))&&(!ack)){
      if(alt_up_rs232_get_available_space_in_write_FIFO(ctrl_uart)){
        alt_up_rs232_write_data(ctrl_uart, 'c');
        last_command = 'c';
        stop_ticks = 0;
      }
    }

    // If UART is free otherwise, send tilt sensor data
    /*if(ctrl_uart&&(!stop_ticks)&&(!ack)&&(!acc_ticks)&&(acc_calib>=CALIBRATION_MAX)){
      if(alt_up_rs232_get_available_space_in_write_FIFO(ctrl_uart)>7){
        alt_up_rs232_write_data(ctrl_uart, 't');
        alt_up_rs232_write_data(ctrl_uart, (y_read/100)+48);
        alt_up_rs232_write_data(ctrl_uart, ((y_read%100)/10)+48);
        alt_up_rs232_write_data(ctrl_uart, (y_read%10)+48);
        alt_up_rs232_write_data(ctrl_uart, (z_read/100)+48);
        alt_up_rs232_write_data(ctrl_uart, ((z_read%100)/10)+48);
        alt_up_rs232_write_data(ctrl_uart, (z_read%10)+48);
        acc_ticks = 3;
      }
    }*/

    //----------------------------------------------------------------- Update ime variables

    if(stop_ticks){ stop_ticks--; }
    if(acc_ticks){ acc_ticks--; }
    if(ack){ ack--; }
    mem_ticks++;
    if(mem_ticks>=32){                                  // Erase memory every ~10 seconds
      mem_ticks = 0;
      ball_sent.used = 0;
    }

    process = 0;
    ball_x_min.used = 0;
    ball_x_max.used = 0;
  }

  IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE, 0);        // Reset timer
}

int main(){

  //////////////////////////////////////////////////////////////////////// - Initialise MIPI, camera, UART, accelerometer, timer

  fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
  printf("Vision subsystem running...\n");

  IOWR(MIPI_PWDN_N_BASE, 0x00, 0x00);
  IOWR(MIPI_RESET_N_BASE, 0x00, 0x00);
  usleep(2000);

  IOWR(MIPI_PWDN_N_BASE, 0x00, 0xFF);
  usleep(2000);

  IOWR(MIPI_RESET_N_BASE, 0x00, 0xFF);
  printf("Image Processor ID: %x\n",IORD(EEE_IMGPROC_0_BASE,EEE_IMGPROC_ID));
  usleep(2000);

  // MIPI Init
  if (!MIPI_Init()){ printf("MIPI_Init failed!\r\n"); }
  else{ printf("MIPI_Init successful!\r\n"); }

  mipi_clear_error();
  usleep(50*1000);
  mipi_clear_error();
  usleep(1000*1000);
  mipi_show_error_info();

  // Init camera
  OV8865SetExposure(exposure);
  OV8865SetGain(gain);
  Focus_Init();
  usleep(2000*1000);

  ctrl_uart = alt_up_rs232_open_dev("/dev/control_uart");
  if(ctrl_uart){ printf("Started control uart...\n"); }
  acc_dev = alt_up_accelerometer_spi_open_dev("/dev/accelerometer_spi");
  if(acc_dev){ printf("Started accelerometer SPI...\n"); }

  initArray_u16(&ball_x_min, 5);              // Init dynamic arrays
  initArray_u16(&ball_x_max, 5);
  initArray_u8(&ball_colours, 5);
  initArray_u8(&ball_distances, 5);
  initArray_u8(&ball_angles, 5);
  initArray_u8(&ball_sent, 5);

  timer_init(sys_timer_isr);                  // Start timer

  //////////////////////////////////////////////////////////////////////// - Main loop, read from UART

  while(1){

    if(ctrl_uart){
      alt_up_rs232_enable_read_interrupt(ctrl_uart);
      if(alt_up_rs232_get_used_space_in_read_FIFO(ctrl_uart)){

        alt_up_rs232_read_data(ctrl_uart, &prompt, &parity);
        alt_up_rs232_disable_read_interrupt(ctrl_uart);
        if     (prompt=='m'){ moving = 1; }
        else if(prompt=='s'){ moving = 0; }
        else if(prompt=='a'){
          appendArray_u8(&ball_sent, last_colour);
          ack = 0;
          stop_reasoned = 1;
        }
        /*else if(prompt=='c'){ moving = 1; }
        else if(prompt=='p'){ moving = 0; }
        else if(prompt=='b'){
          appendArray_u8(&ball_sent, last_colour);
          ack = 0;
          stop_reasoned = 1;
        }*/

        #ifdef VIEW_UART_MSGS
          printf("UART received: %c\n", prompt);
        #endif
      }
    }
  }

  freeArray_u16(&ball_x_min);
  freeArray_u16(&ball_x_max);
  freeArray_u8(&ball_colours);
  freeArray_u8(&ball_distances);
  freeArray_u8(&ball_angles);
  freeArray_u8(&ball_sent);
  return 0;
}
