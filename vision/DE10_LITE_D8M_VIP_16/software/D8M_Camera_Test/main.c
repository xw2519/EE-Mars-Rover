#include "system.h"
#include "altera_avalon_timer_regs.h"
#include "altera_avalon_timer.h"
#include "altera_avalon_pio_regs.h"
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


//offsets
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
Vision ---> Control
s - Emergency Stop                    {'s', <sign>, <2-digit decimal>}
p - Pause to gather data
c - Continue after gathering data
b - Sending ball data                 {'b', colour, 2 digit perpendicular distance, 2 digit other distance}
\n - End Transmission
R,P,Y,G,B,U - Colours

Control ---> Vision
m - Movement command received
s - Stop command received
*/

alt_up_rs232_dev* ctrl_uart;

alt_u16 gain          =  0x7FF;
alt_u32 exposure      = 0x2000;
alt_u16 current_focus =    300;

alt_u16 x_min, x_max, distance, diameter, mid_pos, angle, colour;
alt_u16 filter_x_min[5], filter_x_max[5], filter_y_min[5], filter_y_max[5], ball_bounds[4];
alt_u8  balls_detected, filter_id;
alt_u8  calibration=0, process=0;

array_u8  ball_colours, ball_distances, ball_angles, ball_sent;
array_u16 ball_x_min, ball_x_max;

alt_u8  prompt, parity, ack=0;
alt_u8  closest_distance=0xFF, moving=1, stop_ticks=0, last_command='c', new_data, last_colour='U';

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


void timer_init(void* isr){

    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_BASE, 0x0003);
    IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE, 0);
    IOWR_ALTERA_AVALON_TIMER_PERIODL(TIMER_BASE, 0x0900);
    IOWR_ALTERA_AVALON_TIMER_PERIODH(TIMER_BASE, 0x0000);
    alt_irq_register(TIMER_IRQ, 0, isr);
    IOWR_ALTERA_AVALON_TIMER_CONTROL(TIMER_BASE, 0x0007);
}

