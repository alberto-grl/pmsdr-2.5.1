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


*************************************************************************

 To compile this program and to setup the correct environment on your system, 
 please refer to detailed instructions in README.txt.

*/

#define _GNU_SOURCE
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <assert.h>

/* GNU readline */
#include <readline/readline.h>
#include <readline/history.h>

#include "log.h"
#include "pmsdr.h"
#include "cmdparser.h"
#include "udp_cmd.h"

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#define PMSDR_FILE "/tmp/PMSDRcommands"

// globals
char *pszPmSdrInputFile = PMSDR_FILE;
unsigned short usFirmwareMajor;
unsigned short usFirmwareMinor;
unsigned short usFirmwareSub;




char * GetNextLine ( const char *pszPrompt )
{
    static FILE *pInput = NULL;
    int          bytes_read;
    char         szLine [BUFSIZ];
    char        *pszLine = szLine;
    unsigned int nbytes = sizeof szLine - 1;

    static bool fFirstTime = false;

    if (fFirstTime == false) {
        fFirstTime = true;

        pInput = fopen (pszPmSdrInputFile, "r");
        if ( pInput == NULL) {
            pInput = stdin ;
            fprintf (stderr, "%s FIFO unavailable [%s]\nInput switched to standard input.\n", pszPmSdrInputFile, strerror(errno));
        }

    }

    if (pInput == stdin) {
        /* A static variable for holding the line. */
        static char *line_read = (char *)NULL;

        /* 
         * Read a string, and return a pointer to it.
         * Returns NULL on EOF. 
         */
        
        /* 
         * If the buffer has already been allocated,
         * return the memory to the free pool. 
         */
        if (line_read) {
            free (line_read);
            line_read = (char *)NULL;
        }

        /* Get a line from the user. */
        line_read = readline (pszPrompt);

        /* 
         *  If the line has any text in it,
         *  save it on the history. 
         */
        if (line_read && *line_read) {
            add_history (line_read);
            return (line_read);
        } else {
            if ((pInput == stdin) && feof(stdin)) {
                fprintf (stderr, "%s", "EOF detected on standard input.\n");
            }
            return NULL;
        }
    } else {

        while ( true ) {

            bytes_read = getline (&pszLine, &nbytes, pInput);

            if (bytes_read == -1) {
                fprintf (stderr, "ERROR (%d) ! Going to open FIFO...", bytes_read);
                fclose (pInput);
                pInput = fopen (pszPmSdrInputFile, "r");
                if ( pInput == NULL) {
                    fprintf (stderr, "%s", "Unable to open FIFO.\n");
                    fflush (stderr);
                    return NULL;
                } else {
                    fprintf (stderr, " %s was successfully opened\n", pszPmSdrInputFile);
                    fflush (stderr);
                }
            } else {
                return pszLine;
            }
        }
    }
    assert ("It should not be arrived here !" == 0); 
}


