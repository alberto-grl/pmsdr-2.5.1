/*

 PMSDR command line manager (experimental)

 Control a PMSDR hardware on Linux
 Modified to comply to firmware version 2.1.x

 Copyright (C) 2008,2009,2010  Andrea Montefusco IW0HDV, Martin Pernter IW3AUT

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <libusb-1.0/libusb.h>

#include "log.h"
#include "Si570.h"
#include "pmsdr.h"
#include "pmsdr_ctrl.h"


#define SVN_REV  "$Revision: 52 $"


static int libusb_refcount = 0;


static
int DoTransaction ( PMSDR p,
                    unsigned char *pTxData, int txLen,
                    unsigned char *pRxData, int *pRxLen
                  )
{
    int transferred = 0;
    int i, err = 0;

    int rc = libusb_interrupt_transfer ( p->devh, 0x01,
    		                             pTxData,
    		                             txLen,
    		                             &transferred,
                                         5000
    	                               );
    if (rc < 0) {
        fprintf(stderr, "usb_interrupt_transfer error %d\n", rc );
        err = rc;
    } else {

        p->nBytesSent += transferred;

        LOG( "SENT: %d ------> ", transferred );
        for (i=0; i<transferred; ++i) {
            LOG("%02x ", pTxData [i] );
        }
        LOG("%s", "\n");

        if ( pRxData && *pRxLen) {

            rc = libusb_interrupt_transfer ( p->devh, 0x81,
                                             pRxData,
                                             *pRxLen,
                                             pRxLen,
                                             5000
                                           );
            if (rc < 0) {
                fprintf(stderr, "usb_interrupt_transfer error %d\n", rc );
                err = rc;
            } else {
                p->nBytesRcvd += *pRxLen;

                LOG( "RCVD: %d <-----  ", *pRxLen );
                for (i=0; i < *pRxLen; ++i) {
                    LOG("%02x ", pRxData [i] );
                }
                LOG("%s", "\n");
            }
        } else {
            // transaction doesn't require response, do nothing 
        }
    }

    p->nTransactions += 1;

    return err;
}


/** D E C L A R A T I O N S **************************************************/
//---------------------------------------------------------------------------

#define READ_VERSION        0x00
#define READ_FLASH          0x01
#define WRITE_FLASH         0x02
#define ERASE_FLASH         0x03
#define READ_EEDATA         0x04
#define WRITE_EEDATA        0x05
#define READ_CONFIG         0x06
#define WRITE_CONFIG        0x07

#define ID_BOARD            0x31
#define UPDATE_LED          0x32
#define SET_LO_FREQ         0x33
#define RD_DDS_FREQ         0x34
#define RD_PMSDR            0x35
#define SET_QSD_BIAS        0x36
#define RD_LO_FREQ          0x37
#define LCD_PUTS            0x38
#define SET_PLL             0x39
#define SET_SI570           0x40
#define SET_FILTER          0x41
#define RD_USB_STRING	    0x42
#define RD_SI570            0x43
#define RD_FILTER           0x44
#define WriteI2C_PMSDR      0x45
#define ReadI2C_PMSDR       0x46
#define WriteE2P_PMSDR      0x47
#define ReadE2P_PMSDR       0x48
#define LCD_LIGHT	    0x49
#define SET_SMOOTHSI570     0x4A
#define ScanI2C_PMSDR       0x4B
#define SetFilterboard_PMSDR    0x4C
#define ScanSOFTI2C_PMSDR  0x4D
#define SET_DOWNCONVERTER 0x4E
#define RD_DOWNCONVERTER  0x4F
#define SET_DOWNCONVERTER_SMOOTH 0x50
#define WriteI2CSOFT_PMSDR      0x51
#define ReadI2CSOFT_PMSDR       0x52
#define	SET_DOWNCONVERTER_FILTER  0x53
#define RESET               0xFF

// PIC18F4550 E2Prom mapping
#define SI570_CALIB      0x01
#define SI570_REG_ADR    0x14    // Si570 Startup registers
#define POST_DIV_ADR     0x1A    // Cy2239x Startup postdivider register
#define DEF_FILTER_ADR   0x1B    // Startup filter register
#define DEFAULT_FRQ_ADR  0x1F    // Startup default frequency (long) 4 bytes


static int pmsdr_get_version_internal (PMSDR p, unsigned short *pMajor, unsigned short *pMinor, unsigned short *pSub)
{
    unsigned char txBuf [2];
    unsigned char rxBuf [5];
    int rc, rxLen = sizeof rxBuf;

    LOGF("%s\n", "***");
    memset (rxBuf, 0x55, sizeof rxBuf);

    txBuf [0] = READ_VERSION;
    txBuf [1] = 0x02;

    rc =  DoTransaction ( p, txBuf, sizeof txBuf, rxBuf, &rxLen );

    LOG("Ret code: %d\n", rc);

    if (rc == 0) {
        if (pMajor) *pMajor = (unsigned short) rxBuf[3] ;
        if (pMinor) *pMinor = (unsigned short) rxBuf[2] ;
        if (rxLen == 5 && rxBuf[0] == READ_VERSION && rxBuf[1] == 0x03) {
            LOG("rxLen: %d\n", rxLen);
            if (pSub) *pSub = (unsigned short) rxBuf[4] ;
        }
    }

    return rc;
}

#if 0
/******************************************************************************
 * Function:        int ResetPMSDR (PMSDR *p)
 *
 * PreCondition:    None
 *
 * Input:           None
 *                  
 * Output:          None
 *                  
 * Side Effects:    Reset hard the PIC inside PMSDR
 *
 * Overview:        Reset the external hardware.
 *
 * Note:            Never returns without erros, do not use in production environment.
 *****************************************************************************/
static
int ResetPMSDR(PMSDR p)
{
  LOGF("%s\n", "***");

  unsigned char txBuf [1];
  unsigned char rxBuf [1];
  int rc, rxLen = sizeof rxBuf;

  // mark the buffers
  memset (rxBuf, 0xFF, sizeof rxBuf);
  memset (txBuf, 0xFF, sizeof txBuf);

  // header
  txBuf [0] = RESET;

  rc =  DoTransaction ( p, txBuf, sizeof(txBuf), rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 && rxLen == 1 && rxBuf[0] == RESET) {
      return 0;
  } else {
      fprintf (stderr, "Unable to reset.");
      return rc;
  }
}
#endif


/******************************************************************************
 * Function:        int DeviceManager::SetSi570(unsigned char *Data, unsigned short &Length)
 *
 * PreCondition:    None
 *
 * Input:           register array Data
 *                  size (Length) of array
 * Output:          Return Value:
 *                  0 - Success
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Sets the Si570 register on PMSDR Board.
 *
 * Note:            None
 *****************************************************************************/
static
int SetSi570(PMSDR p, unsigned short *Data, unsigned short Length)
{
  LOGF("%s\n", "***");

  LOGF("Length: %d\n", Length);

  unsigned char txBuf [64];
  unsigned char rxBuf [1];
  int rc, rxLen = sizeof rxBuf;

  // mark the buffers
  memset (rxBuf, 0x55, sizeof rxBuf);
  memset (txBuf, 0x55, sizeof txBuf);

  // header
  txBuf [0] = SET_SI570;
  txBuf [1] = Length;

  // copy the data in the buffer
  for (int i = 2; i < (Length+2); i++) {
      txBuf [i] = Data[i-2];
  }

  rc =  DoTransaction ( p, txBuf, Length + 2, rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 && rxLen == 1 && rxBuf[0] == SET_SI570) {
      return 0;
  } else {
      fprintf (stderr, "Unable to set frequency into Si570.");
      return rc;
  }
}

/******************************************************************************
 * Function:        int SetDownconverter(unsigned char *Data, unsigned short &Length)
 *
 * PreCondition:    None
 *
 * Input:           register array Data
 *                  size (Length) of array
 * Output:          Return Value:
 *                  0 - Success
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Sets the Si570 register on Downconverter Board.
 *
 * Note:            None
 *****************************************************************************/
