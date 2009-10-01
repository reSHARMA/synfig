/* === S Y N F I G ========================================================= */
/*!	\file noise.cpp
**	\brief Implementation of the "Noise Gradient" layer
**
**	$Id$
**
**	\legal
**	Copyright (c) 2002-2005 Robert B. Quattlebaum Jr., Adrian Bentley
**	Copyright (c) 2007 Chris Moore
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

#include "noise.h"

#include <synfig/string.h>
#include <synfig/time.h>
#include <synfig/context.h>
#include <synfig/paramdesc.h>
#include <synfig/renddesc.h>
#include <synfig/surface.h>
#include <synfig/value.h>
#include <synfig/valuenode.h>

#endif

/* === M A C R O S ========================================================= */

using namespace synfig;
using namespace std;
using namespace etl;

/* === G L O B A L S ======================================================= */

SYNFIG_LAYER_INIT(Noise);
SYNFIG_LAYER_SET_NAME(Noise,"noise");
SYNFIG_LAYER_SET_LOCAL_NAME(Noise,N_("Noise Gradient"));
SYNFIG_LAYER_SET_CATEGORY(Noise,N_("Gradients"));
SYNFIG_LAYER_SET_VERSION(Noise,"0.0");
SYNFIG_LAYER_SET_CVS_ID(Noise,"$Id$");

/* === P R O C E D U R E S ================================================= */

/* === M E T H O D S ======================================================= */

Noise::Noise():
	size(1,1),
	gradient(Color::black(), Color::white())
{
	smooth=RandomNoise::SMOOTH_COSINE;
	detail=4;
	speed=0;
	do_alpha=false;
	random.set_seed(time(NULL));
	turbulent=false;
	displacement=Vector(1,1);
	do_displacement=false;
	super_sample=false;
}



inline Color
Noise::color_func(const Point &point, float pixel_size,Context /*context*/)const
{
	Color ret(0,0,0,0);

	float x(point[0]/size[0]*(1<<detail));
	float y(point[1]/size[1]*(1<<detail));
	float x2(0),y2(0);

	if(super_sample&&pixel_size)
	{
		x2=(point[0]+pixel_size)/size[0]*(1<<detail);
		y2=(point[1]+pixel_size)/size[1]*(1<<detail);
	}

	int i;
	Time time;
	time=speed*curr_time;
	RandomNoise::SmoothType smooth((!speed && Noise::smooth == RandomNoise::SMOOTH_SPLINE) ? RandomNoise::SMOOTH_FAST_SPLINE : Noise::smooth);

	float ftime(time);

	{
		float amount=0.0f;
		float amount2=0.0f;
		float amount3=0.0f;
		float alpha=0.0f;
		for(i=0;i<detail;i++)
		{
			amount=random(smooth,0+(detail-i)*5,x,y,ftime)+amount*0.5;
			if(amount<-1)amount=-1;if(amount>1)amount=1;

			if(super_sample&&pixel_size)
			{
				amount2=random(smooth,0+(detail-i)*5,x2,y,ftime)+amount2*0.5;
				if(amount2<-1)amount2=-1;if(amount2>1)amount2=1;

				amount3=random(smooth,0+(detail-i)*5,x,y2,ftime)+amount3*0.5;
				if(amount3<-1)amount3=-1;if(amount3>1)amount3=1;

				if(turbulent)
				{
					amount2=abs(amount2);
					amount3=abs(amount3);
				}

				x2*=0.5f;
				y2*=0.5f;
			}

			if(do_alpha)
			{
				alpha=random(smooth,3+(detail-i)*5,x,y,ftime)+alpha*0.5;
				if(alpha<-1)alpha=-1;if(alpha>1)alpha=1;
			}

			if(turbulent)
			{
				amount=abs(amount);
				alpha=abs(alpha);
			}

			x*=0.5f;
			y*=0.5f;
			//ftime*=0.5f;
		}

		if(!turbulent)
		{
			amount=amount/2.0f+0.5f;
			alpha=alpha/2.0f+0.5f;

			if(super_sample&&pixel_size)
			{
				amount2=amount2/2.0f+0.5f;
				amount3=amount3/2.0f+0.5f;
			}
		}

		if(super_sample && pixel_size)
			ret=gradient(amount,max(amount3,max(amount,amount2))-min(amount3,min(amount,amount2)));
		else
			ret=gradient(amount);

		if(do_alpha)
			ret.set_a(ret.get_a()*(alpha));
	}
	return ret;
}

inline float
Noise::calc_supersample(const synfig::Point &/*x*/, float /*pw*/,float /*ph*/)const
{
	return 0.0f;
}

void
Noise::set_time(synfig::Context context, synfig::Time t)const
{
	curr_time=t;
	context.set_time(t);
}

void
Noise::set_time(synfig::Context context, synfig::Time t, const synfig::Point &point)const
{
	curr_time=t;
	context.set_time(t,point);
}

