#include "zb_mrf24j40.h"
#include "main.h"

zb_uint8_t read_short_reg(zb_uint8_t addr)
{
	zb_uint8_t result;
	select_radio();
	MRF_EXCHANGE( (addr << 1)&0x7E );
	result = MRF_EXCHANGE(0x00);
	deselect_radio();
	return result;
}

void write_short_reg(zb_uint8_t addr,zb_uint8_t tx_data)
{
	select_radio();
	MRF_EXCHANGE( ((addr << 1) & 0x7E) | 0x01 );
	MRF_EXCHANGE(tx_data);
	deselect_radio();
}

zb_uint8_t read_long_reg(zb_uint16_t addr)
{
	zb_uint8_t result;
	select_radio();
	zb_uint8_t high_byte = addr >> 3;
	zb_uint8_t low_byte = addr << 5;
	MRF_EXCHANGE( (high_byte | 0x80) );
	MRF_EXCHANGE( low_byte & 0xE0);
	result = MRF_EXCHANGE(0x00);
	deselect_radio();
	return result;
}

void write_long_reg(zb_uint16_t addr, zb_uint8_t tx_data)
{
	select_radio();
	zb_uint8_t high_byte = addr >> 3;
	zb_uint8_t low_byte = addr << 5;
	MRF_EXCHANGE( (high_byte | 0x80) );
	MRF_EXCHANGE( (low_byte  & 0xE0) | 0x10 );
	MRF_EXCHANGE(tx_data);
	deselect_radio();
}

void select_radio()
{
        HAL_GPIO_WritePin(OLMX_CS_GPIO_PORT, OLMX_CS_PIN, GPIO_PIN_RESET);
}

void deselect_radio()
{
        HAL_GPIO_WritePin(OLMX_CS_GPIO_PORT, OLMX_CS_PIN, GPIO_PIN_SET);
}

void zb_transceiver_select_channel(zb_uint8_t n) //Works
{
    uint8_t val = 0;
    DECLARE_VAR();
    // writing channel number + optimization value
    val = (n - ZB_TRANSCEIVER_START_CHANNEL_NUMBER)<<4;
    val |= 0x03;
    ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL0,val);
    CHECK_L(ZB_LREG_RFCTRL0);

    // perform RF state machine reset as described in datasheet
    ZB_WRITE_SHORT_REG(ZB_SREG_RFCTL,0x04);
    USEC192_DELAY();
    ZB_WRITE_SHORT_REG(ZB_SREG_RFCTL, 0x02);
    USEC192_DELAY();
    ZB_WRITE_SHORT_REG(ZB_SREG_RFCTL, 0x01);
    USEC192_DELAY();
    ZB_WRITE_SHORT_REG(ZB_SREG_RFCTL, 0x00);
}

