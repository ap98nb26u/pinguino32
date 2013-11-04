/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for Petit FatFs (C)ChaN, 2009      */
/*-----------------------------------------------------------------------*/
#ifndef _DISKIOC
#define _DISKIOC

#include <sd/diskio.h>
#include <spi.c>
#include <delay.c>
/* Definitions for MMC/SDC command */
#define CMD0	(0x40+0)	/* GO_IDLE_STATE */
#define CMD1	(0x40+1)	/* SEND_OP_COND (MMC) */
#define ACMD41	(0xC0+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(0x40+8)	/* SEND_IF_COND */
#define CMD9	(0x40+9)	/* SEND_CSD */
#define CMD10	(0x40+10)	/* SEND_CID */
#define CMD12	(0x40+12)	/* STOP_TRANSMISSION */
#define ACMD13	(0xC0+13)	/* SD_STATUS (SDC) */
#define CMD16	(0x40+16)	/* SET_BLOCKLEN */
#define CMD17	(0x40+17)	/* READ_SINGLE_BLOCK */
#define CMD18	(0x40+18)	/* READ_MULTIPLE_BLOCK */
#define CMD23	(0x40+23)	/* SET_BLOCK_COUNT (MMC) */
#define ACMD23	(0xC0+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(0x40+24)	/* WRITE_BLOCK */
#define CMD25	(0x40+25)	/* WRITE_MULTIPLE_BLOCK */
#define CMD55	(0x40+55)	/* APP_CMD */
#define CMD58	(0x40+58)	/* READ_OCR */


/* Port Controls  (Platform dependent) */
#define SELECT()	SD_CS = 0	/* MMC CS = Low */
#define DESELECT()	SD_CS = 1	/* MMC CS = High */

#define	FCLK_SLOW()	(CONFIG = 0x22)	/* Set slow clock (100k-400k) */
#define	FCLK_FAST()	(CONFIG = 0x21)	/* Set fast clock (depends on the CSD) */

#define NCR_TIMEOUT     (u8)20        //Byte times before command response is expected (must be at least 8)
#define WRITE_TIMEOUT   (u32)0xA0000  //SPI byte times to wait before timing out when the media is performing a write operation (should be at least 250ms for SD cards).

/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/
void SPI_init(u8 sync_mode, u8 bus_mode, u8 smp_phase);
u8 SPI_write(u8 datax);
u8 SPI_read();

static u16 CardType;

/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

static u8 wait_ready (void)
{
	u8 res;


	Tim2 = 500;	/* Wait for ready in timeout of 500ms */
	res=SPI_read();
	do
		res = SPI_read();
	while ((res != 0xFF) && (Tim2--));

	return res;
}

/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/

static void release_spi (void)
{
	DESELECT();
	SPI_read();
}
static void power_off (void)
{
	SELECT();				/* Wait for card ready */
	wait_ready();
	release_spi();

	ENABLE = 0;				/* Disable SPI2 */

	return;
}

/*-----------------------------------------------------------------------*/
/* Send a command packet to MMC                                          */
/*-----------------------------------------------------------------------*/

static u8 send_cmd (
	u8 cmd,		/* Command byte */
	u32 arg		/* Argument */
)
{
	u8 n, res;
	u32 longtimeout;

	if (cmd & 0x80) {	/* ACMD<n> is the command sequense of CMD55-CMD<n> */
		cmd &= 0x7F;
		res = send_cmd(CMD55, 0);
		if (res > 1) return res;
	}

	/* Send command packet */
	SELECT();
	SPI_write(cmd);						/* Start + Command index */
	SPI_write((u8)(arg >> 24));		/* Argument[31..24] */
	SPI_write((u8)(arg >> 16));		/* Argument[23..16] */
	SPI_write((u8)(arg >> 8));			/* Argument[15..8] */
	SPI_write((u8)arg);				/* Argument[7..0] */
	n = 0x01;							/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	if (cmd == CMD12) n = 0xC3;
	SPI_write(n);

	/* Receive command response */
	if (cmd == CMD12) SPI_read();		/* Skip a stuff byte when stop reading */
	n = NCR_TIMEOUT;								/* Wait for a valid response in timeout of 10 attempts */
	do
		res = SPI_read();
	while ((res == 0xFF) && (--n) );

	if(cmd==CMD12)
	{
		longtimeout = WRITE_TIMEOUT;
		do
			res = SPI_read();
		while ((res == 0x00) && (--longtimeout));
		res = 0x00;
	}

	SPI_write(0xFF);
	if((cmd != CMD9)&&(cmd != CMD10)&&(cmd != CMD17)&&(cmd != CMD18)&&(cmd != CMD24)&&(cmd != CMD25))
		DESELECT();

	return res;			/* Return with the response value */
}
 