void sys_timer_isr(){

  // touch KEY0 to trigger Auto focus, KEY1 to trigger gain adjustment
  if((IORD(KEY_BASE,0)&0x03) == 0x01){
    current_focus = Focus_Window(320,240);

    if(ctrl_uart){
      if(alt_up_rs232_get_available_space_in_write_FIFO(ctrl_uart)){
        alt_up_rs232_write_data(ctrl_uart, 'm');
      }
    }

  }
  if((IORD(KEY_BASE,0)&0x03) == 0x02){
    calibration = 0;
    gain        = 0x7FF;
    exposure    = 0x2000;

    OV8865SetGain(gain);
    OV8865SetExposure(exposure);

    printf("\nGain = %x\n", gain);
    printf("Exposure = %x\n", exposure);
  }

  //Read messages from the image processor and process them
  while ((IORD(EEE_IMGPROC_0_BASE,EEE_IMGPROC_STATUS)>>8) & 0xff) { 	//Find out if there are words to read
      int word  = IORD(EEE_IMGPROC_0_BASE,EEE_IMGPROC_MSG); 			//Get next word from message buffer
      x_min     = (word>>11) & 0x7FF;
      x_max     =  word      & 0x7FF;
      filter_id = (word>>22) & 0x0FF;

      if((word&0xC0000000)&&(filter_id == 'D')){                //When last part of data from image processor arrives

        if(calibration<CALIBRATION_MAX){                        //If calibrating, reduce gain if ball count is not zero
          if(balls_detected && (exposure>EXPOSURE_STEP)){
            if(gain>GAIN_STEP){
              gain -= GAIN_STEP;
              OV8865SetGain(gain);
              printf("Gain = %x\n", gain);
            }else{
              exposure -= EXPOSURE_STEP;
              OV8865SetExposure(exposure);
              printf("Exposure = %x\n", exposure);
            }
          }else{
            calibration++;
            printf("Calibration = %d\n", calibration);
            if((calibration==CALIBRATION_MAX) && (exposure>EXPOSURE_STEP)){
              if(gain>GAIN_STEP){
                gain -= GAIN_STEP;
                OV8865SetGain(gain);
                printf("Gain = %x\n", gain);
              }else{
                exposure -= EXPOSURE_STEP;
                OV8865SetExposure(exposure);
                printf("Exposure = %x\n", exposure);
              }
            }
          }
        }

        if(calibration >= CALIBRATION_MAX){ process = 1; }     //Start processing the data if calibrated
        else{ balls_detected = 0; }
      }

      if(filter_id){                                          //The word is about filter data, append to arrays
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
      }else if((x_max-x_min)>0x20){                              //The word is about ball data, append to arrays and increment ball count
        if((ball_x_min.used<8)&&(calibration == CALIBRATION_MAX)){
          appendArray_u16(&ball_x_min, x_min);
          appendArray_u16(&ball_x_max, x_max);
        }
        balls_detected = 1;
      }
  }

  // Process last batch of ball data
  if(process){
    ball_colours.used = 0;
    ball_distances.used = 0;
    ball_angles.used = 0;
    new_data = 0;

    printf("Counted %d balls.\n\n", ball_x_min.used);                                                // Ball count

    for(alt_u8 i=0; i<(ball_x_min.used); i++){
      diameter = ball_x_max.data[i] - ball_x_min.data[i];
      distance = 2560/diameter;
      mid_pos  = (ball_x_min.data[i]>>1) + (ball_x_max.data[i]>>1);
      if(mid_pos>320){ angle = ((((mid_pos-320)<<7)/distance)>>6) + ((((mid_pos-320)<<7)/distance)>>7); }
      else           { angle = ((((320-mid_pos)<<7)/distance)>>6) + ((((320-mid_pos)<<7)/distance)>>7); }

      if(distance < closest_distance){ closest_distance = distance; }
      printf("Ball %d:\n", i+1);
      printf("    Distance: %03d\n", distance);                                                      // 2) Distance to each ball
      printf("    Angle: %03d\n", angle);                                                            // 3) Angle to each ball
      printf("    Colour: ");
      for(alt_u8 j=4; j<255; j--){
        if(!( (filter_x_min[j] > ball_x_max.data[i]) || (filter_x_max[j] < ball_x_min.data[i]) )){
          colour = get_filter_id(j);
          printf("%c ", colour);                                                                     // 1) Colour of ball
          break;
        }
        if(!j){
          colour = 'U';
          printf("%c ", colour);
        }
      }
      printf("\n\n");

      for(alt_u8 j=0; j<(ball_sent.used); j++){
        if(ball_sent.data[j]==colour){ break; }
        if(j==(ball_sent.used-1)){ new_data++; }
      }
      if(ball_sent.used==0){ new_data++; }

      appendArray_u8(&ball_colours, colour);
      appendArray_u8(&ball_distances, distance);
      appendArray_u8(&ball_angles, angle);
    }

    // Send messages to UART using processed data
    if(ctrl_uart&&((closest_distance<30)||(ball_bounds[3]>=472))&&moving&&(!stop_ticks)){
      if(alt_up_rs232_get_available_space_in_write_FIFO(ctrl_uart)){
        alt_up_rs232_write_data(ctrl_uart, 's');
        last_command = 's';
        stop_ticks = 3;
      }
    }

    if(ctrl_uart&&new_data&&moving&&(!stop_ticks)){
      if(alt_up_rs232_get_available_space_in_write_FIFO(ctrl_uart)){
        alt_up_rs232_write_data(ctrl_uart, 'p');
        last_command = 'p';
        stop_ticks = 3;
      }
    }

    if(ctrl_uart&&(!moving)&&((last_command=='s')||(last_command=='p'))&&new_data&&(!ack)){

      if(ball_sent.used==0){
        if(alt_up_rs232_get_available_space_in_write_FIFO(ctrl_uart)>6){
          alt_up_rs232_write_data(ctrl_uart, 'b');
          alt_up_rs232_write_data(ctrl_uart, ball_colours.data[0]);
          alt_up_rs232_write_data(ctrl_uart, ((ball_distances.data[0])/10)+48);
          alt_up_rs232_write_data(ctrl_uart, ((ball_distances.data[0])%10)+48);
          alt_up_rs232_write_data(ctrl_uart, ((ball_angles.data[0])/10)+48);
          alt_up_rs232_write_data(ctrl_uart, ((ball_angles.data[0])%10)+48);
          last_colour = ball_colours.data[0];
          ack = 3;
        }
      }

      for(alt_u8 i=0; (i<(ball_colours.used))&&(!ack); i++){
        for(alt_u8 j=0; j<(ball_sent.used); j++){
          if(ball_sent.data[j]==ball_colours.data[i]){ break; }
          if(j==(ball_sent.used-1)){
            if(alt_up_rs232_get_available_space_in_write_FIFO(ctrl_uart)>6){
              alt_up_rs232_write_data(ctrl_uart, 'b');
              alt_up_rs232_write_data(ctrl_uart, ball_colours.data[i]);
              alt_up_rs232_write_data(ctrl_uart, ((ball_distances.data[i])/10)+48);
              alt_up_rs232_write_data(ctrl_uart, ((ball_distances.data[i])%10)+48);
              alt_up_rs232_write_data(ctrl_uart, ((ball_angles.data[i])/10)+48);
              alt_up_rs232_write_data(ctrl_uart, ((ball_angles.data[i])%10)+48);
              last_colour = ball_colours.data[i];
              ack = 3;
            }
          }
        }
      }
    }

    if(ctrl_uart&&((last_command=='s')||(last_command=='p'))&&(!moving)&&(!new_data)){
      if(alt_up_rs232_get_available_space_in_write_FIFO(ctrl_uart)){
        alt_up_rs232_write_data(ctrl_uart, 'c');
        last_command = 'c';
        stop_ticks = 0;
      }
    }

    if(stop_ticks){ stop_ticks--; }
    if(ack){ ack--; }
    closest_distance = 0xFF;
    process = 0;
    ball_x_min.used = 0;
    ball_x_max.used = 0;
  }

  IOWR_ALTERA_AVALON_TIMER_STATUS(TIMER_BASE, 0);
}

int main(){

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

    ctrl_uart = alt_up_rs232_open_dev("/dev/control_uart");
		if(ctrl_uart){ printf("Started control uart...\n"); }

    initArray_u16(&ball_x_min, 5);
    initArray_u16(&ball_x_max, 5);
    initArray_u8(&ball_colours, 5);
    initArray_u8(&ball_distances, 5);
    initArray_u8(&ball_angles, 5);
    initArray_u8(&ball_sent, 5);


		OV8865SetExposure(exposure);
		OV8865SetGain(gain);
		Focus_Init();

    timer_init(sys_timer_isr);


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
          }

          /*else if(prompt=='c'){ moving = 1; }
          else if(prompt=='p'){ moving = 0; }
          else if(prompt=='b'){
            appendArray_u8(&ball_sent, last_colour);
            ack = 0;
          }*/

          printf("UART received: %c\n", prompt);
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