void zb_read_rx_fifo()
{
    HAL_NVIC_DisableIRQ(EXTI1_IRQn);
    DISABLE_AIR();
    uint8_t i;
    uint16_t fifo = ZB_RX_FIFO;
    uint8_t frm_len = ZB_READ_LONG_REG(fifo);
    frm_len += 2;
    for (i = 0; i< frm_len; i++)
    {   
        fifo++;
        ZB_READ_LONG_REG(fifo);
        RXPrint();

    }
    ENABLE_AIR();
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

void zb_init_mrf24j40()
{
        //perform hard reset
        uint16_t j;
        HAL_GPIO_WritePin(OLMX_RST_GPIO_PORT, OLMX_RST_PIN, GPIO_PIN_RESET);
        for(j=0;j<(uint16_t)300;j++){}
        HAL_GPIO_WritePin(OLMX_RST_GPIO_PORT, OLMX_RST_PIN, GPIO_PIN_SET);
        for(j=0;j<(uint16_t)300;j++){}
        
	//TRACE_MSG(TRACE_COMMON1, "zb_init_mrf24j40",(FMT__0));
	DECLARE_VAR();
	/**** MY ****/
	// Baseband register
	//ZB_WRITE_SHORT_REG(BBREG2,0x80); 	// CCA mode 1 - energy above treshold.
	// ENERGY DETECTION THRESHOLD FOR CCA REGISTER
	//ZB_WRITE_SHORT_REG(CCAEDTH,0x60);	//  Clear Channel Assessment (CCA) Energy Detection (ED) Mode bits -69dBm - recommended
	ZB_WRITE_SHORT_REG(ZB_SREG_SOFTRST, 0x07);	
	CHECK_S(ZB_SREG_SOFTRST);
	while(ZB_READ_SHORT_REG(ZB_SREG_SOFTRST) & 0x07) ;
	volatile uint8_t reg  = 0;
	//-------------------------------------------------------
	

	// GATECLK: GATED CLOCK CONTROL REGISTER (ADDRESS: 0x26)
	//MMI module clock off 
	ZB_WRITE_SHORT_REG(ZB_SREG_GATECLK, 0x20);	
	CHECK_S(ZB_SREG_GATECLK);
	// PACON1: POWER AMPLIFIER CONTROL 1 REGISTER (ADDRESS: 0x17)
	//Power amplifier on time before beginning of packet = 4*16us
	ZB_WRITE_SHORT_REG(ZB_SREG_PACON1, 0x08);		
	CHECK_S(ZB_SREG_PACON1);
	
	// PACON2: POWER AMPLIFIER CONTROL 2 REGISTER (ADDRESS: 0x18) in mrf | FIFOEN in UBEC
	// TXFIFO and RXFIFO output always enable,
	// Transmitter on time before beginning of packet = 5*16us
	// à â mrf ñòåêå 0x98
	ZB_WRITE_SHORT_REG(ZB_SREG_FIFOEN, 0x94);	//0b1001.0100
	CHECK_S(ZB_SREG_FIFOEN);
	// TXSTBL: TX STABILIZATION REGISTER (ADDRESS: 0x2E) in MRF. | TXPEMISP in UBEC.
	// The minimum number of symbols forming a Short Interframe Spacing (SIFS) period = 5*16us
	//  VCO Stabilization Period bits = recommended value
	ZB_WRITE_SHORT_REG(ZB_SREG_TXPEMISP, 0x95);
	CHECK_S(ZB_SREG_TXPEMISP);

	// !!!!!! =================================================================!!!!!!!!!!!!!!!!!!!!!!
	
	
	
	// My 5 kopeek. Setup CCA treshold
	// mrf stack = 0x80 0x78
	ZB_WRITE_SHORT_REG(ZB_SREG_BBREG2,0x78);
	CHECK_S(ZB_SREG_BBREG2);
	
	// BBREG3: BASEBAND 3 REGISTER (ADDRESS: 0x3B) in MRF. | BBREG3 : Preamble Search ED  in UBEC.
	// Energy Valid Threshold = 0x5. recommended is 0b1101 was 0x50
	ZB_WRITE_SHORT_REG(ZB_SREG_BBREG3, 0xD0);
	CHECK_S(ZB_SREG_BBREG2);
	// BBREG5 : Preamble Searching Time Threshold/Boundary ONLY IN UBEC
	// PeakEarly = 56
	// PeakLate = 80
	ZB_WRITE_SHORT_REG(ZB_SREG_BBREG4, 0x9c);
	
	ZB_WRITE_SHORT_REG(ZB_SREG_BBREG5, 0x07);
	CHECK_S(ZB_SREG_BBREG5);
	// BBREG6: BASEBAND 6 REGISTER (ADDRESS: 0x3E). RSSI mode in UBEC
	// RSSI Mode 2 bit. calculating RSSI for RX packet. The RSSI value is stored in RXFIFO
	ZB_WRITE_SHORT_REG(ZB_SREG_BBREG6, 0x40);
	CHECK_S(ZB_SREG_BBREG6);
	// ZB_LREG_RFCTRL0: RF CONTROL 0 REGISTER (ADDRESS: 0x200) in MRF. | RFCTRL0 in UBEC
	// Select 11 channel and load optimized 0x03 value
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL0, CHANNEL_11 | 0x03);
	CHECK_L(ZB_LREG_RFCTRL0);
	// RFCON1: RF CONTROL 1 REGISTER	 (ADDRESS: 0x201) in MRF.
	// Set VCO optimize control bits
	// mrf = 0x01
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL1, 0x02);
	CHECK_L(ZB_LREG_RFCTRL1);
	// RFCON2: RF CONTROL 2 REGISTER (ADDRESS: 0x202) 
	// Set MAGIC number. PLL<7> bit is off. I dont know why.
	// mrf stack = 0x80
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL2, 0x66);
	CHECK_L(ZB_LREG_RFCTRL2);
	// mrf stack has PHYSetLongRAMAddr(RFCTRL3,PA_LEVEL);
	
	// REG04 : RFCTRL4 - ONLY IN UBEC
	// set magic number
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL4, 0x09);
	CHECK_L(ZB_LREG_RFCTRL4);
	// RFCON6: RF CONTROL 6 REGISTER (ADDRESS: 0x206) 
	// Recovery from Sleep control. Less than 1 ms (recommended)
	// mrf stack = 0x90
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL6, 0x30);
	CHECK_L(ZB_LREG_RFCTRL6);
	// RFCON7: RF CONTROL 7 REGISTER (ADDRESS: 0x207) 
	// setting strange Sleep Clock Selection bits. there are only 10 and 01 variants.
	// setting magic 0xC number.
	// mrf stack = 0x80
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL7, 0xEC);
	CHECK_L(ZB_LREG_RFCTRL7);
	// RFCON8: RF CONTROL 8 REGISTER (ADDRESS: 0x208) 
	// VCO Control bit = 0 and other magic numbers.
	// mrf stack has = 0x10
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL8, 0x8C);
	CHECK_L(ZB_LREG_RFCTRL8);
	// REG36: ASSO_BCN_LADR6
	// set Long address 6 of associated coordinator . 
	ZB_WRITE_LONG_REG(ZB_LREG_GPIODIR, 0x00);
	CHECK_L(ZB_LREG_GPIODIR);
	// BLACK MAGIC. 
	ZB_WRITE_LONG_REG(ZB_LREG_SECCTRL, 0x20);
	
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL50, 0x05);
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL51, 0xC0);
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL52, 0x01);
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL59, 0x00);
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL73, 0x40);
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL74, 0xC5);
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL75, 0x13);
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL76, 0x07);
	
	// Disable SLPCLKEN, falling edge.
	ZB_WRITE_LONG_REG(ZB_LREG_IRQCTRL,0x80);
	
	// perform soft reset
	ZB_WRITE_SHORT_REG(ZB_SREG_SOFTRST, 0x02);