int CmdInterpreter (PMSDR p, char *szLine, int fLcd, int fdownconverter)
{
    int  newFilter = 0;
    int  newIfGain = 0;
    int  newBias   = 0;
    bool newMute;
    long newFreq;
    char szLcdBuf [BUFSIZ];
    int  lcdRow, lcdCol;
    QsdBiasFilterIfGain qsdBfig;

    LOG_DEB ("%d: [%s]\n", strlen(szLine), szLine);
    if ( ParseExit(szLine) ) {
        goto exit;
    } else

    if ( ParseSetFrequency (szLine, &newFreq) ) {

        fprintf (stderr, "New frequency: %ld Hz\n", newFreq);
        pmsdr_set_hwlo (p, newFreq);

        if (fLcd == true) {
            char szBuf[BUFSIZ];
            sprintf (szBuf, " %6ld,%03ld kHz ",newFreq/1000,newFreq-(newFreq/1000)*1000 );
			pmsdr_lcdputs (p, szBuf, strlen(szBuf), 0, 0);
        }

    } else

    if ( ParseSetFrequency3 (szLine, &newFreq) ) {
        long nf3 = newFreq;
        newFreq /= 3;
        fprintf (stderr, "New frequency: %ld Hz\n", newFreq);
        pmsdr_set_hwlo (p, newFreq);

        if (fLcd == true) {
            char szBuf[BUFSIZ];
            sprintf (szBuf, " %6ld,%03ld* kHz ",nf3/1000,nf3-(nf3/1000)*1000 );
            pmsdr_lcdputs (p, szBuf, strlen(szBuf), 0, 0);
        }

    } else

   if ( (usFirmwareMajor == 2 && ( (usFirmwareMinor >= 2 ) || (usFirmwareMinor >= 1 && usFirmwareSub >= 7))) && ParseReadFrequency (szLine )){

        fprintf (stderr,"Read frequency from EEPROM...");
        
        if (pmsdr_read_hwlo (p, &newFreq) == 0) {
            fprintf (stderr, "%ld\n", newFreq);
            if (fLcd == true) {
                char szBuf[BUFSIZ];
                sprintf (szBuf, "Read EPROM freq ");
                pmsdr_lcdputs (p, szBuf, strlen(szBuf), 0, 0);
                sprintf (szBuf, "freq %9.ldHz", newFreq);
                pmsdr_lcdputs (p, szBuf, strlen(szBuf), 0, 1);
            }
        }
    } else


    if ( (usFirmwareMajor == 2 && ( (usFirmwareMinor >= 2 ) || (usFirmwareMinor >= 1 && usFirmwareSub >= 8))) && ParseSetFrequencyDownconverter (szLine, &newFreq) ) { 		

		if(fdownconverter==true){
		fprintf (stderr, "New Downconverter LO-frequency: %ld Hz\n", newFreq);
        pmsdr_set_downconverterhwlo (p, newFreq);
		}
		else  fprintf (stderr, "%s", "Downconverter not present.\n");
       

	} else
	
	if ( (usFirmwareMajor == 2 && ( (usFirmwareMinor >= 2 ) || (usFirmwareMinor >= 1 && usFirmwareSub >= 8))) && ParseDownconverterFilterNumber (szLine, &newFilter)) {
        pmsdr_get_qsd_bias (p, &qsdBfig);
		
		if(fdownconverter==true){
        	pmsdr_downconverter_setfilter(p,newFilter);
        	fprintf (stderr, "New Downconverter filter: %d ->", newFilter);
			switch (newFilter){
            case 0: fprintf (stderr, "Downconverter disabled (HF input)\n"); break;
            case 1: fprintf (stderr, "VHF Band filter\n");break;
            case 2: fprintf (stderr, "UHF Band filter\n"); break;
            case 3: fprintf (stderr, "Broadband input\n");break;
            }
		}
		else  fprintf (stderr, "%s", "Downconverter not present.\n");
       
    } else


    if ( (usFirmwareMajor == 2 && ( (usFirmwareMinor >= 2 ) || (usFirmwareMinor >= 1 && usFirmwareSub >= 7))) && ParseMemFrequency (szLine, &newFreq) ) {

        fprintf (stderr, "Set new frequency: %ld Hz\n", newFreq);
        pmsdr_set_hwlo (p, newFreq);

        fprintf (stderr, "Store new frequency: %ld Hz\n", newFreq);
        pmsdr_store_hwlo (p, newFreq);

        if (fLcd == true) {
            char szBuf[BUFSIZ];
            sprintf (szBuf, " %6ld,%03ld kHz ",newFreq/1000,newFreq-(newFreq/1000)*1000 );
			pmsdr_lcdputs (p, szBuf, strlen(szBuf), 0, 0);
        }

    } else

    if ( (usFirmwareMajor == 2 && ( (usFirmwareMinor >= 2 ) || (usFirmwareMinor >= 1 && usFirmwareSub >= 7))) && ParseMemFrequency3 (szLine, &newFreq) ) {
        newFreq=newFreq/3;
        fprintf (stderr, "Set new frequency: %ld\n", newFreq);
        pmsdr_set_hwlo (p, newFreq);

        fprintf (stderr, "Store new frequency: %ld\n", newFreq);
        pmsdr_store_hwlo (p, newFreq);

        if (fLcd == true) {
            char szBuf[BUFSIZ];
    //	             1234567890123456
            sprintf (szBuf, "EPROM f=f3/3    ");	
            pmsdr_lcdputs (p, szBuf, strlen(szBuf), 0, 0);
            sprintf (szBuf, "freq %9.ldHz", newFreq);
            pmsdr_lcdputs (p, szBuf, strlen(szBuf), 0, 1);
        }

    } else

    if ( (usFirmwareMajor == 2 && usFirmwareMinor <= 0) && ParseIfGain (szLine, &newIfGain) ) {
        pmsdr_get_qsd_bias (p, &qsdBfig);

        qsdBfig.IfGain = (newIfGain / 10) -1;
        pmsdr_set_qsd_bias (p,
                         qsdBfig.QSDbias,
                         qsdBfig.QSDmute,   
                         qsdBfig.IfGain,    
                         0,
                         qsdBfig.filter );    
        fprintf (stderr, "New IF Gain: %d\n", newIfGain);
    } else

    if ( ParseFilterNumber (szLine, &newFilter, usFirmwareMajor, usFirmwareMinor)) {
        pmsdr_get_qsd_bias (p, &qsdBfig);

        qsdBfig.filter = newFilter;
        pmsdr_set_qsd_bias (p,
                         qsdBfig.QSDbias,
                         qsdBfig.QSDmute,   
                         qsdBfig.IfGain,    
                         0,
                         qsdBfig.filter );    
        fprintf (stderr, "New filter: %d\n", newFilter);
    } else

    if ( ParseQsdBias (szLine, &newBias)) {
        pmsdr_get_qsd_bias (p, &qsdBfig);

        if (newBias < 346 || newBias > 692) {
            fprintf (stderr, "QSD bias out of range: %d\n", newBias);
        } else {
            qsdBfig.QSDbias = newBias;
            pmsdr_set_qsd_bias (p,
                                qsdBfig.QSDbias,
                                qsdBfig.QSDmute,   
                                qsdBfig.IfGain,    
                                0,
                                qsdBfig.filter );    
            fprintf (stderr, "New QSD bias: %d\n", newBias);
        }

    } else

    if ( ParseQsdMute (szLine, &newMute)) {
        pmsdr_get_qsd_bias (p, &qsdBfig);

        qsdBfig.QSDmute = newMute;
        pmsdr_set_qsd_bias (p,
                            qsdBfig.QSDbias,
                            qsdBfig.QSDmute,   
                            qsdBfig.IfGain,    
                            0,
                            qsdBfig.filter );    
        fprintf (stderr, "New QSD bias: %d\n", newMute);
    } else

    if (ParsePrintLcd (szLine, &lcdRow, &lcdCol, szLcdBuf, sizeof(szLcdBuf))) {
        LOG ("LCD: row=%d, col=%d [%s]\n", lcdRow, lcdCol, szLcdBuf);
        if (fLcd == true) {
            pmsdr_lcdputs (p, szLcdBuf, strlen(szLcdBuf), lcdRow, lcdCol);
        } else {
            fprintf (stderr, "%s\n", "Discarding LCD output because release is too low !");
        }

    } else 


    if (ParseHelp (szLine)) {
        ParserHelpCommands ();

    } else {
        ParserHelpCommands ();
        fprintf (stderr, "Bad command.\n");
        LOG_DEB("BAD COMMAND: %s\n", szLine)
    }

    return 0;
exit:
    return 1;
}


