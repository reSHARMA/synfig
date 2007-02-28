/* === S Y N F I G ========================================================= */
/*!	\file time.cpp
**	\brief Template File
**
**	$Id: time.cpp,v 1.1.1.1 2005/01/04 01:23:15 darco Exp $
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**
**	This package is free software; you can redistribute it and/or
**	modify it under the terms of the GNU General Public License as
**	published by the Free Software Foundation; either version 2 of
**	the License, or (at your option) any later version.
**
**	This package is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
**	General Public License for more details.
**	\endlegal
*/
/* ========================================================================= */

/* === H E A D E R S ======================================================= */

#ifdef USING_PCH
#	include "pch.h"
#else
#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include "time.h"
#include <ETL/stringf>
#include <ETL/misc>
#include "general.h"
#include <cmath>
#include <cassert>
#include <ctype.h>
#include <math.h>


#ifdef WIN32
#include <float.h>
#ifndef isnan
extern "C" { int _isnan(double x); }
#define isnan _isnan
#endif
#endif

// For some reason isnan() isn't working on macosx any more.
// This is a quick fix.
#if defined(__APPLE__) && !defined(SYNFIG_ISNAN_FIX)
#ifdef isnan
#undef isnan
#endif
inline bool isnan(double x) { return x != x; }
inline bool isnan(float x) { return x != x; }
#define SYNFIG_ISNAN_FIX 1
#endif


#endif

/* === U S I N G =========================================================== */

using namespace std;
using namespace etl;
using namespace synfig;

#define tolower ::tolower

/* === M A C R O S ========================================================= */

/* === G L O B A L S ======================================================= */

/* === M E T H O D S ======================================================= */

Time::Time(const String &str_, float fps):
	value_(0)
{
	String str(str_);
	std::transform(str.begin(),str.end(),str.begin(),&tolower);

	// Start/Begin Of Time
	if(str=="sot" || str=="bot")
	{
		operator=(begin());
		return;
	}
	// End Of Time
	if(str=="eot")
	{
		operator=(end());
		return;
	}


	unsigned int pos=0;
	int read;
	float amount;
	
	// Now try to read it in the letter-abreviated format
	while(pos<str.size() && sscanf(String(str,pos).c_str(),"%f%n",&amount,&read))
	{
		pos+=read;
		if(pos>=str.size() || read==0)
		{
			// Throw up a warning if there are no units
			// and the amount isn't zero. There is no need
			// to warn about units if the value is zero
			// it is the only case where units are irrelevant.
			if(amount!=0)
				synfig::warning("Time(): No unit provided in time code, assuming SECONDS (\"%s\")",str.c_str());
			value_+=amount;
			return;
		}
		switch(str[pos])
		{
			case 'h':
			case 'H':
				value_+=amount*3600;
				break;
			case 'm':
			case 'M':
				value_+=amount*60;
				break;
			case 's':
			case 'S':
				value_+=amount;
				break;
			case 'f':
			case 'F':
				if(fps)
					value_+=amount/fps;
				else
					synfig::warning("Time(): Individual frames referenced, but frame rate is unknown");
				break;
			case ':':
				// try to read it in as a traditional time format
				{
					int hour,minute,second;
					float frame;
					if(fps && sscanf(str.c_str(),"%d:%d:%d.%f",&hour,&minute,&second,&frame)==4)
					{
							value_=frame/fps+(hour*3600+minute*60+second);
							return;
					}
			
					if(sscanf(str.c_str(),"%d:%d:%d",&hour,&minute,&second)==3)
					{
						value_=hour*3600+minute*60+second;
						return;
					}
				}
				synfig::warning("Time(): Bad time format");
				break;

			default:
				value_+=amount;
				synfig::warning("Time(): Unexpected character '%c' when parsing time string \"%s\"",str[pos],str.c_str());
				break;
		}
		pos++;
		amount=0;
	}
}

String
Time::get_string(float fps, Time::Format format)const
{
	Time time(*this);
	
	if(time<=begin())
		return "SOT";	// Start Of Time
	if(time>=end())
		return "EOT";	// End Of Time
	
	if(fps<0)fps=0;
	
	if(ceil(time.value_)-time.value_<epsilon_())
		time.value_=ceil(time.value_);
	
	int hour,minute;
	
	hour=time/3600;time-=hour*3600;
	minute=time/60;time-=minute*60;
	
	if(format<=FORMAT_VIDEO)
	{
		int second;
		second=time;time-=second;

		if(fps)
		{
			int frame;
			frame=round_to_int(time*fps);

			return strprintf("%02d:%02d:%02d.%02d",hour,minute,second,frame);
		}
		else
			return strprintf("%02d:%02d:%02d",hour,minute,second);
	}
	
	String ret;

	if(format<=FORMAT_FULL || hour)
		ret+=strprintf(format<=FORMAT_NOSPACES?"%dh":"%dh ",hour);
	
	if(format<=FORMAT_FULL || hour || minute)
		ret+=strprintf(format<=FORMAT_NOSPACES?"%dm":"%dm ",minute);
	
	if(fps)
	{
		int second;
		float frame;
		second=time;time-=second;
		frame=time*fps;
		if(format<=FORMAT_FULL || second)
			ret+=strprintf(format<=FORMAT_NOSPACES?"%ds":"%ds ",(int)second);
		
		if(abs(frame-floor(frame)>=epsilon_()))
			ret+=strprintf("%0.3ff",frame);
		else
			ret+=strprintf("%0.0ff",frame);
	}
	else
	{
		float second;
		second=time;
		if(abs(second-floor(second))>=epsilon_())
			ret+=strprintf("%0.8fs",second);
		else
			ret+=strprintf("%0.0fs",second);
	}
	
	return ret;
}

Time
Time::round(float fps)const
{
	assert(fps>0);

	value_type time(*this);

	time*=fps;

	if(abs(time-floor(time))<0.5)
		return floor(time)/fps;
	else
		return ceil(time)/fps;
}

//! \writeme
bool
Time::is_valid()const
{
	return !isnan(value_);
}
