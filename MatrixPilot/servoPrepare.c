// This file is part of MatrixPilot.
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


#include "defines.h"

//	routines to drive the PWM pins for the servos,
//	assumes the use of the 16MHz crystal.

int pitch_control, roll_control, yaw_control, throttle_control ;

void manualPassthrough( void ) ;


void init_servoPrepare( void )	// initialize the PWM
{
	int i;
	for (i=0; i <= NUM_INPUTS; i++)
		udb_pwTrim[i] = udb_pwIn[i] = ((i == THROTTLE_INPUT_CHANNEL) ? 0 : 3000) ;
	
	for (i=0; i <= NUM_OUTPUTS; i++)
		udb_pwOut[i] = ((i == THROTTLE_OUTPUT_CHANNEL) ? 0 : 3000) ;
	
#if (NORADIO == 1)
	udb_pwIn[MODE_SWITCH_INPUT_CHANNEL] = udb_pwTrim[MODE_SWITCH_INPUT_CHANNEL] = 4000 ;
#endif
	
	return ;
}

unsigned int wind_gain ;

void dcm_servo_callback_prepare_outputs(void)
{
	if (dcm_flags._.calib_finished)
	{
#if ( DEADRECKONING == 1 )
		process_flightplan() ;
#endif
		
		updateBehavior() ;
		wind_gain = wind_gain_adjustment () ;
		rollCntrl() ;
		yawCntrl() ;
		altitudeCntrl();
		pitchCntrl() ;
		servoMix() ;
#if ( USE_CAMERA_STABILIZATION == 1 )
		cameraCntrl() ;
#endif
		cameraServoMix() ;
		updateTriggerAction() ;
	}
	else
	{
		// otherwise, there is not anything to do
		manualPassthrough() ;	// Allow manual control while starting up
	}
	
	if ( dcm_flags._.calib_finished ) // start telemetry after calibration
	{
		// This is a simple check to send telemetry at 8hz
		if (udb_heartbeat_counter % 5 == 0)
		{
			serial_output_8hz() ;
		}
	}
	
#if (USE_OSD == 1)
	osd_run_step() ;
#endif
	
	return ;
}

void manualPassthrough( void )
{
	roll_control = pitch_control = yaw_control = throttle_control = 0 ;
	servoMix() ;
	
	return ;
}

