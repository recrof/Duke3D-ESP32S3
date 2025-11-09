//-------------------------------------------------------------------------
/*
Copyright (C) 1996, 2003 - 3D Realms Entertainment

This file is part of Duke Nukem 3D version 1.5 - Atomic Edition

Duke Nukem 3D is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

Original Source: 1996 - Todd Replogle
Prepared for public release: 03/21/2003 - Charlie Wiederhold, 3D Realms
*/
//-------------------------------------------------------------------------
#include "maincfg.h"
#include "duke3d.h"
#include "audiolib/fx_man.h"
#include "audiolib/music.h"
#include "fixedPoint_math.h"
#include "engine.h"
#include "keyboard.h"
#include "sounds.h"
#include "soundefs.h"
#include "audiolib/multivoc.h"
// PRIMITIVE


uint8_t  haltsoundhack;
short callsound(short sn,short whatsprite)
{
    short i;

    if(haltsoundhack)
    {
        haltsoundhack = 0;
        return -1;
    }

    i = headspritesect[sn];
    while(i >= 0)
    {
        if( PN == MUSICANDSFX && SLT < 1000 )
        {
            if(whatsprite == -1) whatsprite = i;

            if(getHittypeTemp(i, 0) == 0)
            {
                if( (soundm[SLT]&16) == 0)
                {
                    if(SLT)
                    {
                        spritesound(SLT,whatsprite);
                        if(SHT && SLT != SHT && SHT < NUM_SOUNDS)
                            stopsound(SHT);
                    }

                    if( (getSector(SECT)->lotag&0xff) != 22)
                        setHittypeTemp(i, 0, 1);
                }
            }
            else if(SHT < NUM_SOUNDS)
            {
                if(SHT) spritesound(SHT,whatsprite);
                if( (soundm[SLT]&1) || ( SHT && SHT != SLT ) )
                    stopsound(SLT);
                setHittypeTemp(i, 0, 0);
            }
            return SLT;
        }
        i = spr_add_data[i].nextspritesect;
    }
    return -1;
}


short check_activator_motion( short lotag )
{
    short i, j;
//    spritetype *s;

    i = headspritestat[8];
    while ( i >= 0 )
    {
        if ( getSprite(i)->_lotag == lotag )
        {
           // s = getSprite(i);

            for ( j = animatecnt-1; j >= 0; j-- )
                if ( getSprite(i)->_sectnum == animatesect[j] )
                    return( 1 );

            j = headspritestat[3];
            while ( j >= 0 )
            {
                if(getSprite(i)->_sectnum == getSprite(j)->_sectnum)
                    switch(getSprite(j)->_lotag)
                    {
                        case 11:
                        case 30:
                            if ( getHittypeTemp(j, 4) )
                                return( 1 );
                            break;
                        case 20:
                        case 31:
                        case 32:
                        case 18:
                            if ( getHittypeTemp(j, 0) )
                                return( 1 );
                            break;
                    }

                j = spr_add_data[j].nextspritestat;
            }
        }
        i = spr_add_data[i].nextspritestat;
    }
    return( 0 );
}

uint8_t  isadoorwall(short dapic)
{
    switch(dapic)
    {
        case DOORTILE1:
        case DOORTILE2:
        case DOORTILE3:
        case DOORTILE4:
        case DOORTILE5:
        case DOORTILE6:
        case DOORTILE7:
        case DOORTILE8:
        case DOORTILE9:
        case DOORTILE10:
        case DOORTILE11:
        case DOORTILE12:
        case DOORTILE14:
        case DOORTILE15:
        case DOORTILE16:
        case DOORTILE17:
        case DOORTILE18:
        case DOORTILE19:
        case DOORTILE20:
        case DOORTILE21:
        case DOORTILE22:
        case DOORTILE23:
            return 1;
    }
    return 0;
}


uint8_t  isanunderoperator(short lotag)
{
    switch(lotag&0xff)
    {
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 22:
        case 26:
            return 1;
    }
    return 0;
}

uint8_t  isanearoperator(short lotag)
{
    switch(lotag&0xff)
    {
        case 9:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 25:
        case 26:
        case 29://Toothed door
            return 1;
    }
    return 0;
}

short checkcursectnums(short sect)
{
    short i;
    for(i=connecthead;i>=0;i=connectpoint2[i])
        if( getSprite(ps[i].i)->_sectnum == sect ) return i;
    return -1;
}

int32_t ldist(const spritetype *s1, const spritetype *s2)
{
    int32_t vx,vy;
    vx = s1->_x - s2->_x;
    vy = s1->_y - s2->_y;
    return(FindDistance2D(vx,vy) + 1);
}
int32_t ldist_sprite_tsprite(const spritetype *s1, const tspritetype *ts2)
{
    int32_t vx,vy;
    vx = s1->_x - ts2->x;
    vy = s1->_y - ts2->y;
    return(FindDistance2D(vx,vy) + 1);
}
// Declaration here just to shut down compiler warning:
// The linker was able to find it :/ regardless !
int FindDistance3D(int ix, int iy, int iz);
int32_t dist(const spritetype *s1, const spritetype *s2)
{
    int32_t vx,vy,vz;
    vx = s1->_x - s2->_x;
    vy = s1->_y - s2->_y;
    vz = s1->_z - s2->_z;
    return(FindDistance3D(vx,vy,vz>>4));
}

short findplayer(const spritetype *s,int32_t *d)
{
    short j, closest_player;
    int32_t x, closest;

    if(ud.multimode < 2)
    {
        *d = klabs(ps[myconnectindex].oposx-s->_x) + klabs(ps[myconnectindex].oposy-s->_y) + ((klabs(ps[myconnectindex].oposz-s->_z+(28<<8)))>>4);
        return myconnectindex;
    }

    closest = 0x7fffffff;
    closest_player = 0;

    for(j=connecthead;j>=0;j=connectpoint2[j])
    {
        x = klabs(ps[j].oposx-s->_x) + klabs(ps[j].oposy-s->_y) + ((klabs(ps[j].oposz-s->_z+(28<<8)))>>4);
        if( x < closest && getSprite(ps[j].i)->_extra > 0 )
        {
            closest_player = j;
            closest = x;
        }
    }

    *d = closest;
    return closest_player;
}

short findotherplayer(short p,int32_t *d)
{
    short j, closest_player;
    int32_t x, closest;

    closest = 0x7fffffff;
    closest_player = p;

    for(j=connecthead;j>=0;j=connectpoint2[j])
        if(p != j && getSprite(ps[j].i)->_extra > 0)
    {
        x = klabs(ps[j].oposx-ps[p].posx) + klabs(ps[j].oposy-ps[p].posy) + (klabs(ps[j].oposz-ps[p].posz)>>4);

        if( x < closest )
        {
            closest_player = j;
            closest = x;
        }
    }

    *d = closest;
    return closest_player;
}



void doanimations(void)
{
	int32_t i, j, a, p, v, dasect;

	for(i=animatecnt-1;i>=0;i--)
	{
#if INTERPOLATION_USES_POSPTR
		a = *animateptr[i];
#else
        a = getIndexedValue(i, animateidx);
#endif
		v = animatevel[i]*TICSPERFRAME;
		dasect = animatesect[i];

        if (a == animategoal[i])
        {
#if INTERPOLATION_USES_POSPTR
            stopinterpolation(animateptr[i]);
#else
            stopinterpolation(animateidx[i]);
#endif
            animatecnt--;
#if INTERPOLATION_USES_POSPTR
            animateptr[i] = animateptr[animatecnt];
#else
            animateidx[i].value = animateidx[animatecnt].value;

#endif
            animategoal[i] = animategoal[animatecnt];
            animatevel[i] = animatevel[animatecnt];
            animatesect[i] = animatesect[animatecnt];
            if( getSector(animatesect[i])->lotag == 18 || getSector(animatesect[i])->lotag == 19 )
            {
#if INTERPOLATION_USES_POSPTR
                if(animateptr[i] == getSector_ceilingzptr(animatesect[i]))
                    continue;
#else
                if(animateidx[i].value == getSector_ceilingzidx(animatesect[i]).value)
                    continue;
#endif
            }
           if( (getSector(dasect)->lotag&0xff) != 22 )
                callsound(dasect,-1);

            continue;
        }

        if (v > 0) { a = min(a+v,animategoal[i]); }
              else { a = max(a+v,animategoal[i]); }
#if INTERPOLATION_USES_POSPTR
        if( animateptr[i] == getSector_floorzptr(animatesect[i]))
#else
        if( animateidx[i].value == getSector_floorzidx(animatesect[i]).value)
#endif
        {
            for(p=connecthead;p>=0;p=connectpoint2[p])
                if (ps[p].cursectnum == dasect)
                    if ((getSector(dasect)->floorz-ps[p].posz) < (64<<8))
                        if (getSprite(ps[p].i)->_owner >= 0)
            {
                ps[p].posz += v;
                ps[p].poszv = 0;
                if (p == myconnectindex)
                {
                    myz += v;
                    myzvel = 0;
                    myzbak[((movefifoplc-1)&(MOVEFIFOSIZ-1))] = ps[p].posz;
                }
            }

            for(j=headspritesect[dasect];j>=0;j=spr_add_data[j].nextspritesect)
                if (getSprite(j)->_statnum != 3)
                {
                    setHittype_bposz(j, getSprite(j)->_z);
                    setSprite_z(j, getSprite(j)->_z + v);
                    setHittype_floorz(j, getSector(dasect)->floorz+v);
                }
        }
#if INTERPOLATION_USES_POSPTR
		*animateptr[i] = a;
#else
		setIndexedValue(i, animateidx, a);
#endif
	}
}
#if INTERPOLATION_USES_POSPTR
int32_t getanimationgoal(int32_t *animptr)
{
	int32_t i, j;

	j = -1;
    for(i=animatecnt-1;i>=0;i--)
        if (animptr == animateptr[i])
		{
			j = i;
			break;
		}
	return(j);
}
int32_t setanimation(short animsect,int32_t *animptr, int32_t thegoal, int32_t thevel)
{
	int32_t i, j;

	if (animatecnt >= MAXANIMATES-1)
		return(-1);

	j = animatecnt;
    for(i=0;i<animatecnt;i++)
    {
		if (animptr == animateptr[i])
		{
			j = i;
			break;
		}
    }

    animatesect[j] = animsect;
	animateptr[j] = animptr;
	animategoal[j] = thegoal;
    if (thegoal >= *animptr)
       animatevel[j] = thevel;
    else
       animatevel[j] = -thevel;

    if (j == animatecnt) animatecnt++;

    setinterpolation(animptr);

    return(j);
}

#else
int32_t getanimationgoal(indexedValue_t animidx)
{
	int32_t i, j;

	j = -1;
    for(i=animatecnt-1;i>=0;i--)
    {
        if (animidx.value == animateidx[i].value)
		{
			j = i;
			break;
		}
    }
	return(j);
}
int32_t setanimation(short animsect, indexedValue_t animidx, int32_t thegoal, int32_t thevel)
{
	int32_t i, j;

	if (animatecnt >= MAXANIMATES-1)
		return(-1);

	j = animatecnt;
    for(i=0;i<animatecnt;i++)
		if (animidx.value == animateidx[i].value)
		{
			j = i;
			break;
		}

    animatesect[j] = animsect;
	animateidx[j].value = animidx.value;
	animategoal[j] = thegoal;
	#if WIN32
	    static int maxvel;
	    if (thevel < 0)
	        FIXME("Negative level %d", thevel);
        if (maxvel < thevel)
        {
            maxvel = thevel;
//            FIXME(">>>>>>>>>>>>>>>>>>>>>>>>MAX VEL: %d\r\n", thevel);
        }
	#endif
    if (thegoal >= getIndexedValueFromIndexedValue(animidx))
       animatevel[j] = thevel;
    else
       animatevel[j] = -thevel;

    if (j == animatecnt) animatecnt++;
    setinterpolation(animidx);

    return(j);
}

#endif


static uint8_t m_useRenderedCamData = 0;
int useRenderedCamData(void)
{
    return m_useRenderedCamData;
}
void animatecamsprite(void)
{
    short i;
    if(camsprite <= 0)
    {
        m_useRenderedCamData = 0;
        return;
    }

    i = camsprite;

    if(getHittypeTemp(i, 0) >= 11 || isFullScreenUpdate())  // we don't render every frame. that would be too intensive. Also, if we had to refresh the full status bar, then we will recalculate anyway.
    {
        setHittypeTemp(i, 0, 0);

        if(ps[screenpeek].newowner >= 0)
        {
            setSprite_owner(i, ps[screenpeek].newowner); //OW = ps[screenpeek].newowner; //bleah
            m_useRenderedCamData = 0;
        }
        else if(OW >= 0 && dist(getSprite(ps[screenpeek].i),getSprite(i)) < 2048)
        {
            xyzmirror(OW,PN);
            m_useRenderedCamData = 1;
    }
    else
        {
            m_useRenderedCamData = 0;
        }
    }
    else
    {
        setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1);

    }
}