static
int SetDownconverter(PMSDR p, unsigned short *Data, unsigned short Length)
{
   LOGF("%s\n", "***");

  LOGF("Length: %d\n", Length);

  unsigned char txBuf [64];
  unsigned char rxBuf [1];
  int rc, rxLen = sizeof rxBuf;

  // mark the buffers
  memset (rxBuf, 0x55, sizeof rxBuf);
  memset (txBuf, 0x55, sizeof txBuf);

  // header
  txBuf [0] = SET_DOWNCONVERTER;
  txBuf [1] = Length;

  // copy the data in the buffer
  for (int i = 2; i < (Length+2); i++) {
      txBuf [i] = Data[i-2];
  }

  rc =  DoTransaction ( p, txBuf, Length + 2, rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 && rxLen == 1 && rxBuf[0] == SET_DOWNCONVERTER) {
      return 0;
  } else {
      fprintf (stderr, "Unable to set frequency into 2nd Si570.");
      return rc;
  }

}


/******************************************************************************
 * Function:        int SetSmoothSi570(unsigned char *Data, unsigned short &Length)
 *
 * PreCondition:    None
 *
 * Input:           register array Data
 *                  size (Length) of array
 * Output:          Return Value:
 *                  0 - Success
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Sets the Si570 register on PMSDR Board.
 *
 * Note:            None
 *****************************************************************************/
static
int SetSmoothSi570(PMSDR p, unsigned short *Data, unsigned short Length)
{
  LOGF("%s\n", "***");

  LOGF("Length: %d\n", Length);

  unsigned char txBuf [64];
  unsigned char rxBuf [1];
  int rc, rxLen = sizeof rxBuf;

  // mark the buffers
  memset (rxBuf, 0x55, sizeof rxBuf);
  memset (txBuf, 0x55, sizeof txBuf);

  // header
  txBuf [0] = SET_SMOOTHSI570;
  txBuf [1] = Length;

  // copy the data in the buffer
  for (int i = 2; i < (Length+2); i++) {
      txBuf [i] = Data[i-2];
  }

  rc =  DoTransaction ( p, txBuf, Length + 2, rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 && rxLen == 1 && rxBuf[0] == SET_SMOOTHSI570) {
      return 0;
  } else {
      fprintf (stderr, "Unable to set smooth frequency into Si570.");
      return rc;
  }
}

/******************************************************************************
 * Function:        int SetDownconverterSmooth(unsigned char *Data, unsigned short &Length)
 *
 * PreCondition:    None
 *
 * Input:           register array Data
 *                  size (Length) of array
 * Output:          Return Value:
 *                  0 - Success
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Sets the Si570 register on Downconverter Board.
 *
 * Note:            None
 *****************************************************************************/
static
int SetDownconverterSmooth(PMSDR p, unsigned short *Data, unsigned short Length)
{
 LOGF("%s\n", "***");

  LOGF("Length: %d\n", Length);

  unsigned char txBuf [64];
  unsigned char rxBuf [1];
  int rc, rxLen = sizeof rxBuf;

  // mark the buffers
  memset (rxBuf, 0x55, sizeof rxBuf);
  memset (txBuf, 0x55, sizeof txBuf);

  // header
  txBuf [0] = SET_DOWNCONVERTER_SMOOTH;
  txBuf [1] = Length;

  // copy the data in the buffer
  for (int i = 2; i < (Length+2); i++) {
      txBuf [i] = Data[i-2];
  }

  rc =  DoTransaction ( p, txBuf, Length + 2, rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 && rxLen == 1 && rxBuf[0] == SET_DOWNCONVERTER_SMOOTH) {
      return 0;
  } else {
      fprintf (stderr, "Unable to set smooth frequency into 2nd Si570.");
      return rc;
  }
}



/******************************************************************************
 * Function:        int DeviceManager::ReadSi570(char *Data)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Data - Array of bytes used to store a string.
 *                  Length - Contains the length of the string.
 *              Return Value:
 *                  0 - Success
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Retrieves the registers from Si570
 *
 * Note:            None
 *****************************************************************************/
static
int ReadSi570 (PMSDR p, unsigned char *Data)
{
  LOGF("%s\n", "***");

  unsigned char txBuf [1];
  unsigned char rxBuf [10];
  int rc, rxLen = sizeof rxBuf;

  // mark the buffer
  memset (rxBuf, 0x55, sizeof rxBuf);

  // header
  txBuf [0] = RD_SI570;

  rc =  DoTransaction ( p, txBuf, sizeof txBuf, rxBuf, &rxLen );

  LOGF("Ret code: %d, received data length: %d\n", rc, (int)rxBuf[1]);

  if ( rc == 0 && rxLen > 2 && rxBuf[0] == RD_SI570) {
      // copy the data in the buffer
      memcpy ( Data, &(rxBuf[2]), rxBuf[1] );
      return 0;
  } else {
      fprintf (stderr, "Unable to read from Si570.\n");
      return rc;
  }
  return 1;
}

/******************************************************************************
 * Function:        int ReadDownconv_Si570(char *Data)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Data - Array of bytes used to store a string.
 *                  Length - Contains the length of the string.
 *              Return Value:
 *                  0 - Success
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Retrieves the registers from downconverter Si570
 *
 * Note:            None
 *****************************************************************************/
static
int ReadDownconv_Si570(PMSDR p, unsigned char *Data)
{
  LOGF("%s\n", "***");

  unsigned char txBuf [1];
  unsigned char rxBuf [10];
  int rc, rxLen = sizeof rxBuf;

  // mark the buffer
  memset (rxBuf, 0x55, sizeof rxBuf);

  // header
  txBuf [0] = RD_DOWNCONVERTER;

  rc =  DoTransaction ( p, txBuf, sizeof txBuf, rxBuf, &rxLen );

  LOGF("Ret code: %d, received data length: %d\n", rc, (int)rxBuf[1]);

  if ( rc == 0 && rxLen > 2 && rxBuf[0] == RD_DOWNCONVERTER) {
      // copy the data in the buffer
      memcpy ( Data, &(rxBuf[2]), rxBuf[1] );
      return 0;
  } else {
      fprintf (stderr, "Unable to read from 2nd Si570.\n");
      return rc;
  }
  return 1;

}



/******************************************************************************
 * Function:        int DeviceManager::WriteI2C(LONG &freq)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Return Value:
 *                  0 - Success
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Write Register to a I2C-Device.
 *
 * Note:            None
 *****************************************************************************/
static
int PMSDRWriteI2C(PMSDR p, unsigned char slave,unsigned char reg,unsigned char data)
{
  LOGF("%s\n", "***");

  unsigned char txBuf [4];
  unsigned char rxBuf [1];
  int rc, rxLen = sizeof rxBuf;

  // header
  txBuf [0] = WriteI2C_PMSDR; 
  txBuf [1] = slave;          
  txBuf [2] = reg;            
  txBuf [3] = data;           

  rc =  DoTransaction ( p, txBuf, sizeof txBuf, rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 && rxLen == 1 && rxBuf[0] == WriteI2C_PMSDR) {
      return 0;
  } else {
      fprintf (stderr, "Unable to do I2C write into PMSDR.");
      return rc;
  }

}


/******************************************************************************
 * Function:        int DeviceManager::PMSDRScanI2C(unsigned char slave)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Return Value:
 *                  0 - I2C Device found
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Scan a I2C-Device.
 *
 * Note:            None
 *****************************************************************************/
static
int PMSDRScanI2C(PMSDR p, unsigned char slave, bool *pfFound)
{
  unsigned char txBuf [2];
  unsigned char rxBuf [3];
  int rc, rxLen = sizeof rxBuf;

  // assume device not found
  *pfFound = false;

  // header
  txBuf [0] = ScanI2C_PMSDR; 
  txBuf [1] = slave;          

  rc =  DoTransaction ( p, txBuf, sizeof txBuf, rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 && rxLen >= 2 && rxBuf[0] == ScanI2C_PMSDR ) {
      *pfFound = (rxBuf [2] == 0);
      return 0;
  } else {
      fprintf (stderr, "Unable to do I2C scan into PMSDR.");
      return rc;
  }
}

/******************************************************************************
 * Function:        int WriteI2CSoft(LONG &freq)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Return Value:
 *                  0 - Success
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Write Register to a I2C-Device on 2nd Master.
 *
 * Note:            None
 *****************************************************************************/
