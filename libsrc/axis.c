/*
 *  Copyright (C) 2003-2006 SICOM Systems, INC.
 *
 *  Authors:  Michael Ibison <ibison@earthtech.org> 
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * $Id$
 * 
 * This module generates a report from the information stored in the current
 * report object.
 * The main entry point is called once at report generation time for each
 * report defined in the rlib object.
 *
 */
 

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "config.h"
#include "rlib-internal.h"

#define GOOD_CONTRAST_THRESHOLD 0.5
#define FALSE_ORGIN_CONTRAST_THRESHOLD 0.2
#define MAXIMUM_SIGNIFICANT_DIGITS 10

#define MAX_COLOR_POOL 4

/*static gint dynamic_range(gdouble a, gdouble b) {
	gdouble min, max, d;
	gint n;
	
	if(a == b)
		return 0;
	if(a > b) {
		min = b;
		max = a;
	} else {
		min = a;
		max = b;
	}

	d = fabs(max - min);
	n = floor(log10(d));
	return d* pow(10, 1-n);
}

#define SMALL .00000001

static gint nsd(gdouble x) {
	gint n;
	gdouble y,z;
	gdouble close;
	gint i;
	if(x == 0)
		return 0;
	n = floor(log10(fabs(x)));
	y = fabs(x) / pow(10, n);
	for(i=1;i<=MAXIMUM_SIGNIFICANT_DIGITS;i++) {
		z = trunc(y * pow(10, i)) * pow(10, -i);
		close = fabs((z-y)/y);
		if(close <= SMALL) {
			return i + 1;
		}
	}
	return 0;
}

static gint sd(gdouble x, gdouble m, gint ud) {
	gdouble n;
	n = floor(log10(fabs(x))) - m + 1;
	if(ud == -1)
		return trunc(x/pow(10, n));
	else
		return ceil(x/pow(10,n));
}

static gdouble nice_limit(gdouble z, gint ud) {
	gint t, abs_t, n;
	gint s;

	n = floor(log10(fabs(z))) - 1;
	t = sd(z, 2, ud);
	abs_t = abs(t);

	if(abs_t < 20) {
		if(abs_t == 13 || abs_t == 11 || abs_t == 17 || abs_t == 19)
			t = t + ud;
		return t * pow(10, n);
	}
		
	if(ud == 1) {
		s = ceil(t / 5.0);	
	} else {
		s = floor(t / 5.0);
	}

	if(s == 19)
		s = 20;
	return 5.0 * pow(10, n) * s;
}

static gdouble round_sd(gdouble z, gint m, gint ud) {
	gint n = floor(log10(fabs(z))) + 1 - m;
	gint t = sd(z, m, ud);
	return t * pow(10, n);
}

static gdouble change_limit(gdouble min, gdouble max, gint ud) {
	gdouble pdr = 1.0/15.0;
	gdouble t;
	gint sd;
	if(min == 0)
		return 0;
		
	sd = MAX(nsd(min), nsd(max));
	
	if(ud == 1) 
		t = max + (pdr * (max - min));
	else
		t = min - (pdr * (max - min));
		
	if(((min > 0 && t < 0) || (min == 0)) && ud == -1)
		return 0;
	return round_sd(t,sd,ud);
}

gint rlib_graph_num_ticks(rlib *r, gdouble a, gdouble b) {
	gdouble min, max;
	gint d = dynamic_range(a, b);
	gint sd;
	if(a > b) {
		min = b;
		max = a;	
	} else {
		min = a;
		max = b;
	}
	
	sd = nsd(max - min);
	
	if(sd <= 2) {	
		if(d == 1 || d == 2 || d == 5 || d == 20 || d == 50 || d == 100 || d == 10)
			return 10;
		else if(d == 4 || d == 8 || d == 40 || d == 80 || d == 16)
			return 8;
		else if(d == 3 || d == 6 || d == 30 || d == 60 || d == 12 || d == 24)
			return 12;
		else if(d == 7 || d == 70 || d == 35 || d == 14 || d == 21 || d == 28)
			return 7;
		else if(d == 18 || d == 90 || d == 45 || d == 9 || d == 27)
			return 9;
		else if(d == 25)
			return 5;
		else if(d == 11 || d == 55 || d == 22 || d == 33)
			return 11;
		else if(d == 13 || d == 65 || d == 26)
			return 13;
		else if(d == 15 || d == 75)
			return 15;
		else if(d == 85)
			return 17;
		else if(d == 95)
			return 19;		
		return d;
	}
	return 10;
}

void rlib_graph_find_y_range(rlib *r, gdouble a, gdouble b, gdouble *y_min, gdouble *y_max, gint graph_type) {
	gdouble contrast;
	gdouble min, max;

	if(a == b) {
		*y_min = *y_max = a;
		return;
	}
	if(a > b) {
		min = b;
		max = a;
	} else {
		min = a;
		max = b;	
	}

	if(graph_type == RLIB_GRAPH_TYPE_ROW_NORMAL || graph_type == RLIB_GRAPH_TYPE_ROW_STACKED || graph_type == RLIB_GRAPH_TYPE_ROW_PERCENT) {
		gdouble mint = change_limit(min, max, -1);		
		max = change_limit(min, max, 1);		
		min = mint;

	}
		
	contrast = fabs((max - min) / (max + min));

	if((max == 0) || (min < 0 && contrast > FALSE_ORGIN_CONTRAST_THRESHOLD)) {
		if(max <= 0)
			*y_max = 0;
		else
			*y_max = nice_limit(max, 1);
		*y_min = nice_limit(min, -1);
		return;
	}
	if((min == 0) || (max > 0 && contrast > FALSE_ORGIN_CONTRAST_THRESHOLD)) {
		if(min >= 0)
			*y_min = 0;
		else
			*y_min = nice_limit(min, -1);
		*y_max = nice_limit(max, 1);
		return;
	}
	*y_min = min;
	*y_max = max;
	return;
}	
*/