void animatewalls(void)
{
    int32_t i, j, p, t;

    for(p=0;p < numanimwalls ;p++)
//    for(p=numanimwalls-1;p>=0;p--)
    {
        i = animwall[p].wallnum;
        j = getWall_picnum(i);

            switch(j)
            {
                case SCREENBREAK1:
                case SCREENBREAK2:
                case SCREENBREAK3:
                case SCREENBREAK4:
                case SCREENBREAK5:

                case SCREENBREAK9:
                case SCREENBREAK10:
                case SCREENBREAK11:
                case SCREENBREAK12:
                case SCREENBREAK13:
                case SCREENBREAK14:
                case SCREENBREAK15:
                case SCREENBREAK16:
                case SCREENBREAK17:
                case SCREENBREAK18:
                case SCREENBREAK19:

                    if( (TRAND&255) < 16)
                    {
                        animwall[p].tag = getWall_picnum(i);
                        setWall_picnum(i, SCREENBREAK6);
                    }

                    continue;

                case SCREENBREAK6:
                case SCREENBREAK7:
                case SCREENBREAK8:

                    if(animwall[p].tag >= 0 && getWall_extra(i) != FEMPIC2 && getWall_extra(i) != FEMPIC3 )
                    {
                        setWall_picnum(i, animwall[p].tag);
                    }
                    else
                    {
                        setWall_picnum(i, getWall_picnum(i) + 1);//wall[i].picnum++;
                        if(getWall_picnum(i) == (SCREENBREAK6+3) )
                            setWall_picnum(i, SCREENBREAK6);
                    }
                    continue;

            }

        if(getWall_cstat(i)&16)
            switch(getWall_overpicnum(i))
        {
            case W_FORCEFIELD:
            case W_FORCEFIELD+1:
            case W_FORCEFIELD+2:

                t = animwall[p].tag;

                if(getWall_cstat(i)&254)
                {
#if NO_MINIMIZE_WALLS
                    wall[i].xpanning -= t>>10; // sintable[(t+512)&2047]>>12;
                    wall[i].ypanning -= t>>10; // sintable[t&2047]>>12;
#else
                    setWall_xpanning(i, getWall_xpanning(i) - (t>>10)); // sintable[(t+512)&2047]>>12;
                    setWall_ypanning(i, getWall_ypanning(i) - (t>>10)); // sintable[t&2047]>>12;

#endif
                    if(getWall_extra(i) == 1)
                    {
                        setWall_extra(i, 0);
                        animwall[p].tag = 0;
                    }
                    else
                        animwall[p].tag+=128;

                    if( animwall[p].tag < (128<<4) )
                    {
                        if( animwall[p].tag&128 )
                            setWall_overpicnum(i, W_FORCEFIELD);
                        else
                            setWall_overpicnum(i,W_FORCEFIELD+1);
                    }
                    else
                    {
                        if( (TRAND&255) < 32 )
                            animwall[p].tag = 128<<(TRAND&3);
                        else
                            setWall_overpicnum(i, W_FORCEFIELD+1);
                    }
                }

                break;
        }
    }
}

uint8_t  activatewarpelevators(short s,short d) //Parm = sectoreffectornum
{
    short i = 0, sn;

    sn = getSprite(s)->_sectnum;

    // See if the sector exists

    i = headspritestat[3];
    while(i >= 0)
    {
        if( SLT == 17 )
            if( SHT == getSprite(s)->_hitag )
                if( (klabs(getSector(sn)->floorz-getHittypeTemp(s, 2)) > SP) ||
                    (getSector(SECT)->hitag == (getSector(sn)->hitag-d) ) )
                        break;
        i = spr_add_data[i].nextspritestat;
    }

    if(i==-1)
    {
        d = 0;
        return 1; // No find
    }
    else
    {
        if(d == 0)
            spritesound(ELEVATOR_OFF,s);
        else spritesound(ELEVATOR_ON,s);
    }


    i = headspritestat[3];
    while(i >= 0)
    {
        if( SLT == 17 )
            if( SHT == getSprite(s)->_hitag )
            {
                setHittypeTemp(i, 0, d);    // d is 1 or -1 or 0 ?
                setHittypeTemp(i, 1, d); //Make all check warp
            }
        i = spr_add_data[i].nextspritestat;
    }
    return 0;
}



void operatesectors(short sn,short ii)
{
    int32_t j=0, l, q, startwall, endwall;
    short i = 0;
//    uint8_t  sect_error;
    //const sectortype *getSector(sn);

//    sect_error = 0;
   // getSector(sn) = getSector(sn);

    switch(getSector(sn)->lotag&(0xffff-49152))
    {

        case 30:
            j = getSector(sn)->hitag;
            if( getHittype_tempang(j) == 0 ||
                getHittype_tempang(j) == 256)
                    callsound(sn,ii);
            if(getSprite(j)->_extra == 1)
                setSprite_extra(j, 3);
            else
                setSprite_extra(j, 1);
            break;

        case 31:

            j = getSector(sn)->hitag;
            if(getHittypeTemp(j, 4) == 0)
                setHittypeTemp(j, 4, 1);

            callsound(sn,ii);
            break;

        case 26: //The split doors
#if INTERPOLATION_USES_POSPTR
            i = getanimationgoal(&getSector(sn)->ceilingz); // next-hack: that might be a bug we made, fixed below
#else
             i = getanimationgoal(getSector_ceilingzidx_from_ptr(getSector(sn)));
#endif
            if(i == -1) //if the door has stopped
            {
                haltsoundhack = 1;
                setSector_lotag(sn, getSector(sn)->lotag & 0xff00);
                setSector_lotag(sn, getSector(sn)->lotag | 22);
                operatesectors(sn,ii);
                setSector_lotag(sn, getSector(sn)->lotag & 0xff00);
                setSector_lotag(sn, getSector(sn)->lotag | 9);
                operatesectors(sn,ii);
                setSector_lotag(sn, getSector(sn)->lotag & 0xff00);
                setSector_lotag(sn, getSector(sn)->lotag | 26);
            }
            return;

        case 9:
        {
            int32_t dax,day,dax2,day2,sp;
            int32_t wallfind[2];

            startwall = constsector[sn].wallptr;
            endwall = startwall+constsector[sn].wallnum-1;

            sp = getSector_extra(sn)>>4;

            //first find center point by averaging all points
            dax = 0L, day = 0L;
            for(i=startwall;i<=endwall;i++)
            {
                dax += getWall(i)->x;
                day += getWall(i)->y;
            }
            dax /= (endwall-startwall+1);
            day /= (endwall-startwall+1);

            //find any points with either same x or same y coordinate
            //  as center (dax, day) - should be 2 points found.
            wallfind[0] = -1;
            wallfind[1] = -1;
            for(i=startwall;i<=endwall;i++)
                if ((getWall(i)->x == dax) || (getWall(i)->y == day))
                {
                    if (wallfind[0] == -1)
                        wallfind[0] = i;
                    else wallfind[1] = i;
                }

            for(j=0;j<2;j++)
            {
                if ((getWall(wallfind[j])->x == dax) && (getWall(wallfind[j])->y == day))
                {
                    //find what direction door should open by averaging the
                    //  2 neighboring points of wallfind[0] & wallfind[1].
                    i = wallfind[j]-1; if (i < startwall) i = endwall;
                    dax2 = ((getWall(i)->x+getWall(getWall_point2(wallfind[j]))->x)>>1)-getWall(wallfind[j])->x;
                    day2 = ((getWall(i)->y+getWall(getWall_point2(wallfind[j]))->y)>>1)-getWall(wallfind[j])->y;
                    if (dax2 != 0)
                    {
                        dax2 = getWall(getWall_point2(getWall_point2(wallfind[j])))->x;
                        dax2 -= getWall(getWall_point2(wallfind[j]))->x;
#if INTERPOLATION_USES_POSPTR
                        setanimation(sn,getWall_xptr(wallfind[j]),getWall(wallfind[j])->x+dax2,sp);
                        setanimation(sn,getWall_xptr(i),getWall(i)->x+dax2,sp);
                        setanimation(sn,getWall_xptr(getWall_point2(wallfind[j])),getWall(getWall_point2(wallfind[j]))->x+dax2,sp);
#else
                        setanimation(sn,getWall_xidx(wallfind[j]),getWall(wallfind[j])->x+dax2,sp);
                        setanimation(sn,getWall_xidx(i),getWall(i)->x+dax2,sp);
                        setanimation(sn,getWall_xidx(getWall_point2(wallfind[j])),getWall(getWall_point2(wallfind[j]))->x+dax2,sp);
#endif
                        callsound(sn,ii);
                    }
                    else if (day2 != 0)
                    {
                        day2 = getWall(getWall_point2(getWall_point2(wallfind[j])))->y;
                        day2 -= getWall(getWall_point2(wallfind[j]))->y;
#if INTERPOLATION_USES_POSPTR
                        setanimation(sn,getWall_yptr(wallfind[j]),getWall(wallfind[j])->y+day2,sp);
                        setanimation(sn,getWall_yptr(i),getWall(i)->y+day2,sp);
                        setanimation(sn,getWall_yptr(getWall_point2(wallfind[j])),getWall(getWall_point2(wallfind[j]))->y+day2,sp);
#else
                        setanimation(sn,getWall_yidx(wallfind[j]),getWall(wallfind[j])->y+day2,sp);
                        setanimation(sn,getWall_yidx(i),getWall(i)->y+day2,sp);
                        setanimation(sn,getWall_yidx(getWall_point2(wallfind[j])),getWall(getWall_point2(wallfind[j]))->y+day2,sp);
#endif
                        callsound(sn,ii);
                    }
                }
                else
                {
                    i = wallfind[j]-1; if (i < startwall) i = endwall;
                    dax2 = ((getWall(i)->x+getWall(getWall_point2(wallfind[j]))->x)>>1)-getWall(wallfind[j])->x;
                    day2 = ((getWall(i)->y+getWall(getWall_point2(wallfind[j]))->y)>>1)-getWall(wallfind[j])->y;
                    if (dax2 != 0)
                    {
#if INTERPOLATION_USES_POSPTR
                        setanimation(sn,getWall_xptr(wallfind[j]),dax,sp);
                        setanimation(sn,getWall_xptr(i),dax+dax2,sp);
                        setanimation(sn,getWall_xptr(getWall_point2(wallfind[j])),dax+dax2,sp);
#else
                        setanimation(sn,getWall_xidx(wallfind[j]),dax,sp);
                        setanimation(sn,getWall_xidx(i),dax+dax2,sp);
                        setanimation(sn,getWall_xidx(getWall_point2(wallfind[j])),dax+dax2,sp);

#endif
                        callsound(sn,ii);
                    }
                    else if (day2 != 0)
                    {
#if INTERPOLATION_USES_POSPTR
                        setanimation(sn,getWall_yptr(wallfind[j]),day,sp);
                        setanimation(sn,getWall_yptr(i),day+day2,sp);
                        setanimation(sn,getWall_yptr(getWall_point2(wallfind[j])),day+day2,sp);
#else
                        setanimation(sn,getWall_yidx(wallfind[j]),day,sp);
                        setanimation(sn,getWall_yidx(i),day+day2,sp);
                        setanimation(sn,getWall_yidx(getWall_point2(wallfind[j])),day+day2,sp);
#endif
                        callsound(sn,ii);
                    }
                }
            }

        }
        return;

        case 15://Warping elevators

            if(getSprite(ii)->_picnum != APLAYER) return;
//            if(ps[getSprite(ii)->_yvel].select_dir == 1) return;

            i = headspritesect[sn];
            while(i >= 0)
            {
                if(PN==SECTOREFFECTOR && SLT == 17 ) break;
                i = spr_add_data[i].nextspritesect;
            }

            if(getSprite(ii)->_sectnum == sn)
            {
                if( activatewarpelevators(i,-1) )
                    activatewarpelevators(i,1);
                else if( activatewarpelevators(i,1) )
                    activatewarpelevators(i,-1);
                return;
            }
            else
            {
                if(getSector(sn)->floorz > SZ)
                    activatewarpelevators(i,-1);
                else
                    activatewarpelevators(i,1);
            }

            return;

        case 16:
        case 17:
#if INTERPOLATION_USES_POSPTR
            i = getanimationgoal(&getSector(sn)->floorz); // possible bug we have introduced here, but fixed below
#else
            i = getanimationgoal(getSector_floorzidx_from_ptr(getSector(sn)));
#endif
            if(i == -1)
            {
                i = nextsectorneighborz(sn,getSector(sn)->floorz,1,1);
                if( i == -1 )
                {
                    i = nextsectorneighborz(sn,getSector(sn)->floorz,1,-1);
                    if( i == -1 ) return;
                    j = getSector(i)->floorz;
#if INTERPOLATION_USES_POSPTR
                    setanimation(sn,&getSector(sn)->floorz,j,getSector_extra(sn));
#else
                    setanimation(sn,getSector_floorzidx_from_ptr(getSector(sn)),j,getSector_extra(sn));
#endif
                }
                else
                {
                    j = getSector(i)->floorz;
#if INTERPOLATION_USES_POSPTR
                    setanimation(sn,&getSector(sn)->floorz,j,getSector_extra(sn));
#else
                    setanimation(sn,getSector_floorzidx_from_ptr(getSector(sn)),j,getSector_extra(sn));
#endif
                }
                callsound(sn,ii);
            }

            return;

        case 18:
        case 19:
#if INTERPOLATION_USES_POSPTR
            i = getanimationgoal(&getSector(sn)->floorz);
#else
            i = getanimationgoal(getSector_floorzidx_from_ptr(getSector(sn)));

#endif
            if(i==-1)
            {
                i = nextsectorneighborz(sn,getSector(sn)->floorz,1,-1);
                if(i==-1) i = nextsectorneighborz(sn,getSector(sn)->floorz,1,1);
                if(i==-1) return;
                j = getSector(i)->floorz;
                q = getSector_extra(sn);
                l = getSector(sn)->ceilingz-getSector(sn)->floorz;
#if INTERPOLATION_USES_POSPTR
                setanimation(sn,&getSector(sn)->floorz,j,q);
                setanimation(sn,&getSector(sn)->ceilingz,j+l,q);
#else
                setanimation(sn,getSector_floorzidx_from_ptr(getSector(sn)),j,q);
                setanimation(sn,getSector_ceilingzidx_from_ptr(getSector(sn)),j+l,q);
#endif
                callsound(sn,ii);
            }
            return;

        case 29:

            if(getSector(sn)->lotag&0x8000)
                j = getSector(nextsectorneighborz(sn,getSector(sn)->ceilingz,1,1))->floorz;
            else
                j = getSector(nextsectorneighborz(sn,getSector(sn)->ceilingz,-1,-1))->ceilingz;

            i = headspritestat[3]; //Effectors
            while(i >= 0)
            {
                if( (SLT == 22) &&
                    (SHT == getSector(sn)->hitag) )
                {
                    setSector_extra(SECT, -getSector_extra(SECT));

                    setHittypeTemp(i, 0, sn); // max: MAXSECTORS
                    setHittypeTemp(i, 1, 1);
                }
                i = spr_add_data[i].nextspritestat;
            }

            setSector_lotag(sn, getSector(sn)->lotag ^ 0x8000);
#if INTERPOLATION_USES_POSPTR
            setanimation(sn,&getSector(sn)->ceilingz,j,getSector_extra(sn));
#else

            setanimation(sn,getSector_ceilingzidx_from_ptr(getSector(sn)),j,getSector_extra(sn));
#endif
            callsound(sn,ii);

            return;

        case 20:

            REDODOOR:

            if(getSector(sn)->lotag&0x8000)
            {
                i = headspritesect[sn];
                while(i >= 0)
                {
                    if(getSprite(i)->_statnum == 3 && SLT==9)
                    {
                        j = SZ;
                        break;
                    }
                    i = spr_add_data[i].nextspritesect;
                }
                if(i==-1)
                    j = getSector(sn)->floorz;
            }
            else
            {
                j = nextsectorneighborz(sn,getSector(sn)->ceilingz,-1,-1);

                if(j >= 0) j = getSector(j)->ceilingz;
                else
                {
                    setSector_lotag(sn, getSector(sn)->lotag | 32768);
                    goto REDODOOR;
                }
            }

            setSector_lotag(sn, getSector(sn)->lotag ^ 0x8000);
#if INTERPOLATION_USES_POSPTR
            setanimation(sn,&getSector(sn)->ceilingz,j,getSector_extra(sn));
#else
            setanimation(sn,getSector_ceilingzidx_from_ptr(getSector(sn)),j,getSector_extra(sn));
#endif
            callsound(sn,ii);

            return;

        case 21:
#if INTERPOLATION_USES_POSPTR
            i = getanimationgoal(&getSector(sn)->floorz);
#else
            i = getanimationgoal(getSector_floorzidx_from_ptr(getSector(sn)));

#endif
            if (i >= 0)
            {
                if (animategoal[sn] == getSector(sn)->ceilingz)
                    animategoal[i] = getSector(nextsectorneighborz(sn,getSector(sn)->ceilingz,1,1))->floorz;
                else animategoal[i] = getSector(sn)->ceilingz;
                j = animategoal[i];
            }
            else
            {
                if (getSector(sn)->ceilingz == getSector(sn)->floorz)
                    j = getSector(nextsectorneighborz(sn,getSector(sn)->ceilingz,1,1))->floorz;
                else j = getSector(sn)->ceilingz;

                setSector_lotag(sn, getSector(sn)->lotag ^ 0x8000);
#if INTERPOLATION_USES_POSPTR
                if(setanimation(sn,&getSector(sn)->floorz,j,getSector_extra(sn)) >= 0)
#else
                if(setanimation(sn,getSector_floorzidx_from_ptr(getSector(sn)),j,getSector_extra(sn)) >= 0)
#endif
                    callsound(sn,ii);
            }
            return;

        case 22:

            // REDODOOR22:

            if ( (getSector(sn)->lotag&0x8000) )
            {
                q = (getSector(sn)->ceilingz+getSector(sn)->floorz)>>1;
#if INTERPOLATION_USES_POSPTR
                j = setanimation(sn,&getSector(sn)->floorz,q,getSector_extra(sn));
                j = setanimation(sn,&getSector(sn)->ceilingz,q,getSector_extra(sn));
#else
                j = setanimation(sn,getSector_floorzidx_from_ptr(getSector(sn)),q,getSector_extra(sn));
                j = setanimation(sn,getSector_ceilingzidx_from_ptr(getSector(sn)),q,getSector_extra(sn));

#endif
            }
            else
            {
                q = getSector(nextsectorneighborz(sn,getSector(sn)->floorz,1,1))->floorz;
#if INTERPOLATION_USES_POSPTR
                j = setanimation(sn,&getSector(sn)->floorz,q,getSector_extra(sn));
                q = getSector(nextsectorneighborz(sn,getSector(sn)->ceilingz,-1,-1))->ceilingz;
                j = setanimation(sn,&getSector(sn)->ceilingz,q,getSector_extra(sn));
#else
                j = setanimation(sn,getSector_floorzidx_from_ptr(getSector(sn)),q,getSector_extra(sn));
                q = getSector(nextsectorneighborz(sn,getSector(sn)->ceilingz,-1,-1))->ceilingz;
                j = setanimation(sn,getSector_ceilingzidx_from_ptr(getSector(sn)),q,getSector_extra(sn));

#endif
            }

            setSector_lotag(sn, getSector(sn)->lotag ^ 0x8000);

            callsound(sn,ii);

            return;

        case 23: //Swingdoor

            j = -1;
            q = 0;

            i = headspritestat[3];
            while(i >= 0)
            {
                if( SLT == 11 && SECT == sn && !getHittypeTemp(i, 4))
                {
                    j = i;
                    break;
                }
                i = spr_add_data[i].nextspritestat;
            }

            //Why would this ever be -1?
			if(i < 0)
			{
				return;
			}//FIXME: CRASH HERE (the "l = getSector(SECT)->lotag&0x8000; " code)

            l = getSector(SECT)->lotag&0x8000;

            if(j >= 0)
            {
                i = headspritestat[3];
                while(i >= 0)
                {
                    if( l == (getSector(SECT)->lotag&0x8000) && SLT == 11 && getSprite(j)->_hitag == SHT && !getHittypeTemp(i, 4) )
                    {
                        if(getSector(SECT)->lotag&0x8000)
                            setSector_lotag(SECT, getSector(SECT)->lotag & 0x7fff);
                        else
                            setSector_lotag(SECT, getSector(SECT)->lotag | 0x8000);
                        setHittypeTemp(i, 4, 1);
                        setHittypeTemp(i, 3, -getHittypeTemp(i, 3));
                        if(q == 0)
                        {
                            callsound(sn,i);
                            q = 1;
                        }
                    }
                    i = spr_add_data[i].nextspritestat;
                }
            }
            return;

        case 25: //Subway type sliding doors

            j = headspritestat[3];
            while(j >= 0)//Find the sprite
            {
                if( (getSprite(j)->_lotag) == 15 && getSprite(j)->_sectnum == sn )
                    break; //Found the sectoreffector.
                j = spr_add_data[j].nextspritestat;
            }

            if(j < 0)
                return;

            i = headspritestat[3];
            while(i >= 0)
            {
                if( SHT==getSprite(j)->_hitag )
                {
                    if( SLT == 15 )
                    {
                        setSector_lotag(SECT, getSector(SECT)->lotag ^ 0x8000); // Toggle the open or close
                        setSprite_ang(i, getSprite_ang(i) + 1024); //SA += 1024; // that's horrible code
                        if(getHittypeTemp(i, 4)) callsound(SECT,i);
                        callsound(SECT,i);
                        if(getSector(SECT)->lotag&0x8000) setHittypeTemp(i, 4, 1);
                        else setHittypeTemp(i, 4, 2);
                    }
                }
                i = spr_add_data[i].nextspritestat;
            }
            return;

        case 27:  //Extended bridge

            j = headspritestat[3];
            while(j >= 0)
            {
                if( (getSprite(j)->_lotag&0xff)==20 && getSprite(j)->_sectnum == sn) //Bridge
                {

                    setSector_lotag(sn, getSector(sn)->lotag ^ 0x8000);
                    if(getSector(sn)->lotag&0x8000) //OPENING
                        setHittypeTemp(j, 0, 1);
                    else
                        setHittypeTemp(j, 0, 2);
                    callsound(sn,ii);
                    break;
                }
                j = spr_add_data[j].nextspritestat;
            }
            return;


        case 28:
            //activate the rest of them

            j = headspritesect[sn];
            while(j >= 0)
            {
                if(getSprite(j)->_statnum==3 && (getSprite(j)->_lotag&0xff)==21)
                    break; //Found it
                j = spr_add_data[j].nextspritesect;
            }

            j = getSprite(j)->_hitag;

            l = headspritestat[3];
            while(l >= 0)
            {
                if( (getSprite(i)->_lotag&0xff)==21 && !getHittypeTemp(l, 0) &&
                    (getSprite(i)->_hitag) == j )
                    setHittypeTemp(l, 0, 1);
                l = spr_add_data[l].nextspritestat;
            }
            callsound(sn,ii);

            return;
    }
}