static
int PMSDRWriteI2CSoft(PMSDR p, unsigned char slave,unsigned char reg,unsigned char data)
{
 LOGF("%s\n", "***");

  unsigned char txBuf [4];
  unsigned char rxBuf [1];
  int rc, rxLen = sizeof rxBuf;

  // header
  txBuf [0] = WriteI2CSOFT_PMSDR; 
  txBuf [1] = slave;          
  txBuf [2] = reg;            
  txBuf [3] = data;           

  rc =  DoTransaction ( p, txBuf, sizeof txBuf, rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 && rxLen == 1 && rxBuf[0] == WriteI2CSOFT_PMSDR) {
      return 0;
  } else {
      fprintf (stderr, "Unable to write into 2nd I2C channel on PMSDR.");
      return rc;
  }

}

#if 0
/******************************************************************************
 * Function:        int ReadI2CSoft(LONG &freq)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Return Value:
 *                  0 - Success
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Read a Register from a I2C-Device.
 *
 * Note:            None
 *****************************************************************************/
static
int PMSDRReadI2CSoft(PMSDR p, unsigned char slave,unsigned char reg,unsigned char *data)
{
	unsigned char txBuf [3];
	unsigned char rxBuf [1];
	int rc, rxLen = sizeof rxBuf;


  txBuf[0] = ReadI2CSOFT_PMSDR;
  txBuf[1] = slave;
  txBuf[2] = reg;

	rc =  DoTransaction ( p, txBuf, sizeof txBuf, rxBuf, &rxLen );
	
  LOGF("Ret code: %d, received data length: %d\n", rc, (int)rxBuf[1]);

  if ( rc == 0 && rxLen > 2 && rxBuf[0] == RD_SI570) {
      // copy the data in the buffer
      memcpy ( data, &(rxBuf[2]), rxBuf[1] );
      return 0;
  } else {
      fprintf (stderr, "Unable to read from 2nd I2C channel.\n");
      return rc;
  }
  return 1;

}
#endif

/******************************************************************************
 * Function:        int PMSDRScanSOFTI2C(unsigned char slave)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Return Value:
 *                  0 - I2C Device found
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Scan a I2C-Device on the 2nd I2C Master channel.
 *
 * Note:            None
 *****************************************************************************/
static
int PMSDRScanSOFTI2C(PMSDR p, unsigned char slave, bool *pfFound)
{
  unsigned char txBuf [2];
  unsigned char rxBuf [3];
  int rc, rxLen = sizeof rxBuf;

  // assume device not found
  *pfFound = false;

  // header
  txBuf [0] = ScanSOFTI2C_PMSDR; 
  txBuf [1] = slave;          

  rc =  DoTransaction ( p, txBuf, sizeof txBuf, rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 && rxLen >= 2 && rxBuf[0] == ScanSOFTI2C_PMSDR ) {
      *pfFound = (rxBuf [2] == 0);
      return 0;
  } else {
      fprintf (stderr, "Unable to do I2C scan into PMSDR.");
      return rc;
  }
}



static
int PMSDRWriteE2P (PMSDR p, unsigned char reg, unsigned char data)
{
  unsigned char txBuf [3];
  unsigned char rxBuf [1];
  int rc, rxLen = sizeof rxBuf;

  // header
  txBuf[0] = WriteE2P_PMSDR;
  txBuf[1] = reg;
  txBuf[2] = data;

  rc =  DoTransaction ( p, txBuf, sizeof txBuf, rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 && rxLen == 1 && rxBuf[0] == WriteE2P_PMSDR ) {
      return 0;
  } else {
      fprintf (stderr, "Unable to do write EEPROM into PMSDR.");
      return rc;
  }
}

//------------------------------------------------------------------------------------------
static
int PMSDRreadE2P (PMSDR p, unsigned char reg, unsigned char *data)
{
  unsigned char txBuf [2];
  unsigned char rxBuf [3];
  int rc, rxLen = sizeof rxBuf;

  // header
  txBuf[0] = ReadE2P_PMSDR;
  txBuf[1] = reg;

  rc =  DoTransaction ( p, txBuf, sizeof txBuf, rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 && rxLen == 3 && rxBuf[0] == ReadE2P_PMSDR && rxBuf[1] == reg ) {
      *data = rxBuf[2];
      return 0;
  } else {
      fprintf (stderr, "Unable to do read EEPROM from PMSDR.");
      return rc;
  }
}
//------------------------------------------------------------------------------------------

/******************************************************************************
 * Function:        int pmsdr_downconverter_setfilter(unsigned char filter)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Return Value:
 *                  0 - Success
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Set the filter on DOwnconverter board.
 *
 * Note:            None
 *****************************************************************************/
int pmsdr_downconverter_setfilter(PMSDR p, unsigned char filter)
{
  unsigned char txBuf [2];
  unsigned char rxBuf [1];
  int rc, rxLen = sizeof rxBuf;

  // header
  txBuf[0] = SET_DOWNCONVERTER_FILTER;
  txBuf[1] = filter;

  rc =  DoTransaction ( p, txBuf, sizeof txBuf, rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 && rxLen == 1 && rxBuf[0] == SET_DOWNCONVERTER_FILTER ) {
      return 0;
  } else {
      fprintf (stderr, "Unable to set the filters on downconverter.");
      return rc;
  }  

}


///////////////////////////////////////////////////////////////
//
//  Si570 Helper functions
//
///////////////////////////////////////////////////////////////


static
unsigned char SetBits(unsigned char original, unsigned char reset_mask, unsigned char new_val) 
{ 
    return (( original & reset_mask ) | new_val ); 
} 