/*
	ZB_WRITE_SHORT_REG(ZB_SREG_PANIDH,0xff);
	ZB_WRITE_SHORT_REG(ZB_SREG_PANIDL,0xff);
	ZB_WRITE_SHORT_REG(ZB_SREG_SADRH,0xff);
	ZB_WRITE_SHORT_REG(ZB_SREG_SADRL,0xff);
*/

	/*	Initial config.
		1. Set reception mode RXMCR<1:0>
			00 - normal
			10 - error
			01 - promiscuous
		2. Setup frame format filter. RXFLUSH<3:1>
			000 - all frames
			100 - command only
			010 - data only
			001 - beacon only
		3. Don't forget about acknowledgement
		4. Set correct interrupt mask in INTCON
	*/	
	ZB_WRITE_SHORT_REG(ZB_SREG_RXMCR,0x00); // 1
	//CHECK_S(ZB_SREG_RXMCR);
	ZB_WRITE_SHORT_REG(ZB_SREG_RXFLUSH, 0x00); // 2
	CHECK_S(ZB_SREG_RXFLUSH);
	ZB_WRITE_SHORT_REG(ZB_SREG_INTMSK,0x00); 
	
	// from UBEC DS
	//ZB_WRITE_LONG_REG(ZB_LREG_RXFRMTYPE, 0x0F);
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL2, 0x80);
	ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL4, 0x66);
	//ZB_WRITE_SHORT_REG(ZB_SREG_RFCTL, 0x03);
	

	// perform correct RF state machine reset and TURN ON tx transmit.

	ZB_WRITE_SHORT_REG(ZB_SREG_RFCTL,0x04);
	/* TODO: check, is it enough for "192us waiting" */
	USEC192_DELAY();
	ZB_WRITE_SHORT_REG(ZB_SREG_RFCTL, 0x02);
	USEC192_DELAY();
	ZB_WRITE_SHORT_REG(ZB_SREG_RFCTL, 0x01);
	USEC192_DELAY();
	ZB_WRITE_SHORT_REG(ZB_SREG_RFCTL, 0x00);
	/* LREG 0x23C RXFRMTYPE
     Now, it's disabled, according to DS-2400-51_v0_6_RN.pdf, p.158
     RXFTYPE[7:0]: RX Frame Type Filter
     00001011: (default - Do Not Change)
     bit 7-4 reserver
     bit3 command
     bit2 ack                                                                                          ?
     bit1 data
     bit0 beacon */
     	ZB_WRITE_LONG_REG(ZB_LREG_RXFRMTYPE, 0x0B); /* we accept all frames */
	CHECK_L(ZB_LREG_RXFRMTYPE);
	/*
	LREG SCLKDIV
	Bit 7 I2CWDTEN: I2C watchdog timer enable
	Bit 4-0 SCLKDIV: sleep clock division selection.
	n: the sleep clock is divided by 2^n before being fed to logic circuit.
	
	SCLKDIV = 1
	I2CWDTEN = 0
	0000.0001 == 0x1
	*/
	ZB_WRITE_LONG_REG(ZB_LREG_SCLKDIV, 0x01);
}