void operaterespawns(short low)
{
    short i, j, nexti;

    i = headspritestat[11];
    while(i >= 0)
    {
        nexti = spr_add_data[i].nextspritestat;
        if(SLT == low) switch(PN)
        {
            case RESPAWN:
                if( badguypic(SHT) && ud.monsters_off ) break;

                j = spawn(i,TRANSPORTERSTAR);
                setSprite_z(j, getSprite(j)->_z - (32<<8));

                setSprite_extra(i, 66-12);   // Just a way to killit
                break;
        }
        i = nexti;
    }
}

void operateactivators(short low,short snum)
{
    short i, j, k, *p;

    for(i=numcyclers-1;i>=0;i--)
    {
        p = &cyclers[i][0];

        if(p[4] == low)
        {
            p[5] = !p[5];

            setSector_floorshade(p[0], p[3]);
            setSector_ceilingshade(p[0], p[3]);
            int wallIdx = constsector[p[0]].wallptr;
//                WARNING("SHADE\r\n");
            for(j=constsector[p[0]].wallnum;j > 0;j--,wallIdx++)
                setWall_shade(wallIdx, p[3]);
        }
    }

    i = headspritestat[8];
    k = -1;
    while(i >= 0)
    {
        if(getSprite(i)->_lotag == low)
        {
            if( getSprite(i)->_picnum == ACTIVATORLOCKED )
            {
                if(getSector(SECT)->lotag&16384)
                    setSector_lotag(SECT, getSector(SECT)->lotag & (65535-16384));
                else
                    setSector_lotag(SECT, getSector(SECT)->lotag | 16384);

                if(snum >= 0)
                {
                    if(getSector(SECT)->lotag&16384)
                        FTA(4,&ps[snum],0);
                    else FTA(8,&ps[snum],0);
                }
            }
            else
            {
                switch(SHT)
                {
                    case 0:
                        break;
                    case 1:
                        if(getSector(SECT)->floorz != getSector(SECT)->ceilingz)
                        {
                            i = spr_add_data[i].nextspritestat;
                            continue;
                        }
                        break;
                    case 2:
                        if(getSector(SECT)->floorz == getSector(SECT)->ceilingz)
                        {
                            i = spr_add_data[i].nextspritestat;
                            continue;
                        }
                        break;
                }

                if( getSector(getSprite(i)->_sectnum)->lotag < 3 )
                {
                    j = headspritesect[getSprite(i)->_sectnum];
                    while(j >= 0)
                    {
                        if( getSprite(j)->_statnum == 3 ) switch(getSprite(j)->_lotag)
                        {
                            case 36:
                            case 31:
                            case 32:
                            case 18:
                                setHittypeTemp(j, 0, 1-getHittypeTemp(j, 0));
                                callsound(SECT,j);
                                break;
                        }
                        j = spr_add_data[j].nextspritesect;
                    }
                }

                if( k == -1 && (getSector(SECT)->lotag&0xff) == 22 )
                    k = callsound(SECT,i);

                operatesectors(SECT,i);
            }
        }
        i = spr_add_data[i].nextspritestat;
     }

    operaterespawns(low);
}

void operatemasterswitches(short low)
{
    short i;

    i = headspritestat[6];
    while(i >= 0)
    {
        if( PN == MASTERSWITCH && SLT == low && SP == 0 )
            setSprite_yvel(i, 1);//SP = 1; // horrible, really horrible.
        i = spr_add_data[i].nextspritestat;
    }
}



void operateforcefields(short s, short low)
{
    short i, p;

    for(p=numanimwalls;p>=0;p--)
    {
        i = animwall[p].wallnum;
        if(low == getWall_lotag(i) || low == -1)
            switch(getWall_overpicnum(i))
        {
            case W_FORCEFIELD  :
            case W_FORCEFIELD+1:
            case W_FORCEFIELD+2:
            case BIGFORCE:

                animwall[p].tag = 0;

                if( getWall_cstat(i) )
                {
                    setWall_cstat(i, 0);

                    if( s >= 0 && getSprite(s)->_picnum == SECTOREFFECTOR &&
                        getSprite(s)->_lotag == 30)
                            setWall_lotag(i, 0);
                }
                else
                    setWall_cstat(i, 85);
                break;
        }
    }
}


