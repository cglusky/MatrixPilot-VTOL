// This file is part of the MatrixPilot FlashOSD tool.
//
//    http://code.google.com/p/gentlenav/
//
// Copyright 2009-2011 MatrixPilot Team
// See the AUTHORS.TXT file for a list of authors of MatrixPilot.
//
// MatrixPilot is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// MatrixPilot is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with MatrixPilot.  If not, see <http://www.gnu.org/licenses/>.


#include "../../libUDB/libUDB.h"
#include "font_data.h"


int charPosition = 0 ;
boolean didDisplay = 0 ;


void osd_update_glyph( void )
{
	osd_spi_write(0x9, charPosition) 	;		// CMAH: set glyph to overwrite
	
	unsigned char i ;
	for (i = 0; i < 54; i++)
	{
		osd_spi_write(0xA, i) ;					// CMAL: set the 4-px chunk of the glyph to overwrite
		osd_spi_write(0xB, font_data[charPosition*64 + i]) ;	// CMDI: update the data representing the 4-px chunk of the glyph
	}
	
	osd_spi_write(0x8, 0xA0) ;					// CMM: write glyph to NVRAM
	
	charPosition++ ;
	
	udb_led_toggle(LED_GREEN) ;					// Flash the green LED after each char is updated
}



int countdown = 40 ;
char skip = 0 ;

// Called every 25ms
void udb_servo_callback_prepare_outputs( void )
{
	if (countdown) {
		// delay for countdown/40 seconds
		countdown-- ;
		if (countdown == 0)
		{
			osd_spi_write(0x0, 0x00) ;	// VM0: disable display of OSD image
		}
		return ;
	}
	
	if (!skip)
	{
		if (charPosition < 256)
		{
			osd_update_glyph() ;
		}
	}
	
	skip = !skip ;
}


// Called every 1/2 second at low priority
void udb_background_callback_periodic(void)
{
	if (charPosition == 256 && !didDisplay)
	{
		LED_GREEN = LED_ON ;
		LED_GREEN = LED_ON ;
		
		osd_spi_write(0x04, 0) ;	// DMM set to 0
		osd_spi_write(0x0, 0x08) ;	// VM0: enable display of OSD image
		
		int row ;
		for (row = 0; row < 11; row++)
		{
			osd_spi_write_location(OSD_LOC(row+1, 3)) ;
			osd_spi_write(0x04, 1) ;	// DMM: Enable auto-increment mode
			int col ;
			for (col = 0; col<24; col++)
			{
				osd_spi_write_byte(row*24 + col) ;
			}
			osd_spi_write_byte(0xFF) ;
			didDisplay = 1 ;
		}
	}
	else
	{
		udb_led_toggle(LED_RED) ;
	}
	
	return ;
}


int main (void)
{
	// Set up the UDB library
	udb_init() ;
	
	// Start it up!
	udb_run() ;  // This never returns.
	
	return 0 ;
}


// Unused callbacks
void udb_background_callback_triggered(void) {}
void udb_gps_callback_received_byte( char rxchar ) {}
int udb_gps_callback_get_byte_to_send( void ) { return 0 ; }
void udb_serial_callback_received_byte( char rxchar ) {}
int udb_serial_callback_get_byte_to_send( void ) { return 0 ; }
void udb_magnetometer_callback_data_available( void ) {}
void udb_callback_read_sensors(void) {}
void udb_callback_radio_did_turn_off( void ) {}
