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
parameter MESSAGE_BUF_MAX = 256;                  // Size of message FIFO
parameter MSG_INTERVAL    = 20;                   // Interval between sending data to FIFO, in frames
parameter BB_COL_DEFAULT  = 24'h00ff00;           // Bounding box colour

wire [7:0] red, green, blue;                      // Input RGB values of pixels
wire [7:0] red_out, green_out, blue_out;          // Output to VGA
wire       sop, eop, in_valid, out_ready;         // Streaming and packet info
reg        packet_video;

//////////////////////////////////////////////////////////////////////// - Detect ball pixels and colours using filters

// Ball filters, decide if a given pixel could be part of a ball, low chance of false negatives
wire   red_detect, pink_detect, green_detect, blue_detect, bright_detect, ball_detect;

// Colour filters, detect the 5 colours, low chance of false positives
wire   red_unique, pink_unique, yellow_unique, green_unique, blue_unique;


assign    red_detect = (red>=64)&(((red>>1)+(red>>3))>=green)&((red>>1)>=blue);
assign   pink_detect = (red>=192)&(green<128)&(blue<128);
assign  green_detect = (green>=32)&(((green>>1)+(green>>3))>=red)&(green>=(blue>>1));
assign   blue_detect = (blue>=48)&(blue>=(red<<1))&(blue>=green)&(blue<144);
assign bright_detect = (red>=160)|(green>=192)|(blue>=128);
assign   ball_detect = (bright_detect|blue_detect|green_detect|pink_detect|red_detect)&(y>=224);

assign    red_unique = (red>=128)&(green<  96)&(blue<  64)&(y>=224);
assign   pink_unique = (red>=192)&(green< 128)&(blue>=128)&(y>=224);
assign yellow_unique = (red>=192)&(green>=224)&(blue< 128)&(y>=224);
assign  green_unique = (red< 128)&(green>=192)&(blue>=160)&(y>=224);
assign   blue_unique = (red<  96)&(green<  96)&(blue> 128)&(y>=224);

//////////////////////////////////////////////////////////////////////// - Generate VGA output

wire        bb_active;         // High if current pixel is part of bounding box
wire [23:0] new_image;         // Debug view, shows bounding box and filter data
wire [23:0] ball_high;         // Shows which filters are triggered