synfig::Layer::Handle
Noise::hit_check(synfig::Context context, const synfig::Point &point)const
{
	if(get_blend_method()==Color::BLEND_STRAIGHT && get_amount()>=0.5)
		return const_cast<Noise*>(this);
	if(get_amount()==0.0)
		return context.hit_check(point);
	if(color_func(point,0,context).get_a()>0.5)
		return const_cast<Noise*>(this);
	return false;
}

bool
Noise::set_param(const String & param, const ValueBase &value)
{
	if(param=="seed" && value.same_type_as(int()))
	{
		random.set_seed(value.get(int()));
		return true;
	}
	IMPORT(size);
	IMPORT(speed);
	IMPORT(smooth);
	IMPORT(detail);
	IMPORT(do_alpha);
	IMPORT(gradient);
	IMPORT(turbulent);
	IMPORT(super_sample);

	return Layer_Composite::set_param(param,value);
}

ValueBase
Noise::get_param(const String & param)const
{
	if(param=="seed")
		return random.get_seed();
	EXPORT(size);
	EXPORT(speed);
	EXPORT(smooth);
	EXPORT(detail);
	EXPORT(do_alpha);
	EXPORT(gradient);
	EXPORT(turbulent)
	EXPORT(super_sample);

	EXPORT_NAME();
	EXPORT_VERSION();

	return Layer_Composite::get_param(param);
}

Layer::Vocab
Noise::get_param_vocab()const
{
	Layer::Vocab ret(Layer_Composite::get_param_vocab());

	ret.push_back(ParamDesc("gradient")
		.set_local_name(_("Gradient"))
	);
	ret.push_back(ParamDesc("seed")
		.set_local_name(_("RandomNoise Seed"))
	);
	ret.push_back(ParamDesc("size")
		.set_local_name(_("Size"))
	);
	ret.push_back(ParamDesc("smooth")
		.set_local_name(_("Interpolation"))
		.set_description(_("What type of interpolation to use"))
		.set_hint("enum")
		.add_enum_value(RandomNoise::SMOOTH_DEFAULT,	"nearest",	_("Nearest Neighbor"))
		.add_enum_value(RandomNoise::SMOOTH_LINEAR,	"linear",	_("Linear"))
		.add_enum_value(RandomNoise::SMOOTH_COSINE,	"cosine",	_("Cosine"))
		.add_enum_value(RandomNoise::SMOOTH_SPLINE,	"spline",	_("Spline"))
		.add_enum_value(RandomNoise::SMOOTH_CUBIC,	"cubic",	_("Cubic"))
	);
	ret.push_back(ParamDesc("detail")
		.set_local_name(_("Detail"))
	);
	ret.push_back(ParamDesc("speed")
		.set_local_name(_("Animation Speed"))
	);
	ret.push_back(ParamDesc("turbulent")
		.set_local_name(_("Turbulent"))
	);
	ret.push_back(ParamDesc("do_alpha")
		.set_local_name(_("Do Alpha"))
	);
	ret.push_back(ParamDesc("super_sample")
		.set_local_name(_("Super Sampling"))
	);

	return ret;
}

Color
Noise::get_color(Context context, const Point &point)const
{
	const Color color(color_func(point,0,context));

	if(get_amount()==1.0 && get_blend_method()==Color::BLEND_STRAIGHT)
		return color;
	else
		return Color::blend(color,context.get_color(point),get_amount(),get_blend_method());
}

bool
Noise::accelerated_render(Context context,Surface *surface,int quality, const RendDesc &renddesc, ProgressCallback *cb)const
{
	SuperCallback supercb(cb,0,9500,10000);

	if(get_amount()==1.0 && get_blend_method()==Color::BLEND_STRAIGHT)
	{
		surface->set_wh(renddesc.get_w(),renddesc.get_h());
	}
	else
	{
		if(!context.accelerated_render(surface,quality,renddesc,&supercb))
			return false;
		if(get_amount()==0)
			return true;
	}


	int x,y;

	Surface::pen pen(surface->begin());
	const Real pw(renddesc.get_pw()),ph(renddesc.get_ph());
	Point pos;
	Point tl(renddesc.get_tl());
	const int w(surface->get_w());
	const int h(surface->get_h());
	float supersampleradius((abs(pw)+abs(ph))*0.5f);
	if(quality>=8)
		supersampleradius=0;

	if(get_amount()==1.0 && get_blend_method()==Color::BLEND_STRAIGHT)
	{
		for(y=0,pos[1]=tl[1];y<h;y++,pen.inc_y(),pen.dec_x(x),pos[1]+=ph)
			for(x=0,pos[0]=tl[0];x<w;x++,pen.inc_x(),pos[0]+=pw)
				pen.put_value(color_func(pos,supersampleradius,context));
	}
	else
	{
		for(y=0,pos[1]=tl[1];y<h;y++,pen.inc_y(),pen.dec_x(x),pos[1]+=ph)
			for(x=0,pos[0]=tl[0];x<w;x++,pen.inc_x(),pos[0]+=pw)
				pen.put_value(Color::blend(color_func(pos,supersampleradius,context),pen.get_value(),get_amount(),get_blend_method()));
	}

	// Mark our progress as finished
	if(cb && !cb->amount_complete(10000,10000))
		return false;

	return true;
}