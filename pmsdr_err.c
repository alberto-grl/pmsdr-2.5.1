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


*************************************************************************/

#include <stdbool.h>
#include "pmsdr.h"

typedef struct pmerr_tbl_ {
    const enum errcode code;
    const char msg [256];
} PMSDR_ERR_TBL;


const PMSDR_ERR_TBL tbl [] =
{
    { PMSDR_NO_ERROR,               ""                                                },
    { PMSDR_NO_SUCH_DEVICE,         "No such device"                                  },
    { PMSDR_INVALID_CTX,            "Invalid pointer to PMSDR structure"              },
    { PMSDR_FOUND_UNABLE_TO_OPEN,   "Device found, but unable to open on bus %d.%d.\n"},
    { PMSDR_UNABLE_TO_GET_USB_LIST, "Unable to get a list of USB devices"             },
    { PMSDR_CANT_OPEN_DEVICE,       "Could not find/open device"                      },
    { PMSDR_DEVICE_NOT_READY,       "Device not ready"                                },

    { PMSDR_GENERIC_ERROR,          "Generic error"                                   },
};

const char *pmsdr_error (int error_code)
{
    int i;
    
    if (error_code < 0) return "Libusb internal error";
    
    for (i=0; i < (sizeof(tbl)/sizeof(tbl[0])); ++i) {
        if (error_code == tbl[i].code) {
            return tbl[i].msg;
        }
    }
    return "Not codified error";
}

