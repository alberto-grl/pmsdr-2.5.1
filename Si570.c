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
#include <stdbool.h>

#include "log.h"
#include "Si570.h"


void calc_si570registers(unsigned char* reg, si570data* si570struct, bool calc_fxtal)
{
	LOGF("%s\n", "***");
	// HS_DIV conversion 
	si570struct->initial_hsdiv = ((reg[0] & 0xE0) >> 5) + 4; // get reg 7 bits 5, 6, 7
	// initial_hsdiv's value could be verified here to ensure that it is one
	// of the valid HS_DIV values from the datasheet. 
	// initial_n1 conversion 
	si570struct->initial_n1 = (( reg[0] & 0x1F ) << 2 ) + // get reg 7 bits 0 to 4 
	(( reg[1] & 0xC0 ) >> 6 );  // add with reg 8 bits 7 and 8 
	if(si570struct->initial_n1 == 0)  si570struct->initial_n1 = 1;
        else if((si570struct->initial_n1 & 1) != 0) // add one to an odd number
	 si570struct->initial_n1 = si570struct->initial_n1 + 1;

	si570struct->frac_bits = (( reg[2] & 0xF ) * POW_2_24 ); 
	si570struct->frac_bits = si570struct->frac_bits + (reg[3] * POW_2_16);
	si570struct->frac_bits = si570struct->frac_bits + (reg[4] * 256); 
	si570struct->frac_bits = si570struct->frac_bits + reg[5]; 
	
	
	si570struct->rfreq = si570struct->frac_bits; 
	si570struct->rfreq = si570struct->rfreq / POW_2_28;
	si570struct->rfreq = si570struct->rfreq + ( (( reg[1] & 0x3F ) << 4 ) + (( reg[2] & 0xF0 ) >> 4 ) );
	if (calc_fxtal)
	  si570struct->fxtal = (fout0 * si570struct->initial_n1 * si570struct->initial_hsdiv) / si570struct->rfreq; //MHz
}

