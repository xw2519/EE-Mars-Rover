module EEE_IMGPROC(
	// global clock & reset
	input clk,
	input reset_n,
	// mm slave
	input             s_chipselect,
	input             s_read,
	input             s_write,
	output reg [31:0] s_readdata,
	input	     [31:0] s_writedata,
	input	      [2:0] s_address,
	// stream sink
	input [23:0] sink_data,
	input        sink_valid,
	output       sink_ready,
	input        sink_sop,
	input        sink_eop,
	// streaming source
	output [23:0] source_data,
	output        source_valid,
	input         source_ready,
	output        source_sop,
	output        source_eop,
	// conduit
	input mode
);

////////////////////////////////////////////////////////////////////////

parameter IMAGE_W         = 11'd640;
parameter IMAGE_H         = 11'd480;
parameter MESSAGE_BUF_MAX = 256;
parameter MSG_INTERVAL    = 20;
parameter BB_COL_DEFAULT  = 24'h00ff00;

wire [7:0] red, green, blue;
wire [7:0] red_out, green_out, blue_out;
wire       sop, eop, in_valid, out_ready;

//////////////////////////////////////////////////////////////////////// - Detect ball pixels, generate VGA output

// Detect ball pixels
wire   red_detect, pink_detect, green_detect, blue_detect, bright_detect, ball_detect;
wire   red_unique, pink_unique, yellow_unique, green_unique, blue_unique;


assign    red_detect = (red>=64)&(((red>>1)+(red>>3))>=green)&((red>>1)>=blue);
assign   pink_detect = (red>=192)&(green<128)&(blue<128);
assign  green_detect = (green>=32)&(((green>>1)+(green>>3))>=red)&(green>=(blue>>1));
assign   blue_detect = (blue>=48)&(blue>=red)&(blue>=green)&(blue<144);
assign bright_detect = (red>=160)|(green>=192)|(blue>=128);
assign   ball_detect = (bright_detect|blue_detect|green_detect|pink_detect|red_detect)&(y>=272);

assign    red_unique = (red>=128)&(green<  96)&(blue<  64)&(y>=272);
assign   pink_unique = (red>=192)&(green< 128)&(blue>=128)&(y>=272);
assign yellow_unique = (red>=192)&(green>=224)&(blue< 128)&(y>=272);
assign  green_unique = (red< 128)&(green>=240)&(blue>=224)&(y>=272);
assign   blue_unique = (red<  96)&(green<  96)&(blue> 128)&(y>=272);