static
int FreqProgSi570 (PMSDR p, double currentFrequency)
{ 
	unsigned char counter;
	unsigned short reg[8];
	unsigned int divider_max;
	unsigned int curr_div; 
	unsigned int whole;
	unsigned char validCombo;
    float curr_n1;
	float n1_tmp;
	static double FreqLow;
    static double FreqHigh;
    static double last_freq;
    bool smooth_tune;
	
	if (currentFrequency > FreqHigh || currentFrequency < FreqLow) {
       FreqLow = currentFrequency-(currentFrequency*0.0035);
       FreqHigh = currentFrequency+(currentFrequency*0.0035);
       smooth_tune = false;
    } else { // SI570 smooth tuning
       if (( p->usFirmwareMajor >= 2 && p->usFirmwareMinor >= 1 && p->usFirmwareSub >= 4 ) ||
           ( p->usFirmwareMajor == 2 && p->usFirmwareMinor >= 2 ) ) 
          smooth_tune = true;

       else smooth_tune = false;
    }


    reg[6]=1;   // default set the CY2239X Postdivider =1
    currentFrequency*=4.0; // QSD Clock = 4 * frx

    while(currentFrequency < SI570_FOUT_MIN)  // find first currentFrequency above SI570_FOUT_MIN
    {
       reg[6]*=2;       // set the CY2239X Postdivider
       currentFrequency*=2.0;
    }
    p->postdivider=reg[6];
    
    if (smooth_tune){
            LOGF ("%s\n", "SMOOTH TUNE !!!!");
                p->si570pmsdr.rfreq = p->si570pmsdr.rfreq * currentFrequency /last_freq; //using float
	        for(counter = 0; counter < 6; counter++)
			    reg[counter] = 0; //clear registers
	        // (old method)
	        // convert new RFREQ to the binary representation
	        // separate the integer part
	        whole = floor(p->si570pmsdr.rfreq);
	        // get the binary representation of the fractional part
	        p->si570pmsdr.frac_bits = floor((p->si570pmsdr.rfreq - whole) * POW_2_28);
	        // set reg 12 to 10 making frac_bits smaller by
	        // shifting off the last 8 bits everytime
	        for(counter=5; counter >=3; counter--){
		        reg[counter] = p->si570pmsdr.frac_bits & 0xFF;
		        p->si570pmsdr.frac_bits = p->si570pmsdr.frac_bits >> 8;
	          }
	        // set the last 4 bits of the fractional portion in reg 9
	        reg[2] = SetBits(reg[2], 0xF0, (p->si570pmsdr.frac_bits & 0xF));
	        // set the integer portion of RFREQ across reg 8 and 9
	        reg[2] = SetBits(reg[2], 0x0F, (whole & 0xF) << 4);
	        reg[1] = SetBits(reg[1], 0xC0, (whole >> 4) & 0x3F);

            SetSmoothSi570(p, reg, 8); // set Registers on PMSDR-Board
            last_freq = currentFrequency;
	        return 0;
         }
            
    
	// find dividers (get the max and min divider range for the HS_DIV and N1 combo)
	divider_max = floor(FDCO_MAX / currentFrequency); //floorf for SDCC 
	curr_div = ceil(FDCO_MIN / currentFrequency); //ceilf for SDCC 
	validCombo = 0; 
	while (curr_div <= divider_max) 
	{ 
		//check all the HS_DIV values with the next curr_div 
		for(counter=0; counter<6; counter++) 
		{ 
			// get the next possible n1 value 
			p->si570pmsdr.hsdiv = HS_DIV[counter]; 
			curr_n1 = (curr_div * 1.0) / (p->si570pmsdr.hsdiv * 1.0); 
			// determine if curr_n1 is an integer and an even number or one
			// then it will be a valid divider option for the new frequency 
			n1_tmp = floor(curr_n1); 
			n1_tmp = curr_n1 - n1_tmp; 
			if(n1_tmp == 0.0) 
			{ 
				//then curr_n1 is an integer 
				p->si570pmsdr.n1 = (unsigned char) curr_n1; 
				if( (p->si570pmsdr.n1 == 1) || ((p->si570pmsdr.n1 & 1) == 0) ) 
				{ 
					// then the calculated N1 is either 1 or an even number 
					validCombo = 1; 
				} 
			}
			if(validCombo == 1) break; 
		} 
		if(validCombo == 1) break; 
		//increment curr_div to find the next divider 
		//since the current one was not valid 
		curr_div = curr_div + 1; 
	} 


	LOGF_DEB ("validcombo: %d  n1: %x hsdiv: %x fxtal: %f\n",validCombo,p->si570pmsdr.n1,p->si570pmsdr.hsdiv,p->si570pmsdr.fxtal);

	// if(validCombo == 0) at this point then there's an error 
	// in the calculatio. Check if the provided fout0 and fout1 
	// are not valid frequencies 
	// (old method) new RFREQ calculation -- kept for comparison purposes 
	
	if (validCombo == 0) {
        fprintf (stderr, "FreqProgSi570: error in calculation !!! \n");
		return -1;
    }
	p->si570pmsdr.rfreq = (currentFrequency * p->si570pmsdr.n1 * p->si570pmsdr.hsdiv) / p->si570pmsdr.fxtal; //using float
	for(counter = 0; counter < 6; counter++) 
	{
		reg[counter] = 0; //clear registers 
	} 
	
	// new HS_DIV conversion 
	p->si570pmsdr.hsdiv = p->si570pmsdr.hsdiv - 4; 
	//reset this memory 
	reg[0] = 0; 
	//set the top 3 bits of reg 13 
	reg[0] = (p->si570pmsdr.hsdiv << 5); 
	// convert new N1 to the binary representation 
	if(p->si570pmsdr.n1 == 1) p->si570pmsdr.n1 = 0; 
	else if((p->si570pmsdr.n1 & 1) == 0) p->si570pmsdr.n1 = p->si570pmsdr.n1 - 1; //if n1 is even, subtract one 
	// set reg 7 bits 0 to 4 
	reg[0] = SetBits(reg[0], 0xE0, p->si570pmsdr.n1 >> 2); 
	// set reg 8 bits 6 and 7 
	reg[1] = (p->si570pmsdr.n1 & 3) << 6; 
	
	// (old method) 
	// convert new RFREQ to the binary representation 
	// separate the integer part 
	whole = floor(p->si570pmsdr.rfreq); 
	// get the binary representation of the fractional part 
	p->si570pmsdr.frac_bits = floor((p->si570pmsdr.rfreq - whole) * POW_2_28); 
	// set reg 12 to 10 making frac_bits smaller by 
	// shifting off the last 8 bits everytime 
	for(counter=5; counter >=3; counter--) 
	{ 
		reg[counter] = p->si570pmsdr.frac_bits & 0xFF; 
		p->si570pmsdr.frac_bits = p->si570pmsdr.frac_bits >> 8;
	} 
	// set the last 4 bits of the fractional portion in reg 9 
	reg[2] = SetBits(reg[2], 0xF0, (p->si570pmsdr.frac_bits & 0xF)); 
	// set the integer portion of RFREQ across reg 8 and 9 
	reg[2] = SetBits(reg[2], 0x0F, (whole & 0xF) << 4); 
	reg[1] = SetBits(reg[1], 0xC0, (whole >> 4) & 0x3F); 

	SetSi570 (p, reg, 8); // set Registers on PMSDR-Board
	last_freq = currentFrequency;
	return 0;
}