assign ball_high =   pink_detect ? {8'hdd, 8'h00, 8'hdd} :
										green_detect ? {8'h00, 8'hdd, 8'h00} :
									   blue_detect ? {8'h00, 8'h00, 8'hdd} :
									    red_detect ? {8'hdd, 8'h00, 8'h00} :
									 bright_detect ? {8'hdd, 8'hdd, 8'h00} :
 									                 {8'h00, 8'h00, 8'h00} ;
/*
assign ball_high = yellow_unique ? {8'hdd, 8'hdd, 8'h00} :
									   pink_unique ? {8'hdd, 8'h00, 8'hdd} :
									   blue_unique ? {8'h00, 8'h00, 8'hdd} :
									    red_unique ? {8'hdd, 8'h00, 8'h00} :
									  green_unique ? {8'h00, 8'hdd, 8'h00} :
									   ball_detect ? {8'hdd, 8'hdd, 8'hdd} :
 									                 {8'h00, 8'h00, 8'h00} ;
*/
assign bb_active = (x==left)|(x==right)|(y==top)|(y==bottom);
assign new_image = bb_active ? bb_col : ball_high;

// Switch output between debug view and video from the camera, depending on mode(SW0)
assign {red_out, green_out, blue_out} = (mode & ~sop & packet_video) ? new_image : {red,green,blue};

//////////////////////////////////////////////////////////////////////// - Generate image coordinates(x,y)

reg [10:0] x, y;

//Count valid pixels to get the image coordinates. Reset and detect packet type on Start of Packet.
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

//////////////////////////////////////////////////////////////////////// - Process ball filter data and send messages

reg [10:0] x_min, y_min, x_max, y_max;
wire boundary_detect;
assign boundary_detect = (frame_count == MSG_INTERVAL-1) ?    red_unique :
												 (frame_count == MSG_INTERVAL-2) ?   pink_unique :
												 (frame_count == MSG_INTERVAL-3) ? yellow_unique :
												 (frame_count == MSG_INTERVAL-4) ?  green_unique :
												 (frame_count == MSG_INTERVAL-5) ?   blue_unique :
												 																	   ball_detect ;

// Find the bounding box a given filter
always@(posedge clk) begin
	if (boundary_detect & in_valid) begin	         // Update bounds
		if (x < x_min) x_min <= x;
		if (x > x_max) x_max <= x;
		if (y < y_min) y_min <= y;
		y_max <= y;
	end
	if (sop & in_valid) begin	                    // Reset bounds on start of packet
		x_min <= IMAGE_W-11'h1;
		x_max <= 0;
		y_min <= IMAGE_H-11'h1;
		y_max <= 0;
	end
end

// Group columns to find balls
reg [IMAGE_W-1:0] column_detect;

always @(posedge clk) begin
	if(sop & in_valid) begin
		column_detect <= 0;
	end
	if(ball_detect & in_valid) begin
		column_detect[x] <= 1;
	end
end

// When receiving last line of pixels, generate messages for CPU from ball data
reg in_ball, send_msg;
reg [10:0] ball_min, ball_max;

always@(posedge clk) begin
	if (in_valid&(y == IMAGE_H-1)&packet_video) begin
		in_ball <= column_detect[x];
		if((in_ball==0) & (column_detect[x]==1)) begin
			ball_min <= x;
		end
		if((in_ball==1) & (column_detect[x]==0)) begin
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

//////////////////////////////////////////////////////////////////////// - Process bounding box and send colour data

`define    RED_ID "R"
`define   PINK_ID "P"
`define YELLOW_ID "Y"
`define  GREEN_ID "G"
`define   BLUE_ID "B"
`define DETECT_ID "D"

reg [1:0]  msg_state;
reg [7:0]  msg_id;
reg [10:0] left, right, top, bottom;
reg [7:0]  frame_count;

always@(posedge clk) begin
	if (eop & in_valid & packet_video) begin  // At the end of each video packet

		// Latch edges for bounding box on next frame
		left <= x_min;
		right <= x_max;
		top <= y_min;
		bottom <= y_max;

		frame_count <= frame_count - 1;
		if (frame_count == 0) frame_count <= MSG_INTERVAL-1;

		// Send data from colour filters once every MSG_INTERVAL frames, if there is room in the FIFO
		if(msg_buf_size < MESSAGE_BUF_MAX-2) begin
			case(frame_count)            // Send data from different filters depending on frame_count
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
				MSG_INTERVAL-6: begin
					msg_id <= `DETECT_ID;
					msg_state <= 2'b01;
				end
				default: begin
					msg_id <= `RED_ID;
					msg_state <= 2'b0;
				end
			endcase
		end
	end
	if (msg_state != 2'b00) msg_state <= msg_state + 2'b01;     // Update state of message writer FSM
end

//////////////////////////////////////////////////////////////////////// - Write messages to FIFO

reg  [31:0] msg_buf_in;
wire [31:0] msg_buf_out;
reg         msg_buf_wr;
wire        msg_buf_rd;
wire        msg_buf_flush;
wire  [7:0] msg_buf_size;
wire        msg_buf_empty;

always@(*) begin	                                    // Write words to FIFO as state machine advances
	case(msg_state)
		2'b00: begin                                      // Send data from ball filters in this state
			msg_buf_in = {10'h0, ball_min, ball_max};
			msg_buf_wr = send_msg  & (frame_count==0) & (msg_buf_size < MESSAGE_BUF_MAX - 1);
		end
		2'b01: begin                                      // Send data from colour filters, first 2 bits indicate x/y data
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

//////////////////////////////////////////////////////////////////////// - FIFO and register connections

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