// Highlight detected areas
wire [23:0] ball_high;
assign ball_high = yellow_unique ? {8'hdd, 8'hdd, 8'h00} :
									   pink_unique ? {8'hdd, 8'h00, 8'hdd} :
									   blue_unique ? {8'h00, 8'h00, 8'hdd} :
									    red_unique ? {8'hdd, 8'h00, 8'h00} :
									  green_unique ? {8'h00, 8'hdd, 8'h00} :
									   ball_detect ? {8'hdd, 8'hdd, 8'hdd} :
 									                 {8'h00, 8'h00, 8'h00} ;

// Show bounding box
wire        bb_active;
wire [23:0] new_image;

assign bb_active = (x==left)|(x==right)|(y==top)|(y==bottom);
assign new_image = bb_active ? bb_col : ball_high;

// Switch output pixels depending on mode
// Don't modify the start-of-packet word or data in non-video packets
assign {red_out, green_out, blue_out} = (mode & ~sop & packet_video) ? new_image : {red,green,blue};

//////////////////////////////////////////////////////////////////////// - Generate image coordinates

//Count valid pixels to get the image coordinates. Reset and detect packet type on Start of Packet.
reg [10:0] x, y;
reg packet_video;
always@(posedge clk) begin
	if (sop) begin
		x <= 11'h0;
		y <= 11'h0;
		packet_video <= (blue[3:0] == 3'h0);
	end
	else if (in_valid) begin
		if (x == IMAGE_W-1) begin
			x <= 11'h0;
			y <= y + 11'h1;
		end
		else begin
			x <= x + 11'h1;
		end
	end
end

//////////////////////////////////////////////////////////////////////// - Processing to find balls

//Find first and last pixels that triger a filter
reg [10:0] x_min, y_min, x_max, y_max;
wire boundary_detect;
assign boundary_detect = (frame_count == MSG_INTERVAL-1) ?    red_unique :
												 (frame_count == MSG_INTERVAL-2) ?   pink_unique :
												 (frame_count == MSG_INTERVAL-3) ? yellow_unique :
												 (frame_count == MSG_INTERVAL-4) ?  green_unique :
												 (frame_count == MSG_INTERVAL-5) ?   blue_unique :
												 																	   ball_detect ;

always@(posedge clk) begin
	if (boundary_detect & in_valid) begin	//Update bounds
		if (x < x_min) x_min <= x;
		if (x > x_max) x_max <= x;
		if (y < y_min) y_min <= y;
		y_max <= y;
	end
	if (sop & in_valid) begin	//Reset bounds on start of packet
		x_min <= IMAGE_W-11'h1;
		x_max <= 0;
		y_min <= IMAGE_H-11'h1;
		y_max <= 0;
	end
end

//Group columns to find balls
reg [IMAGE_W-1:0] ball_in_col;
always @(posedge clk) begin
	if(sop & in_valid) begin
		ball_in_col <= 0;
	end
	if(ball_detect & in_valid) begin
		ball_in_col[x] <= 1;
	end
end

//During last line of pixels, send data as messages
reg in_ball, send_msg;
reg [10:0] ball_min, ball_max;
always@(posedge clk) begin
	if (in_valid&(y == IMAGE_H-1)&packet_video) begin
		in_ball <= ball_in_col[x];
		if((in_ball==0) & (ball_in_col[x]==1)) begin
			ball_min <= x;
		end
		if((in_ball==1) & (ball_in_col[x]==0)) begin
			ball_max <= x;
			send_msg <= 1;
		end
	end
	else if(in_valid&packet_video) begin
		in_ball <= 0;
	end
	if(send_msg) begin
		send_msg <= 0;
	end
end

//////////////////////////////////////////////////////////////////////// - Process colours

//Process bounding box at the end of the frame.
`define    RED_ID "R"
`define   PINK_ID "P"
`define YELLOW_ID "Y"
`define  GREEN_ID "G"
`define   BLUE_ID "B"

reg [1:0]  msg_state;
reg [7:0]  msg_id;
reg [10:0] left, right, top, bottom;
reg [7:0]  frame_count;

always@(posedge clk) begin
	if (eop & in_valid & packet_video) begin  //Ignore non-video packets
		//Latch edges for display overlay on next frame
		left <= x_min;
		right <= x_max;
		top <= y_min;
		bottom <= y_max;

		//Start message writer FSM once every MSG_INTERVAL frames, if there is room in the FIFO
		frame_count <= frame_count - 1;
		if (frame_count == 0) frame_count <= MSG_INTERVAL-1;

		if(msg_buf_size < MESSAGE_BUF_MAX-2) begin
			case(frame_count)
				MSG_INTERVAL-1: begin
					msg_id <= `RED_ID;
					msg_state <= 2'b01;
				end
				MSG_INTERVAL-2: begin
					msg_id <= `PINK_ID;
					msg_state <= 2'b01;
				end
				MSG_INTERVAL-3: begin
					msg_id <= `YELLOW_ID;
					msg_state <= 2'b01;
				end
				MSG_INTERVAL-4: begin
					msg_id <= `GREEN_ID;
					msg_state <= 2'b01;
				end
				MSG_INTERVAL-5: begin
					msg_id <= `BLUE_ID;
					msg_state <= 2'b01;
				end
				default: begin
					msg_id <= `RED_ID;
					msg_state <= 2'b0;
				end
			endcase
		end
	end
	if (msg_state != 2'b00) msg_state <= msg_state + 2'b01;
end

//////////////////////////////////////////////////////////////////////// - Send messages

//Generate output messages for CPU
reg [31:0] msg_buf_in;
wire [31:0] msg_buf_out;
reg msg_buf_wr;
wire msg_buf_rd, msg_buf_flush;
wire [7:0] msg_buf_size;
wire msg_buf_empty;

always@(*) begin	//Write words to FIFO as state machine advances
	case(msg_state)
		2'b00: begin
			msg_buf_in = {10'h0, ball_min, ball_max};
			msg_buf_wr = send_msg  & (frame_count==0) & (msg_buf_size < MESSAGE_BUF_MAX - 1);
		end
		2'b01: begin
			msg_buf_in = {2'h0, msg_id, x_min, x_max};
			msg_buf_wr = 1'b1;
		end
		2'b10: begin
			msg_buf_in = {2'h3, msg_id, y_min, y_max};
			msg_buf_wr = 1'b1;
		end
		2'b11: begin
			msg_buf_in = 32'b0;
			msg_buf_wr = 1'b0;
		end
	endcase
end

////////////////////////////////////////////////////////////////////////

//Output message FIFO
MSG_FIFO	MSG_FIFO_inst (
	.clock (clk),
	.data (msg_buf_in),
	.rdreq (msg_buf_rd),
	.sclr (~reset_n | msg_buf_flush),
	.wrreq (msg_buf_wr),
	.q (msg_buf_out),
	.usedw (msg_buf_size),
	.empty (msg_buf_empty)
);


//Streaming registers to buffer video signal
STREAM_REG #(.DATA_WIDTH(26)) in_reg (
	.clk(clk),
	.rst_n(reset_n),
	.ready_out(sink_ready),
	.valid_out(in_valid),
	.data_out({red,green,blue,sop,eop}),
	.ready_in(out_ready),
	.valid_in(sink_valid),
	.data_in({sink_data,sink_sop,sink_eop})
);

STREAM_REG #(.DATA_WIDTH(26)) out_reg (
	.clk(clk),
	.rst_n(reset_n),
	.ready_out(out_ready),
	.valid_out(source_valid),
	.data_out({source_data,source_sop,source_eop}),
	.ready_in(source_ready),
	.valid_in(in_valid),
	.data_in({red_out, green_out, blue_out, sop, eop})
);


/////////////////////////////////
/// Memory-mapped port		 /////
/////////////////////////////////

// Addresses
`define REG_STATUS    		0
`define READ_MSG    			1
`define READ_ID    				2
`define REG_BBCOL					3

//Status register bits
// 31:16 - unimplemented
// 15:8 - number of words in message buffer (read only)
// 7:5 - unused
// 4 - flush message buffer (write only - read as 0)
// 3:0 - unused


// Process write

reg  [7:0]   reg_status;
reg	[23:0]	bb_col;

always @ (posedge clk)
begin
	if (~reset_n)
	begin
		reg_status <= 8'b0;
		bb_col <= BB_COL_DEFAULT;
	end
	else begin
		if(s_chipselect & s_write) begin
		   if      (s_address == `REG_STATUS)	reg_status <= s_writedata[7:0];
		   if      (s_address == `REG_BBCOL)	bb_col <= s_writedata[23:0];
		end
	end
end


//Flush the message buffer if 1 is written to status register bit 4
assign msg_buf_flush = (s_chipselect & s_write & (s_address == `REG_STATUS) & s_writedata[4]);


// Process reads
reg read_d; //Store the read signal for correct updating of the message buffer

// Copy the requested word to the output port when there is a read.
always @ (posedge clk)
begin
   if (~reset_n) begin
	   s_readdata <= {32'b0};
		read_d <= 1'b0;
	end

	else if (s_chipselect & s_read) begin
		if   (s_address == `REG_STATUS) s_readdata <= {16'b0,msg_buf_size,reg_status};
		if   (s_address == `READ_MSG) s_readdata <= {msg_buf_out};
		if   (s_address == `READ_ID) s_readdata <= 32'h1234EEE2;
		if   (s_address == `REG_BBCOL) s_readdata <= {8'h0, bb_col};
	end

	read_d <= s_read;
end

//Fetch next word from message buffer after read from READ_MSG
assign msg_buf_rd = s_chipselect & s_read & ~read_d & ~msg_buf_empty & (s_address == `READ_MSG);



endmodule