static
int FreqProgDownconv (PMSDR p, double currentFrequency)
{ 
	unsigned char counter;
	unsigned short reg[8];
	unsigned int divider_max;
	unsigned int curr_div; 
	unsigned int whole;
	unsigned char validCombo;
    float curr_n1;
	float n1_tmp;
	static double FreqLow;
    static double FreqHigh;
    static double last_freq;
    bool smooth_tune;
	
	if (currentFrequency > FreqHigh || currentFrequency < FreqLow) {
       FreqLow = currentFrequency-(currentFrequency*0.0035);
       FreqHigh = currentFrequency+(currentFrequency*0.0035);
       smooth_tune = false;
    } else { // SI570 smooth tuning
       if (( p->usFirmwareMajor >= 2 && p->usFirmwareMinor >= 1 && p->usFirmwareSub >= 4 ) ||
           ( p->usFirmwareMajor == 2 && p->usFirmwareMinor >= 2) ) 
            smooth_tune = true;
       else smooth_tune = false;
    }
    
    if (smooth_tune){
            LOGF ("%s\n", "SMOOTH TUNE !!!!");
                p->si570downconv.rfreq = p->si570downconv.rfreq * currentFrequency /last_freq; //using float
	        for(counter = 0; counter < 6; counter++)
			    reg[counter] = 0; //clear registers
	        // (old method)
	        // convert new RFREQ to the binary representation
	        // separate the integer part
	        whole = floor(p->si570downconv.rfreq);
	        // get the binary representation of the fractional part
	        p->si570downconv.frac_bits = floor((p->si570downconv.rfreq - whole) * POW_2_28);
	        // set reg 12 to 10 making frac_bits smaller by
	        // shifting off the last 8 bits everytime
	        for(counter=5; counter >=3; counter--){
		        reg[counter] = p->si570downconv.frac_bits & 0xFF;
		        p->si570downconv.frac_bits = p->si570downconv.frac_bits >> 8;
	          }
	        // set the last 4 bits of the fractional portion in reg 9
	        reg[2] = SetBits(reg[2], 0xF0, (p->si570downconv.frac_bits & 0xF));
	        // set the integer portion of RFREQ across reg 8 and 9
	        reg[2] = SetBits(reg[2], 0x0F, (whole & 0xF) << 4);
	        reg[1] = SetBits(reg[1], 0xC0, (whole >> 4) & 0x3F);

            SetDownconverterSmooth(p, reg, 8); // set Registers on DOWNCONVERTER-Board
            last_freq = currentFrequency;
	        return 0;
         }
            
    
	// find dividers (get the max and min divider range for the HS_DIV and N1 combo)
	divider_max = floor(FDCO_MAX / currentFrequency); //floorf for SDCC 
	curr_div = ceil(FDCO_MIN / currentFrequency); //ceilf for SDCC 
	validCombo = 0; 
	while (curr_div <= divider_max) 
	{ 
		//check all the HS_DIV values with the next curr_div 
		for(counter=0; counter<6; counter++) 
		{ 
			// get the next possible n1 value 
			p->si570downconv.hsdiv = HS_DIV[counter]; 
			curr_n1 = (curr_div * 1.0) / (p->si570downconv.hsdiv * 1.0); 
			// determine if curr_n1 is an integer and an even number or one
			// then it will be a valid divider option for the new frequency 
			n1_tmp = floor(curr_n1); 
			n1_tmp = curr_n1 - n1_tmp; 
			if(n1_tmp == 0.0) 
			{ 
				//then curr_n1 is an integer 
				p->si570downconv.n1 = (unsigned char) curr_n1; 
				if( (p->si570downconv.n1 == 1) || ((p->si570downconv.n1 & 1) == 0) ) 
				{ 
					// then the calculated N1 is either 1 or an even number 
					validCombo = 1; 
				} 
			}
			if(validCombo == 1) break; 
		} 
		if(validCombo == 1) break; 
		//increment curr_div to find the next divider 
		//since the current one was not valid 
		curr_div = curr_div + 1; 
	} 


	LOGF_DEB ("validcombo: %d  n1: %x hsdiv: %x fxtal: %f\n",validCombo,p->si570pmsdr.n1,p->si570pmsdr.hsdiv,p->si570pmsdr.fxtal);

	// if(validCombo == 0) at this point then there's an error 
	// in the calculatio. Check if the provided fout0 and fout1 
	// are not valid frequencies 
	// (old method) new RFREQ calculation -- kept for comparison purposes 
	
	if (validCombo == 0) {
        fprintf (stderr, "FreqProgSi570: error in calculation !!! \n");
		return -1;
    }
	p->si570downconv.rfreq = (currentFrequency * p->si570downconv.n1 * p->si570downconv.hsdiv) / p->si570downconv.fxtal; //using float
	for(counter = 0; counter < 6; counter++) 
	{
		reg[counter] = 0; //clear registers 
	} 
	
	// new HS_DIV conversion 
	p->si570downconv.hsdiv = p->si570downconv.hsdiv - 4; 
	//reset this memory 
	reg[0] = 0; 
	//set the top 3 bits of reg 13 
	reg[0] = (p->si570downconv.hsdiv << 5); 
	// convert new N1 to the binary representation 
	if(p->si570downconv.n1 == 1) p->si570downconv.n1 = 0; 
	else if((p->si570downconv.n1 & 1) == 0) p->si570downconv.n1 = p->si570downconv.n1 - 1; //if n1 is even, subtract one 
	// set reg 7 bits 0 to 4 
	reg[0] = SetBits(reg[0], 0xE0, p->si570downconv.n1 >> 2); 
	// set reg 8 bits 6 and 7 
	reg[1] = (p->si570downconv.n1 & 3) << 6; 
	
	// (old method) 
	// convert new RFREQ to the binary representation 
	// separate the integer part 
	whole = floor(p->si570downconv.rfreq); 
	// get the binary representation of the fractional part 
	p->si570downconv.frac_bits = floor((p->si570downconv.rfreq - whole) * POW_2_28); 
	// set reg 12 to 10 making frac_bits smaller by 
	// shifting off the last 8 bits everytime 
	for(counter=5; counter >=3; counter--) 
	{ 
		reg[counter] = p->si570downconv.frac_bits & 0xFF; 
		p->si570downconv.frac_bits = p->si570downconv.frac_bits >> 8;
	} 
	// set the last 4 bits of the fractional portion in reg 9 
	reg[2] = SetBits(reg[2], 0xF0, (p->si570downconv.frac_bits & 0xF)); 
	// set the integer portion of RFREQ across reg 8 and 9 
	reg[2] = SetBits(reg[2], 0x0F, (whole & 0xF) << 4); 
	reg[1] = SetBits(reg[1], 0xC0, (whole >> 4) & 0x3F); 

	SetDownconverter (p, reg, 8); // set Registers on DOWNCONVERTER-Board
	last_freq = currentFrequency;
	return 0;
}


static
void read_si570(PMSDR p, bool calc_fxtal)
{
    int rc = ReadSi570 (p, p->si570_reg);

    if ( rc == 0 ) {
        p->postdivider=p->si570_reg[6];
        calc_si570registers(p->si570_reg,&p->si570pmsdr,calc_fxtal);
    }
}

static
int read_downconv_si570(PMSDR p, bool calc_fxtal)
{
    int rc = ReadDownconv_Si570 (p, p->downconv_si570_reg);

    if ( rc == 0 ) {
        calc_si570registers(p->downconv_si570_reg,&p->si570downconv,calc_fxtal);
        return  0;
    } else {
        return 1;
    }
}


//
// end of helper functions
//

//
// USB related helper functions
//

static
int GetUsbData ( libusb_device_handle *hDev, unsigned char **pszManBuf, unsigned char **pszProdBuf)
{
    unsigned char szManufacturer [BUFSIZ];
    unsigned char szProduct [BUFSIZ];

    libusb_device *pDev = 0;

    *pszManBuf = 0, *pszProdBuf = 0;
    pDev = libusb_get_device (hDev) ;
    if (pDev) {
        struct libusb_device_descriptor Desc;

        if ( libusb_get_device_descriptor ( pDev, &Desc) == 0 ) {

            if ( libusb_get_string_descriptor_ascii (hDev, Desc.iManufacturer, szManufacturer, sizeof szManufacturer) >= 0 
                                                   &&
                 libusb_get_string_descriptor_ascii (hDev, Desc.iProduct, szProduct, sizeof szProduct) >= 0      ) {

                *pszManBuf = szManufacturer, *pszProdBuf = szProduct;
                return 2;
            }
        }
    }
    return 0;
}


/* Public functions */

int pmsdr_getrevision (void)
{
    int rev = 0;
    if (sscanf (SVN_REV, "$Revision : %d", &rev) == 1) {
        return rev;
    } else {
        printf ("-----------------------------------------------------------------------------\n");
        printf ("Please download the source files with the following command: \n");
        printf ("    svn co https://pmsdr.svn.sourceforge.net/svnroot/pmsdr/trunk/linux pmsdr \n");
        printf ("-----------------------------------------------------------------------------\n");
        return PMSDR_GENERIC_ERROR;
    }
}


int pmsdr_set_hwlo(PMSDR p, long freq)
{
  LOGF("%s\n", "***");

  FreqProgSi570 (p, (double) (freq/1000000.0));
  read_si570 (p, 0);
  if (p->postdivider == 0) // if CY2239x is present on PMSDR-board
        p->LOfreq = (long) (1000000*(p->si570pmsdr.rfreq*p->si570pmsdr.fxtal)/(p->si570pmsdr.initial_hsdiv*p->si570pmsdr.initial_n1*4.0));
  else
        p->LOfreq = (long) (1000000*(p->si570pmsdr.rfreq*p->si570pmsdr.fxtal)/(p->si570pmsdr.initial_hsdiv*p->si570pmsdr.initial_n1*4.0*p->postdivider));

  return 0;
}

int pmsdr_set_downconverterhwlo(PMSDR p, long freq)
{
  LOGF("%s\n", "***");

  FreqProgDownconv (p, (double) (freq/1000000.0));
  if ( read_downconv_si570 (p, 0) == 0) {
      p->DownconverterLOfreq = (long) (1000000*(p->si570downconv.rfreq*p->si570downconv.fxtal)/(p->si570downconv.initial_hsdiv*p->si570downconv.initial_n1));

      LOGF("*** new downconverter frequency: %ld\n", p->DownconverterLOfreq);
      return 0;
  } else {
      return 1;
  }
}

int pmsdr_store_hwlo (PMSDR p, long newFreq)
{
    // write the SI570 registers into the EEPROM
    for(int i=0; i < 6; i++) 
        if (PMSDRWriteE2P (p, SI570_REG_ADR+i, p->si570_reg[i])) return 1;
    if (PMSDRWriteE2P (p, POST_DIV_ADR, p->si570_reg[6])) return 1;
    if (PMSDRWriteE2P (p, DEF_FILTER_ADR, p->qsdBfig.filter)) return 1;

    // scrive la frequenza attualmente sintonizzata nella EEPROM: 
    // serve solo per la visualizzazione sul LCD all'avvio del PMSDR, 
    // cosi' non serve a far fare i calcoli al PIC ;)
    // write the current generated frequency into the EEPROM
    // it is useful only to display the frequency on hardware startup;
    // in this way the PMSDR micro doesn't need to do heavy computation
    if (PMSDRWriteE2P (p, DEFAULT_FRQ_ADR + 0, (newFreq & 0x000000FF)       )) return 1;
    if (PMSDRWriteE2P (p, DEFAULT_FRQ_ADR + 1, (newFreq & 0x0000FF00) >> 8  )) return 1;
    if (PMSDRWriteE2P (p, DEFAULT_FRQ_ADR + 2, (newFreq & 0x00FF0000) >> 16 )) return 1;
    if (PMSDRWriteE2P (p, DEFAULT_FRQ_ADR + 3, (newFreq & 0xFF000000) >> 24 )) return 1;

    return 0;
}

