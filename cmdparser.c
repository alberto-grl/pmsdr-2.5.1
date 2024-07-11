/*

 PMSDR command line manager (experimental)

 Control a PMSDR hardware on Linux
 Modified to comply to firmware version 2.1.x

 Copyright (C) 2008,2009  Andrea Montefusco IW0HDV, Martin Pernter IW3AUT

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

#define _GNU_SOURCE
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

#define RE_SYNTAX_EGRE
#include <regex.h>

#include "log.h"
#include "cmdparser.h"


//
// Command parser functions
//


int ParseIfGain (char *pszLine, int *pNewIfGain)
{
    struct re_registers re_reg;
    struct re_pattern_buffer pattern_buffer;
    const char exit_re[] = "ifgain \\([0-9]+\\)\0";

    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB ("[%s]\n", exit_re);
    if ( re_compile_pattern (exit_re, strlen(exit_re), &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);
        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)&re_reg) >= 0) {
            LOG_DEB ("IFIF: %d\n", re_reg.num_regs);

            for (int x = 0; x < re_reg.num_regs; ++x) {
                LOG_DEB ("%d %d\n", re_reg.start[x], re_reg.end[x]);
            }
            if (re_reg.num_regs >= 2) {
                long newGain;

                LOGF_DEB ("%d %d\n", re_reg.start[1], re_reg.end[1]);
                pszLine[re_reg.end[1]] = 0;
                LOGF_DEB ("IFGAIN: %s\n", pszLine+re_reg.start[1]);
                if (sscanf (pszLine+re_reg.start[1], "%ld", &newGain) == 1) {

                    if ( newGain == 10 || newGain == 20 || newGain == 30 || newGain == 40) {
                        LOG_DEB ("New IF GAIN: %ld\n", newGain);
                        *pNewIfGain = newGain;
                        return 1;
                    } else {
                        fprintf (stderr, "Out of range value ! [%ld]\n", newGain);
                        return 0;
                    }

                }
            }

        }
    }
    return 0;
}



int ParseFilterNumber (char *pszLine, int *pNewFilter, int v_major, int v_minor)
{
    struct re_registers re_reg;
    struct re_pattern_buffer pattern_buffer;
    char * filter_re = "filter \\([0123]\\)$\0";
    int upperLimit = 3;

    if (v_major == 2 && v_minor >= 1) {
       filter_re = "filter \\([01234]\\)$\0";
       upperLimit = 4;
    }
    

    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB ("[%s]\n", filter_re);
    if ( re_compile_pattern (filter_re, strlen(filter_re), &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);
        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)&re_reg) >= 0) {
            LOG_DEB ("FILTER: %d\n", re_reg.num_regs);

            for (int x = 0; x < re_reg.num_regs; ++x) {
                LOG_DEB ("%d %d\n", re_reg.start[x], re_reg.end[x]);
            }
            if (re_reg.num_regs >= 2) {
                int newFilter;

                LOG_DEB ("%d %d\n", re_reg.start[1], re_reg.end[1]);
                pszLine[re_reg.end[1]] = 0;
                LOG_DEB ("FILTER: %s\n", pszLine+re_reg.start[1]);
                if (sscanf (pszLine+re_reg.start[1], "%d", &newFilter) == 1 && newFilter >= 0 && newFilter <= upperLimit ) {
                    *pNewFilter = newFilter ;
                    return 1;
                } else {
                    fprintf (stderr, "Out of range value ! [%d]\n", newFilter);
                    return 0;
                }
            }

        }
    }
    return 0;
}

int ParseDownconverterFilterNumber (char *pszLine, int *pNewFilter)
{
    struct re_registers re_reg;
    struct re_pattern_buffer pattern_buffer;
    const char filter_re[] = "dfilter \\([0123]\\)$\0";
    int upperLimit = 3;
    
    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB ("[%s]\n", filter_re);
    if ( re_compile_pattern (filter_re, strlen(filter_re), &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);
        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)&re_reg) >= 0) {
            LOG_DEB ("Downconverter FILTER: %d\n", re_reg.num_regs);

            for (int x = 0; x < re_reg.num_regs; ++x) {
                LOG_DEB ("%d %d\n", re_reg.start[x], re_reg.end[x]);
            }
            if (re_reg.num_regs >= 2) {
                int newFilter;

                LOG_DEB ("%d %d\n", re_reg.start[1], re_reg.end[1]);
                pszLine[re_reg.end[1]] = 0;
                LOG_DEB ("Downconverter FILTER: %s\n", pszLine+re_reg.start[1]);
                if (sscanf (pszLine+re_reg.start[1], "%d", &newFilter) == 1 && newFilter >= 0 && newFilter <= upperLimit ) {
                    *pNewFilter = newFilter ;
                    return 1;
                } else {
                    fprintf (stderr, "Out of range value ! [%d]\n", newFilter);
                    return 0;
                }
            }

        }
    }
    return 0;
}

int ParseQsdBias (char *pszLine, int *pNewBias)
{
    struct re_registers re_reg;
    struct re_pattern_buffer pattern_buffer;
    const char exit_re[] = "qsdbias \\([0-9]+\\)$\0";

    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB ("[%s]\n", exit_re);
    if ( re_compile_pattern (exit_re, strlen(exit_re), &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);
        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)&re_reg) >= 0) {
            LOG_DEB ("QSD BIAS: %d\n", re_reg.num_regs);

            for (int x = 0; x < re_reg.num_regs; ++x) {
                LOG_DEB ("%d %d\n", re_reg.start[x], re_reg.end[x]);
            }
            if (re_reg.num_regs >= 2) {
                int newBias;

                LOG_DEB ("%d %d\n", re_reg.start[1], re_reg.end[1]);
                pszLine[re_reg.end[1]] = 0;
                LOG_DEB ("QSD BIAS: %s\n", pszLine+re_reg.start[1]);
                if ( sscanf (pszLine+re_reg.start[1], "%d", &newBias) == 1  && newBias >= 0 && newBias < 1024 ) {
                    *pNewBias = newBias ;
                    return 1;
                } else {
                    fprintf (stderr, "Out of range value ! [%d]\n", newBias);
                    return 0;
                }
            }

        }
    }
    return 0;
}


int ParseQsdMute (char *pszLine, bool *pNewMute)
{
    struct re_registers re_reg;
    struct re_pattern_buffer pattern_buffer;
    const char exit_re[] = "qsdmute \\(on\\|off\\)$\0";

    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB ("[%s]\n", exit_re);
    if ( re_compile_pattern (exit_re, strlen(exit_re), &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);
        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)&re_reg) >= 0) {
            LOG_DEB ("QSD MUTE: %d\n", re_reg.num_regs);

            for (int x = 0; x < re_reg.num_regs; ++x) {
                LOG_DEB ("%d %d\n", re_reg.start[x], re_reg.end[x]);
            }
            if (re_reg.num_regs >= 2) {
                const char *pNewValue = pszLine+re_reg.start[1];

                LOG_DEB ("%d %d\n", re_reg.start[1], re_reg.end[1]);
                pszLine[re_reg.end[1]] = 0;
                LOG_DEB ("QSD MUTE: %s\n", pszLine+re_reg.start[1]);

                if ( strcmp (pNewValue, "off") == 0) {
                    *pNewMute = false;
                    return 1;
                } else 
                    if ( strcmp (pNewValue, "on") == 0) {
                        *pNewMute = true;
                        return 1;
                    } else {
                        fprintf (stderr, "Out of range value ! [%s]\n", pNewValue);
                        return 0;
                    }
            }
        }
    }
    return 0;
}




int ParseExit (const char *pszLine) 
{
    struct re_pattern_buffer pattern_buffer;
    const char exit_re[] = "^q\\(uit\\)*$";

    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB ("[%s] [%s]\n", exit_re, pszLine);
    if ( re_compile_pattern (exit_re, strlen(exit_re)-1, &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);
        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)0) >= 0) {
            return 1;
        }

    }
    return 0;
}

int ParseSetFrequency (char *pszLine, long *pNewFreq) 
{
    struct re_registers re_reg;
    struct re_pattern_buffer pattern_buffer;
    const char exit_re[] = "f\\(requency\\)* \\([0-9]+\\)\0";

    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB("Enter: %s\n", pszLine);
    if ( re_compile_pattern (exit_re, strlen(exit_re), &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);
        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)&re_reg) >= 0) {
            LOG_DEB ("NNN: %d\n", re_reg.num_regs);

            for (int x = 0; x < re_reg.num_regs; ++x) {
                LOG_DEB ("%d %d\n", re_reg.start[x], re_reg.end[x]);
            }
            if (re_reg.num_regs >= 2) {
                long newFreq;

                LOG_DEB ("%d %d\n", re_reg.start[2], re_reg.end[2]);
                pszLine[re_reg.end[2]] = 0;
                LOG_DEB ("FREQUENCY: %s\n", pszLine+re_reg.start[2]);
                if (sscanf (pszLine+re_reg.start[2], "%ld", &newFreq) == 1) {
                    LOG_DEB ("New frequency: %ld\n", newFreq);
                    *pNewFreq = newFreq;
                    return 1;
                }

            }

        }
    }
    return 0;
}


int ParseSetFrequency3 (char *pszLine, long *pNewFreq) 
{
    struct re_registers re_reg;
    struct re_pattern_buffer pattern_buffer;
    const char exit_re[] = "f\\(requency\\)*3 \\([0-9]+\\)\0";

    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB("Enter: %s\n", pszLine);
    if ( re_compile_pattern (exit_re, strlen(exit_re), &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);

        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)&re_reg) >= 0) {
            LOG_DEB ("NNN: %d\n", re_reg.num_regs);

            for (int x = 0; x < re_reg.num_regs; ++x) {
                LOG_DEB ("%d %d\n", re_reg.start[x], re_reg.end[x]);
            }
            if (re_reg.num_regs >= 2) {
                long newFreq;

                LOG_DEB ("%d %d\n", re_reg.start[2], re_reg.end[2]);
                pszLine[re_reg.end[2]] = 0;
                LOG_DEB ("FREQUENCY: %s\n", pszLine+re_reg.start[2]);
                if (sscanf (pszLine+re_reg.start[2], "%ld", &newFreq) == 1) {
                    LOG_DEB ("New frequency (*3): %ld\n", newFreq);
                    *pNewFreq = newFreq;
                    return 1;
                }

            }

        }
    }
    return 0;
}

int ParseSetFrequencyDownconverter (char *pszLine, long *pNewFreq) 
{
    struct re_registers re_reg;
    struct re_pattern_buffer pattern_buffer;
    const char exit_re[] = "fd \\([0-9]+\\)\0";

    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB("Enter: %s\n", pszLine);
    if ( re_compile_pattern (exit_re, strlen(exit_re), &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);
        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)&re_reg) >= 0) {
            LOG_DEB ("NNN: %d\n", re_reg.num_regs);

            for (int x = 0; x < re_reg.num_regs; ++x) {
                LOG_DEB ("%d %d\n", re_reg.start[x], re_reg.end[x]);
            }
            if (re_reg.num_regs >= 2) {
                long newFreq;

                LOG_DEB ("%d %d\n", re_reg.start[1], re_reg.end[1]);
                pszLine[re_reg.end[1]] = 0;
                LOG_DEB ("FREQUENCY: %s\n", pszLine+re_reg.start[1]);
                if ((sscanf (pszLine+re_reg.start[1], "%ld", &newFreq) == 1) && newFreq>10000000 && newFreq<810000000) {
                    LOG_DEB ("New Downconverter LO frequency:  %6ld,%03ld kHz\n",newFreq/1000,newFreq-(newFreq/1000)*1000);
                    *pNewFreq = newFreq;
                    return 1;
                } else {
                    fprintf (stderr, "Out of range value ! [%ld] Hz\n", newFreq);
                    return 0;
                }

            }

        }
    }
    return 0;
}


int ParseMemFrequency (char *pszLine, long *pNewFreq) 
{
    struct re_registers re_reg;
    struct re_pattern_buffer pattern_buffer;
    const char exit_re[] = "memf \\([0-9]+\\)\0";

    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB("Enter: %s\n", pszLine);
    if ( re_compile_pattern (exit_re, strlen(exit_re), &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);
        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)&re_reg) >= 0) {
            LOG_DEB ("NNN: %d\n", re_reg.num_regs);

            for (int x = 0; x < re_reg.num_regs; ++x) {
                LOG_DEB ("%d %d\n", re_reg.start[x], re_reg.end[x]);
            }
            if (re_reg.num_regs >= 2) {
                long newFreq;

                LOG_DEB ("%d %d\n", re_reg.start[1], re_reg.end[1]);
                pszLine[re_reg.end[1]] = 0;
                LOG_DEB ("FREQUENCY: %s\n", pszLine+re_reg.start[1]);
                if (sscanf (pszLine+re_reg.start[1], "%ld", &newFreq) == 1) {
                    LOG_DEB ("New frequency: %ld\n", newFreq);
                    *pNewFreq = newFreq;
                    return 1;
                }

            }

        }
    }
    return 0;
}


int ParseMemFrequency3 (char *pszLine, long *pNewFreq) 
{
    struct re_registers re_reg;
    struct re_pattern_buffer pattern_buffer;
    const char exit_re[] = "memf3 \\([0-9]+\\)\0";

    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB("Enter: %s\n", pszLine);
    if ( re_compile_pattern (exit_re, strlen(exit_re), &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);
        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)&re_reg) >= 0) {
            LOG_DEB ("NNN: %d\n", re_reg.num_regs);

            for (int x = 0; x < re_reg.num_regs; ++x) {
                LOG_DEB ("%d %d\n", re_reg.start[x], re_reg.end[x]);
            }
            if (re_reg.num_regs >= 2) {
                long newFreq;

                LOG_DEB ("%d %d\n", re_reg.start[1], re_reg.end[1]);
                pszLine[re_reg.end[1]] = 0;
                LOG_DEB ("FREQUENCY: %s\n", pszLine+re_reg.start[1]);
                if (sscanf (pszLine+re_reg.start[1], "%ld", &newFreq) == 1) {
                    LOG_DEB ("New frequency: %ld\n", newFreq);
                    *pNewFreq = newFreq;
                    return 1;
                }

            }

        }
    }
    return 0;
}

int ParsePrintLcd (char *pszLine, int *pRow, int *pCol, char *pszBuf, int len) 
{
    struct re_registers re_reg;
    struct re_pattern_buffer pattern_buffer;
    const char exit_re[] = "plcd \\([0-9]+\\) \\([0-9]+\\) \\(.+\\)\0";

    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB("Enter: %s\n", pszLine);
    if ( re_compile_pattern (exit_re, strlen(exit_re), &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);
        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)&re_reg) >= 0) {
            LOG_DEB ("NNN: %d\n", re_reg.num_regs);

            for (int x = 0; x < re_reg.num_regs; ++x) {
                LOG_DEB ("%d %d\n", re_reg.start[x], re_reg.end[x]);
            }

            if (re_reg.num_regs >= 2) {
                int row;

                LOG_DEB (".%d %d\n", re_reg.start[1], re_reg.end[1]);
                pszLine[re_reg.end[1]] = 0;
                LOG_DEB ("ROW: %s\n", pszLine+re_reg.start[1]);
                if (sscanf (pszLine+re_reg.start[1], "%d", &row) == 1) {
                    LOG_DEB ("Row: %d\n", row);
                    *pRow = row;
                } else {
                    return 0;
                }
            }

            if (re_reg.num_regs >= 3) {
                int col;

                LOG_DEB (".%d %d\n", re_reg.start[2], re_reg.end[2]);
                pszLine[re_reg.end[2]] = 0;
                LOG_DEB ("COL: %s\n", pszLine+re_reg.start[2]);
                if (sscanf (pszLine+re_reg.start[2], "%d", &col) == 1) {
                    LOG_DEB ("Col: %d\n", col);
                    *pCol = col;
                } else {
                    return 0;
                }
            }

            if (re_reg.num_regs >= 4) {

                LOG_DEB (".%d %d\n", re_reg.start[3], re_reg.end[3]);
                pszLine[re_reg.end[3]] = 0;
                LOG_DEB ("LCD: %s\n", pszLine+re_reg.start[3]);

                strcpy (pszBuf, pszLine+re_reg.start[3]);
                return 1;
            } else {
                return 0;
            }

        }
    }
    return 0;
}

int ParseReadFrequency ( char *pszLine)  
{
    struct re_pattern_buffer pattern_buffer;
    const char exit_re[] = "^rmemf.*$";

    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB ("[%s] [%s]\n", exit_re, pszLine);
    if ( re_compile_pattern (exit_re, strlen(exit_re)-1, &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);
        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)0) >= 0) {
            return 1;
        }
 
    }
    return 0;
}

void ParserHelpCommands (void)
{
    
    fprintf (stderr,   
          "\n"
          "<command> [<value>]               //\n"
          "FREQUENCY    : f       <d>        // f    7050000 == (   7.050,000 kHz)\n"
          "F3Harmonic   : f3      <d>        // f3 100300000 == ( 100.300     MHz)\n" 
          "FREQUENCY Dc : fd      <d>        // down converter conversion frequency in Hz\n" 
          "FILTER       : filter  <d>        // 0 (no filter),1 (2-6MHz),2 (5-12MHz),3 (10-24MHz), 4 (<2MHz)\n"
          "DFILTER      : dfilter <d>        // 0 (down converter bypass), 1 (VHF filter), 2 (UHF filter), 3 (unfiltered)\n"
          "QSDbias      : qsdbias <d>        // qsdbias 512 (default), 346-692 (interval allowed) \n"
          "QSDmute      : qsdmute <on,off>   // qsdmute on, qsd off (i.e. 0,1           \n"
          "STOREfreq    : memf    <d>        // store init freq into PMSDR EEPROM f 585000 == (585,000 kHz) \n"
          "STOREfreq3   : memf3   <d>        // store init f3/3 into PMSDR EEPROM  \n"
          "RESTORE freq : rmemf              // restore init freq from PMSDR EEPROM\n"
          "PRINTlcd     : plcd    <col> <row> <message_text> // print the message text at specified coord on the LCD\n"
          "QUIT         : quit               // exit from pmsdr\n"
          "HELP         : help               // this help!\n" 
          "\n"
          "INPUT-SEQUENCE-example:\n"	
          "  ->help,filter 4,f 1611000,f 585000,memf 585000,filter 0,f3 100300000,quit"	     
	     "\n\n");
}


int ParseHelp (const char *pszLine) 
{
    struct re_pattern_buffer pattern_buffer;
    const char exit_re[] = "^\\(\\(h\\(elp\\)*\\)\\|\\(\\?\\)\\)$";

    memset (&pattern_buffer, 0, sizeof pattern_buffer);

    LOGF_DEB ("[%s] [%s]\n", exit_re, pszLine);
    if ( re_compile_pattern (exit_re, strlen(exit_re)-1, &pattern_buffer) == 0) {
        LOG_DEB ("[%s]\n", pszLine);
        if (re_match (&pattern_buffer, 
                      pszLine, strlen (pszLine), 
                      0, (struct re_registers *)0) >= 0) {
            return 1;
        }

    }
    return 0;
}

