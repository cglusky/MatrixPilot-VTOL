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

#if (FLIGHT_PLAN_TYPE == FP_WAYPOINTS)


struct relWaypointDef { struct relative3D loc ; int flags ; struct relative3D viewpoint ; } ;
struct waypointDef { struct waypoint3D loc ; int flags ; struct waypoint3D viewpoint ; } ;

#include "waypoints.h"


#define NUMBER_POINTS (( sizeof waypoints ) / sizeof ( struct waypointDef ))
#define NUMBER_RTL_POINTS (( sizeof rtlWaypoints ) / sizeof ( struct waypointDef ))

int waypointIndex = 0 ;

struct waypointDef *currentWaypointSet = (struct waypointDef*)waypoints ;
int numPointsInCurrentSet = NUMBER_POINTS ;

struct waypointDef wp_inject ;
unsigned char wp_inject_pos = 0 ;
#define WP_INJECT_READY 255
const unsigned char wp_inject_byte_order[] = {3, 2, 1, 0, 7, 6, 5, 4, 9, 8, 11, 10, 15, 14, 13, 12, 19, 18, 17, 16, 21, 20 } ;

// For a relative waypoint, wp_to_relative() just passes the relative
// waypoint location through unchanged.
// For an absolute waypoint, wp_to_relative() converts the waypoint's
// location from absolute to relative.
struct relWaypointDef wp_to_relative(struct waypointDef wp)
{
	struct relWaypointDef rel ;
	
	if ( wp.flags & F_ABSOLUTE )
	{
		rel.loc = dcm_absolute_to_relative(wp.loc) ;
		rel.viewpoint = dcm_absolute_to_relative(wp.viewpoint) ;
		
		rel.flags = wp.flags - F_ABSOLUTE ;
	}
	else
	{
		rel.loc.x = wp.loc.x ;
		rel.loc.y = wp.loc.y ;
		rel.loc.z = wp.loc.z ;
		
		rel.viewpoint.x = wp.viewpoint.x ;
		rel.viewpoint.y = wp.viewpoint.y ;
		rel.viewpoint.z = wp.viewpoint.z ;
		
		rel.flags = wp.flags ;
	}
	
	return rel;
}


// In the future, we could include more than 2 waypoint sets...
// flightplanNum is 0 for main waypoints, and 1 for RTL waypoints
void init_flightplan ( int flightplanNum )
{
	if ( flightplanNum == 1 ) // RTL waypoint set
	{
		currentWaypointSet = (struct waypointDef*)rtlWaypoints ;
		numPointsInCurrentSet = NUMBER_RTL_POINTS ;
	}
	else if ( flightplanNum == 0 ) // Main waypoint set
	{
		currentWaypointSet = (struct waypointDef*)waypoints ;
    	numPointsInCurrentSet = NUMBER_POINTS ;
    }
	
	waypointIndex = 0 ;
	struct relWaypointDef current_waypoint = wp_to_relative(currentWaypointSet[0]) ;
	set_goal( GPSlocation , current_waypoint.loc ) ;
	set_camera_view(current_waypoint.viewpoint) ;
	setBehavior(current_waypoint.flags) ;
	
	// udb_background_trigger() ;			// trigger navigation immediately
	
	return ;
}


boolean use_fixed_origin( void )
{
#if ( USE_FIXED_ORIGIN == 1 )
	return 1 ;
#else
	return 0 ;
#endif
}


struct absolute3D get_fixed_origin( void )
{
	struct fixedOrigin3D origin = FIXED_ORIGIN_LOCATION ;
	
	struct absolute3D standardizedOrigin ;
	standardizedOrigin.x = origin.x ;
	standardizedOrigin.y = origin.y ;
	standardizedOrigin.z = (long)(origin.z * 100) ;
	
	return standardizedOrigin ;
}