int pmsdr_read_hwlo (PMSDR p, long *pNewFreq)
{
    unsigned char x;
    // read the SI570 registers from EEPROM
    for(int i=0; i < 6; i++) 
        if (PMSDRreadE2P (p, SI570_REG_ADR+i, &p->si570_reg[i])) return 1;
    if (PMSDRreadE2P (p, POST_DIV_ADR, &p->si570_reg[6])) return 1;
    if (PMSDRWriteE2P (p, DEF_FILTER_ADR, p->qsdBfig.filter)) return 1;

    // legge la frequenza attualmente sintonizzata nella EEPROM: 
    // read the current generated frequency into the EEPROM
    *pNewFreq = 0;
    if (PMSDRreadE2P (p, DEFAULT_FRQ_ADR + 3, &x)) return 1; *pNewFreq =   (*pNewFreq)       + x ; 
    if (PMSDRreadE2P (p, DEFAULT_FRQ_ADR + 2, &x)) return 1; *pNewFreq =  ((*pNewFreq) << 8) + x ; 
    if (PMSDRreadE2P (p, DEFAULT_FRQ_ADR + 1, &x)) return 1; *pNewFreq =  ((*pNewFreq) << 8) + x ; 
    if (PMSDRreadE2P (p, DEFAULT_FRQ_ADR + 0, &x)) return 1; *pNewFreq =  ((*pNewFreq) << 8) + x ; 
    return 0;
}


int pmsdr_init   (struct pmsdr_ **pp)
{
   struct pmsdr_ *p = malloc (sizeof(struct pmsdr_));
   if (p) {
       memset (p, 0, sizeof(struct pmsdr_));  // zeroize the structure

       if (libusb_refcount == 0) {
           int r = libusb_init(NULL);
           if (r < 0) {
               fprintf(stderr, "Failed to initialise libusb\n");
               return -1;
           } else {
               libusb_refcount++;
           }
           // try to reduce the library-generated messages, ineffective if the library was compiled with verbose flag
           libusb_set_debug (NULL,0);
       }
       // init data structure
       p->usVid         = 0;
       p->usPid         = 0;
       p->qsdBfig       = qsdBfigDefault;
       p->devh          = NULL;
       p->nTransactions = 0;
       p->nBytesSent    = 0;
       p->nBytesRcvd    = 0;
       p->fLcd          = false;
       p->fdownconverter= false;
       p->LOfreq        = 7050000;

       *pp = p;
       return 0;
   } else {
       return PMSDR_GENERIC_ERROR;
   }
}


static int pmsdr_open_device (PMSDR p)
{
    if (p) {
        int  usbInterface = 0;

        //
        // Reset the device 
        // In many systems (Ubuntu 8.04 and some Ubuntu 9.04) the PMSDR got stuck 
        // on the first transaction if, in a previous run, it had run an odd number
        // of commands (transactions).
        // Therefore I added a patch to do, on exit, a further transaction to bring
        // the total count of transaction to an even number.
        // I believed the problem was  due to some defect in the firmware but, 
        // after an update to my Ubuntu 9.04 that I did before Renon 2009 convention, 
        // the defect disappeared and I removed my patch from code.
        // However, in the older systems the problem was left.
        // The preliminary reset, as proposed by Patrick DH2SPK, overcomes this problem on all systems.
        // Thanks to Patrick Kulle DH2SPK for the patch.
        //
        if (libusb_reset_device(p->devh) != 0) {
           fprintf(stderr, "Could not reset device with ID %04X:%04X\n", p->usVid, p->usPid);
           goto out;
        }


        if ( GetUsbData (p->devh, &(p->pszManufacturer), &(p->pszProduct)) >= 2 ) {
            fprintf (stderr, "%s - %s\n", p->pszManufacturer, p->pszProduct);
        }

        //
        // check if the device has been got by the kernel
        //
        int r = libusb_kernel_driver_active ( p->devh, 0);

        switch (r) {
        case 0:   // no kernel driver is active, do nothing
            break;
        case 1:   // a kernel driver is active
            {
                fprintf (stderr, "Warning: Kernel device driver already active. Trying to detach....");
                if ( (r = libusb_detach_kernel_driver (p->devh, 0)) ) {
                    fprintf (stderr, "FAILED: %d.\n", r);
                } else {
                    fprintf (stderr, "SUCCESS.\n");
                }
            }
            break;
        case LIBUSB_ERROR_NO_DEVICE:   // the device has been disconnected
            fprintf (stderr, "Device disconnected.\n");
            goto out;
            break;
        default:
            fprintf (stderr, "Unexpected error: %d\n", r);
            goto out;
            break;
        }

        r = libusb_claim_interface(p->devh, usbInterface);
        if (r < 0) {
            fprintf(stderr, "usb_claim_interface error %d claiming interface %d\n", r, usbInterface );
            goto out;
        }
        printf("Interface %d claimed.\n", usbInterface);
		
		p->fdownconverter=false;
        for (int i = 0, r = -1; r < 0 && i < 3; ++i) {
            p->usFirmwareSub = 255;
            r = pmsdr_get_version_internal (p, &p->usFirmwareMajor, &p->usFirmwareMinor, &p->usFirmwareSub);
        }
        fprintf (stderr, "Firmware reports version %d.%d", p->usFirmwareMajor, p->usFirmwareMinor);
        if (p->usFirmwareSub != 255) {
            char *pszLcdPrompt = "* PM SDR Linux *";
                             //   012345678901234567890
            fprintf (stderr, ".%d\n", p->usFirmwareSub);

            if ( (p->usFirmwareMajor >= 2 &&p->usFirmwareMinor >= 2) 
                                       ||
                 (p->usFirmwareMajor >= 2 && p->usFirmwareMinor >= 0 && p->usFirmwareSub >= 2 ) ) {
                p->fLcd = true;
                pmsdr_lcdputs (p, pszLcdPrompt, strlen(pszLcdPrompt), 0, 1);
                                                                 // pos  row
            }

        } else {
            fprintf (stderr, "\n");
        }


        //
        // Check for Si570 and CY22393 on I2C internal bus.
        //
        if ( ((p->usFirmwareMajor == 2) && (p->usFirmwareMinor >= 2)) || (p->usFirmwareMajor >= 2 && p->usFirmwareMinor >= 1 && p->usFirmwareSub >= 7 )) {

            bool fFound = false;
			
            if ( PMSDRScanI2C (p, 0x55, &fFound) == 0 ) {
                if (fFound == true) {
                    fprintf (stderr, "Si570 detected.\n");
                } else {
                    fprintf (stderr, "Si570 not found !\n");
                }
            } else {
                fprintf (stderr, "Error searching for Si570 !\n");
            }
            fFound = false;

            if ( PMSDRScanI2C (p, 0x69, &fFound) == 0 ) {
                if (fFound == true) {
                    fprintf (stderr, "CY22393 detected.\n");
                } else {
                    fprintf (stderr, "CY22393 not found !\n");
                }
            } else {
                fprintf (stderr, "Error searching for CY22393 !\n");
            }
 			if ( PMSDRScanI2C (p, 0x21, &fFound) == 0 ) {
                if (fFound == true) {
                    fprintf (stderr, "Downconverter Board detected.\n");
					p->fdownconverter=true;
					if ( PMSDRScanSOFTI2C (p, 0x55, &fFound) == 0 ) {
		                if (fFound == true) {
                           fprintf (stderr, "Si570 on Downconverter Board detected.\n");
                           p->fdownconverter=true;
                           //
                           // TBE !!!!!!!!!
                           //
                           PMSDRWriteI2CSoft (p, 0x55, 0x87, 0x01);
                           read_downconv_si570(p, 1);
		                } else {
                            fprintf (stderr, "Si570 on Downconverter Board not found !\n");
                        }
                    }
                } 
            }
        }

        PMSDRWriteI2C(p, I2C_SLA_SI570, 135, 0x01);
        read_si570(p, 1);
        return 0;

 out:
        return PMSDR_GENERIC_ERROR;

    } else {
        return PMSDR_INVALID_CTX;
    }
}