void ref_init_mrf24j40()
{
        //? - commands present in datasheet ex., but absent in zb stack version, perhaps no need of them
        //?! - absent in zb stack, where they should be, maybe they present in somewhere other place in stack
        //! - commands absent in datasheet example, but mrf doesn't work w/o one of them (setting ZB_SREG_INTMSK)
  
        //perform hard reset
        uint16_t j;                                                          //?!
        HAL_GPIO_WritePin(OLMX_RST_GPIO_PORT, OLMX_RST_PIN, GPIO_PIN_RESET); //?!
        for(j=0;j<(uint16_t)300;j++){}                                       //?!
        HAL_GPIO_WritePin(OLMX_RST_GPIO_PORT, OLMX_RST_PIN, GPIO_PIN_SET);   //?!
        for(j=0;j<(uint16_t)300;j++){}                                       //?!
        
	ZB_WRITE_SHORT_REG(ZB_SREG_RFCTL, 0x04);
        ZB_WRITE_SHORT_REG(ZB_SREG_RFCTL, 0x00);
        
        ZB_WRITE_SHORT_REG(ZB_SREG_RXFLUSH, 0x01);
        
 	ZB_WRITE_SHORT_REG(ZB_SREG_PANIDH,0xff);
	ZB_WRITE_SHORT_REG(ZB_SREG_PANIDL,0xff);
	ZB_WRITE_SHORT_REG(ZB_SREG_SADRH,0xff);
	ZB_WRITE_SHORT_REG(ZB_SREG_SADRL,0xff);  

        ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL2, 0x80);
        ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL3, 0x00); //?
        ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL6, 0x80);
        ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL8, 0x10);
        ZB_WRITE_SHORT_REG(ZB_SREG_BBREG2,0x78);
        ZB_WRITE_SHORT_REG(ZB_SREG_BBREG6, 0x40);
        ZB_WRITE_SHORT_REG(ZB_SREG_BBREG7, 0x00); //?
        ZB_WRITE_LONG_REG(ZB_LREG_RFCTRL0, CHANNEL_11);
        ZB_WRITE_SHORT_REG(ZB_SREG_INTMSK,0xF7);   //! enable only RX interrupt
        ZB_WRITE_SHORT_REG(ZB_SREG_RXMCR,0x03);  //! receive all packets (including with errors: CRC, frametypes, etc.)
        
	ZB_WRITE_SHORT_REG(ZB_SREG_RFCTL, 0x04);
        ZB_WRITE_SHORT_REG(ZB_SREG_RFCTL, 0x00);
}