uint8_t  checkhitswitch(short snum,int32_t w,uint8_t  switchtype)
{
    uint8_t  switchpal;
    short i, x, lotag,hitag,picnum,correctdips,numdips;
    int32_t sx,sy;

    if(w < 0) return 0;
    correctdips = 1;
    numdips = 0;

    if(switchtype == 1) // A wall sprite
    {
        lotag = getSprite(w)->_lotag; if(lotag == 0) return 0;
        hitag = getSprite(w)->_hitag;
        sx = getSprite(w)->_x;
        sy = getSprite(w)->_y;
        picnum = getSprite(w)->_picnum;
        switchpal = getSprite(w)->_pal;
    }
    else
    {
        lotag = getWall_lotag(w); if(lotag == 0) return 0;
        hitag = getWall_hitag(w);
        sx = getWall(w)->x;
        sy = getWall(w)->y;
        picnum = getWall_picnum(w);
#if NO_MINIMIZE_WALLS
        switchpal = wall[w].pal;
#else
        switchpal = getWall_pal(w);
#endif
    }

    switch(picnum)
    {
        case DIPSWITCH:
        case DIPSWITCH+1:
        case TECHSWITCH:
        case TECHSWITCH+1:
        case ALIENSWITCH:
        case ALIENSWITCH+1:
            break;
        case ACCESSSWITCH:
        case ACCESSSWITCH2:
            if(ps[snum].access_incs == 0)
            {
                if( switchpal == 0 )
                {
                    if( (ps[snum].got_access&1) )
                        ps[snum].access_incs = 1;
                    else FTA(70,&ps[snum],1);
                }

                else if( switchpal == 21 )
                {
                    if( ps[snum].got_access&2 )
                        ps[snum].access_incs = 1;
                    else FTA(71,&ps[snum],1);
                }

                else if( switchpal == 23 )
                {
                    if( ps[snum].got_access&4 )
                        ps[snum].access_incs = 1;
                    else FTA(72,&ps[snum],1);
                }

                if( ps[snum].access_incs == 1 )
                {
                    if(switchtype == 0)
                        ps[snum].access_wallnum = w;
                    else
                        ps[snum].access_spritenum = w;
                }

                return 0;
            }
        case DIPSWITCH2:
        case DIPSWITCH2+1:
        case DIPSWITCH3:
        case DIPSWITCH3+1:
        case MULTISWITCH:
        case MULTISWITCH+1:
        case MULTISWITCH+2:
        case MULTISWITCH+3:
        case PULLSWITCH:
        case PULLSWITCH+1:
        case HANDSWITCH:
        case HANDSWITCH+1:
        case SLOTDOOR:
        case SLOTDOOR+1:
        case LIGHTSWITCH:
        case LIGHTSWITCH+1:
        case SPACELIGHTSWITCH:
        case SPACELIGHTSWITCH+1:
        case SPACEDOORSWITCH:
        case SPACEDOORSWITCH+1:
        case FRANKENSTINESWITCH:
        case FRANKENSTINESWITCH+1:
        case LIGHTSWITCH2:
        case LIGHTSWITCH2+1:
        case POWERSWITCH1:
        case POWERSWITCH1+1:
        case LOCKSWITCH1:
        case LOCKSWITCH1+1:
        case POWERSWITCH2:
        case POWERSWITCH2+1:
            if( check_activator_motion( lotag ) ) return 0;
            break;
        default:
            if( isadoorwall(picnum) == 0 ) return 0;
            break;
    }

    i = headspritestat[0];
    while(i >= 0)
    {
        if( lotag == SLT ) switch(PN)
        {
            case DIPSWITCH:
            case TECHSWITCH:
            case ALIENSWITCH:
                if( switchtype == 1 && w == i )
                    setSprite_picnum(i, getSprite(i)->_picnum + 1);//PN++; // bleurgh
                else if( SHT == 0 ) correctdips++;
                numdips++;
                break;
            case TECHSWITCH+1:
            case DIPSWITCH+1:
            case ALIENSWITCH+1:
                if( switchtype == 1 && w == i )
                    setSprite_picnum(i, getSprite(i)->_picnum - 1);//PN--; // bleurgh
                else if( SHT == 1 ) correctdips++;
                numdips++;
                break;
            case MULTISWITCH:
            case MULTISWITCH+1:
            case MULTISWITCH+2:
            case MULTISWITCH+3:
                setSprite_picnum(i, getSprite(i)->_picnum + 1);
                if( getSprite(i)->_picnum > (MULTISWITCH+3) )
                    setSprite_picnum(i, MULTISWITCH);
                break;
            case ACCESSSWITCH:
            case ACCESSSWITCH2:
            case SLOTDOOR:
            case LIGHTSWITCH:
            case SPACELIGHTSWITCH:
            case SPACEDOORSWITCH:
            case FRANKENSTINESWITCH:
            case LIGHTSWITCH2:
            case POWERSWITCH1:
            case LOCKSWITCH1:
            case POWERSWITCH2:
            case HANDSWITCH:
            case PULLSWITCH:
            case DIPSWITCH2:
            case DIPSWITCH3:
                setSprite_picnum(i, getSprite(i)->_picnum + 1);
                break;
            case PULLSWITCH+1:
            case HANDSWITCH+1:
            case LIGHTSWITCH2+1:
            case POWERSWITCH1+1:
            case LOCKSWITCH1+1:
            case POWERSWITCH2+1:
            case SLOTDOOR+1:
            case LIGHTSWITCH+1:
            case SPACELIGHTSWITCH+1:
            case SPACEDOORSWITCH+1:
            case FRANKENSTINESWITCH+1:
            case DIPSWITCH2+1:
            case DIPSWITCH3+1:
                setSprite_picnum(i, getSprite(i)->_picnum - 1);
                break;
        }
        i = spr_add_data[i].nextspritestat;
    }

    for(i=0;i<numwalls;i++)
    {
        x = i;
        if(lotag == getWall_lotag(x))
            switch(getWall_picnum(x))
        {
            case DIPSWITCH:
            case TECHSWITCH:
            case ALIENSWITCH:
                if( switchtype == 0 && i == w ) setWall_picnum(x, getWall_picnum(x) + 1);//wall[x].picnum++;
                else if( getWall_hitag(x) == 0 ) correctdips++;
                numdips++;
                break;
            case DIPSWITCH+1:
            case TECHSWITCH+1:
            case ALIENSWITCH+1:
                if( switchtype == 0 && i == w ) setWall_picnum(x, getWall_picnum(x) - 1);//wall[x].picnum--;
                else if( getWall_hitag(x) == 1 ) correctdips++;
                numdips++;
                break;
            case MULTISWITCH:
            case MULTISWITCH+1:
            case MULTISWITCH+2:
            case MULTISWITCH+3:
                setWall_picnum(x, getWall_picnum(x) + 1);//                wall[x].picnum++;
                if(getWall_picnum(x) > (MULTISWITCH+3) )
                    setWall_picnum(x, MULTISWITCH);
                break;
            case ACCESSSWITCH:
            case ACCESSSWITCH2:
            case SLOTDOOR:
            case LIGHTSWITCH:
            case SPACELIGHTSWITCH:
            case SPACEDOORSWITCH:
            case LIGHTSWITCH2:
            case POWERSWITCH1:
            case LOCKSWITCH1:
            case POWERSWITCH2:
            case PULLSWITCH:
            case HANDSWITCH:
            case DIPSWITCH2:
            case DIPSWITCH3:
                setWall_picnum(x, getWall_picnum(x) + 1);//wall[x].picnum++;
                break;
            case HANDSWITCH+1:
            case PULLSWITCH+1:
            case LIGHTSWITCH2+1:
            case POWERSWITCH1+1:
            case LOCKSWITCH1+1:
            case POWERSWITCH2+1:
            case SLOTDOOR+1:
            case LIGHTSWITCH+1:
            case SPACELIGHTSWITCH+1:
            case SPACEDOORSWITCH+1:
            case DIPSWITCH2+1:
            case DIPSWITCH3+1:
                setWall_picnum(x, getWall_picnum(x) - 1);//wall[x].picnum--;
                break;
        }
    }

    if(lotag == (short) 65535)
    {
        ps[myconnectindex].gm = MODE_EOL;
        if(ud.from_bonus)
        {
            ud.level_number = ud.from_bonus;
            ud.m_level_number = ud.level_number;
            ud.from_bonus = 0;
        }
        else
        {
            ud.level_number++;
            if( (ud.volume_number && ud.level_number > 10 ) || ( ud.volume_number == 0 && ud.level_number > 5 ) )
                ud.level_number = 0;
            ud.m_level_number = ud.level_number;
        }
        return 1;
    }

    switch(picnum)
    {
        default:
            if(isadoorwall(picnum) == 0) break;
        case DIPSWITCH:
        case DIPSWITCH+1:
        case TECHSWITCH:
        case TECHSWITCH+1:
        case ALIENSWITCH:
        case ALIENSWITCH+1:
            if( picnum == DIPSWITCH  || picnum == DIPSWITCH+1 ||
                picnum == ALIENSWITCH || picnum == ALIENSWITCH+1 ||
                picnum == TECHSWITCH || picnum == TECHSWITCH+1 )
            {
                if( picnum == ALIENSWITCH || picnum == ALIENSWITCH+1)
                {
                    if(switchtype == 1)
                        xyzsound(ALIEN_SWITCH1,w,sx,sy,ps[snum].posz);
                    else xyzsound(ALIEN_SWITCH1,ps[snum].i,sx,sy,ps[snum].posz);
                }
                else
                {
                    if(switchtype == 1)
                        xyzsound(SWITCH_ON,w,sx,sy,ps[snum].posz);
                    else xyzsound(SWITCH_ON,ps[snum].i,sx,sy,ps[snum].posz);
                }
                if(numdips != correctdips) break;
                xyzsound(END_OF_LEVEL_WARN,ps[snum].i,sx,sy,ps[snum].posz);
            }
        case DIPSWITCH2:
        case DIPSWITCH2+1:
        case DIPSWITCH3:
        case DIPSWITCH3+1:
        case MULTISWITCH:
        case MULTISWITCH+1:
        case MULTISWITCH+2:
        case MULTISWITCH+3:
        case ACCESSSWITCH:
        case ACCESSSWITCH2:
        case SLOTDOOR:
        case SLOTDOOR+1:
        case LIGHTSWITCH:
        case LIGHTSWITCH+1:
        case SPACELIGHTSWITCH:
        case SPACELIGHTSWITCH+1:
        case SPACEDOORSWITCH:
        case SPACEDOORSWITCH+1:
        case FRANKENSTINESWITCH:
        case FRANKENSTINESWITCH+1:
        case LIGHTSWITCH2:
        case LIGHTSWITCH2+1:
        case POWERSWITCH1:
        case POWERSWITCH1+1:
        case LOCKSWITCH1:
        case LOCKSWITCH1+1:
        case POWERSWITCH2:
        case POWERSWITCH2+1:
        case HANDSWITCH:
        case HANDSWITCH+1:
        case PULLSWITCH:
        case PULLSWITCH+1:

                if( picnum == MULTISWITCH || picnum == (MULTISWITCH+1) ||
                    picnum == (MULTISWITCH+2) || picnum == (MULTISWITCH+3) )
                        lotag += picnum-MULTISWITCH;

                x = headspritestat[3];
                while(x >= 0)
                {
                   if( (getSprite(x)->_hitag) == lotag )
                   {
                       switch(getSprite(x)->_lotag)
                       {
                           case 12:
                                setSector_floorpal(getSprite(x)->_sectnum, 0);
                                setHittypeTemp(x, 0, getHittypeTemp(x, 0) + 1);
                                if(getHittypeTemp(x, 0) == 2)
                                   setHittypeTemp(x, 0, getHittypeTemp(x, 0) + 1);

                                break;
                           case 24:
                           case 34:
                           case 25:
                               setHittypeTemp(x, 4, !getHittypeTemp(x, 4));
                               if(getHittypeTemp(x, 4))
                                   FTA(15,&ps[snum],1);
                               else FTA(2,&ps[snum],1);
                               break;
                           case 21:
                               FTA(2,&ps[screenpeek],1);
                               break;
                       }
                   }
                   x = spr_add_data[x].nextspritestat;
                }

                operateactivators(lotag,snum);
                operateforcefields(ps[snum].i,lotag);
                operatemasterswitches(lotag);

                if( picnum == DIPSWITCH || picnum == DIPSWITCH+1 ||
                    picnum == ALIENSWITCH || picnum == ALIENSWITCH+1 ||
                    picnum == TECHSWITCH || picnum == TECHSWITCH+1 ) return 1;

                if( hitag == 0 && isadoorwall(picnum) == 0 )
                {
                    if(switchtype == 1)
                        xyzsound(SWITCH_ON,w,sx,sy,ps[snum].posz);
                    else xyzsound(SWITCH_ON,ps[snum].i,sx,sy,ps[snum].posz);
                }
                else if(hitag != 0)
                {
                    if(switchtype == 1 && (soundm[hitag]&4) == 0)
                        xyzsound(hitag,w,sx,sy,ps[snum].posz);
                    else spritesound(hitag,ps[snum].i);
                }

               return 1;
    }
    return 0;
}


void activatebysector(short sect,short j)
{
    short i,didit;

    didit = 0;

    i = headspritesect[sect];
    while(i >= 0)
    {
        if(PN == ACTIVATOR)
        {
            operateactivators(SLT,-1);
            didit = 1;
//            return;
        }
        i = spr_add_data[i].nextspritesect;
    }

    if(didit == 0)
        operatesectors(sect,j);
}

void breakwall(short newpn,short spr,short dawallnum)
{
    setWall_picnum(dawallnum, newpn);
    spritesound(VENT_BUST,spr);
    spritesound(GLASS_HEAVYBREAK,spr);
    lotsofglass(spr,dawallnum,10);
}

