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

#if !defined __PMSDR_CTRL_H__
#define      __PMSDR_CTRL_H__

const static QsdBiasFilterIfGain qsdBfigDefault = {
    512,  // bias at half scale
    0,    // mute off
    0,    // no filter
    1     // 20 dB gain
};

struct pmsdr_ {

   unsigned short usVid;
   unsigned short usPid;

   unsigned short usFirmwareMajor;
   unsigned short usFirmwareMinor;
   unsigned short usFirmwareSub;

   unsigned char *pszManufacturer;
   unsigned char *pszProduct;

   QsdBiasFilterIfGain qsdBfig;

   struct libusb_device_handle *devh;

   int nTransactions;
   int nBytesSent   ;
   int nBytesRcvd   ;

   bool fLcd;
   bool fdownconverter;			
   // vars for Si570 calculations

   long          LOfreq;
   long DownconverterLOfreq;
   unsigned long fREF;
   unsigned long data;
   unsigned char postdivider, fref_ppm, Q;
   unsigned short int P;
   unsigned char si570_reg[8];
   unsigned char downconv_si570_reg[8];
   char          USBstring[62];
   int           USBstring_len;

   si570data si570pmsdr;
   si570data si570downconv;
};


#endif
