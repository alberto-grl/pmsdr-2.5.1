 #ifndef SI570_H
 #define SI570_H
 
 /*
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions
  * are met:
  *
  * 1. Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright
  *    notice, this list of conditions and the following disclaimer in the
  *    documentation and/or other materials provided with the distribution.
  * 3. Neither the name of the copyright holders nor the names of
  *    contributors may be used to endorse or promote products derived
  *    from this software without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY EGNITE SOFTWARE GMBH AND CONTRIBUTORS
  * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL EGNITE
  * SOFTWARE GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
  * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
  * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  * SUCH DAMAGE.
  */
 
//-----------------------------------------------------------------------------
// Global CONSTANTS
//-----------------------------------------------------------------------------


//these must be floating point number especially 2^28 so that 
//there is enough memory to use them in the calculation 
#define POW_2_16              65536.0 
#define POW_2_24           16777216.0 
#define POW_2_28          268435456.0 
#define FOUT_START_UP	 56.320       //MHz 
#define FXTAL_DEVICE	 114.27902015


#ifndef I2C_SLA_SI570
#define I2C_SLA_SI570     0x55
#endif
 
#ifndef STARTUP_FREF
#define STARTUP_FREF   56320000UL
#endif

static const float FDCO_MAX = 5670; //MHz 
static const float FDCO_MIN = 4850; //MHz 
static const unsigned char HS_DIV[6] = {11, 9, 7, 6, 5, 4};
static const float fout0 = FOUT_START_UP;
static const float SI570_FOUT_MIN = 10; //MHz


//static unsigned long initial_rfreq_long;
//static unsigned long final_rfreq_long;
//static int f;

typedef
struct si570data_ {
        unsigned char n1;
        unsigned char initial_n1;
        unsigned char hsdiv;
        unsigned char initial_hsdiv;
        unsigned long frac_bits;
        double rfreq;
        double fxtal;
        double fxtal_startup;

} si570data;

//typedef struct si570data_ si570data;

//#define putst_debug putstUSART
 
/* Prototypes */
void calc_si570registers(unsigned char* reg, si570data* si570struct, bool calc_fxtal);

 /* End of prototypes */
#endif