int FileCmdInterpreter (PMSDR pmsdr, int fLcd, int fdownconverter)
{
    char          *szLine;

    // check for input file name change (useful for multi device environments)
    char *p = 0;
    if ((p = getenv("PMSDR_FILE")) && strlen(p)) {
       pszPmSdrInputFile = p;
    }
    fprintf (stderr, "Type 'help' or '?' for get online help.\n");
    while ( (szLine = GetNextLine ( "---> " )) ) {
        if (CmdInterpreter (pmsdr, szLine, fLcd, fdownconverter) != 0) break;
    }
    return 0;
}


int UdpCommandInterpreter (PMSDR pmsdr, int fLcd, int fdownconverter)
{
    char szLine [BUFSIZ];
    int  rLen = 0;

    while (1) 
      if ((rLen = udpGetNextCommand (szLine, sizeof(szLine))) > 0) {
          szLine[rLen] = 0;
          if (CmdInterpreter (pmsdr, szLine, fLcd, fdownconverter) != 0) {
              udpSendAnswer ("Closing...\n");
              break;
          } else {
              printf("xxxxxxxxxxxx: %d\n", udpSendAnswer ("OK\n") );
              continue;
          }
      } else {
          break;
      }         

    return 0;
}


void PrintHelp (void)
{
    printf ("Usage: ./pmsdr [options]\n");
    printf ("  -l ................... list all USB devices\n");
    printf ("  -N <number> .......... uses the nth PMSDR device (the device number can be found using the -l option)\n");
    printf ("  -B <number> .......... uses the first PMSDR found on nth USB bus\n");
    printf ("  -u <number> .......... listen for commands at UDP port specified\n");    
    printf ("  -h ................... this help\n");
}