void next_waypoint ( void ) 
{
	waypointIndex++ ;
	
	if ( waypointIndex >= numPointsInCurrentSet ) waypointIndex = 0 ;
	
	if ( waypointIndex == 0 )
	{
		if (numPointsInCurrentSet > 1)
		{
			struct relWaypointDef previous_waypoint = wp_to_relative( currentWaypointSet[numPointsInCurrentSet-1] ) ;
			struct relWaypointDef current_waypoint  = wp_to_relative( currentWaypointSet[0] ) ;
			set_goal( previous_waypoint.loc, current_waypoint.loc ) ;
			set_camera_view( current_waypoint.viewpoint ) ;
		}
		else
		{
			struct relWaypointDef current_waypoint = wp_to_relative( currentWaypointSet[0] ) ;
			set_goal( GPSlocation, current_waypoint.loc ) ;
			set_camera_view( current_waypoint.viewpoint ) ;
		}
		setBehavior( currentWaypointSet[0].flags ) ;
	}
	else
	{
		struct relWaypointDef previous_waypoint = wp_to_relative( currentWaypointSet[waypointIndex-1] ) ;
		struct relWaypointDef current_waypoint = wp_to_relative( currentWaypointSet[waypointIndex] ) ;
		set_goal( previous_waypoint.loc, current_waypoint.loc ) ;
		set_camera_view( current_waypoint.viewpoint ) ;
		setBehavior( current_waypoint.flags ) ;
	}
	
#if	( DEADRECKONING == 0 )
	compute_bearing_to_goal() ;
#endif
	
	return ;
}


void run_flightplan( void )
{
	// first run any injected wp from the serial port
	if (wp_inject_pos == WP_INJECT_READY)
	{
		struct relWaypointDef current_waypoint = wp_to_relative( wp_inject ) ;
		set_goal( GPSlocation, current_waypoint.loc ) ;
		set_camera_view( current_waypoint.viewpoint ) ;
		setBehavior( current_waypoint.flags ) ;
		compute_bearing_to_goal() ;
		wp_inject_pos = 0 ;
		
		return ;
	}
	
	// steering is based on cross track error.
 	// waypoint arrival is detected computing distance to the "finish line".
	
	// note: locations are measured in meters
	//		 velocities are in centimeters per second
	
	// locations have a range of +-32000 meters (20 miles) from origin
	
	if ( desired_behavior._.altitude )
	{
		if ( abs(IMUheight - goal.height) < ((int) HEIGHT_MARGIN ))
			next_waypoint() ;
	}
	else
	{
		if ( desired_behavior._.cross_track )
		{
			if ( tofinish_line < WAYPOINT_RADIUS ) // crossed the finish line
			{
				if ( desired_behavior._.loiter )
					set_goal( GPSlocation, wp_to_relative(currentWaypointSet[waypointIndex]).loc ) ;
				else
					next_waypoint() ;
			}
		}
		else
		{
			if ( (tofinish_line < WAYPOINT_RADIUS) || (togoal.x < WAYPOINT_RADIUS) ) // crossed the finish line
			{
				if ( desired_behavior._.loiter )
					set_goal( GPSlocation, wp_to_relative(currentWaypointSet[waypointIndex]).loc ) ;
				else
					next_waypoint() ;
			}
		}
	}
	
	return ;
}


void flightplan_live_begin( void )
{
	wp_inject_pos = 0 ;
	return ;
}


void flightplan_live_received_byte( unsigned char inbyte )
{
	if (wp_inject_pos < sizeof(wp_inject_byte_order))
	{
		((unsigned char*)(&wp_inject))[wp_inject_byte_order[wp_inject_pos++]] = inbyte ;
	}
	else if (wp_inject_pos == sizeof(wp_inject_byte_order))
	{
		wp_inject_pos++ ;
	}
	
	return ;
}


void flightplan_live_commit( void )
{
	if (wp_inject_pos == sizeof(wp_inject_byte_order))
	{
		wp_inject_pos = WP_INJECT_READY ;
	}
	else
	{
		wp_inject_pos = 0 ;
	}
	return ;
}


#endif