/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~ start of c code ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*

________________________________________________________________________________

author                  Michael Ibison
email                   ibison@earthtech.org
date of this version    January 5th, 2005
external entry point    adjustLimits(...)
________________________________________________________________________________


*/

#include "stdio.h"
#include "math.h"

#define Delta   0.0000001
#define LongStringLength 1024
#define MaxNumGoodIncs 100
#define ROUND(x) ((x)<0?(int)((x)-0.5+Delta):(int)((x)+0.5-Delta))
#define ROUNDUP(x) ((x)==((int)(x))?(x):((x)>0?(int)((x)+1):(int)(x)))
#define ROUNDDOWN(x) ((x)==((int)(x))?(x):((x)>0?(int)(x):(int)((x)-1)))
#define POWER10(n) ((n)==0?1:(pow((double)10.0,(double)(n))))
#define INTPOWER10(n) ROUND(POWER10(n))
#define MAPMAX(sd) (INTPOWER10(sd)-1)
#define HASFACTOR(x,y) ((x)==(y)*(int)((x)/(y)))


gint localError(const gchar* s) {
	fprintf(stderr, "%s\n",s);
	return -1;
}

gint imod(gint y, gint x, gint *quo, gint *rem) {
	if (x==0)   
		return (localError("imod: received zero denominator"),-1);

	*quo=(int)(((double)y)/(double)x);
	*rem=y - x*(*quo);
	return 0;
}

gint zeroRem(gint upDown, gint y, gint x, gint *quo, gint *newY) {
	if (x==0) 
		return (localError("zeroRem: received zero denominator"),-1);
	if (upDown!=-1 && upDown!=1) 
		return (localError("zeroRem: upDown must be +/-1"),-1);
	if (y==0) {
		*quo=0;
		*newY=y;
		return 0;
	} else {
		gint rem;
		imod(y,x,quo,&rem);
		if (rem==0) {
			*newY=y; /*leave quo as is*/
			return 0;
		} else {
			*newY=y+upDown*x-rem;
			(*quo)+=upDown;
			return 0;
		}
	}
}

gint mapData(gdouble dataMin, gdouble dataMax, gint sd, gint *mappedDataMin, gint *mappedDataMax, gint *raise) {
	gdouble  mx=MAX(fabs((gdouble)dataMin),fabs((gdouble)dataMax));
	*raise=sd-(gint)(log10(mx)+Delta)-1; /*only the integer part*/
	*mappedDataMin=ROUNDDOWN(dataMin*POWER10(*raise));
	*mappedDataMax=ROUNDUP(dataMax*POWER10(*raise));
	return 0;
}