int main(int argc, char **argv)
{
    bool           fLcd = false;
	bool           fdownconverter=false;
    int            rc = 0;
    int            nth_device = -1, bus = -1;
    int            print_list = 0;
    int            udp_port = -1;
    PMSDR          pmsdr = 0;

    fprintf (stderr, 
             "Linux PMSDR 2.0/2.1 control program, "
#if defined HAVE_CONFIG_H
             "version %s\n"
#else            
             "revision %d\n"
#endif             
             "Original code by Martin Pernter IW3AUT\n"
             "Linux porting by Andrea Montefusco IW0HDV\n", 
#if defined HAVE_CONFIG_H
              VERSION
#else
             pmsdr_getrevision ()
#endif
             );
	     
    // parse the command line
    int opt;
    while ((opt = getopt(argc, argv, "lB:N:D:u:h")) != -1) {
        switch (opt) {
        case 'l':
            print_list = 1;
            break;
        case 'N':
            nth_device = atoi(optarg);
            break;
        case 'B':
            bus = atoi(optarg);
            break;
        case 'u':
            udp_port = atoi(optarg);
            if (udpListen ((short) udp_port) < 0) {
                fprintf (stderr, "Unable to bind to UDP port %d.\n", udp_port);
                goto out;
            }
            break;
        case 'h':
            PrintHelp();
            goto out;
        default: ;/* '?' */
        }
    }

    if ((rc = pmsdr_init (&pmsdr)) != 0) {
        goto out;
    }

    if (print_list == 1) {
        pmsdr_print_usb_list ();
        goto out;
    }

    if (nth_device != -1) {
       if ((rc = pmsdr_open_device_n (pmsdr, nth_device)) != 0) {
           goto out;
       }
    } else {
       if (bus != -1) {
          if (pmsdr_open_device_on_bus (pmsdr, bus) != 0) {
             goto out;
          }
       } else {
          if ((rc = pmsdr_open (pmsdr)) != 0) {
             goto out;
          }
       }
    }

    if ((rc = pmsdr_get_version (pmsdr, &usFirmwareMajor, &usFirmwareMinor, &usFirmwareSub)) != 0) goto out;
       
    if ((rc = pmsdr_get_lcd_flag (pmsdr, &fLcd)) != 0) goto out;
    pmsdr_get_downconverter_flag (pmsdr, &fdownconverter);	
	
    //
    // Loop on command interpreter
    //
    if (udp_port > 0) {
        UdpCommandInterpreter (pmsdr, fLcd, fdownconverter);
    } else {
        FileCmdInterpreter (pmsdr, fLcd, fdownconverter);
    }



//exit:
    if (fLcd == true) {
        char *pszEndMsg1 = "program detached";
        if ((rc = pmsdr_lcdputs (pmsdr, pszEndMsg1, strlen(pszEndMsg1), 0, 1)) != 0) goto out;
    }
    {
        int nTransactions; int nBytesSent; int nBytesRcvd;
	
        pmsdr_get_transactions (pmsdr, &nTransactions, &nBytesSent, &nBytesRcvd);
        fprintf (stderr, "Transaction(s): %d Sent: %d Received: %d\n", nTransactions, nBytesSent, nBytesRcvd );
    }
    
//out_release:
	//pmsdr_release (pmsdr); //libusb_release_interface(devh, 0);
    

out:
    if (rc != 0) puts (pmsdr_error(rc));
           
    pmsdr_deinit (pmsdr);

	return rc;
}
