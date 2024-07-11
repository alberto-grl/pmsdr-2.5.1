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

#if !defined __CMD_PARSER_H__
#define      __CMD_PARSER_H__

int ParseFilterNumber (char *pszLine, int *pNewFilter, int v_major, int v_minor);
int ParseDownconverterFilterNumber (char *pszLine, int *pNewFilter);
int ParseQsdBias (char *pszLine, int *pNewBias);
int ParseQsdMute (char *pszLine, bool *pNewMute);
int ParseExit (const char *pszLine);
int ParseSetFrequency (char *pszLine, long *pNewFreq);
int ParseSetFrequency3 (char *pszLine, long *pNewFreq);
int ParseSetFrequencyDownconverter (char *pszLine, long *pNewFreq);
int ParseMemFrequency (char *pszLine, long *pNewFreq);
int ParseMemFrequency3 (char *pszLine, long *pNewFreq);
int ParsePrintLcd (char *pszLine, int *pRow, int *pCol, char *pszBuf, int len);
int ParseIfGain (char *pszLine, int *pNewIfGain);
int ParseMemFrequency (char *pszLine, long *pNewFreq);
int ParseReadFrequency (char *pszLine);
int ParseHelp (const char *pszLine);

void ParserHelpCommands (void);

#endif