/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(void)
{
	u8 n, cmd, ty, ocr[4], rep, timeout;
	u16 tmr;

	SPI_init(2,0,0);							/* Force socket power on */

	for (n = 10; n; n--) SPI_write(0xFF);	/* Dummy clocks */
	ty = 0;
	timeout=100;
// Trying to enter Idle state
	do {
		DESELECT();
		SPI_write(0xFF);
		SELECT();
		rep = send_cmd(CMD0,0);
	} while ((rep != 1) && (--timeout) );
    if(timeout == 0)
    {
		DESELECT();
		SPI_write(0xFF);
		SELECT();
		rep = send_cmd(CMD12,0);
		rep = send_cmd(CMD0,0);
		if (rep != 1) 	return STA_NOINIT;
	}

	rep = send_cmd(CMD8, 0x1AA);

	if ( rep == 1) {	/* SDHC */
		for (n = 0; n < 4; n++) ocr[n] = SPI_read();		/* Get trailing return value of R7 resp */

		if (ocr[2] == 0x01 && ocr[3] == 0xAA) {				/* The card can work at vdd range of 2.7-3.6V */
			for (tmr = 25000; tmr && send_cmd(ACMD41, 1UL << 30); tmr--) ;	/* Wait for leaving idle state (ACMD41 with HCS bit) */

			if (tmr && send_cmd(CMD58, 0) == 0) {		/* Check CCS bit in the OCR */
				for (n = 0; n < 4; n++) ocr[n] = SPI_read();
				ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* SDv2 */
			}
		}

	} else {							/* SDSC or MMC */

		if (send_cmd(ACMD41, 0) <= 1) 	{
			ty = CT_SD1; cmd = ACMD41;	/* SDv1 */
		} else {
			ty = CT_MMC; cmd = CMD1;	/* MMCv3 */
		}

		for (tmr = 25000; tmr && send_cmd(cmd, 0); tmr--) ;	/* Wait for leaving idle state */

		if (!tmr || send_cmd(CMD16, 512) != 0)			/* Set R/W block length to 512 */
			ty = 0;

	}

	CardType = ty;
	release_spi();

	if (ty) {			/* Initialization succeded */
		FCLK_FAST();
		return RES_OK;
	} else {			/* Initialization failed */
		power_off();
		return STA_NOINIT;
	}
}

/*-----------------------------------------------------------------------*/
/* Read Partial Sector                                                   */
/*-----------------------------------------------------------------------*/

DRESULT disk_readp (
	u8 *buff,		/* Pointer to the read buffer (NULL:Read bytes are forwarded to the stream) */
	u32 lba,		/* Sector number (LBA) */
	u16 ofs,		/* Byte offset to read from (0..511) */
	u16 cnt		/* Number of bytes to read (ofs + cnt mus be <= 512) */
)
{
	DRESULT res;
	u8 rc;
	u16 bc;

	if (!(CardType & CT_BLOCK)) lba *= 512;		/* Convert to byte address if needed */

	res = RES_ERROR;
	if (send_cmd(CMD17, lba) == 0) {		/* READ_SINGLE_BLOCK */
		Tim1 = 200;
		do							/* Wait for data packet in timeout of 200ms */
			rc = SPI_read();
//			Tim1 = decreasetim(Tim1);
		while (rc == 0xFF && (Tim1--));
	}

	if(Tim1) {
		if (rc == 0xFE) {				/* A data packet arrived */
			bc = 514 - ofs - cnt;


			/* Skip leading bytes */
			if (ofs) {
				do SPI_read(); while (--ofs);
			}

			/* Receive a part of the sector */
				do
					*buff++ = SPI_read();
				while (--cnt);

			/* Skip trailing bytes and CRC */
			do SPI_read(); while (--bc);

			res = cnt ? 1 : RES_OK;
		}
	}

	release_spi();

	return res;
}

/*-----------------------------------------------------------------------*/
/* Write partial sector                                                  */
/*-----------------------------------------------------------------------*/

DRESULT disk_writep (
	const u8 *buff,	/* Pointer to the bytes to be written (NULL:Initiate/Finalize sector write) */
	u32 sa			/* Number of bytes to send, Sector number (LBA) or zero */
)
{
	DRESULT res;
	u16 bc;
	static u16 wc;


	res = RES_ERROR;

	if (buff) {		/* Send data bytes */
		bc = (u16)sa;
		while (bc && wc) {		/* Send data bytes to the card */
			SPI_write(*buff++);
			wc--; bc--;
		}
		res = RES_OK;
	} else {
		if (sa) {	/* Initiate sector write process */
			if (!(CardType & CT_BLOCK)) sa *= 512;	/* Convert to byte address if needed */
			if (send_cmd(CMD24, sa) == 0) {			/* WRITE_SINGLE_BLOCK */
				SPI_write(0xFF); SPI_write(0xFE);		/* Data block header */
				wc = 512;							/* Set byte counter */
				res = RES_OK;
			}
		} else {	/* Finalize sector write process */
			bc = wc + 2;
			while (bc--) SPI_write(0);	/* Fill left bytes and CRC with zeros */
			if ((SPI_read() & 0x1F) == 0x05) {	/* Receive data resp and wait for end of write process in timeout of 300ms */
				for (bc = 65000; SPI_read() != 0xFF && bc; bc--) ;	/* Wait ready */
				if (bc) res = RES_OK;
			}
			release_spi();
		}
	}

	return res;
}

u16 decreasetim(u16 Tim)
{ u16 i;
  for (i=0; i<1; i++);
  if (Tim) Tim--;
  return Tim;
}

/********************************************************************
 * Function:        void PrintSectorData( BYTE* data )
 *
 * PreCondition:    None
 *
 * Input:           Pointer to a 512 byte buffer
 *
 * Output:          Humen readable data
 *
 * Side Effects:    None
 *
 * Overview:        Data is outputed in groups of 16 bytes per row
 *
 * Note:            None
 *******************************************************************/
/*
void disk_printp( u8* datx )
{
	u16 k, px;

	for(k = 0; k < 512; k++)
	{
		serial_printf("%2X ",datx[k]);

		if( ((k + 1) % 16) == 0)
		{
			serial_printf("  ");

			for(px = (k - 15); px <= k; px++)
			{
				if( ((datx[px] > 33) && (datx[px] < 126)) || (datx[px] == 0x20) )
				{
					serial_printf("%c ",datx[px]);
				}
				else
				{
					serial_printf(".");
				}
			}

			serial_printf("\n\r");
		}
	}

	return;
}
*/
#endif