int pmsdr_open (struct pmsdr_ *p)
{
   if (p) {
       //
       // no device select option specified, try to open the first PMSDR found
       //
       p->devh = libusb_open_device_with_vid_pid (NULL, SR_VENDOR_ID ,SR_PRODUCT_ID );
       if (p->devh == 0) {
           //fprintf(stderr, "Could not find/open device with ID %04X:%04X\n", PMSDR_VENDOR_ID ,PMSDR_PRODUCT_ID);
           p->devh = libusb_open_device_with_vid_pid (NULL, PMSDR_VENDOR_ID ,PMSDR_PRODUCT_ID );
           if (p->devh == 0) {
              return PMSDR_CANT_OPEN_DEVICE;
           } else {
              p->usVid         = PMSDR_VENDOR_ID;
              p->usPid         = PMSDR_PRODUCT_ID;
              return pmsdr_open_device (p);
           }
       } else {
           p->usVid         = SR_VENDOR_ID;
           p->usPid         = SR_PRODUCT_ID;
           return pmsdr_open_device (p);
       }
   } else {
      return PMSDR_INVALID_CTX;
   }
}

int pmsdr_deinit (PMSDR p)
{
    if (p) {
        if (p->devh) {
            libusb_release_interface(p->devh, 0);
            libusb_close(p->devh);
        }
        if (--libusb_refcount == 0) {
            libusb_exit(NULL);
        }
        free (p);
        return 0;
    } else 
        return PMSDR_INVALID_CTX;
}

int pmsdr_release (PMSDR p)
{
    if (p) {
        libusb_release_interface(p->devh, 0);
        libusb_close(p->devh);
        return 0;
    } else 
        return PMSDR_INVALID_CTX;
}


int pmsdr_get_qsd_bias (PMSDR p, QsdBiasFilterIfGain *pQsd)
{
    if (p) {
        memcpy (pQsd, &p->qsdBfig, sizeof(QsdBiasFilterIfGain));
        return 0;
    } else 
        return PMSDR_INVALID_CTX;
}


int pmsdr_get_version (PMSDR p, unsigned short *pMajor, unsigned short *pMinor, unsigned short *pSub)
{
    if (p) {
        *pMajor = p->usFirmwareMajor;
        *pMinor = p->usFirmwareMinor;
        *pSub   = p->usFirmwareSub; 
        return 0;
    } else 
        return PMSDR_INVALID_CTX;

}

int pmsdr_get_lcd_flag (PMSDR p, bool *pfLcd)
{
    if (p) {
        *pfLcd = p->fLcd;
        return 0;
    } else 
        return PMSDR_INVALID_CTX;
}

int pmsdr_get_downconverter_flag (PMSDR p, bool *pfdownconverter)
{
    if (p) {
        *pfdownconverter = p->fdownconverter;
        return 0;
    } else 
        return PMSDR_INVALID_CTX;
}

int pmsdr_get_transactions (PMSDR p, int *nTransactions, int *nBytesSent, int *nBytesRcvd)
{
    if (p) {
        *nTransactions = p->nTransactions; 
        *nBytesSent    = p->nBytesSent   ;
        *nBytesRcvd    = p->nBytesRcvd   ;
        return 0;
    } else 
        return PMSDR_INVALID_CTX;
}


/******************************************************************************
 * Function:        int DeviceManager::SetQSDBias(ULONG &bias)
 *
 * PreCondition:    None
 *
 * Input:           None
 *
 * Output:          Return Value:
 *                  0 - Success
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Sets the boards QSD Bias & Enable.
 *
 * Note:            None
 *****************************************************************************/
int pmsdr_set_qsd_bias (PMSDR p, unsigned long bias, bool mute, unsigned char if_gain, bool *ref_src, unsigned char filter )
{
  LOGF("%s\n", "***");

  unsigned char txBuf [6];
  unsigned char rxBuf [1];
  int rc, rxLen = sizeof rxBuf;

  // mark the buffers
  memset (rxBuf, 0x55, sizeof rxBuf);
  memset (txBuf, 0x55, sizeof txBuf);

  // header
  txBuf[0] = SET_QSD_BIAS;
  txBuf[1] = bias & 0xFF;
  txBuf[2] = (bias & 0xFF00)>>8;
  if (ref_src) {
      txBuf[3] = mute + (*ref_src)*2 ;
  } else {
      txBuf[3] = mute;
  }
  txBuf[4] = if_gain;
  txBuf[5] = filter;

  rc =  DoTransaction ( p, txBuf, sizeof txBuf, rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 && rxLen == 1 && rxBuf[0] == SET_QSD_BIAS) {
      return 0;
  } else {
      fprintf (stderr, "Unable to set QSD bias, filtering and IF gain into PMSDR.");
      return rc;
  }
}


/******************************************************************************
 * Function:        int DeviceManager::lcdputs(char *Data, unsigned short Length, unsigned short pos, unsigned short row)
 *
 * PreCondition:    None
 *
 * Input:           register array Data
 *                  size (Length) of array
 *                  position on row (pos)
 *                  row on LCD (row) 0-based
 * Output:          Return Value:
 *                  0 - Success
 *                  1 - Send/Receive Failed
 *                  2 - Data is not as expected
 *                  100 - Aborted Opeation
 * Side Effects:    None
 *
 * Overview:        Write string to LCD at pos in row on PMSDR Board.
 *
 * Note:            None
 *****************************************************************************/
int pmsdr_lcdputs(PMSDR p, char *Data, unsigned short Length, unsigned short pos, unsigned short row)
{
  LOGF("%s\n", "***");

  unsigned char txBuf [64];
  unsigned char rxBuf [1];
  int i, rc, rxLen = sizeof rxBuf;

  // mark the buffers
  memset (txBuf, 0x55, sizeof txBuf);

  // header
  txBuf[0] = LCD_PUTS;
  txBuf[1] = Length;
  txBuf[2] = row;
  txBuf[3] = pos;
  for (i = 4; i < Length+4; i++) {
      txBuf[i]= Data[i-4];
  }

  if ( p->usFirmwareMajor >= 2 && p->usFirmwareMinor > 0 ) {
      // firmware 2.1 or more
      rc =  DoTransaction ( p, txBuf, Length+4, 0, 0 );
  } else {
      // firmware 2.0
      rc =  DoTransaction ( p, txBuf, Length+4, rxBuf, &rxLen );
  }

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 ) {
      return 0;
  } else {
      fprintf (stderr, "Unable to send LCD string into PMSDR.");
      return rc;
  }


}

#define RPMSDR_PACKET_LEN  (sizeof (*fref_ppm) +sizeof (*Q)+sizeof (*P)+ sizeof (*fREF)+ sizeof (*fLO)+ sizeof (*postdivider) + 1)