void checkhitwall(short spr,short dawallnum,int32_t x,int32_t y,int32_t z,short atwith)
{
    short j, i, sn = -1, darkestwall;

    if(getWall_overpicnum(dawallnum) == MIRROR)
    {
        switch(atwith)
        {
            case HEAVYHBOMB:
            case RADIUSEXPLOSION:
            case RPG:
            case HYDRENT:
            case SEENINE:
            case OOZFILTER:
            case EXPLODINGBARREL:
                lotsofglass(spr,dawallnum,70);
                setWall_cstat(dawallnum, getWall_cstat(dawallnum) & ~16);
                setWall_overpicnum(dawallnum, MIRRORBROKE);
                spritesound(GLASS_HEAVYBREAK,spr);
                return;
        }
    }

   if( ( (getWall_cstat(dawallnum)&16) || getWall_overpicnum(dawallnum) == BIGFORCE ) && constwall[dawallnum].nextsector >= 0 )
       if( getSector(constwall[dawallnum].nextsector)->floorz > z )
           if( getSector(constwall[dawallnum].nextsector)->floorz-getSector(constwall[dawallnum].nextsector)->ceilingz )
               switch(getWall_overpicnum(dawallnum))
    {
        case W_FORCEFIELD:
        case W_FORCEFIELD+1:
        case W_FORCEFIELD+2:
            setWall_extra(dawallnum, 1); // tell the forces to animate
        case BIGFORCE:
            updatesector(x,y,&sn);
            if( sn < 0 ) return;

            if(atwith == -1)
                i = EGS(sn,x,y,z,FORCERIPPLE,-127,8,8,0,0,0,spr,5);
            else
            {
                if(atwith == CHAINGUN)
                    i = EGS(sn,x,y,z,FORCERIPPLE,-127,16+getSprite(spr)->_xrepeat,16+getSprite(spr)->_yrepeat,0,0,0,spr,5);
                else i = EGS(sn,x,y,z,FORCERIPPLE,-127,32,32,0,0,0,spr,5);
            }

            setSprite_cstat(i, getSprite_cstat(i) | (18 + 128)); //CS |= 18+128;   // bad, really bad.
            setSprite_ang(i,getangle(getWall_x(dawallnum)-getWall(getWall_point2(dawallnum))->x, getWall_y(dawallnum)-getWall(getWall_point2(dawallnum))->y)-512); //SA = getangle(wal->x-getWall(getWall_point2(dawallnum))->x, wal->y-getWall(getWall_point2(dawallnum))->y)-512;// idem

            spritesound(SOMETHINGHITFORCE,i);

            return;

        case FANSPRITE:
            setWall_overpicnum(dawallnum, FANSPRITEBROKE);
            setWall_cstat(dawallnum, getWall_cstat(dawallnum) & (65535-65)); // remove bit 0 and 6
            if(constwall[dawallnum].nextwall >= 0)
            {
                setWall_overpicnum(constwall[dawallnum].nextwall, FANSPRITEBROKE);
                setWall_cstat(constwall[dawallnum].nextwall, getWall_cstat(constwall[dawallnum].nextwall) & (65535-65));  // remove bit 0 and 6
            }
            spritesound(VENT_BUST,spr);
            spritesound(GLASS_BREAKING,spr);
            return;

        case GLASS:
            updatesector(x,y,&sn); if( sn < 0 ) return;
            setWall_overpicnum(dawallnum, GLASS2);
            lotsofglass(spr,dawallnum,10);
            setWall_cstat(dawallnum, 0);

            if(constwall[dawallnum].nextwall >= 0)
            {
                setWall_cstat(constwall[dawallnum].nextwall, 0);
            }

            i = EGS(sn,x,y,z,SECTOREFFECTOR,0,0,0,ps[0].ang,0,0,spr,3);
            setSprite_lotag(i, 128);//SLT = 128;
            setHittypeTemp(i, 1, 5);
            setHittypeTemp(i, 2, dawallnum);
            spritesound(GLASS_BREAKING,i);
            return;
        case STAINGLASS1:
            updatesector(x,y,&sn); if( sn < 0 ) return;
            lotsofcolourglass(spr,dawallnum,80);
            setWall_cstat(dawallnum, 0);
            if(constwall[dawallnum].nextwall >= 0)
            {
                setWall_cstat(constwall[dawallnum].nextwall, 0);
            }
            spritesound(VENT_BUST,spr);
            spritesound(GLASS_BREAKING,spr);
            return;
    }

    switch(getWall_picnum(dawallnum))
    {
            case COLAMACHINE:
            case VENDMACHINE:
                breakwall(getWall_picnum(dawallnum)+2,spr,dawallnum);
                spritesound(VENT_BUST,spr);
                return;

            case OJ:
            case FEMPIC2:
            case FEMPIC3:

            case SCREENBREAK6:
            case SCREENBREAK7:
            case SCREENBREAK8:

            case SCREENBREAK1:
            case SCREENBREAK2:
            case SCREENBREAK3:
            case SCREENBREAK4:
            case SCREENBREAK5:

            case SCREENBREAK9:
            case SCREENBREAK10:
            case SCREENBREAK11:
            case SCREENBREAK12:
            case SCREENBREAK13:
            case SCREENBREAK14:
            case SCREENBREAK15:
            case SCREENBREAK16:
            case SCREENBREAK17:
            case SCREENBREAK18:
            case SCREENBREAK19:
            case BORNTOBEWILDSCREEN:

                lotsofglass(spr,dawallnum,30);
                setWall_picnum(dawallnum, W_SCREENBREAK+(TRAND%3));
                spritesound(GLASS_HEAVYBREAK,spr);
                return;

            case W_TECHWALL5:
            case W_TECHWALL6:
            case W_TECHWALL7:
            case W_TECHWALL8:
            case W_TECHWALL9:
                breakwall(getWall_picnum(dawallnum)+1,spr,dawallnum);
                return;
            case W_MILKSHELF:
                breakwall(W_MILKSHELFBROKE,spr,dawallnum);
                return;

            case W_TECHWALL10:
                breakwall(W_HITTECHWALL10,spr,dawallnum);
                return;

            case W_TECHWALL1:
            case W_TECHWALL11:
            case W_TECHWALL12:
            case W_TECHWALL13:
            case W_TECHWALL14:
                breakwall(W_HITTECHWALL1,spr,dawallnum);
                return;

            case W_TECHWALL15:
                breakwall(W_HITTECHWALL15,spr,dawallnum);
                return;

            case W_TECHWALL16:
                breakwall(W_HITTECHWALL16,spr,dawallnum);
                return;

            case W_TECHWALL2:
                breakwall(W_HITTECHWALL2,spr,dawallnum);
                return;

            case W_TECHWALL3:
                breakwall(W_HITTECHWALL3,spr,dawallnum);
                return;

            case W_TECHWALL4:
                breakwall(W_HITTECHWALL4,spr,dawallnum);
                return;

            case ATM:
                setWall_picnum(dawallnum, ATMBROKE);
                lotsofmoney(getSprite(spr),1+(TRAND&7));
                spritesound(GLASS_HEAVYBREAK,spr);
                break;

            case WALLLIGHT1:
            case WALLLIGHT2:
            case WALLLIGHT3:
            case WALLLIGHT4:
            case TECHLIGHT2:
            case TECHLIGHT4:

                if( rnd(128) )
                    spritesound(GLASS_HEAVYBREAK,spr);
                else spritesound(GLASS_BREAKING,spr);
                lotsofglass(spr,dawallnum,30);

                if(getWall_picnum(dawallnum) == WALLLIGHT1)
                    setWall_picnum(dawallnum, WALLLIGHTBUST1);
                else if(getWall_picnum(dawallnum) == WALLLIGHT2)
                    setWall_picnum(dawallnum, WALLLIGHTBUST2);
                else if(getWall_picnum(dawallnum) == WALLLIGHT3)
                    setWall_picnum(dawallnum, WALLLIGHTBUST3);
                else if(getWall_picnum(dawallnum) == WALLLIGHT4)
                    setWall_picnum(dawallnum, WALLLIGHTBUST4);
                else if(getWall_picnum(dawallnum) == TECHLIGHT2)
                    setWall_picnum(dawallnum, TECHLIGHTBUST2);
                else if(getWall_picnum(dawallnum) == TECHLIGHT4)
                    setWall_picnum(dawallnum, TECHLIGHTBUST4);

                if(!getWall_lotag(dawallnum)) return;

                sn = constwall[dawallnum].nextsector;
                if(sn < 0) return;
                darkestwall = 0;

                int wallIdx = constsector[sn].wallptr;
                for(i=constsector[sn].wallnum;i > 0;i--, wallIdx++)
                    if(getWall_shade(wallIdx) > darkestwall)
                        darkestwall=getWall_shade(wallIdx);

                j = TRAND&1;
                i= headspritestat[3];
                while(i >= 0)
                {
                    if(SHT == getWall_lotag(dawallnum) && SLT == 3 )
                    {
                        setHittypeTemp(i, 2, j);
                        setHittypeTemp(i, 3, darkestwall);
                        setHittypeTemp(i, 4, 1);
                    }
                    i = spr_add_data[i].nextspritestat;
                }
                break;
    }
}


void checkplayerhurt(struct player_struct *p,short j)
{
    if( (j&0xC000) == 49152 )
    {
#if !NEW_MAXSPRITES
        j &= (MAXSPRITES-1);
#else
        CHECK_SPRITE(j);
        j &= MAX_SPRITESMASK;
#endif
        switch(getSprite(j)->_picnum)
        {
            case CACTUS:
                if(p->hurt_delay < 8 )
                {
                    setSprite_extra(p->i, getSprite(p->i)->_extra - 5);

                    p->hurt_delay = 16;
                    p->pals_time = 32;
                    p->pals[0] = 32;
                    p->pals[1] = 0;
                    p->pals[2] = 0;
                    spritesound(DUKE_LONGTERM_PAIN,p->i);
                }
                break;
        }
        return;
    }

    if( (j&0xC000) != 32768) return;
//    j &= (MAXWALLS-1);
    j &= ~0xC000;
    if( p->hurt_delay > 0 )
        p->hurt_delay--;
    else
    {
        if( getWall_cstat(j)&85 )
        {
            switch(getWall_overpicnum(j))
            {
                case W_FORCEFIELD:
                case W_FORCEFIELD+1:
                case W_FORCEFIELD+2:
                     setSprite_extra(p->i, getSprite(p->i)->_extra - 5);

                     p->hurt_delay = 16;
                     p->pals_time = 32;
                     p->pals[0] = 32;
                     p->pals[1] = 0;
                     p->pals[2] = 0;

                     p->posxv = -(sintable[(p->ang+512)&2047]<<8);
                     p->posyv = -(sintable[(p->ang)&2047]<<8);
                     spritesound(DUKE_LONGTERM_PAIN,p->i);

                     checkhitwall(p->i,j,
                         p->posx+(sintable[(p->ang+512)&2047]>>9),
                         p->posy+(sintable[p->ang&2047]>>9),
                         p->posz,-1);

                    break;

                case BIGFORCE:
                    p->hurt_delay = 26;
                    checkhitwall(p->i,j,
                        p->posx+(sintable[(p->ang+512)&2047]>>9),
                        p->posy+(sintable[p->ang&2047]>>9),
                        p->posz,-1);
                    break;

            }
        }
    }
}


uint8_t  checkhitceiling(short sn)
{
    short i, j;



    switch(getSector(sn)->ceilingpicnum)
    {
        case WALLLIGHT1:
        case WALLLIGHT2:
        case WALLLIGHT3:
        case WALLLIGHT4:
        case TECHLIGHT2:
        case TECHLIGHT4:

                ceilingglass(ps[myconnectindex].i,sn,10);
                spritesound(GLASS_BREAKING,ps[screenpeek].i);

                if(getSector(sn)->ceilingpicnum == WALLLIGHT1)
                    setSector_ceilingpicnum(sn, WALLLIGHTBUST1);

                if(getSector(sn)->ceilingpicnum == WALLLIGHT2)
                    setSector_ceilingpicnum(sn, WALLLIGHTBUST2);

                if(getSector(sn)->ceilingpicnum == WALLLIGHT3)
                    setSector_ceilingpicnum(sn, WALLLIGHTBUST3);

                if(getSector(sn)->ceilingpicnum == WALLLIGHT4)
                    setSector_ceilingpicnum(sn, WALLLIGHTBUST4);

                if(getSector(sn)->ceilingpicnum == TECHLIGHT2)
                    setSector_ceilingpicnum(sn, TECHLIGHTBUST2);

                if(getSector(sn)->ceilingpicnum == TECHLIGHT4)
                    setSector_ceilingpicnum(sn, TECHLIGHTBUST4);


                if(!getSector(sn)->hitag)
                {
                    i = headspritesect[sn];
                    while(i >= 0)
                    {
                        if( PN == SECTOREFFECTOR && SLT == 12 )
                        {
                            j = headspritestat[3];
                            while(j >= 0)
                            {
                                if( getSprite(j)->_hitag == SHT )
                                    setHittypeTemp(j, 3, 1);
                                j = spr_add_data[j].nextspritestat;
                            }
                            break;
                        }
                        i = spr_add_data[i].nextspritesect;
                    }
                }

                i = headspritestat[3];
                j = TRAND&1;
                while(i >= 0)
                {
                    if(SHT == (getSector(sn)->hitag) && SLT == 3 )
                    {
                        setHittypeTemp(i, 2, j);
                        setHittypeTemp(i, 4, 1);
                    }
                    i = spr_add_data[i].nextspritestat;
                }

                return 1;
    }

    return 0;
}

