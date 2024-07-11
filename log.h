#if !defined __LOG_H__
#define      __LOG_H__

/**************************************************************************

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


*************************************************************************/
// $Revision: 38 $

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#if defined _LOG
#include <stdio.h>
#define LOGF(fs, ...)     { fprintf (stderr, "%s: ", __FUNCTION__); fprintf (stderr, fs, __VA_ARGS__) ; }
#define LOG(fs, ...)      { fprintf (stderr, fs, __VA_ARGS__) ; }
#define LOGF_DEB(fs, ...) 
#define LOG_DEB(fs, ...)  

#elif defined _LOG_DEB
#include <stdio.h>
#define LOGF(fs, ...)     { fprintf (stderr, "%s: ", __FUNCTION__); fprintf (stderr, fs, __VA_ARGS__) ; }
#define LOG(fs, ...)      { fprintf (stderr, fs, __VA_ARGS__) ; }
#define LOGF_DEB(fs, ...) { fprintf (stderr, "%s: ", __FUNCTION__); fprintf (stderr, fs, __VA_ARGS__) ; }
#define LOG_DEB(fs, ...)  { fprintf (stderr, fs, __VA_ARGS__) ; }

#else

#define LOGF(fs, ...)
#define LOG(fs, ...) 
#define LOGF_DEB(fs, ...) 
#define LOG_DEB(fs, ...)  

#endif

#endif