#if 0
int pmsdr_read ( PMSDR p,
                 unsigned short *P,    unsigned char *Q,   unsigned char *fref_ppm, 
                 unsigned long  *fREF, unsigned long *fLO, unsigned char *postdivider)
{
  LOGF("%s\n", "***");
  

  unsigned char txBuf [1];
  unsigned char rxBuf [64];
  unsigned char cnt;
  int rc, rxLen = sizeof rxBuf;

  // mark the buffer
  memset (rxBuf, 0x55, sizeof rxBuf);

  // header
  txBuf [0] = RD_PMSDR;

  rxLen = RPMSDR_PACKET_LEN;
  rc =  DoTransaction ( p, txBuf, sizeof txBuf, rxBuf, &rxLen );

  LOGF("Ret code: %d\n", rc);

  if ( rc == 0 &&  rxLen > 2  && rxBuf[0] == RD_PMSDR) {
      // copy the data in the buffer

      cnt=0;
      *fref_ppm = rxBuf[cnt + sizeof (*fref_ppm)+sizeof (*Q)+ sizeof (*P)];
      *Q =  rxBuf[cnt +sizeof (*Q)+sizeof (*P)];
      *P =  rxBuf[cnt +sizeof (*P)]* 0x100;
      *P += rxBuf[cnt +sizeof (*P)-1];

      cnt+=sizeof (*fref_ppm);
      cnt+=sizeof (*Q);
      cnt+=sizeof (*P);

      *fREF =  rxBuf [cnt+ sizeof (*fREF)    ] * 0x1000000;
      *fREF += rxBuf [cnt+ sizeof (*fREF) -1 ] * 0x10000  ;
      *fREF += rxBuf [cnt+ sizeof (*fREF) -2 ] * 0x100    ;
      *fREF += rxBuf [cnt+ sizeof (*fREF) -3 ]            ;

      cnt+= sizeof (fREF);

      *fLO  = rxBuf [cnt + sizeof (*fLO)     ] * 0x1000000;
      *fLO += rxBuf [cnt + sizeof (*fLO) - 1 ] * 0x10000  ;
      *fLO += rxBuf [cnt + sizeof (*fLO) - 2 ] * 0x100    ;
      *fLO += rxBuf [cnt + sizeof (*fLO) - 3 ]            ;

      cnt+= sizeof (*fLO);
      *postdivider = rxBuf[ cnt + sizeof (*postdivider)];

      return 0;
  } else {
      fprintf (stderr, "Unable to read from PMSDR. Rec dat len: %d/%d\n", rxLen, (int)rxBuf[1]);
      return rc;
  }
  return 1;

}
#endif


int pmsdr_print_usb_list (void)
{
  libusb_device **ulist;

  ssize_t nud = libusb_get_device_list (NULL, &ulist);                    
  if ( nud <= 0) {
      fprintf(stderr, "Unable to get a list of USB devices.\n");
      return PMSDR_UNABLE_TO_GET_USB_LIST;
  } else {

      int i = 0;
      libusb_device *pDev;
      struct libusb_device_descriptor Desc;

      //
      // List all devices
      //
      {
          for (i = 0; i < nud; ++i) {

              pDev = ulist[i];

              //printf("Bus %03d Device %03d ", libusb_get_bus_number(pDev),  libusb_get_device_address(pDev) );

              if ( libusb_get_device_descriptor ( pDev, &Desc) == 0 ) {

                  printf("%d: %04x:%04x (bus %d, device %d)", i,
                         Desc.idVendor, Desc.idProduct,
                         libusb_get_bus_number(pDev), libusb_get_device_address(pDev));

                  if (Desc.idVendor == PMSDR_VENDOR_ID && Desc.idProduct == PMSDR_PRODUCT_ID) {
                      printf(" ***\n");
                  } else
                      if (Desc.idVendor == SR_VENDOR_ID && Desc.idProduct == SR_PRODUCT_ID) {
                         printf(" <<<\n");
                      } else {
                         printf("\n");
                      }
              }
          }

      }
  }
  return 0;
}



int pmsdr_open_device_n (struct pmsdr_ *p, int nth_device)
{
   if (p) {
       //
       // no device select option specified, try to open the first PMSDR found
       //
       libusb_device **ulist;

       ssize_t nud = libusb_get_device_list (NULL, &ulist);                    
       if ( nud <= 0) {
           fprintf(stderr, "Unable to get a list of USB devices.\n");
           return PMSDR_UNABLE_TO_GET_USB_LIST;
       } else {

           libusb_device *pDev;
           unsigned char szManufacturer [BUFSIZ];
           unsigned char szProduct [BUFSIZ];
           struct libusb_device_descriptor Desc;

           //
           // Select device (multi device operations)
           //

           if (nth_device != -1) {
               if (nth_device >= nud || nth_device < 0) {
                   fprintf (stderr, "Fatal error: the %dth device doesn't exist.\n", nth_device);
                   return PMSDR_NO_SUCH_DEVICE;
               } else {
                   pDev = ulist[nth_device];
                   if ( (libusb_get_device_descriptor ( pDev, &Desc) == 0 )
                                            && 
                        ( (Desc.idVendor == PMSDR_VENDOR_ID && Desc.idProduct == PMSDR_PRODUCT_ID)
                                                     ||
                          (Desc.idVendor == SR_VENDOR_ID && Desc.idProduct == SR_PRODUCT_ID) )
                        )
                   {
                       //      Open a device and obtain a device handle. 
                       if (libusb_open (pDev, &p->devh) == 0) {
                           if ( libusb_get_string_descriptor_ascii (p->devh, Desc.iManufacturer, szManufacturer, sizeof szManufacturer) >= 0
                                                                  &&
                                libusb_get_string_descriptor_ascii (p->devh, Desc.iProduct, szProduct, sizeof szProduct) >= 0      ) {

                               printf ("ID %04x:%04x %s/%s\n", Desc.idVendor, Desc.idProduct, szManufacturer, szProduct );
                           }
                           //libusb_close (devh);
                           return pmsdr_open_device (p);
                       } else {
                           printf("Unable to open the device.\n");
                           fprintf(stderr, "Could not find/open device with ID %04X:%04X\n", PMSDR_VENDOR_ID ,PMSDR_PRODUCT_ID);
                           return PMSDR_CANT_OPEN_DEVICE;
                       }
                   } else {
                       fprintf(stderr, "The device selected is not a PMSDR.\n");
                       return PMSDR_GENERIC_ERROR;
                   }

               }
           } else {
                return PMSDR_NO_SUCH_DEVICE;
           }
       }
   } else {
      fprintf (stderr, "Invalid pointer to PMSDR structure.\n");
      return PMSDR_INVALID_CTX;
   }
}


int pmsdr_open_device_on_bus (struct pmsdr_ *p, int bus)
{
   if (p) {
       libusb_device **ulist;

       ssize_t nud = libusb_get_device_list (NULL, &ulist);                    
       if ( nud <= 0) {
           return PMSDR_UNABLE_TO_GET_USB_LIST;
       } else {

         int i = 0;
         libusb_device *pDev;
         unsigned char szManufacturer [BUFSIZ];
         unsigned char szProduct [BUFSIZ];
         struct libusb_device_descriptor Desc;

         //fprintf(stderr, "Searching PMSDR on BUS %d.... [%d]\n", bus, nud);

         p->devh = 0;
         for (i = 0; i < nud; ++i) {

            pDev = ulist[i];

            int bus_n = libusb_get_bus_number(pDev);
            int dev_n = libusb_get_device_address(pDev);

            //fprintf(stderr, "Bus %03d Device %03d \n", bus_n, dev_n);

            if ( bus_n == bus && dev_n > 1 ) {
               if ( (libusb_get_device_descriptor ( pDev, &Desc) == 0 )
                                      && 
                    ( (Desc.idVendor == PMSDR_VENDOR_ID && Desc.idProduct == PMSDR_PRODUCT_ID)
                                                     ||
                      (Desc.idVendor == SR_VENDOR_ID && Desc.idProduct == SR_PRODUCT_ID) )
                  )
               {
                  //fprintf(stderr, "found !\n");
                  //  Open a device and obtain a device handle.
                  if (libusb_open (pDev, &p->devh) == 0) {
                     if ( libusb_get_string_descriptor_ascii (p->devh, Desc.iManufacturer, szManufacturer, sizeof szManufacturer) >= 0
                                                                    &&
                          libusb_get_string_descriptor_ascii (p->devh, Desc.iProduct, szProduct, sizeof szProduct) >= 0      ) {

                        fprintf (stderr, "ID %04x:%04x %s/%s\n", Desc.idVendor, Desc.idProduct, szManufacturer, szProduct );
                     }
                       // FOUND !
                       return pmsdr_open_device (p);
                  } else {
                     fprintf(stderr, "Device found, but unable to open on bus %d.%d.\n", bus_n, dev_n);
                     return PMSDR_FOUND_UNABLE_TO_OPEN;
                  }
               }
           }
         }
         printf ("\nNo ready PMSDR found on bus %d.\n", bus);
         return PMSDR_DEVICE_NOT_READY;
       }
   } else {
       return PMSDR_INVALID_CTX;
   }
}