void checkhitsprite(short i,short sn)
{
    short j, k, p;
//    spritetype *s;
#if !NEW_MAXSPRITES
    i &= (MAXSPRITES-1);
#else
    CHECK_SPRITE(i);
    i &= MAX_SPRITESMASK;
#endif
    switch(PN)
    {
        case OCEANSPRITE1:
        case OCEANSPRITE2:
        case OCEANSPRITE3:
        case OCEANSPRITE4:
        case OCEANSPRITE5:
            spawn(i,SMALLSMOKE);
            deletesprite(i);
            break;
        case QUEBALL:
        case STRIPEBALL:
            if(getSprite(sn)->_picnum == QUEBALL || getSprite(sn)->_picnum == STRIPEBALL)
            {
                setSprite_xvel(sn, (getSprite(i)->_xvel>>1)+(getSprite(i)->_xvel>>2));
                setSprite_ang(sn, getSprite(sn)->_ang - ((SA<<1)+1024));
                setSprite_ang(i, getangle(SX-getSprite(sn)->_x,SY-getSprite(sn)->_y)-512);
//                if(Sound[POOLBALLHIT].num < 2)
                if(getSoundNum(POOLBALLHIT) < 2)
                    spritesound(POOLBALLHIT,i);
            }
            else
            {
                if( TRAND&3 )
                {
                    setSprite_xvel(i, 164);
                    setSprite_ang(i, getSprite(sn)->_ang);
                }
                else
                {
                    lotsofglass(i,-1,3);
                    deletesprite(i);
                }
            }
            break;
        case TREE1:
        case TREE2:
        case TIRE:
        case CONE:
        case BOX:
            switch(getSprite(sn)->_picnum)
            {
                case RADIUSEXPLOSION:
                case RPG:
                case FIRELASER:
                case HYDRENT:
                case HEAVYHBOMB:
                    if(getHittypeTemp(i, 0) == 0)
                    {
                        setSprite_cstat(i, getSprite_cstat(i) & ~257);// CS &= ~257; // no words for this crap
                        setHittypeTemp(i, 0, 1);
                        spawn(i,BURNING);
                    }
                    break;
            }
            break;
        case CACTUS:
//        case CACTUSBROKE:
            switch(getSprite(sn)->_picnum)
            {
                case RADIUSEXPLOSION:
                case RPG:
                case FIRELASER:
                case HYDRENT:
                case HEAVYHBOMB:
                    for(k=0;k<64;k++)
                    {
                        j = EGS(SECT,SX,SY,SZ-(TRAND%(48<<8)),SCRAP3+(TRAND&3),-8,48,48,TRAND&2047,(TRAND&63)+64,-(TRAND&4095)-(getSprite(i)->_zvel>>2),i,5);
                        setSprite_pal(j, 8);
                    }

                    if(PN == CACTUS)
                        setSprite_picnum(i,  CACTUSBROKE);
                    setSprite_cstat(i, getSprite_cstat(i) & ~257);
             //       else deletesprite(i);
                    break;
            }
            break;

        case HANGLIGHT:
        case GENERICPOLE2:
            for(k=0;k<6;k++)
                EGS(SECT,SX,SY,SZ-(8<<8),SCRAP1+(TRAND&15),-8,48,48,TRAND&2047,(TRAND&63)+64,-(TRAND&4095)-(getSprite(i)->_zvel>>2),i,5);
            spritesound(GLASS_HEAVYBREAK,i);
            deletesprite(i);
            break;


        case FANSPRITE:
            setSprite_picnum(i, FANSPRITEBROKE);
            setSprite_cstat(i, getSprite_cstat(i) & (65535-257));
            if( getSector(SECT)->floorpicnum == FANSHADOW )
                setSector_floorpicnum(SECT, FANSHADOWBROKE);

            spritesound(GLASS_HEAVYBREAK,i);

            const spritetype *spritei = getSprite(i);
            for(j=0;j<16;j++) RANDOMSCRAP(i, spritei);

            break;
        case WATERFOUNTAIN:
        case WATERFOUNTAIN+1:
        case WATERFOUNTAIN+2:
        case WATERFOUNTAIN+3:
            setSprite_picnum(i, WATERFOUNTAINBROKE);
            spawn(i,TOILETWATER);
            break;
        case SATELITE:
        case FUELPOD:
        case SOLARPANNEL:
        case ANTENNA:
#if USE_ACTORSCRPTR
            if(getSprite_extra(sn) != *actorscrptr[SHOTSPARK1] )
#else
            if(getSprite_extra(sn) != *getscriptptr(actorscridx[SHOTSPARK1]) )
#endif
            {
                for(j=0;j<15;j++)
                    EGS(SECT,SX,SY,getSector(SECT)->floorz-(12<<8)-(j<<9),SCRAP1+(TRAND&15),-8,64,64,
                        TRAND&2047,(TRAND&127)+64,-(TRAND&511)-256,i,5);
                spawn(i,EXPLOSION2);
                deletesprite(i);
            }
            break;
        case BOTTLE1:
        case BOTTLE2:
        case BOTTLE3:
        case BOTTLE4:
        case BOTTLE5:
        case BOTTLE6:
        case BOTTLE8:
        case BOTTLE10:
        case BOTTLE11:
        case BOTTLE12:
        case BOTTLE13:
        case BOTTLE14:
        case BOTTLE15:
        case BOTTLE16:
        case BOTTLE17:
        case BOTTLE18:
        case BOTTLE19:
        case WATERFOUNTAINBROKE:
        case DOMELITE:
        case SUSHIPLATE1:
        case SUSHIPLATE2:
        case SUSHIPLATE3:
        case SUSHIPLATE4:
        case SUSHIPLATE5:
        case WAITTOBESEATED:
        case VASE:
        case STATUEFLASH:
        case STATUE:
            if(PN == BOTTLE10)
                lotsofmoney(getSprite(i),4+(TRAND&3));
            else if(PN == STATUE || PN == STATUEFLASH)
            {
                lotsofcolourglass(i,-1,40);
                spritesound(GLASS_HEAVYBREAK,i);
            }
            else if(PN == VASE)
                lotsofglass(i,-1,40);

            spritesound(GLASS_BREAKING,i);
            setSprite_ang(i, TRAND&2047);
            lotsofglass(i,-1,8);
            deletesprite(i);
            break;
        case FETUS:
            setSprite_picnum(i, FETUSBROKE);
            spritesound(GLASS_BREAKING,i);
            lotsofglass(i,-1,10);
            break;
        case FETUSBROKE:
            for(j=0;j<48;j++)
            {
                shoot(i,BLOODSPLAT1);
                setSprite_ang(i, SA + 333);
            }
            spritesound(GLASS_HEAVYBREAK,i);
            spritesound(SQUISHED,i);
        case BOTTLE7:
            spritesound(GLASS_BREAKING,i);
            lotsofglass(i,-1,10);
            deletesprite(i);
            break;
        case HYDROPLANT:
            setSprite_picnum(i, BROKEHYDROPLANT);
            spritesound(GLASS_BREAKING,i);
            lotsofglass(i,-1,10);
            break;

        case FORCESPHERE:
            setSprite_xrepeat(i, 0);
            setHittypeTemp(OW, 0, 32);
            setHittypeTemp(OW, 1, !getHittypeTemp(OW, 1));
            setHittypeTemp(OW, 2, getHittypeTemp(OW, 2) + 1);
            spawn(i,EXPLOSION2);
            break;

        case BROKEHYDROPLANT:
            if(CS&1)
            {
                spritesound(GLASS_BREAKING,i);
                setSprite_z(i, SZ + (16<<8));
                setSprite_cstat(i, 0); //CS = 0;
                lotsofglass(i,-1,5);
            }
            break;

        case TOILET:
            setSprite_picnum(i, TOILETBROKE);
            setSprite_cstat(i, CS | ((TRAND&1)<<2));
            setSprite_cstat(i, getSprite_cstat(i) & ~257);
            spawn(i,TOILETWATER);
            spritesound(GLASS_BREAKING,i);
            break;

        case STALL:
            setSprite_picnum(i, STALLBROKE);
            setSprite_cstat(i, CS | ((TRAND&1)<<2));
            setSprite_cstat(i, getSprite_cstat(i) & ~257);
            spawn(i,TOILETWATER);
            spritesound(GLASS_HEAVYBREAK,i);
            break;

        case HYDRENT:
            setSprite_picnum(i, BROKEFIREHYDRENT);
            spawn(i,TOILETWATER);

//            for(k=0;k<5;k++)
  //          {
    //            j = EGS(SECT,SX,SY,SZ-(TRAND%(48<<8)),SCRAP3+(TRAND&3),-8,48,48,TRAND&2047,(TRAND&63)+64,-(TRAND&4095)-(getSprite(i)->_zvel>>2),i,5);
      //          getSprite(j)->_pal = 2;
        //    }
            spritesound(GLASS_HEAVYBREAK,i);
            break;

        case GRATE1:
            setSprite_picnum(i, BGRATE1);
            setSprite_cstat(i, CS & (65535-256-1));
            spritesound(VENT_BUST,i);
            break;

        case CIRCLEPANNEL:
            setSprite_picnum(i, CIRCLEPANNELBROKE);
            setSprite_cstat(i, CS & (65535-256-1));
            spritesound(VENT_BUST,i);
            break;
        case PANNEL1:
        case PANNEL2:
            setSprite_picnum(i, BPANNEL1);
            setSprite_cstat(i, CS & (65535-256-1));
            spritesound(VENT_BUST,i);
            break;
        case PANNEL3:
            setSprite_picnum(i, BPANNEL3);
            setSprite_cstat(i, CS & (65535-256-1));
            spritesound(VENT_BUST,i);
            break;
        case PIPE1:
        case PIPE2:
        case PIPE3:
        case PIPE4:
        case PIPE5:
        case PIPE6:
            switch(PN)
            {
                case PIPE1: setSprite_picnum(i, PIPE1B);break;
                case PIPE2:setSprite_picnum(i, PIPE2B);break;
                case PIPE3:setSprite_picnum(i, PIPE3B);break;
                case PIPE4:setSprite_picnum(i, PIPE4B);break;
                case PIPE5:setSprite_picnum(i, PIPE5B);break;
                case PIPE6:setSprite_picnum(i, PIPE6B);break;
            }

            j = spawn(i,STEAM);
            setSprite_z(j, getSector(SECT)->floorz-(32<<8));
            break;

        case MONK:
        case LUKE:
        case INDY:
        case JURYGUY:
            spritesound(SLT,i);
            spawn(i,SHT);
        case SPACEMARINE:
            setSprite_extra(i, getSprite_extra(i) - getSprite_extra(sn));
            if(getSprite_extra(i) > 0) break;
            setSprite_ang(i, TRAND&2047);
            shoot(i,BLOODSPLAT1);
            setSprite_ang(i, TRAND&2047);
            shoot(i,BLOODSPLAT2);
            setSprite_ang(i, TRAND&2047);
            shoot(i,BLOODSPLAT3);
            setSprite_ang(i, TRAND&2047);
            shoot(i,BLOODSPLAT4);
            setSprite_ang(i, TRAND&2047);
            shoot(i,BLOODSPLAT1);
            setSprite_ang(i, TRAND&2047);
            shoot(i,BLOODSPLAT2);
            setSprite_ang(i, TRAND&2047);
            shoot(i,BLOODSPLAT3);
            setSprite_ang(i, TRAND&2047);
            shoot(i,BLOODSPLAT4);
            guts(getSprite(i),JIBS1,1,myconnectindex);
            guts(getSprite(i),JIBS2,2,myconnectindex);
            guts(getSprite(i),JIBS3,3,myconnectindex);
            guts(getSprite(i),JIBS4,4,myconnectindex);
            guts(getSprite(i),JIBS5,1,myconnectindex);
            guts(getSprite(i),JIBS3,6,myconnectindex);
            sound(SQUISHED);
            deletesprite(i);
            break;
        case CHAIR1:
        case CHAIR2:
            setSprite_picnum(i, BROKENCHAIR);
            setSprite_cstat(i, 0);
            break;
        case CHAIR3:
        case MOVIECAMERA:
        case SCALE:
        case VACUUM:
        case CAMERALIGHT:
        case IVUNIT:
        case POT1:
        case POT2:
        case POT3:
        case TRIPODCAMERA:
        {

            spritesound(GLASS_HEAVYBREAK,i);
            const spritetype *spritei = getSprite(i);
            for(j=0;j<16;j++) RANDOMSCRAP(i, spritei);
            deletesprite(i);
        }
            break;
        case PLAYERONWATER:
            i = OW;
        default:
            if( (getSprite(i)->_cstat&16) && SHT == 0 && SLT == 0 && getSprite(i)->_statnum == 0)
                break;

            if( ( getSprite(sn)->_picnum == FREEZEBLAST || getSprite(sn)->_owner != i ) && getSprite(i)->_statnum != 4)
            {
                if( badguy(getSprite(i)) == 1)
                {
                    if(getSprite(sn)->_picnum == RPG)
                        setSprite_extra(sn, getSprite_extra(sn) << 1);

                    if( (PN != DRONE) && (PN != ROTATEGUN) && (PN != COMMANDER) && (PN < GREENSLIME || PN > GREENSLIME+7) )
                        if(getSprite(sn)->_picnum != FREEZEBLAST )
                            if( getActorType(PN) == 0 )
                    {
                        j = spawn(sn,JIBS6);
                        if(getSprite(sn)->_pal == 6)
                            setSprite_pal(j, 6);
                        setSprite_z(j, getSprite(j)->_z + (4<<8));
                        setSprite_xvel(j, 16);
                        setSprite_xy_repeats(j, 24, 24);
                        setSprite_ang(j, getSprite(j)->_ang + 32-(TRAND&63));
                    }

                    j = getSprite(sn)->_owner;

                    if( j >= 0 && getSprite(j)->_picnum == APLAYER && PN != ROTATEGUN && PN != DRONE )
                        if( ps[getSprite(j)->_yvel].curr_weapon == SHOTGUN_WEAPON )
                    {
                        shoot(i,BLOODSPLAT3);
                        shoot(i,BLOODSPLAT1);
                        shoot(i,BLOODSPLAT2);
                        shoot(i,BLOODSPLAT4);
                    }

                    if( PN != TANK && PN != BOSS1 && PN != BOSS4 && PN != BOSS2 && PN != BOSS3 && PN != RECON && PN != ROTATEGUN )
                    {
                        if( (getSprite(i)->_cstat&48) == 0 )
                            setSprite_ang(i, (getSprite(sn)->_ang+1024)&2047);
                        setSprite_xvel(i, -(getSprite_extra(sn)<<2));
                        j = SECT;
                        // next-hack: this to let us use bitfields.
                        int32_t sprx = getSprite(i)->_x;
                        int32_t spry = getSprite(i)->_y;
                        int32_t sprz = getSprite(i)->_z;
                        pushmove(&sprx,&spry,&sprz,&j,128L,(4L<<8),(4L<<8),CLIPMASK0);
                        setSprite_x(i, sprx);
                        setSprite_y(i, spry);
                        setSprite_z(i, sprz);

                        if(j != SECT && j >= 0 && j < MAXSECTORS)
                            changespritesect(i,j);
                    }

                    if(getSprite(i)->_statnum == 2)
                    {
                        changespritestat(i,1);
                        setHittype_timetosleep(i, SLEEPTIME);
                    }
                    if( ( RX < 24 || PN == SHARK) && getSprite(sn)->_picnum == SHRINKSPARK) return;
                }

                if( getSprite(i)->_statnum != 2 )
                {
                    if( getSprite(sn)->_picnum == FREEZEBLAST && ( (PN == APLAYER && getSprite_pal(i) == 1 ) || ( freezerhurtowner == 0 && getSprite(sn)->_owner == i ) ) )
                        return;

                    setHittype_picnum(i, getSprite(sn)->_picnum);
                    setHittype_extra(i, getHittype(i)->extra + getSprite_extra(sn));
                    setHittype_ang(i, getSprite(sn)->_ang);
                    setHittype_owner(i, getSprite(sn)->_owner);
                }

                if(getSprite(i)->_statnum == 10)
                {
                    p = getSprite(i)->_yvel;
                    if(ps[p].newowner >= 0)
                    {
                        ps[p].newowner = -1;
                        ps[p].posx = ps[p].oposx;
                        ps[p].posy = ps[p].oposy;
                        ps[p].posz = ps[p].oposz;
                        ps[p].ang = ps[p].oang;

                        updatesector(ps[p].posx,ps[p].posy,&ps[p].cursectnum);
                        setpal(&ps[p]);

                        j = headspritestat[1];
                        while(j >= 0)
                        {
                            if(getSprite(j)->_picnum==CAMERA1)
                                setSprite_yvel(j, 0);
                            j = spr_add_data[j].nextspritestat;
                        }
                    }

                    if( RX < 24 && getSprite(sn)->_picnum == SHRINKSPARK)
                        return;

                    if( getSprite(getHittype(i)->owner)->_picnum != APLAYER)
                        if(ud.player_skill >= 3)
                            setSprite_extra(sn, getSprite_extra(sn) + (getSprite_extra(sn)>>1));
                }

            }
            break;
    }
}