gint isGoodInc(gint inc, gint sd, gint* goodIncs, gint num) {
	gint i,j;
	for(i=0;i<num;i++)
		for(j=0;j<sd;j++)
		if (inc==goodIncs[i]*((int)(pow((double)10,(double)j)+0.5))) 
			return 1;
	return 0;
}

gint adjustPosAndNegLimits(gint mappedMin, gint mappedMax, gint minTMs, gint maxTMs, gint sd, gint *goodIncs, gint numGoodIncs,
	gint* numTms, gint* tmi, gint* adjMin, gint* adjMax) {
	gint bestAdj=32767;
	gint inc;
	gint found=0;
	gint bestNewMin=0,bestNewMax=0,bestTm=0,bestInc=0;
	gint mapMax=MAPMAX(sd);

	if (MAX(abs(mappedMin),abs(mappedMax))>mapMax)
   	 return (localError("adjustPosAndNegLimits: some data is > supplied mapping upper limit"),-1);

//	for (inc=1;inc<=mapMax;inc++) {
	for (inc=1;inc<=maxTMs;inc++) {
		gint quo1,quo2;
		gint newMin,newMax;
		gint adj,tm;
		zeroRem(-1,mappedMin,inc,&quo1,&newMin);
		zeroRem(1,mappedMax,inc,&quo2,&newMax);
		adj=abs(mappedMin-newMin)+abs(mappedMax-newMax);/* total adjustment                         */
		tm=abs(quo1)+abs(quo2);      /* tick marks; convention here is to not include the first,
						                 (though really it is 1 more than this).                         */
		if (tm>=minTMs && tm<=maxTMs && adj<bestAdj && isGoodInc(inc,sd,goodIncs,numGoodIncs)) {
			bestAdj=adj;
			bestTm=tm;
			bestInc=inc;
			bestNewMin=newMin;
			bestNewMax=newMax;
			found=1;
		}
	}
	if (!found)
		return (localError("adjustPosAndNegLimits: found no acceptable divisor into the limits"),-1);
	*adjMin=bestNewMin;
	*adjMax=bestNewMax;
	*numTms=bestTm;
	*tmi=bestInc;
	return 0;
}

gint tryToZeroize(gdouble mn, gdouble mx, gdouble criticalRatio, gdouble *adjMn, gdouble *adjMx) {
	gdouble amn=MIN(fabs(mn),fabs(mx));
	gdouble amx=MAX(fabs(mn),fabs(mx));
	gdouble r=amn/amx;

	*adjMn=mn;
	*adjMx=mx;
	if (r<criticalRatio) {
		if (fabs(mn)<fabs(mx)) 
			*adjMn=0; 
		else 
			*adjMx=0;
	}
	return 0;
}

gint tryToZeroizeSaved(gint mn, gint mx, gdouble criticalRatio, gint *adjMn, gint *adjMx) {
	gdouble amn=MIN(abs(mn),abs(mx));
	gdouble amx=MAX(abs(mn),abs(mx));
	gdouble r=amn/amx;

	*adjMn=mn;
	*adjMx=mx;
	if (r<criticalRatio) {
		if (abs(mn)<abs(mx)) 
			*adjMn=0; 
		else 
			*adjMx=0;
	}
	return 0;
}

