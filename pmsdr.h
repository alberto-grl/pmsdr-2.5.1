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

#if !defined __PMSDR_H__
#define      __PMSDR_H__


#include <stdbool.h>


struct pmsdr_;
typedef struct pmsdr_  *PMSDR;

typedef struct {
    unsigned short   QSDbias;           // 0 .. 1023, default 512
    bool             QSDmute;           // True mute the receiver
    unsigned char    filter;            // 0 no filter
                                        // 1 pass band filter 2-6   MHz
                                        // 2 pass band filter 5-12  MHz
                                        // 3 pass band filter 10-24 MHz
                                        // the following value is suitable for 2.1 HW only !
                                        // 4 low pass  filter 2 MHz
    // the following is suitable for 2.0 HW only !
    unsigned char    IfGain;            // 0 10 dB 
                                        // 1 20 dB 
                                        // 2 30 dB
                                        // 3 40 dB
} QsdBiasFilterIfGain;

/* Function declarations */

int pmsdr_init          (PMSDR *pp);
int pmsdr_open          (PMSDR p);
int pmsdr_open_device_n (PMSDR p, int nth_device);
int pmsdr_open_device_on_bus (struct pmsdr_ *p, int bus);


int pmsdr_release       (PMSDR p);
int pmsdr_deinit        (PMSDR p);

int pmsdr_set_hwlo   (PMSDR p, long freq);
int pmsdr_store_hwlo (PMSDR p, long newFreq);
int pmsdr_read_hwlo  (PMSDR p, long *pOldFreq);
int pmsdr_set_downconverterhwlo (PMSDR p, long freq);
int pmsdr_downconverter_setfilter(PMSDR p, unsigned char filter);
int pmsdr_set_qsd_bias (PMSDR p, unsigned long bias, bool mute, unsigned char if_gain, bool *ref_src, unsigned char filter );
int pmsdr_get_qsd_bias (PMSDR p, QsdBiasFilterIfGain *pQsd);
int pmsdr_read ( PMSDR p, unsigned short *P, unsigned char *Q, unsigned char *fref_ppm,
                 unsigned long  *fREF, unsigned long *fLO, unsigned char *postdivider);
int pmsdr_get_version (PMSDR p, unsigned short *pMajor, unsigned short *pMinor, unsigned short *pSub);
int pmsdr_get_transactions (PMSDR p, int *nTransactions, int *nBytesSent, int *nBytesRcvd);
int pmsdr_get_lcd_flag (PMSDR p, bool *pfLcd);
int pmsdr_get_downconverter_flag (PMSDR p, bool *pfdownconverter);
int pmsdr_lcdputs(PMSDR p, char *Data, unsigned short Length, unsigned short pos, unsigned short row);
int pmsdr_getrevision (void);

int pmsdr_print_usb_list (void);



enum errcode {
    PMSDR_NO_ERROR               = 0,
    PMSDR_NO_SUCH_DEVICE         = 1,
    PMSDR_INVALID_CTX            = 2,
    PMSDR_FOUND_UNABLE_TO_OPEN   = 3,
    PMSDR_UNABLE_TO_GET_USB_LIST = 4,
    PMSDR_CANT_OPEN_DEVICE       = 5,
    PMSDR_DEVICE_NOT_READY       = 6,
    
    PMSDR_GENERIC_ERROR          = -32678,
};

const char *pmsdr_error (int error_code);


#define PMSDR_VENDOR_ID    0x04d8 
#define PMSDR_PRODUCT_ID   0x000c

#define SR_VENDOR_ID       0x16c0 
#define SR_PRODUCT_ID      0x05dc

#endif
