#include "system.h"
#include "altera_avalon_timer_regs.h"
#include "altera_avalon_timer.h"
#include "altera_avalon_pio_regs.h"
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

//EEE_IMGPROC defines

//offsets
#define EEE_IMGPROC_STATUS 0
#define EEE_IMGPROC_MSG    1
#define EEE_IMGPROC_ID     2
#define EEE_IMGPROC_BBCOL  3

#define GAIN_STEP      0x040
#define EXPOSURE_STEP 0x1000
#define CALIBRATION_MAX    8

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


typedef struct{
  alt_u16 *data;
  size_t used;
  size_t size;
} Array;

void initArray(Array *a, size_t init_size){

  a->data = malloc(init_size * sizeof(alt_u16));
  a->used = 0;
  a->size = init_size;
}

void appendArray(Array *a, alt_u16 element){

  if (a->used == a->size){
    a->data = realloc(a->data, a->size * sizeof(alt_u16));
    a->size *= 2;
  }
  a->data[a->used] = element;
  a->used++;
}

void freeArray(Array *a){
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
    case 'G': {
      return 2;}
    case 'B': {
      return 3;}
    case 'L': {
      return 4;}
  }
}

alt_u8 get_filter_id(alt_u8 index){
  switch(index){
    case 0: {
      return 'R';}
    case 1: {
      return 'P';}
    case 2: {
      return 'G';}
    case 3: {
      return 'B';}
    case 4: {
      return 'L';}
  }
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

		FILE* ctrl_uart;
	  ctrl_uart = fopen("/dev/control_uart", "r+");

		printf("Started control uart...\n");

	  usleep(2000);

		// MIPI Init
		if (!MIPI_Init()){ printf("MIPI_Init failed!\r\n"); }
		else{ printf("MIPI_Init successful!\r\n"); }

		mipi_clear_error();
		usleep(50*1000);
		mipi_clear_error();
		usleep(1000*1000);
		mipi_show_error_info();

		//////////////////////////////////////////////////////////
		alt_u16 gain          =   0x7FF;
    alt_u32 exposure      = 0xFFEFF;
    alt_u16 current_focus =     300;

		OV8865SetExposure(exposure);
		OV8865SetGain(gain);
		Focus_Init();

    //////////////////////////////////////////////////////////
    alt_u16 x_min, x_max, distance;
    alt_u8 ball_count, filter_id;
    alt_u8 calibration = 0, process = 0;

    Array ball_x_min, ball_x_max;
    alt_u16 filter_x_min[5], filter_x_max[5], filter_y_min[5], filter_y_max[5];

    initArray(&ball_x_min, 4);
    initArray(&ball_x_max, 4);

		while(1){

				// touch KEY0 to trigger Auto focus, KEY1 to trigger gain sdjustment
				if((IORD(KEY_BASE,0)&0x03) == 0x01){
          //current_focus = Focus_Window(320,240);
          if(ctrl_uart){ fprintf(ctrl_uart, "control message\n"); }
        }
        if((IORD(KEY_BASE,0)&0x03) == 0x02){
          calibration = 0;
          gain = 0x7FF;
          OV8865SetGain(gain);
          printf("\nGain = %x ", gain);
        }

				//Read messages from the image processor and process them
				while ((IORD(EEE_IMGPROC_0_BASE,EEE_IMGPROC_STATUS)>>8) & 0xff) { 	//Find out if there are words to read
						int word = IORD(EEE_IMGPROC_0_BASE,EEE_IMGPROC_MSG); 			//Get next word from message buffer
            x_min = (word>>11)&0x7FF;
            x_max = word&0x7FF;
            filter_id = (word&0x3FC00000) >> 22;

            if((word&0xC0000000)&&(filter_id == 'L')){                //Last part of data from image processor arrives

              if(calibration<CALIBRATION_MAX){                       //If calibrating, reduce gain if ball_count is not zero
                if(ball_count && (exposure>EXPOSURE_STEP)){
                  if(gain>GAIN_STEP){
                    gain -= GAIN_STEP;
                  }else{
                    gain = 0x07FF;
                    exposure -= EXPOSURE_STEP;
                    OV8865SetExposure(exposure);
                    printf("Exposure = %x\n", exposure);
                  }
            		  OV8865SetGain(gain);
            		  printf("Gain = %x\n", gain);
                }else{
                  calibration++;
                }
                printf("Calibration = %d\n", calibration);
              }
              if(calibration == CALIBRATION_MAX){ process = 1; }     //Start processing the data
              else{ ball_count = 0; }
            }

            if(word&0xFFC00000){                                     //The word is about filter data, append to arrays

              if(word&0xC0000000){
                filter_y_min[filter_index(filter_id)] = x_min;
                filter_y_max[filter_index(filter_id)] = x_max;
              }else{
                filter_x_min[filter_index(filter_id)] = x_min;
                filter_x_max[filter_index(filter_id)] = x_max;
              }

              /*if(word&0xC0000000){ printf("\nY "); }
              else{ printf("\nX "); }
              printf("%c %03x %03x\n", filter_id, x_max, x_min);*/

            }else if((x_max-x_min)>20){                              //The word is about ball data, append to arrays and increment ball_count
              if((ball_x_min.used<8)&&(calibration == CALIBRATION_MAX)){
                appendArray(&ball_x_min, x_min);
                appendArray(&ball_x_max, x_max);
              }
              ball_count++;
            }
				}

        if(process){
          printf("Counted %d balls.\n\n", ball_x_min.used);
          for(alt_u8 i=0; i<(ball_x_min.used); i++){
            printf("Ball %d:\n", i+1);
            printf("    Distance: %03d\n", (2560/(ball_x_max.data[i] - ball_x_min.data[i])));
            printf("    Filters triggered: ");
            for(alt_u8 j=0; j<5; j++){
              if(!( (filter_x_min[j] > ball_x_max.data[i]) || (filter_x_max[j] < ball_x_min.data[i]) )){
                printf("%c ", get_filter_id(j));
              }
            }
            printf("\n\n");
          }

          process = 0;
          ball_count = 0;
          ball_x_min.used = 0;
          ball_x_max.used = 0;
        }

  			//Main loop delay
  			usleep(10000);
		}
		return 0;
}