int adjust_limits(gdouble  dataMin, gdouble dataMax, gint denyMinEqualsAdjMin, gint minTMs, gint maxTMs, 
	gint* numTms, gdouble* tmi, gdouble* adjMin, gdouble* adjMax, gint *goodIncs, gint numGoodIncs) {

	int     sd = 2;
	int     maxPossibleGoodInc=MAPMAX(sd);
	double  zMin,zMax;      /* zeroized limits (after unmapped above) */
	double  szMin,szMax;    /* shifted zeroized limits */
	int     mszMin,mszMax;  /* mapped shifted zeroized limits */
	int     amszMin,amszMax;/* adjusted mapped shifted zeroized limits */
	double  aszMin,aszMax;  /* adjusted shifted zeroized limits */
	double  azMin,azMax;    /* adjusted zeroized limits */

	int     falseOrigin;    /* if true, we have decided on a false origin        */
	int     mTmi;           /* tick mark interval in mapped range               */
	int     raise;          /* power to which original limits must be raised
                           so as to map into range specified by sd          */
	int     i;              /* loop index */
	int     adjMaxTMs=(denyMinEqualsAdjMin?maxTMs-1:maxTMs);


/*
This is the requested maxTMs UNLESS denyMinEqualsAdjMin is true. In that case
it is possible that one of the limits will be shifted by an extra tickmark
interval after the call to adjustPosAndNegLimits. To account for this
possibility, need to decrement the maximum number of tickmarks available to
the adjstment algorithm in advance, so that the total is gauranteed to be
no more than the specified maximum.
*/

/* test to see parameters make sense */
	if (dataMin>=dataMax)
		return(localError("adjustLimits: bad data range"),-1);

	if (minTMs<=0 || maxTMs<=0)
		return(localError("adjustLimits: negative or zero tickmarks specified"),-1);

if (maxTMs==1 && denyMinEqualsAdjMin)
        return(localError("adjustLimits: cannot specify 1 tickmark AND require a margin"),-1);

if (sd<1 || sd >4)
        return(localError("adjustLimits: sd out of range [1,4]"),-1);


	if(denyMinEqualsAdjMin && dataMin < 0 && dataMax < 0)
		dataMax = 0;


/*check to see that the acceptable increments do not have more significant figures than allowed
 for the limits */
for (i=0;i<numGoodIncs;i++)
    {
    if (goodIncs[i]>maxPossibleGoodInc)
        return (localError("adjustLimits: supplied with a good increment that is too large given the number of significant digits"),-1);
    if (goodIncs[i]<=0)
        return (localError("adjustLimits: supplied with a good increment that is <=0"),-1);
    }

/* see if either limit can be set to zero */
tryToZeroize(dataMin,dataMax,1.0/(double)maxTMs,&zMin,&zMax);

/*
Shift the origin.
Note that shifting the origin may reduce the number of significant figures.
For example (1000,1003) will be shifted to (0,3).
Therefore need to shift the data BEFORE performing the map.
Hence get (0,30) in this case.
*/
if (zMin*zMax>0) /*both of same sign and non-zero */
    {
    if (zMax>0)
        {
        falseOrigin=1; /* positive false origin */
        szMax=zMax-zMin; /* remove offset */
        szMin=0;
        }
    else {
        falseOrigin=-1; /* negative false origin */
        szMin=zMin-zMax; /* remove offset (zMappedMax<0 here) */
        szMax=0;
        }
    }
else
    {
    /* If here, then either zMin =0, or zMax =0,
    or they are both non-zero and of same sign.
    In all these cases do not shift the origin */
    falseOrigin=0;
    szMax=zMax;
    szMin=zMin;
    }

/* map the data */
mapData(szMin,szMax,sd,&mszMin,&mszMax,&raise);

/* adjust limits */
	if(adjustPosAndNegLimits(mszMin,mszMax,minTMs,adjMaxTMs,sd,goodIncs,numGoodIncs,numTms, &mTmi,&amszMin,&amszMax) == -1) {
		*adjMin = dataMin;
		*adjMax = dataMax;
		*numTms = 10;
		return -1;
	}

/* unmap the data */
aszMin=((double)amszMin)*POWER10(-raise);
aszMax=((double)amszMax)*POWER10(-raise);
*tmi=((double)mTmi)*POWER10(-raise);

/* unshift the origin */
if (falseOrigin==1)
    {
    azMin=aszMin+zMin;
    azMax=aszMax+zMin;
    }
else if (falseOrigin==-1)
    {
    azMin=aszMin+zMax;
    azMax=aszMax+zMax;
    }
else
    {
    azMin=aszMin;
    azMax=aszMax;
    }

/* set outputs whilst performing margin adjustment */
if (denyMinEqualsAdjMin
&& falseOrigin==1
&& azMin-dataMin<*tmi)      /* requested margin is absent */
    {
    *adjMin=azMin-*tmi;     /* shift down by extra tickmark interval */
    if(azMin>= 0.0 && *adjMin < 0.0) /* do not allow min to go below zero if it was originally positive */
		*adjMin = 0.0;
    *adjMax=azMax;
    (*numTms)++;
    }
else if (denyMinEqualsAdjMin
&& falseOrigin==-1
&& azMax-dataMax<*tmi)     /* requested margin is absent */
    {
    *adjMin=azMin;        /* shift down by extra tickmark interval */
    *adjMax=azMax+*tmi;        /* shift up by extra tickmark interval */
    (*numTms)++;
    }
else
    {
    *adjMin=azMin;
    *adjMax=azMax;
    }

return 0;
}