void alignwarpelevators(void)
{
    short i, j;

    i = headspritestat[3];
    while(i >= 0)
    {
        if( SLT == 17 && SS > 16)
        {
            j = headspritestat[3];
            while(j >= 0)
            {
                if( (getSprite(j)->_lotag) == 17 && i != j &&
                    (SHT) == (getSprite(j)->_hitag) )
                {
                    setSector_floorz(getSprite(j)->_sectnum, getSector(SECT)->floorz);
                    setSector_ceilingz(getSprite(j)->_sectnum, getSector(SECT)->ceilingz);
                }

                j = spr_add_data[j].nextspritestat;
            }
        }
        i = spr_add_data[i].nextspritestat;
    }
}




void cheatkeys(short snum)
{
    short i, k;
    uint8_t  dainv;
    uint32_t sb_snum;
    int j;
    struct player_struct *p;
	uint8_t  playing_old_demo = 0;

    sb_snum = sync[snum].bits;
    p = &ps[snum];

    if(p->cheat_phase == 1) return;

    i = p->aim_mode;
    p->aim_mode = (sb_snum>>23)&1;
    if(p->aim_mode < i)
        p->return_to_center = 9;

    if( (sb_snum&(1<<22)) && p->quick_kick == 0)
    {
        if( !PLUTOPAK || p->curr_weapon != KNEE_WEAPON || p->kickback_pic == 0 )
		// FIX_00066: Removed the QuickKick restrictions for 1.3/1.3d (like 1.3d dos version behavior)
    {
        p->quick_kick = 14;
        FTA(80,p,0);
    }
    }
	// FIX_00040: Preventing multi keypress locks
	playing_old_demo =	ud.playing_demo_rev == BYTEVERSION_27     ||
						ud.playing_demo_rev == BYTEVERSION_28     ||
						ud.playing_demo_rev == BYTEVERSION_116    ||
						ud.playing_demo_rev == BYTEVERSION_117;

	if(!playing_old_demo)
	{
		// A more efficient toggle (make old demos going oos):
		j = sb_snum & ((15<<8)|(1<<12)|(1<<15)|(1<<16)|(1<<22)|(1<<19)|(1<<20)|(1<<21)|(1<<24)| \
						(1<<25)|(1<<27)|(1<<28)|(1<<29)|(1<<30)|(1<<31));
		sb_snum = j & ~p->interface_toggle_flag;
		p->interface_toggle_flag = j;
	}

    if( playing_old_demo && !(sb_snum&((15<<8)|(1<<12)|(1<<15)|(1<<16)|(1<<22)|(1<<19)|(1<<20)|(1<<21)|(1<<24)|(1<<25)|(1<<27)|(1<<28)|(1<<29)|(1<<30)|(1<<31))) )
        p->interface_toggle_flag = 0;
    else if(((p->interface_toggle_flag == 0 && ( sb_snum&(1<<17) ) == 0) && playing_old_demo) ||
		((sb_snum && ( sync[snum].bits&(1<<17) ) == 0) && !playing_old_demo))
    {
		if(playing_old_demo)
			p->interface_toggle_flag = 1;

        if( sb_snum&(1<<21) )
        {
            KB_ClearKeyDown( sc_Pause );
            ud.pause_on = !ud.pause_on;
            if( ud.pause_on == 1 && sb_snum&(1<<5) ) ud.pause_on = 2;
            if(ud.pause_on)
            {
                MUSIC_Pause();
                FX_StopAllSounds();
                clearsoundlocks();
            }
            else
            {
                if(MusicToggle) MUSIC_Continue();
                pub = NUMPAGES;
                pus = NUMPAGES;
            }
        }

        if(ud.pause_on) return;

        if(getSprite(p->i)->_extra <= 0) return;

        if( sb_snum&(1<<30) && p->newowner == -1 )
        {
            switch(p->inven_icon)
            {
                case 4: sb_snum |= (1<<25);break;
                case 3: sb_snum |= (1<<24);break;
                case 5: sb_snum |= (1<<15);break;
                case 1: sb_snum |= (1<<16);break;
                case 2: sb_snum |= (1<<12);break;
            }
        }

        if( (sb_snum&(1<<15)) && p->heat_amount > 0 )
        {
            p->heat_on = !p->heat_on;
            setpal(p);
            p->inven_icon = 5;
            spritesound(NITEVISION_ONOFF,p->i);
            FTA(106+(!p->heat_on),p,0);
        }

        if( (sb_snum&(1<<12)) )
        {
            if(p->steroids_amount == 400 )
            {
                p->steroids_amount--;
                spritesound(DUKE_TAKEPILLS,p->i);
                p->inven_icon = 2;
                FTA(12,p,0);
            }
            return;
        }

        if(p->newowner == -1)
            if( (sb_snum&(1<<20)) || (sb_snum&(1<<27)) || p->refresh_inventory)
        {
            p->invdisptime = 26*2;

            if( sb_snum&(1<<27) ) k = 1;
            else k = 0;

            if(p->refresh_inventory) p->refresh_inventory = 0;
            dainv = p->inven_icon;

            i = 0;
            CHECKINV1:

            if(i < 9)
            {
                i++;

                switch(dainv)
                {
                    case 4:
                        if(p->jetpack_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 5;
                        else dainv = 3;
                        goto CHECKINV1;
                    case 6:
                        if(p->scuba_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 7;
                        else dainv = 5;
                        goto CHECKINV1;
                    case 2:
                        if(p->steroids_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 3;
                        else dainv = 1;
                        goto CHECKINV1;
                    case 3:
                        if(p->holoduke_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 4;
                        else dainv = 2;
                        goto CHECKINV1;
                    case 0:
                    case 1:
                        if(p->firstaid_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 2;
                        else dainv = 7;
                        goto CHECKINV1;
                    case 5:
                        if(p->heat_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 6;
                        else dainv = 4;
                        goto CHECKINV1;
                    case 7:
                        if(p->boot_amount > 0 && i > 1)
                            break;
                        if(k) dainv = 1;
                        else dainv = 6;
                        goto CHECKINV1;
                }
            }
            else dainv = 0;
            p->inven_icon = dainv;

            switch(dainv)
            {
                case 1: FTA(3,p,0);break;
                case 2: FTA(90,p,0);break;
                case 3: FTA(91,p,0);break;
                case 4: FTA(88,p,0);break;
                case 5: FTA(101,p,0);break;
                case 6: FTA(89,p,0);break;
                case 7: FTA(6,p,0);break;
            }
        }

        j = ( (sb_snum&(15<<8))>>8 ) - 1;

        if( j > 0 && p->kickback_pic > 0)
            p->wantweaponfire = j;

        if(p->last_pissed_time <= (26*218) && p->show_empty_weapon == 0 && p->kickback_pic == 0 && p->quick_kick == 0 && getSprite(p->i)->_xrepeat > 32 && p->access_incs == 0 && p->knee_incs == 0 )
        {
            if(  ( p->weapon_pos == 0 || ( p->holster_weapon && p->weapon_pos == -9 ) ) )
            {
                if(j == 10 || j == 11)
                {
                    k = p->curr_weapon;
                    j = ( j == 10 ? -1 : 1 );
                    i = 0;

                    while( ( k >= 0 && k < 10 ) || ( k == GROW_WEAPON && (p->subweapon&(1<<GROW_WEAPON) ) ) )
                    {
                        if(k == GROW_WEAPON)
                        {
                            if(j == -1)
                                k = 5;
                            else k = 7;

                        }
                        else
                        {
                           k += j;
                           if( k == 6 && (p->subweapon&(1<<GROW_WEAPON)) )
                               k = GROW_WEAPON;
                        }

                        if(k == -1) k = 9;
                        else if(k == 10) k = 0;

                        if( p->gotweapon[k] && p->ammo_amount[k] > 0 )
                        {
                            if( k == SHRINKER_WEAPON && (p->subweapon&(1<<GROW_WEAPON)) )
                                k = GROW_WEAPON;
                            j = k;
                            break;
                        }
                        else
                            if(k == GROW_WEAPON && p->ammo_amount[GROW_WEAPON] == 0 && p->gotweapon[SHRINKER_WEAPON] && p->ammo_amount[SHRINKER_WEAPON] > 0)
                        {
                            j = SHRINKER_WEAPON;
                            p->subweapon &= ~(1<<GROW_WEAPON);
                            break;
                        }
                        else
                            if(k == SHRINKER_WEAPON && p->ammo_amount[SHRINKER_WEAPON] == 0 && p->gotweapon[SHRINKER_WEAPON] && p->ammo_amount[GROW_WEAPON] > 0)
                        {
                            j = GROW_WEAPON;
                            p->subweapon |= (1<<GROW_WEAPON);
                            break;
                        }

                        i++;
                        if(i == 10)
                        {
                            addweapon( p, KNEE_WEAPON );
                            break;
                        }
                    }
                }

                k = -1;


                if( j == HANDBOMB_WEAPON && p->ammo_amount[HANDBOMB_WEAPON] == 0 )
                {
                    k = headspritestat[1];
                    while(k >= 0)
                    {
                        if( getSprite(k)->_picnum == HEAVYHBOMB && getSprite(k)->_owner == p->i )
                        {
                            p->gotweapon[HANDBOMB_WEAPON] = 1;
                            j = HANDREMOTE_WEAPON;
                            break;
                        }
                        k = spr_add_data[k].nextspritestat;
                    }
                }

                if(j == SHRINKER_WEAPON)
                {
                    if(screenpeek == snum) pus = NUMPAGES;

                    if( p->curr_weapon != GROW_WEAPON && p->curr_weapon != SHRINKER_WEAPON )
                    {
                        if( p->ammo_amount[GROW_WEAPON] > 0 )
                        {
                            if( (p->subweapon&(1<<GROW_WEAPON)) == (1<<GROW_WEAPON) )
                                j = GROW_WEAPON;
                            else if(p->ammo_amount[SHRINKER_WEAPON] == 0)
                            {
                                j = GROW_WEAPON;
                                p->subweapon |= (1<<GROW_WEAPON);
                            }
                        }
                        else if( p->ammo_amount[SHRINKER_WEAPON] > 0 )
                            p->subweapon &= ~(1<<GROW_WEAPON);
                    }
                    else if( p->curr_weapon == SHRINKER_WEAPON )
                    {
                        p->subweapon |= (1<<GROW_WEAPON);
                        j = GROW_WEAPON;
                    }
                    else
                        p->subweapon &= ~(1<<GROW_WEAPON);
                }

                if(p->holster_weapon)
                {
                    sb_snum |= 1<<19;
                    p->weapon_pos = -9;
                }
                else if(j < MAX_WEAPONS && p->gotweapon[j] && p->curr_weapon != j ) switch(j)
                {
                    case KNEE_WEAPON:
                        addweapon( p, KNEE_WEAPON );
                        break;
                    case PISTOL_WEAPON:
                        if ( p->ammo_amount[PISTOL_WEAPON] == 0 )
                            if(p->show_empty_weapon == 0)
                        {
                            p->last_full_weapon = p->curr_weapon;
                            p->show_empty_weapon = 32;
                        }
                        addweapon( p, PISTOL_WEAPON );
                        break;
                    case SHOTGUN_WEAPON:
                        if( p->ammo_amount[SHOTGUN_WEAPON] == 0 && p->show_empty_weapon == 0)
                        {
                            p->last_full_weapon = p->curr_weapon;
                            p->show_empty_weapon = 32;
                        }
                        addweapon( p, SHOTGUN_WEAPON);
                        break;
                    case CHAINGUN_WEAPON:
                        if( p->ammo_amount[CHAINGUN_WEAPON] == 0 && p->show_empty_weapon == 0)
                        {
                            p->last_full_weapon = p->curr_weapon;
                            p->show_empty_weapon = 32;
                        }
                        addweapon( p, CHAINGUN_WEAPON);
                        break;
                    case RPG_WEAPON:
                        if( p->ammo_amount[RPG_WEAPON] == 0 )
                            if(p->show_empty_weapon == 0)
                        {
                            p->last_full_weapon = p->curr_weapon;
                            p->show_empty_weapon = 32;
                        }
                        addweapon( p, RPG_WEAPON );
                        break;
                    case DEVISTATOR_WEAPON:
                        if( p->ammo_amount[DEVISTATOR_WEAPON] == 0 && p->show_empty_weapon == 0 )
                        {
                            p->last_full_weapon = p->curr_weapon;
                            p->show_empty_weapon = 32;
                        }
                        addweapon( p, DEVISTATOR_WEAPON );
                        break;
                    case FREEZE_WEAPON:
                        if( p->ammo_amount[FREEZE_WEAPON] == 0 && p->show_empty_weapon == 0)
                        {
                            p->last_full_weapon = p->curr_weapon;
                            p->show_empty_weapon = 32;
                        }
                        addweapon( p, FREEZE_WEAPON );
                        break;
                    case GROW_WEAPON:
                    case SHRINKER_WEAPON:

                        if( p->ammo_amount[j] == 0 && p->show_empty_weapon == 0)
                        {
                            p->show_empty_weapon = 32;
                            p->last_full_weapon = p->curr_weapon;
                        }

                        addweapon(p, j);
                        break;
                    case HANDREMOTE_WEAPON:
                        if(k >= 0) // Found in list of [1]'s
                        {
                            p->curr_weapon = HANDREMOTE_WEAPON;
                            p->last_weapon = -1;
                            p->weapon_pos = 10;
                        }
                        break;
                    case HANDBOMB_WEAPON:
                        if( p->ammo_amount[HANDBOMB_WEAPON] > 0 && p->gotweapon[HANDBOMB_WEAPON] )
                            addweapon( p, HANDBOMB_WEAPON );
                        break;
                    case TRIPBOMB_WEAPON:
                        if( p->ammo_amount[TRIPBOMB_WEAPON] > 0 && p->gotweapon[TRIPBOMB_WEAPON] )
                            addweapon( p, TRIPBOMB_WEAPON );
                        break;
                }
            }

            if( sb_snum&(1<<19) )
			{
                if( p->curr_weapon > KNEE_WEAPON )
                {
                    if(p->holster_weapon == 0 && p->weapon_pos == 0)
                    {
                        p->holster_weapon = 1;
                        p->weapon_pos = -1;
                        FTA(73,p,1);
                    }
                    else if(p->holster_weapon == 1 && p->weapon_pos == -9)
                    {
                        p->holster_weapon = 0;
                        p->weapon_pos = 10;
                        FTA(74,p,1);
                    }
                }
            }
        }

        if( (sb_snum&(1<<24)) && p->newowner == -1 )
        {
            if( p->holoduke_on == -1 )
            {

                if( p->holoduke_amount > 0 )
                {
                    p->inven_icon = 3;

                    p->holoduke_on = i =
                        EGS(p->cursectnum,
                        p->posx,
                        p->posy,
                        p->posz+(30<<8),APLAYER,-64,0,0,p->ang,0,0,-1,10);
                    setHittypeTemp(i, 3, 0);
                    setHittypeTemp(i, 4, 0);
                    setSprite_yvel(i, snum);
                    setSprite_extra(i, 0);
                    FTA(47,p,0);
                }
                else FTA(49,p,0);
                spritesound(TELEPORTER,p->holoduke_on);

            }
            else
            {
                spritesound(TELEPORTER,p->holoduke_on);
                p->holoduke_on = -1;
                FTA(48,p,0);
            }
        }

        if( sb_snum&(1<<16) )
        {
            if( p->firstaid_amount > 0 && getSprite(p->i)->_extra < max_player_health )
            {
                j = max_player_health-getSprite(p->i)->_extra;

                if(p->firstaid_amount > j)
                {
                    p->firstaid_amount -= j;
                    setSprite_extra(p->i, max_player_health);
                    p->inven_icon = 1;
                }
                else
                {
                    setSprite_extra(p->i, getSprite(p->i)->_extra + p->firstaid_amount);
                    p->firstaid_amount = 0;
                    checkavailinven(p);
                }
                spritesound(DUKE_USEMEDKIT,p->i);
            }
        }

        if( (sb_snum&(1<<25)) && p->newowner == -1)
        {
            if( p->jetpack_amount > 0 )
            {
                p->jetpack_on = !p->jetpack_on;
                if(p->jetpack_on)
                {
                    p->inven_icon = 4;
                    if(p->scream_voice > FX_Ok)
                    {
                        FX_StopSound(p->scream_voice);
                        testcallback(DUKE_SCREAM);
                        p->scream_voice = FX_Ok;
                    }

                    spritesound(DUKE_JETPACK_ON,p->i);

                    FTA(52,p,0);
                }
                else
                {
                    p->hard_landing = 0;
                    p->poszv = 0;
                    spritesound(DUKE_JETPACK_OFF,p->i);
                    stopsound(DUKE_JETPACK_IDLE);
                    stopsound(DUKE_JETPACK_ON);
                    FTA(53,p,0);
                }
            }
            else FTA(50,p,0);
        }

        if(sb_snum&(1<<28) && p->one_eighty_count == 0)
            p->one_eighty_count = -1024;
    }
}

void checksectors(short snum)
{
    int32_t i = -1,oldz;
    struct player_struct *p;
    short j,hitscanwall;

    p = &ps[snum];

    switch(getSector(p->cursectnum)->lotag)
    {

        case 32767:
            setSector_lotag(p->cursectnum, 0);
            FTA(9,p,0); // secret place found
            p->secret_rooms++;
            return;
        case -1:
            for(i=connecthead;i>=0;i=connectpoint2[i])
                ps[i].gm = MODE_EOL;
            setSector_lotag(p->cursectnum, 0);
            if(ud.from_bonus)
            {
                ud.level_number = ud.from_bonus;
                ud.m_level_number = ud.level_number;
                ud.from_bonus = 0;
            }
            else
            {
                ud.level_number++;
                if( (ud.volume_number && ud.level_number > 10 ) || ud.level_number > 5 )
                    ud.level_number = 0;
                ud.m_level_number = ud.level_number;
            }
            return;
        case -2:
            setSector_lotag(p->cursectnum, 0);
            p->timebeforeexit = 26*8;
            p->customexitsound = getSector(p->cursectnum)->hitag;
            return;
        default:
            if(getSector(p->cursectnum)->lotag >= 10000 && getSector(p->cursectnum)->lotag < 16383)
            {
                if(snum == screenpeek || ud.coop == 1 )
                    spritesound(getSector(p->cursectnum)->lotag-10000,p->i);
                setSector_lotag(p->cursectnum, 0);
            }
            break;

    }

    //After this point the the player effects the map with space

    if((p->gm&MODE_TYPE) || getSprite(p->i)->_extra <= 0) return;

    if( ud.cashman && sync[snum].bits&(1<<29) )
        lotsofmoney(getSprite(p->i),2);

    if(p->newowner >= 0)
    {
        if( klabs(sync[snum].svel) > 768 || klabs(sync[snum].fvel) > 768 )
        {
            i = -1;
            goto CLEARCAMERAS;
        }
    }

    if( !(sync[snum].bits&(1<<29)) && !(sync[snum].bits&(1<<31)))
        p->toggle_key_flag = 0;

    else if(!p->toggle_key_flag)
    {

        if( (sync[snum].bits&(1<<31)) )
        {
            if( p->newowner >= 0 )
            {
                i = -1;
                goto CLEARCAMERAS;
            }
            return;
        }

        neartagsprite = -1;
        p->toggle_key_flag = 1;
        hitscanwall = -1;

        i = hitawall(p,&hitscanwall);

        if(i < 1280 && hitscanwall >= 0 && getWall_overpicnum(hitscanwall) == MIRROR)
        {
            int lotag = getWall_lotag(hitscanwall);
//            if(  lotag > 0 && Sound[lotag].num == 0 && snum == screenpeek)
            if(  lotag > 0 && getSoundNum(lotag) == 0 && snum == screenpeek)
            {
                spritesound(lotag,p->i);
                return;
            }
        }

        if(hitscanwall >= 0 && (getWall_cstat(hitscanwall)&16) )
            switch(getWall_overpicnum(hitscanwall))
        {
            default:
                if(getWall_lotag(hitscanwall))
                    return;
        }

        if(p->newowner >= 0)
            neartag(p->oposx,p->oposy,p->oposz,getSprite(p->i)->_sectnum,p->oang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,1280L,1);
        else
        {
            neartag(p->posx,p->posy,p->posz,getSprite(p->i)->_sectnum,p->oang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,1280L,1);
            if(neartagsprite == -1 && neartagwall == -1 && neartagsector == -1)
                neartag(p->posx,p->posy,p->posz+(8<<8),getSprite(p->i)->_sectnum,p->oang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,1280L,1);
            if(neartagsprite == -1 && neartagwall == -1 && neartagsector == -1)
                neartag(p->posx,p->posy,p->posz+(16<<8),getSprite(p->i)->_sectnum,p->oang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,1280L,1);
            if(neartagsprite == -1 && neartagwall == -1 && neartagsector == -1)
            {
                neartag(p->posx,p->posy,p->posz+(16<<8),getSprite(p->i)->_sectnum,p->oang,&neartagsector,&neartagwall,&neartagsprite,&neartaghitdist,1280L,3);
                if(neartagsprite >= 0)
                {
                    switch(getSprite(neartagsprite)->_picnum)
                    {
                        case FEM1:
                        case FEM2:
                        case FEM3:
                        case FEM4:
                        case FEM5:
                        case FEM6:
                        case FEM7:
                        case FEM8:
                        case FEM9:
                        case FEM10:
                        case PODFEM1:
                        case NAKED1:
                        case STATUE:
                        case TOUGHGAL:
                            return;
                    }
                }

                neartagsprite = -1;
                neartagwall = -1;
                neartagsector = -1;
            }
        }

        if(p->newowner == -1 && neartagsprite == -1 && neartagsector == -1 && neartagwall == -1 )
            if( isanunderoperator(getSector(getSprite(p->i)->_sectnum)->lotag) )
                neartagsector = getSprite(p->i)->_sectnum;

        if( neartagsector >= 0 && (getSector(neartagsector)->lotag&16384) )
            return;

        if( neartagsprite == -1 && neartagwall == -1)
            if(getSector(p->cursectnum)->lotag == 2 )
            {
                oldz = hitasprite(p->i,&neartagsprite);
                if(oldz > 1280) neartagsprite = -1;
            }

        if(neartagsprite >= 0)
        {
            if( checkhitswitch(snum,neartagsprite,1) ) return;

            switch(getSprite(neartagsprite)->_picnum)
            {
                case TOILET:
                case STALL:
                    if(p->last_pissed_time == 0)
                    {
                        if(ud.lockout == 0) spritesound(DUKE_URINATE,p->i);

                        p->last_pissed_time = 26*220;
                        p->transporter_hold = 29*2;
                        if(p->holster_weapon == 0)
                        {
                            p->holster_weapon = 1;
                            p->weapon_pos = -1;
                        }
                        if(getSprite(p->i)->_extra <= (max_player_health-(max_player_health/10) ) )
                        {
                            setSprite_extra(p->i, getSprite(p->i)->_extra + max_player_health/10);
                            p->last_extra = getSprite(p->i)->_extra;
                        }
                        else if(getSprite(p->i)->_extra < max_player_health )
                             setSprite_extra(p->i, max_player_health);
                    }
//                    else if(Sound[FLUSH_TOILET].num == 0)
                    else if(getSoundNum(FLUSH_TOILET) == 0)
                        spritesound(FLUSH_TOILET,p->i);
                    return;

                case NUKEBUTTON:

                    hitawall(p,&j);
                    if(j >= 0 && getWall_overpicnum(j) == 0)
                        if(getHittypeTemp(neartagsprite, 0) == 0)
                    {
                        setHittypeTemp(neartagsprite, 0, 1);
                        setSprite_owner(neartagsprite, p->i);
                        p->buttonpalette = getSprite(neartagsprite)->_pal;
                        if(p->buttonpalette)
                            ud.secretlevel = getSprite(neartagsprite)->_lotag;
                        else ud.secretlevel = 0;
                    }
                    return;
                case WATERFOUNTAIN:
                    if(getHittypeTemp(neartagsprite, 0) != 1)
                    {
                        setHittypeTemp(neartagsprite, 0, 1);
                        setSprite_owner(neartagsprite, p->i);

                        if(getSprite(p->i)->_extra < max_player_health)
                        {
                            setSprite_extra(p->i, getSprite(p->i)->_extra + 1);
                            spritesound(DUKE_DRINKING,p->i);
                        }
                    }
                    return;
                case PLUG:
                    spritesound(SHORT_CIRCUIT,p->i);
                    setSprite_extra(p->i, getSprite(p->i)->_extra - (2+(TRAND&3)));
                    p->pals[0] = 48;
                    p->pals[1] = 48;
                    p->pals[2] = 64;
                    p->pals_time = 32;
                    break;
                case VIEWSCREEN:
                case VIEWSCREEN2:
                    {
                        i = headspritestat[1];

                        while(i >= 0)
                        {
                            if( PN == CAMERA1 && SP == 0 && getSprite(neartagsprite)->_hitag == SLT )
                            {
                                setSprite_yvel(i, 1); //Using this camera
                                spritesound(MONITOR_ACTIVE,neartagsprite);

                                setSprite_owner(neartagsprite, i);
                                setSprite_yvel(neartagsprite, 1);


                                j = p->cursectnum;
                                p->cursectnum = SECT;
                                setpal(p);
                                p->cursectnum = j;

                                // parallaxtype = 2;
                                p->newowner = i;
                                return;
                            }
                            i = spr_add_data[i].nextspritestat;
                       }
                    }

                    CLEARCAMERAS:

                    if(i < 0)
                    {
                        p->posx = p->oposx;
                        p->posy = p->oposy;
                        p->posz = p->oposz;
                        p->ang = p->oang;
                        p->newowner = -1;

                        updatesector(p->posx,p->posy,&p->cursectnum);
                        setpal(p);


                        i = headspritestat[1];
                        while(i >= 0)
                        {
                            if(PN==CAMERA1) setSprite_yvel(i, 0);
                            i = spr_add_data[i].nextspritestat;
                        }
                    }
                    else if(p->newowner >= 0)
                        p->newowner = -1;

                    if( KB_KeyPressed(sc_Escape) )
                        KB_ClearKeyDown(sc_Escape);

                    return;
            }
        }

        if( (sync[snum].bits&(1<<29)) == 0 ) return;
        else if(p->newowner >= 0) { i = -1; goto CLEARCAMERAS; }

        if(neartagwall == -1 && neartagsector == -1 && neartagsprite == -1)
            if( klabs(hits(p->i)) < 512 )
        {
            if( (TRAND&255) < 16 )
                spritesound(DUKE_SEARCH2,p->i);
            else spritesound(DUKE_SEARCH,p->i);
            return;
        }

        if( neartagwall >= 0 )
        {
            if( getWall_lotag(neartagwall) > 0 && isadoorwall(getWall_picnum(neartagwall)) )
            {
                if(hitscanwall == neartagwall || hitscanwall == -1)
                    checkhitswitch(snum,neartagwall,0);
                return;
            }
            else if(p->newowner >= 0)
            {
                i = -1;
                goto CLEARCAMERAS;
            }
        }

        if( neartagsector >= 0 && (getSector(neartagsector)->lotag&16384) == 0 && isanearoperator(getSector(neartagsector)->lotag) )
        {
            i = headspritesect[neartagsector];
            while(i >= 0)
            {
                if( PN == ACTIVATOR || PN == MASTERSWITCH )
                    return;
                i = spr_add_data[i].nextspritesect;
            }
            operatesectors(neartagsector,p->i);
        }
        else if( (getSector(getSprite(p->i)->_sectnum)->lotag&16384) == 0 )
        {
            if( isanunderoperator(getSector(getSprite(p->i)->_sectnum)->lotag) )
            {
                i = headspritesect[getSprite(p->i)->_sectnum];
                while(i >= 0)
                {
                    if(PN == ACTIVATOR || PN == MASTERSWITCH) return;
                    i = spr_add_data[i].nextspritesect;
                }
                operatesectors(getSprite(p->i)->_sectnum,p->i);
            }
            else checkhitswitch(snum,neartagwall,0);
        }
    }
}



