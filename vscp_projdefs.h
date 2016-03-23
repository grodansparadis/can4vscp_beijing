/* ******************************************************************************
 * 	VSCP (Very Simple Control Protocol) 
 * 	http://www.vscp.org
 *
 *  Copyright (C) 1995-2016 Ake Hedman, Grodans Paradis AB,
 *                              <akhe@grodansparadis.com>
 *
 *  This work is licensed under the Creative Common 
 *  Attribution-NonCommercial-ShareAlike 3.0 Unported license. The full
 *  license is available in the top folder of this project (LICENSE) or here
 *  http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 *  It is also available in a human readable form here 
 *  http://creativecommons.org/licenses/by-nc-sa/3.0/
 * 
 *	This file is part of VSCP - Very Simple Control Protocol 	
 *	http://www.vscp.org
 *
 * ******************************************************************************
*/

#ifndef BEIJING_PROJDEFS_H
#define BEIJING_PROJDEFS_H

// VSCP Defines
// ============
//
// Described in vscp.h

// Enable error reporting
#define VSCP_FIRMWARE_ENABLE_ERROR_REPORTING

// Enable logging functionality
#define VSCP_FIRMWARE_ENABLE_LOGGING

#define ENABLE_WRITE_2PROTECTED_LOCATIONS // Enable write to protected locations

// Enable drop nickname specials
//#define DROP_NICKNAME_EXTENDED_FEATURES

// EEPROM bigger then 256 bytes
//#define EEADRH

#endif
