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

#include "duke3d.h"
#include "audiolib/fx_man.h"
#include "engine.h"
#include "soundefs.h"
#include "sounds.h"
#ifdef WIN32
#include <stdio.h>
#else
#include <printf.h>
#endif
#include "audiolib/multivoc.h"

extern int32_t numenvsnds;
extern uint8_t  actor_tog;
#define INTERPOLATION_STATE_NONE        0
#define INTERPOLATION_STATE_UPDATED     1
#define INTERPOLATION_STATE_DONE        2
#define INTERPOLATION_STATE_RESTORED    3
uint8_t interpolationState = INTERPOLATION_STATE_NONE;
int32_t interpolationSmoothRatio = 0;
// next-hack: How did the interpolation work:
// the api "setInterpolation" will store a pointer to the variable being interpolated. This will be the interpolation
// identifier. We are not using the pointer, but a particular index value, so that we use only 16 bits, but it does not
// change.
// After all the interpolations have been setup:
// The API updateinterpolations, called before actually moving things, will save the current variables value into
// oldipos array. NOTE! THIS IS NOT CALLED BEFORE EVERY FRAME because for each gametic we might be running much faster
// so we render more times. This basically holds the value of the before the new gametic is calculated.
//
// Then the game logic will calculate the NEW value of the variable it should have at the end of the gametic.
// We might be rendering faster than the supposed internal game logic update rate floor(120 / (120/ 26)) = 30
// so a fraction smoothfactor is calculated as how much time has elapsed vs the expected nominal update rate (1s/30 ).
// after everything has been moved, the API doInterpolations will calculate for each interpolated variable, an intermediate
// value between oldIpos and current variable. The current variable value is actually overwritten, and this would create
// garbage if that value would not be restored before next game logic frame is calculated. To prevent this, before actually
// changing the variable value, this is stored in a backup array.
// Now all the entities that needed to be interpolated have the intermediate value ready for smooth rendering, and they are rendered.
// After that, we need to restore the values to the pre-smooth, so we call restoreInterpolations() for this.
//
// This uses quite a large amount of RAM. What can we do:
// 1) Reduce number of interpolation.
// 2) Use only oldipos, the current position, and the interpolation state and last smooth factor.
// oldipos will store different things, based on the interpolation state.
// In particular, at the beginning of domovethings(), updateinterpolations() is called,
// copying in oldipos the current position (the final position is calculated at the end of
// domovethings()), and we set the state to UPDATED.
// Then, before drawing to screen, if the state is UPDATED, the current position holds the
// final position after the "gametic", while oldipos contains the position before the gametick.
// Because the frame rendering will not occur exactly at the gametic, an interpolation  must
// be done. For this the smoothfactor is calculated. and dointerpolations()
// is called. Here, if the state is UPDATED (i.e. called just after dointerpolation()), then
// we copy in oldipos the delta between current position and oldipos, i.e. the delta between
// the initial and final (i.e. if the smoothfactor would be 100%).
// After this, we calculate the new current position as: current position - delta * (1 - interpolation factor)
// (note this is the same as what the game did, i.e. oldipos + interpolationfactor * delta.
// We also set the interpolation state as "DONE".
// After everything is drawn, in currentposition we have the interpolated (and not final)
// position. We must restore it back (so that we can we use again the formula
// "current position - delta * (1 - interpolation factor)" in dointerpolations(), if
// we are running extremely fast and we render two frames before calling again domovethings().
// Therefore we calculate the final position and put currentposition by calling restoreinterpolations().
// which also sets the state to RESTORED. If a new frame is to be rendered, dointerpolation()
// will skip copying in oldipos the delta, because the delta is already calculated.
// The old code used a third variable (bakipos) so that currentipos is restored by simply
// copying bakipos to curipos.
void updateinterpolations()  //Stick at beginning of domovethings
{
   // printf("upd\r\n");
	int32_t i;
#if INTERPOLATION_USES_POSPTR
	for(i=numinterpolations-1;i>=0;i--) oldipos[i] = *curipos[i];
#else
	for(i=numinterpolations-1;i>=0;i--) oldipos[i] = getIndexedValue(i, curposidx);
    interpolationState = INTERPOLATION_STATE_UPDATED;
#endif
}

#if INTERPOLATION_USES_POSPTR
void setinterpolation(int32_t *posptr)
{

	int32_t i;
    // let's comment a bit.
    // start from end and if we already find the pointer, skip.
    // otherwise add at the end.
	if (numinterpolations >= MAXINTERPOLATIONS)
    {
        FIXME("Warning: too many interpolations");
        return;
    }
	for(i=numinterpolations-1;i>=0;i--)
		if (curipos[i] == posptr) return;
	curipos[numinterpolations] = posptr;
	oldipos[numinterpolations] = *posptr;
	numinterpolations++;
}
void stopinterpolation(int32_t *posptr)
{
	int32_t i;
    // How it works: it finds the interpolation we want to stop.
    // after the index is found, it replaces the interpolation with the
    // last one, and the number of total interpolations is reduced by 1.
	for(i=numinterpolations-1;i>=startofdynamicinterpolations;i--)
		if (curipos[i] == posptr)
		{
			numinterpolations--;
			oldipos[i] = oldipos[numinterpolations];
			bakipos[i] = bakipos[numinterpolations];
			curipos[i] = curipos[numinterpolations];
		}
}

#else
#if WIN32
void setinterpolation2(indexedValue_t iv, char *file, int line)
#else
void setinterpolation(indexedValue_t iv)
#endif
{
#if WIN32
//    printf("Set Interpolation called from file %s at line %d\r\n", file, line);
#endif
	int32_t i;
    // let's comment a bit.
    // start from end and if we already find the pointer, skip.
    // otherwise add at the end.
	if (numinterpolations >=  MAXINTERPOLATIONS)
    {
        #if WIN32
            WARNING("Warning: too many interpolations"); // just warn, but nothing more.
        #endif
        return;
    }
	for(i=numinterpolations-1;i>=0;i--)
    {
		if (curposidx[i].value == iv.value)
        {
            return;
        }
    }
	curposidx[numinterpolations].value = iv.value;
	oldipos[numinterpolations] = getIndexedValue(numinterpolations, curposidx);
	numinterpolations++;
}

void stopinterpolation(indexedValue_t iv)
{
	int32_t i;
    // How it works: it finds the interpolation we want to stop.
    // after the index is found, it replaces the interpolation with the
    // last one, and the number of total interpolations is reduced by 1.
	for(i=numinterpolations-1;i>=startofdynamicinterpolations;i--)
		if (curposidx[i].value == iv.value)
		{
			numinterpolations--;
			oldipos[i] = oldipos[numinterpolations];
#if INTERPOLATION_HAS_BAKIPOS
			bakipos[i] = bakipos[numinterpolations];
#endif
			curposidx[i].value = curposidx[numinterpolations].value;
		}
}
#endif

void dointerpolations(int32_t smoothratio)       //Stick at beginning of drawscreen
{
	int32_t i, j, odelta, ndelta;
#if !INTERPOLATION_HAS_BAKIPOS
    interpolationSmoothRatio = 65536 - smoothratio;
    if (interpolationState == INTERPOLATION_STATE_UPDATED)
    {
        // put in oldipos all the deltas instead
    	for(i=numinterpolations-1;i>=0;i--)
        {
            int32_t p = getIndexedValue(i, curposidx);
            oldipos[i] = p - oldipos[i];
        }
    }
#endif
	ndelta = 0; j = 0;
	for(i=numinterpolations-1;i>=0;i--)
	{
#if INTERPOLATION_USES_POSPTR
		bakipos[i] = *curipos[i];
		odelta = ndelta; ndelta = (*curipos[i])-oldipos[i];
		if (odelta != ndelta) j = mulscale16(ndelta,smoothratio);
		*curipos[i] = oldipos[i]+j;
#else
#if INTERPOLATION_HAS_BAKIPOS
        bakipos[i] = getIndexedValue(i, curposidx);
        // why this crap? to save on mulscale16 calls ?
		odelta = ndelta;
		ndelta = (bakipos[i])-oldipos[i];
		if (odelta != ndelta) j = mulscale16(ndelta,smoothratio);
		setIndexedValue(i, curposidx, oldipos[i]+j);
#else
		odelta = ndelta;
		ndelta = oldipos[i];
		if (odelta != ndelta)
		    j = mulscale16(ndelta,interpolationSmoothRatio);
        setIndexedValue(i, curposidx, getIndexedValue(i, curposidx) - j);
#endif

#endif
	}
	interpolationState = INTERPOLATION_STATE_DONE;
}

void restoreinterpolations()  //Stick at end of drawscreen
{
	int32_t i;
#if INTERPOLATION_USES_POSPTR
	for(i=numinterpolations-1;i>=0;i--) *curipos[i] = bakipos[i];
#else
#if INTERPOLATION_HAS_BAKIPOS
	for(i=numinterpolations-1;i>=0;i--)
	    setIndexedValue(i, curposidx, bakipos[i]);
#else
	if (interpolationState == INTERPOLATION_STATE_DONE)
    {
        // for sure this is called after dointerpolations(). Therefore, in oldipos we have the delta
        // not multiplied by smooth ratio.
        int odelta = 0, ndelta = 0;
        int step = 0;
        for(i=numinterpolations-1;i>=0;i--)
        {
            odelta = ndelta;
            ndelta = oldipos[i];
            // get how much we did increase the position in interpolation done
            if (odelta != ndelta)
                step = mulscale16(ndelta,interpolationSmoothRatio);
            setIndexedValue(i, curposidx, getIndexedValue(i, curposidx) + step);
        }
    }
    // other cases should not be covered.

	interpolationState = INTERPOLATION_STATE_RESTORED;

#endif
#endif
}

int32_t ceilingspace(int16_t sectnum)
{
    if( (getSector(sectnum)->ceilingstat&1) && getSector(sectnum)->ceilingpal == 0 )
    {
        switch(getSector(sectnum)->ceilingpicnum)
        {
            case MOONSKY1:
            case BIGORBIT1:
                return 1;
        }
    }
    return 0;
}

int32_t floorspace(int16_t sectnum)
{
    if( (getSector(sectnum)->floorstat&1) && getSector(sectnum)->ceilingpal == 0 )
    {
        switch(getSector(sectnum)->floorpicnum)
        {
            case MOONSKY1:
            case BIGORBIT1:
                return 1;
        }
    }
    return 0;
}

void addammo( int16_t weapon,struct player_struct *p,int16_t amount)
{
   p->ammo_amount[weapon] += amount;

   if( p->ammo_amount[weapon] > max_ammo_amount[weapon] )
        p->ammo_amount[weapon] = max_ammo_amount[weapon];
}

void addweapon( struct player_struct *p,int16_t weapon)
{
	int added_new_weapon = false;

    if ( p->gotweapon[weapon] == 0 )
    {
        p->gotweapon[weapon] = 1;
        if(weapon == SHRINKER_WEAPON)
            p->gotweapon[GROW_WEAPON] = 1;

	// FIX_00012: added "weapon autoswitch" toggle allowing to turn the autoswitch off
	// when picking up new weapons. The weapon sound on pickup will remain on, to not
	// affect the opponent's gameplay (so he can still hear you picking up new weapons)
  	if(p->weaponautoswitch)  // Anti antiswitch ordered
			added_new_weapon = true;
    }

	if (added_new_weapon==false ||
		ud.playing_demo_rev == BYTEVERSION_27     ||
		ud.playing_demo_rev == BYTEVERSION_28     ||
		ud.playing_demo_rev == BYTEVERSION_116    ||
		ud.playing_demo_rev == BYTEVERSION_117 )
		// don't block the weapon change on 1st pick up if playing an old demo
	{
		p->random_club_frame = 0;

		if(p->holster_weapon == 0)
		{
			p->weapon_pos = -1;
			p->last_weapon = p->curr_weapon;
		}
		else
		{
			p->weapon_pos = 10;
			p->holster_weapon = 0;
			p->last_weapon = -1;
		}

		p->kickback_pic = 0;
		p->curr_weapon = weapon;
	}

    switch(weapon)
    {
        case KNEE_WEAPON:
        case TRIPBOMB_WEAPON:
        case HANDREMOTE_WEAPON:
        case HANDBOMB_WEAPON:     break;
        case SHOTGUN_WEAPON:      spritesound(SHOTGUN_COCK,p->i);break;
        case PISTOL_WEAPON:       spritesound(INSERT_CLIP,p->i);break;
                    default:      spritesound(SELECT_WEAPON,p->i);break;
    }

	vscrn(); // FIX_00056: Refresh issue w/FPS, small Weapon and custom FTA, when screen resized down

}

void checkavailinven( struct player_struct *p )
{

    if(p->firstaid_amount > 0)
        p->inven_icon = 1;
    else if(p->steroids_amount > 0)
        p->inven_icon = 2;
    else if(p->holoduke_amount > 0)
        p->inven_icon = 3;
    else if(p->jetpack_amount > 0)
        p->inven_icon = 4;
    else if(p->heat_amount > 0)
        p->inven_icon = 5;
    else if(p->scuba_amount > 0)
        p->inven_icon = 6;
    else if(p->boot_amount > 0)
        p->inven_icon = 7;
    else p->inven_icon = 0;
}

void checkavailweapon( struct player_struct *p )
{
    short i,snum;
    int32_t weap;

    if(p->wantweaponfire >= 0)
    {
        weap = p->wantweaponfire;
        p->wantweaponfire = -1;

        if(weap == p->curr_weapon) return;
        else if( p->gotweapon[weap] && p->ammo_amount[weap] > 0 )
        {
            addweapon(p,weap);
            return;
        }
    }

    weap = p->curr_weapon;
    if( p->gotweapon[weap] && p->ammo_amount[weap] > 0 )
        return;

    snum = getSprite_yvel(p->i);

    for(i=0;i<10;i++)
    {
        weap = ud.wchoice[snum][i];
		if (VOLUMEONE)
			if(weap > 6) continue;

        if(weap == 0) weap = 9;
        else weap--;

        if( weap == 0 || ( p->gotweapon[weap] && p->ammo_amount[weap] > 0 ) )
            break;
    }

    if(i == 10) weap = 0;

    // Found the weapon

    p->last_weapon  = p->curr_weapon;
    p->random_club_frame = 0;
    p->curr_weapon  = weap;
    p->kickback_pic = 0;
    if(p->holster_weapon == 1)
    {
        p->holster_weapon = 0;
        p->weapon_pos = 10;
    }
    else p->weapon_pos   = -1;
}

 /*
void checkavailweapon( struct player_struct *p )
{
    short i,okay,check_shoot,check_bombs;

    if(p->ammo_amount[p->curr_weapon] > 0) return;
    okay = check_shoot = check_bombs = 0;

    switch(p->curr_weapon)
    {
        case PISTOL_WEAPON:
        case CHAINGUN_WEAPON:
        case SHOTGUN_WEAPON:
#ifndef VOLUMEONE
        case FREEZE_WEAPON:
        case DEVISTATOR_WEAPON:
        case SHRINKER_WEAPON:
        case GROW_WEAPON:
#endif
        case RPG_WEAPON:
        case KNEE_WEAPON:
            check_shoot = 1;
            break;
        case HANDBOMB_WEAPON:
        case HANDREMOTE_WEAPON:
#ifndef VOLUMEONE
        case TRIPBOMB_WEAPON:
#endif
            check_bombs = 1;
            break;
    }

    CHECK_SHOOT:
    if(check_shoot)
    {
        for(i = p->curr_weapon+1; i < MAX_WEAPONS;i++)
            switch(i)
            {
                case PISTOL_WEAPON:
                case CHAINGUN_WEAPON:
                case SHOTGUN_WEAPON:
#ifndef VOLUMEONE
                case FREEZE_WEAPON:
                case SHRINKER_WEAPON:
                case GROW_WEAPON:
                case DEVISTATOR_WEAPON:
#endif
                    if ( p->gotweapon[i] && p->ammo_amount[i] > 0 )
                    {
                        okay = i;
                        goto OKAY_HERE;
                    }
                    break;
            }

        for(i = p->curr_weapon-1; i > 0;i--)
            switch(i)
            {
                case PISTOL_WEAPON:
                case CHAINGUN_WEAPON:
                case SHOTGUN_WEAPON:
#ifndef VOLUMEONE
                case FREEZE_WEAPON:
                case DEVISTATOR_WEAPON:
                case SHRINKER_WEAPON:
                case GROW_WEAPON:
#endif
                    if ( p->gotweapon[i] && p->ammo_amount[i] > 0 )
                    {
                        okay = i;
                        goto OKAY_HERE;
                    }
                    break;
            }

        if( p->gotweapon[RPG_WEAPON] && p->ammo_amount[RPG_WEAPON] > 0 )
        {
            okay = RPG_WEAPON;
            goto OKAY_HERE;
        }

        if(check_bombs == 0)
            check_bombs = 1;
        else
        {
            addweapon(p,KNEE_WEAPON);
            return;
        }
    }

    if(check_bombs)
    {
        for(i = p->curr_weapon-1; i > 0;i--)
            switch(i)
            {
                case HANDBOMB_WEAPON:
#ifndef VOLUMEONE
                case TRIPBOMB_WEAPON:
#endif
                    if ( p->gotweapon[i] && p->ammo_amount[i] > 0 )
                    {
                        okay = i;
                        goto OKAY_HERE;
                    }
                    break;
            }

        for(i = p->curr_weapon+1; i < MAX_WEAPONS;i++)
            switch(i)
            {
                case HANDBOMB_WEAPON:
#ifdef VOLUMEONE
                case TRIPBOMB_WEAPON:
#endif
                    if ( p->gotweapon[i] && p->ammo_amount[i] > 0 )
                    {
                        okay = i;
                        goto OKAY_HERE;
                    }
                    break;
            }

        if(check_shoot == 0)
        {
            check_shoot = 1;
            goto CHECK_SHOOT;
        }
        else
        {
            addweapon(p,KNEE_WEAPON);
            return;
        }
    }

    OKAY_HERE:

    if(okay)
    {
        p->last_weapon  = p->curr_weapon;
        p->random_club_frame = 0;
        p->curr_weapon  = okay;
        p->kickback_pic = 0;
        if(p->holster_weapon == 1)
        {
            p->holster_weapon = 0;
            p->weapon_pos = 10;
        }
        else p->weapon_pos   = -1;
        return;
    }
}
   */

int32_t ifsquished(short i, short p)
{
    const sectortype *sc;
    uint8_t  squishme;
    int32_t floorceildist;

    if(PN == APLAYER && ud.clipping)
        return 0;

    sc = getSector(SECT);
    floorceildist = sc->floorz - sc->ceilingz;

    if(sc->lotag != 23)
    {
        if(getSprite(i)->_pal == 1)
            squishme = floorceildist < (32<<8) && (sc->lotag&32768) == 0;
        else
            squishme = floorceildist < (12<<8); // && (sc->lotag&32768) == 0;
    }
    else squishme = 0;

    if( squishme )
    {
        FTA(10,&ps[p],0);

        if(badguy(getSprite(i)))
           setSprite_xvel(i, 0);

        if(getSprite_pal(i) == 1)
        {
            setHittype_picnum(i,  SHOTSPARK1);
            setHittype_extra(i, 1);
            return 0;
        }

        return 1;
    }
    return 0;
}

void hitradius( short i, int32_t  r, int32_t  hp1, int32_t  hp2, int32_t  hp3, int32_t  hp4 )
{
//    const spritetype *getSprite(i);
//    const spritetype *sj;
    walltype *wal;
    int32_t d, q, x1, y1;
    int32_t sectcnt, sectend, dasect, startwall, endwall, nextsect;
    short j,k,p,x,nextj,sect;
    uint8_t  statlist[] = {0,1,6,10,12,2,5};
    short tempshort[1024];  // next-hack: probably could be much lower, as we have only MAXSECTORS < 1024. But here stack is not under stress.


    if(getSprite(i)->_picnum == RPG && getSprite(i)->_xrepeat < 11) goto SKIPWALLCHECK;

    if(getSprite(i)->_picnum != SHRINKSPARK)
    {
        tempshort[0] = getSprite(i)->_sectnum;
        dasect = getSprite(i)->_sectnum;
        sectcnt = 0; sectend = 1;

        do
        {
            dasect = tempshort[sectcnt++];
            if(((getSector(dasect)->ceilingz-getSprite(i)->_z)>>8) < r)
            {
                int sec_wallptr = constsector[dasect].wallptr;
               d = klabs(getWall(sec_wallptr)->x-getSprite(i)->_x)+klabs(getWall(sec_wallptr)->y-getSprite(i)->_y);
               if(d < r)
                    checkhitceiling(dasect);
               else
               {
                    d = klabs(getWall(getWall_point2(getWall_point2(sec_wallptr)))->x-getSprite(i)->_x)+klabs(getWall(getWall_point2(getWall_point2(sec_wallptr)))->y-getSprite(i)->_y);
                    if(d < r)
                        checkhitceiling(dasect);
               }
           }

           startwall = constsector[dasect].wallptr;
           endwall = startwall+ constsector[dasect].wallnum;
           for(x=startwall,wal=getWall(startwall);x<endwall;x++,wal = getWall(x))
               if( ( klabs(wal->x-getSprite(i)->_x)+klabs(wal->y-getSprite(i)->_y) ) < r)
           {
               nextsect = constwall[x].nextsector;
               if (nextsect >= 0)
               {
                   for(dasect=sectend-1;dasect>=0;dasect--)
                       if (tempshort[dasect] == nextsect) break;
                   if (dasect < 0) tempshort[sectend++] = nextsect;
               }
               x1 = (((wal->x+getWall(getWall_point2(x))->x)>>1)+getSprite(i)->_x)>>1;
               y1 = (((wal->y+getWall(getWall_point2(x))->y)>>1)+getSprite(i)->_y)>>1;
               updatesector(x1,y1,&sect);
               if( sect >= 0 && cansee(x1,y1,getSprite(i)->_z,sect,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z,getSprite(i)->_sectnum ) )
                   checkhitwall(i,x,wal->x,wal->y,getSprite(i)->_z,getSprite(i)->_picnum);
           }
        }
        while (sectcnt < sectend);
    }

    SKIPWALLCHECK:

    q = -(16<<8)+(TRAND&((32<<8)-1));

    for(x = 0;x<7;x++)
    {
        j = headspritestat[statlist[x]];
        while(j >= 0)
        {
            nextj = spr_add_data[j].nextspritestat;
            if( x == 0 || x >= 5 || AFLAMABLE(getSprite(j)->_picnum) )
            {
                if( getSprite(i)->_picnum != SHRINKSPARK || (getSprite(j)->_cstat&257) )
                    if( dist( getSprite(i), getSprite(j) ) < r )
                    {
                        if( badguy(getSprite(j)) && !cansee( getSprite(j)->_x, getSprite(j)->_y,getSprite(j)->_z+q, getSprite(j)->_sectnum, getSprite(i)->_x, getSprite(i)->_y, getSprite(i)->_z+q, getSprite(i)->_sectnum) )
                            goto BOLT;
                        checkhitsprite( j, i );
                    }
            }
            else if( getSprite(j)->_extra >= 0 && getSprite(j) != getSprite(i) && ( getSprite(j)->_picnum == TRIPBOMB || badguy(getSprite(j)) || getSprite(j)->_picnum == QUEBALL || getSprite(j)->_picnum == STRIPEBALL || (getSprite(j)->_cstat&257) || getSprite(j)->_picnum == DUKELYINGDEAD ) )
            {
                if( getSprite(i)->_picnum == SHRINKSPARK && getSprite(j)->_picnum != SHARK && ( j == getSprite(i)->_owner || getSprite(j)->_xrepeat < 24 ) )
                {
                    j = nextj;
                    continue;
                }
                if( getSprite(i)->_picnum == MORTER && j == getSprite(i)->_owner)
                {
                    j = nextj;
                    continue;
                }

                if(getSprite(j)->_picnum == APLAYER)
                    setSprite_z(j, getSprite(j)->_z - PHEIGHT);
                d = dist( getSprite(i), getSprite(j) );
                if(getSprite(j)->_picnum == APLAYER)
                    setSprite_z(j, getSprite(j)->_z + PHEIGHT);

                if ( d < r && cansee( getSprite(j)->_x, getSprite(j)->_y, getSprite(j)->_z-(8<<8), getSprite(j)->_sectnum, getSprite(i)->_x, getSprite(i)->_y, getSprite(i)->_z-(12<<8), getSprite(i)->_sectnum) )
                {
                    setHittype_ang(j, getangle(getSprite(j)->_x-getSprite(i)->_x,getSprite(j)->_y-getSprite(i)->_y));

                    if ( getSprite(i)->_picnum == RPG && getSprite(j)->_extra > 0)
                        setHittype_picnum(j, RPG);
                    else
                    {
                        if( getSprite(i)->_picnum == SHRINKSPARK )
                            setHittype_picnum(j, SHRINKSPARK);
                        else setHittype_picnum(j, RADIUSEXPLOSION);
                    }

                    if(getSprite(i)->_picnum != SHRINKSPARK)
                    {
                        if ( d < r/3 )
                        {
                            if(hp4 == hp3) hp4++;
                            setHittype_extra(j, hp3 + (TRAND%(hp4-hp3)));
                        }
                        else if ( d < 2*r/3 )
                        {
                            if(hp3 == hp2) hp3++;
                            setHittype_extra(j,  hp2 + (TRAND%(hp3-hp2)));
                        }
                        else if ( d < r )
                        {
                            if(hp2 == hp1) hp2++;
                            setHittype_extra(j,  hp1 + (TRAND%(hp2-hp1)));
                        }

                        if( getSprite(j)->_picnum != TANK && getSprite(j)->_picnum != ROTATEGUN &&
                           getSprite(j)->_picnum != RECON && getSprite(j)->_picnum != BOSS1 &&
                           getSprite(j)->_picnum != BOSS2 && getSprite(j)->_picnum != BOSS3 && getSprite(j)->_picnum != BOSS4 )
                        {
                            if(getSprite(j)->_xvel < 0)
                                setSprite_xvel(j, 0);
                            setSprite_xvel(j, getSprite(j)->_xvel + (getSprite(i)->_extra<<2));
                        }

                        if( getSprite(j)->_picnum == PODFEM1 || getSprite(j)->_picnum == FEM1 ||
                            getSprite(j)->_picnum == FEM2 || getSprite(j)->_picnum == FEM3 ||
                            getSprite(j)->_picnum == FEM4 || getSprite(j)->_picnum == FEM5 ||
                            getSprite(j)->_picnum == FEM6 || getSprite(j)->_picnum == FEM7 ||
                            getSprite(j)->_picnum == FEM8 || getSprite(j)->_picnum == FEM9 ||
                            getSprite(j)->_picnum == FEM10 || getSprite(j)->_picnum == STATUE ||
                            getSprite(j)->_picnum == STATUEFLASH || getSprite(j)->_picnum == SPACEMARINE || getSprite(j)->_picnum == QUEBALL || getSprite(j)->_picnum == STRIPEBALL)
                                checkhitsprite( j, i );
                    }
                    else if(getSprite(i)->_extra == 0) setHittype_extra(j,  0);

                    if ( getSprite(j)->_picnum != RADIUSEXPLOSION &&
                        getSprite(i)->_owner >= 0 && getSprite(getSprite(i)->_owner)->_statnum < MAXSTATUS )
                    {
                        if(getSprite(j)->_picnum == APLAYER)
                        {
                            p = getSprite(j)->_yvel;
                            if(ps[p].newowner >= 0)
                            {
                                ps[p].newowner = -1;
                                ps[p].posx = ps[p].oposx;
                                ps[p].posy = ps[p].oposy;
                                ps[p].posz = ps[p].oposz;
                                ps[p].ang = ps[p].oang;
                                updatesector(ps[p].posx,ps[p].posy,&ps[p].cursectnum);
                                setpal(&ps[p]);

                                k = headspritestat[1];
                                while(k >= 0)
                                {
                                    if(getSprite(k)->_picnum==CAMERA1)
                                        setSprite_yvel(k, 0);
                                    k = spr_add_data[k].nextspritestat;
                                }
                            }
                        }
                        setHittype_owner(j, getSprite(i)->_owner);
                    }
                }
            }
            BOLT:
            j = nextj;
        }
    }
}


int movesprite(short spritenum, int32_t xchange, int32_t ychange, int32_t zchange, uint32_t cliptype)
{
    int32_t daz,h, oldx, oldy;
    short retval, dasectnum, cd;
    uint8_t  bg;

    bg = badguy(getSprite(spritenum));

    if(getSprite(spritenum)->_statnum == 5 || (bg && getSprite(spritenum)->_xrepeat < 4 ) )
    {
        setSprite_x(spritenum, getSprite_x(spritenum) + ((xchange*TICSPERFRAME)>>2));
        setSprite_y(spritenum, getSprite_y(spritenum) + ((ychange*TICSPERFRAME)>>2));
        setSprite_z(spritenum, getSprite_z(spritenum) + ((zchange*TICSPERFRAME)>>2));
        if(bg)
            setsprite(spritenum,getSprite_x(spritenum),getSprite_y(spritenum),getSprite_z(spritenum));
        return 0;
    }

    dasectnum = getSprite(spritenum)->_sectnum;

	daz = getSprite(spritenum)->_z;
    h = ((tiles[getSprite(spritenum)->_picnum].dim.height * getSprite(spritenum)->_yrepeat)<<1);
    daz -= h;

    if( bg )
    {
        oldx = getSprite(spritenum)->_x;
        oldy = getSprite(spritenum)->_y;

        if( getSprite(spritenum)->_xrepeat > 60 )
        {
            // next-hack: done this to be able to use bitfields.
            int32_t sprx = getSprite(spritenum)->_x;
            int32_t spry = getSprite(spritenum)->_y;
            retval = clipmove(&sprx,&spry,&daz,&dasectnum,((xchange*TICSPERFRAME)<<11),((ychange*TICSPERFRAME)<<11),1024L,(4<<8),(4<<8),cliptype);
            setSprite_x(spritenum, sprx);
            setSprite_y(spritenum, spry);
        }
        else
        {
            if(getSprite(spritenum)->_picnum == LIZMAN)
                cd = 292L;
            else if( (getActorType(getSprite(spritenum)->_picnum)&3) )
                cd = spr_add_data[spritenum].clipdist<<2;
            else
                cd = 192L;
            // next-hack: done this to be able to use bitfields.
            int32_t sprx = getSprite(spritenum)->_x;
            int32_t spry = getSprite(spritenum)->_y;
            retval = clipmove(&sprx,&spry,&daz,&dasectnum,((xchange*TICSPERFRAME)<<11),((ychange*TICSPERFRAME)<<11),cd,(4<<8),(4<<8),cliptype);
            setSprite_x(spritenum, sprx);
            setSprite_y(spritenum, spry);
        }

        if( dasectnum < 0 || ( dasectnum >= 0 &&
            ( ( getHittype(spritenum)->actorstayput >= 0 && getHittype(spritenum)->actorstayput != dasectnum ) ||
              ( ( getSprite(spritenum)->_picnum == BOSS2 ) && getSprite(spritenum)->_pal == 0 && getSector(dasectnum)->lotag != 3 ) ||
              ( ( getSprite(spritenum)->_picnum == BOSS1 || getSprite(spritenum)->_picnum == BOSS2 ) && getSector(dasectnum)->lotag == 1 ) ||
              ( getSector(dasectnum)->lotag == 1 && ( getSprite(spritenum)->_picnum == LIZMAN ||
              ( getSprite(spritenum)->_picnum == LIZTROOP && getSprite(spritenum)->_zvel == 0 ) ) )
            ) )
          )
        {
                setSprite_x(spritenum, oldx);
                setSprite_y(spritenum, oldy);

                if(getSector(dasectnum)->lotag == 1 && getSprite(spritenum)->_picnum == LIZMAN)
                    setSprite_ang(spritenum, (TRAND&2047));
                else if( (getHittypeTemp(spritenum, 0) &3) == 1 && getSprite(spritenum)->_picnum != COMMANDER )
                    setSprite_ang(spritenum,  (TRAND&2047));
                setsprite(spritenum,oldx,oldy, getSprite_z(spritenum));
                if(dasectnum < 0) dasectnum = 0;
                return (16384+dasectnum);
        }
        if( (retval&49152) >= 32768 && (getHittype(spritenum)->cgg==0) )
                setSprite_ang(spritenum, getSprite_ang(spritenum) +  768);
    }
    else
    {
        if(getSprite(spritenum)->_statnum == 4)
        {
            // next-hack: done this to be able to use bitfields.
            int32_t sprx = getSprite(spritenum)->_x;
            int32_t spry = getSprite(spritenum)->_y;
            retval = clipmove(&sprx,&spry,&daz,&dasectnum,((xchange*TICSPERFRAME)<<11),((ychange*TICSPERFRAME)<<11),8L,(4<<8),(4<<8),cliptype);

            setSprite_x(spritenum,  sprx);
            setSprite_y(spritenum,  spry);
        }
        else
        {
            // next-hack: done this to be able to use bitfields.
            int32_t sprx = getSprite(spritenum)->_x;
            int32_t spry = getSprite(spritenum)->_y;
            retval = clipmove(&sprx,&spry,&daz,&dasectnum,((xchange*TICSPERFRAME)<<11),((ychange*TICSPERFRAME)<<11),(int32_t)(spr_add_data[spritenum].clipdist<<2),(4<<8),(4<<8),cliptype);
            setSprite_x(spritenum,  sprx);
            setSprite_y(spritenum,  spry);
        }

    }

    if( dasectnum >= 0)
        if ( (dasectnum != getSprite(spritenum)->_sectnum) )
            changespritesect(spritenum,dasectnum);
    daz = getSprite(spritenum)->_z + ((zchange*TICSPERFRAME)>>3);
    if ((daz > getHittype(spritenum)->ceilingz) && (daz <= getHittype(spritenum)->floorz))
        setSprite_z(spritenum, daz);
    else
    {
        if (retval == 0)
        {
            return(16384+dasectnum);
        }

    }
	return(retval);
}


short ssp(short i,uint32_t cliptype) //The set sprite function
{
//    const spritetype *getSprite(i);
    int32_t movetype;


    movetype = movesprite(i,
        (getSprite(i)->_xvel*(sintable[(getSprite(i)->_ang+512)&2047]))>>14,
        (getSprite(i)->_xvel*(sintable[getSprite(i)->_ang&2047]))>>14,getSprite(i)->_zvel,
        cliptype);

    return (movetype==0);
}

void insertspriteq(short i)
{
    if(spriteqamount > 0)
    {
        if(spriteq[spriteqloc] >= 0)
            setSprite_xrepeat(spriteq[spriteqloc], 0);
        spriteq[spriteqloc] = i;
        spriteqloc = (spriteqloc+1)%spriteqamount;
    }
    else
    {
        setSprite_xrepeat(i, 0);
        setSprite_yrepeat(i, 0);
    }
}

void lotsofmoney(const spritetype *s, short n)
{
    short i ,j;
    for(i=n;i>0;i--)
    {
        j = EGS(s->_sectnum, s->_x, s->_y, s->_z-(TRAND%(47<<8)),MONEY,-32,8,8,TRAND&2047,0,0,0,5);
        setSprite_cstat(j, TRAND&12 );
    }
}

void lotsofmail(const spritetype *s, short n)
{
    short i ,j;
    for(i=n;i>0;i--)
    {
        j = EGS(s->_sectnum,s->_x, s->_y, s->_z-(TRAND%(47<<8)),MAIL,-32,8,8,TRAND&2047,0,0,0,5);
        setSprite_cstat(j, TRAND&12 );
    }
}

void lotsofpaper(const spritetype *s, short n)
{
    short i ,j;
    for(i=n;i>0;i--)
    {
        j = EGS(s->_sectnum, s->_x, s->_y, s->_z-(TRAND%(47<<8)),PAPER,-32,8,8,TRAND&2047,0,0,0,5);
        setSprite_cstat(j, TRAND&12 );
    }
}



void guts(const spritetype *s,short gtype, short n, short p)
{
    int32_t gutz,floorz;
    short i,a,j;
    uint8_t  sx,sy;
    int8_t pal;

    if(badguy(s) && s->_xrepeat < 16)
        sx = sy = 8;
    else sx = sy = 32;

    gutz = s->_z-(8<<8);
    floorz = getflorzofslope(s->_sectnum,s->_x,s->_y);

    if( gutz > ( floorz-(8<<8) ) )
        gutz = floorz-(8<<8);

    if(s->_picnum == COMMANDER)
        gutz -= (24<<8);

    if( badguy(s) && s->_pal == 6)
        pal = 6;
    else pal = 0;

    for(j=0;j<n;j++)
    {
        a = TRAND&2047;
        i = EGS(s->_sectnum,s->_x+(TRAND&255)-128,s->_y+(TRAND&255)-128,gutz-(TRAND&8191),gtype,-32,sx,sy,a,48+(TRAND&31),-512-(TRAND&2047),ps[p].i,5);
        if(PN == JIBS2)
        {
            setSprite_xrepeat(i, getSprite_xrepeat(i) >> 2);
            setSprite_yrepeat(i, getSprite_yrepeat(i) >> 2);
        }
        if(pal == 6)
            setSprite_pal(i, 6);
    }
}

void gutsdir(const spritetype *s,short gtype, short n, short p)
{
    int32_t gutz,floorz;
    short a,j;
    uint8_t  sx,sy;

    if(badguy(s) && s->_xrepeat < 16)
        sx = sy = 8;
    else sx = sy = 32;

    gutz = s->_z-(8<<8);
    floorz = getflorzofslope(s->_sectnum,s->_x,s->_y);

    if( gutz > ( floorz-(8<<8) ) )
        gutz = floorz-(8<<8);

    if(s->_picnum == COMMANDER)
        gutz -= (24<<8);

    for(j=0;j<n;j++)
    {
        a = TRAND&2047;
        EGS(s->_sectnum,s->_x,s->_y,gutz,gtype,-32,sx,sy,a,256+(TRAND&127),-512-(TRAND&2047),ps[p].i,5);
    }
}

void setsectinterpolate(short i)
{
    int32_t j, k, startwall,endwall;

    startwall = constsector[SECT].wallptr;
    endwall = startwall+constsector[SECT].wallnum;

    for(j=startwall;j<endwall;j++)
    {
#if INTERPOLATION_USES_POSPTR
        setinterpolation(getWall_xptr(j));
        setinterpolation(getWall_yptr(j));
#else
        setinterpolation(getWall_xidx(j));
        setinterpolation(getWall_yidx(j));
#endif
        k = constwall[j].nextwall;
        if(k >= 0)
        {
#if INTERPOLATION_USES_POSPTR
            setinterpolation(getWall_xptr(k));
            setinterpolation(getWall_yptr(k));
            k = getWall_point2(k);
            setinterpolation(getWall_xptr(k));
            setinterpolation(getWall_yptr(k));
#else
            setinterpolation(getWall_xidx(k));
            setinterpolation(getWall_yidx(k));
            k = getWall_point2(k);
            setinterpolation(getWall_xidx(k));
            setinterpolation(getWall_yidx(k));
#endif
        }
    }
}

void clearsectinterpolate(short i)
{
    short j,startwall,endwall;

    startwall = constsector[SECT].wallptr;
    endwall = startwall+constsector[SECT].wallnum;
    for(j=startwall;j<endwall;j++)
    {
#if INTERPOLATION_USES_POSPTR
        stopinterpolation(getWall_xptr(j));
        stopinterpolation(getWall_yptr(j));
        if(constwall[j].nextwall >= 0)
        {
            stopinterpolation(getWall_xptr(constwall[j].nextwall));
            stopinterpolation(getWall_yptr(constwall[j].nextwall));
        }
#else
        stopinterpolation(getWall_xidx(j));
        stopinterpolation(getWall_yidx(j));
        if(constwall[j].nextwall >= 0)
        {
            stopinterpolation(getWall_xidx(constwall[j].nextwall));
            stopinterpolation(getWall_yidx(constwall[j].nextwall));
        }

#endif
    }
}

void ms(short i)
{
    //getHittypeTempPtr(i)[0],getHittypeTemp(i, 1) and getHittypeTemp(i, 2) are used for all the getSector moving stuff!!!

    short startwall,endwall,x;
    int32_t tx,ty,j,k;


    setSprite_x(i, getSprite(i)->_x + ((getSprite(i)->_xvel*(sintable[(getSprite(i)->_ang+512)&2047]))>>14));
    setSprite_y(i, getSprite(i)->_y + ((getSprite(i)->_xvel*(sintable[getSprite(i)->_ang&2047]))>>14));

    j = getHittypeTemp(i, 1) ;
    k = getHittypeTemp(i, 2);
    int sectNum =  getSprite(i)->_sectnum;
    startwall = constsector[sectNum].wallptr;
    endwall = startwall+constsector[sectNum].wallnum;
    for(x=startwall;x<endwall;x++)
    {
        rotatepoint(
            0,0,
            msx[j],msy[j],
            k&2047,&tx,&ty);
        dragpoint(x,getSprite(i)->_x+tx,getSprite(i)->_y+ty);
        j++;
    }
}

void movefta(void)
{
    int32_t x, px, py, sx, sy;
    short i, j, p, psect, ssect, nexti;
//    const spritetype *getSprite(i);

    i = headspritestat[2];
    while(i >= 0)
    {
        nexti = spr_add_data[i].nextspritestat;

        //getSprite(i) = getSprite(i);
        p = findplayer(getSprite(i),&x);

        ssect = psect = getSprite(i)->_sectnum;

        if(getSprite(ps[p].i)->_extra > 0 )
        {
            if( x < 30000 )
            {
                setHittype_timetosleep(i, getHittype_timetosleep(i) + 1);
                if( getHittype_timetosleep(i) >= (x>>8) )
                {
                    if(badguy(getSprite(i)))
                    {
                        px = ps[p].oposx+64-(TRAND&127);
                        py = ps[p].oposy+64-(TRAND&127);
                        updatesector(px,py,&psect);
                        if(psect == -1)
                        {
                            i = nexti;
                            continue;
                        }
                        sx = getSprite(i)->_x+64-(TRAND&127);
                        sy = getSprite(i)->_y+64-(TRAND&127);
                        updatesector(px,py,&ssect);
                        if(ssect == -1)
                        {
                            i = nexti;
                            continue;
                        }
                        j = cansee(sx,sy,getSprite(i)->_z-(TRAND%(52<<8)),getSprite(i)->_sectnum,px,py,ps[p].oposz-(TRAND%(32<<8)),ps[p].cursectnum);
                    }
                    else
                        j = cansee(getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z-((TRAND&31)<<8),getSprite(i)->_sectnum,ps[p].oposx,ps[p].oposy,ps[p].oposz-((TRAND&31)<<8),ps[p].cursectnum);

       //             j = 1;

                    if(j) switch(getSprite(i)->_picnum)
                    {
                        case RUBBERCAN:
                        case EXPLODINGBARREL:
                        case WOODENHORSE:
                        case HORSEONSIDE:
                        case CANWITHSOMETHING:
                        case CANWITHSOMETHING2:
                        case CANWITHSOMETHING3:
                        case CANWITHSOMETHING4:
                        case FIREBARREL:
                        case FIREVASE:
                        case NUKEBARREL:
                        case NUKEBARRELDENTED:
                        case NUKEBARRELLEAKED:
                        case TRIPBOMB:
                            if (getSector(getSprite(i)->_sectnum)->ceilingstat&1)
                                setSprite_shade(i,  getSector_ceilingshade(getSprite(i)->_sectnum));
                            else
                                setSprite_shade(i, getSector_floorshade(getSprite(i)->_sectnum));

                            setHittype_timetosleep(i, 0);
                            changespritestat(i,6);
                            break;
                        default:
                            setHittype_timetosleep(i, 0);
                            check_fta_sounds(i);
                            changespritestat(i,1);
                            break;
                    }
                    else
                        setHittype_timetosleep(i, 0);
                }
            }
            if( badguy( getSprite(i) ) )
            {
                if (getSector(getSprite(i)->_sectnum)->ceilingstat&1)
                    setSprite_shade(i, getSector_ceilingshade(getSprite(i)->_sectnum));
                else
                    setSprite_shade(i, getSector_floorshade(getSprite(i)->_sectnum));
            }
        }
        i = nexti;
    }
}

short ifhitsectors(short sectnum)
{
    short i;

    i = headspritestat[5];
    while(i >= 0)
    {
        if( PN == EXPLOSION2 && sectnum == SECT )
            return i;
        i = spr_add_data[i].nextspritestat;
    }
    return -1;
}

short ifhitbyweapon(short sn)
{
    short j, p;
//    const spritetype *getSprite(sn);

    if( getHittype(sn)->extra >= 0 )
    {
        if(getSprite_extra(sn) >= 0 )
        {
//            getSprite(sn) = getSprite(sn);

            if(getSprite(sn)->_picnum == APLAYER)
            {
                if(ud.god && getHittype(sn)->picnum != SHRINKSPARK ) return -1;

                p = getSprite(sn)->_yvel;
                j = getHittype(sn)->owner;

                if( j >= 0 &&
                    getSprite(j)->_picnum == APLAYER &&
                    ud.coop == 1 &&
                    ud.ffire == 0 )
                        return -1;

                setSprite_extra(sn, getSprite(sn)->_extra - getHittype(sn)->extra);

                if(j >= 0)
                {
                    if(getSprite(sn)->_extra <= 0 && getHittype(sn)->picnum != FREEZEBLAST)
                    {
                        setSprite_extra(sn, 0);

                        ps[p].wackedbyactor = j;

                        if( getSprite(getHittype(sn)->owner)->_picnum == APLAYER && p != getSprite(getHittype(sn)->owner)->_yvel )
                            ps[p].frag_ps = getSprite(j)->_yvel;

                        setHittype_owner(sn, ps[p].i);
                    }
                }

                switch(getHittype(sn)->picnum)
                {
                    case RADIUSEXPLOSION:
                    case RPG:
                    case HYDRENT:
                    case HEAVYHBOMB:
                    case SEENINE:
                    case OOZFILTER:
                    case EXPLODINGBARREL:
                        ps[p].posxv +=
                            getHittype(sn)->extra*(sintable[(getHittype_ang(sn)+512)&2047])<<2;
                        ps[p].posyv +=
                            getHittype(sn)->extra*(sintable[getHittype_ang(sn)&2047])<<2;
                        break;
                    default:
                        ps[p].posxv +=
                            getHittype(sn)->extra*(sintable[(getHittype_ang(sn)+512)&2047])<<1;
                        ps[p].posyv +=
                            getHittype(sn)->extra*(sintable[getHittype_ang(sn)&2047])<<1;
                        break;
                }
            }
            else
            {
                if(getHittype(sn)->extra == 0 )
                    if( getHittype(sn)->picnum == SHRINKSPARK && getSprite(sn)->_xrepeat < 24 )
                        return -1;

                setSprite_extra(sn, getSprite(sn)->_extra - getHittype(sn)->extra);
                if(getSprite(sn)->_picnum != RECON && getSprite(sn)->_owner >= 0 && getSprite(getSprite(sn)->_owner)->_statnum < MAXSTATUS )
                    setSprite_owner(sn, getHittype(sn)->owner);
            }

            setHittype_extra(sn, -1);
            return getHittype(sn)->picnum;
        }
    }

    setHittype_extra(sn, -1);
    return -1;
}

void movecyclers(void)
{
    short q, j, x, _t, s, *c;
    uint8_t  cshade;

    for(q=numcyclers-1;q>=0;q--)
    {

        c = &cyclers[q][0];
        s = c[0];

        _t = c[3];
        j = _t+(sintable[c[1]&2047]>>10);
        cshade = c[2];

        if( j < cshade ) j = cshade;
        else if( j > _t )  j = _t;

        c[1] += getSector_extra(s);
        if(c[5])
        {
            int wallIdx = constsector[s].wallptr;
            for(x = constsector[s].wallnum;x>0;x--, wallIdx++)
                if( getWall_hitag(wallIdx) != 1 )
            {
                //wal->shade = j;
                setWall_shade(wallIdx, j);
                if( (getWall_cstat(wallIdx)&2) && constwall[wallIdx].nextwall >= 0)
                    setWall_shade(constwall[wallIdx].nextwall, j);

            }
            setSector_floorshade(s, j);
            setSector_ceilingshade(s, j);
//            getSector(s)->floorshade = getSector(s)->ceilingshade = j;
        }
    }
}

void movedummyplayers(void)
{  // next-hack: checked: This function does not use t[]
    short i, p, nexti;
    i = headspritestat[13];
    while(i >= 0)
    {
//        const int32_t *t = &getHittypeTempPtr(i)[0];
        nexti = spr_add_data[i].nextspritestat;

        p = getSprite(OW)->_yvel;

        if( ps[p].on_crane >= 0 || getSector(ps[p].cursectnum)->lotag != 1 || getSprite(ps[p].i)->_extra <= 0 )
        {
            ps[p].dummyplayersprite = -1;
            KILLIT(i);
        }
        else
        {
            if(ps[p].on_ground && ps[p].on_warping_sector == 1 && getSector(ps[p].cursectnum)->lotag == 1 )
            {
                setSprite_cstat(i, 257); //crap CS = 257;
                setSprite_z(i, getSector(SECT)->ceilingz+(27<<8));//crap SZ = getSector(SECT)->ceilingz+(27<<8);
                setSprite_ang(i, ps[p].ang);   //crap SA = ps[p].ang;
                if(getHittypeTemp(i, 0) == 8)
                    setHittypeTemp(i, 0, 0); // t[0] = 0;
                else
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //t[0]++;
            }
            else
            {
                if(getSector(SECT)->lotag != 2)
                    setSprite_z(i, getSector(SECT)->floorz); //crap SZ = getSector(SECT)->floorz;
                setSprite_cstat(i, (short) SPRITE_INVISIBLE);
            }
        }

        setSprite_x(i, getSprite_x(i) + (ps[p].posx-ps[p].oposx)); //crap SX += (ps[p].posx-ps[p].oposx);
        setSprite_y(i, getSprite_y(i) +  (ps[p].posy-ps[p].oposy));//crap SY += (ps[p].posy-ps[p].oposy);
        setsprite(i,SX,SY,SZ);

        BOLT:

        i = nexti;
    }
}


short otherp;
void moveplayers(void) //Players
{
    short i , nexti;
    int32_t otherx;
    struct player_struct *p;

    i = headspritestat[10];
    while(i >= 0)
    {
        nexti = spr_add_data[i].nextspritestat;

        p = &ps[getSprite(i)->_yvel];
        if(getSprite(i)->_owner >= 0)
        {
            if(p->newowner >= 0 ) //Looking thru the camera
            {
                setSprite_x(i, p->oposx);
                setSprite_y(i, p->oposy);
                setHittype_bposz(i, p->oposz+PHEIGHT);
                setSprite_z(i, getHittype(i)->bposz);
                setSprite_ang(i, p->oang);
                setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
            }
            else
            {
                if(ud.multimode > 1)
                    otherp = findotherplayer(getSprite(i)->_yvel,&otherx);
                else
                {
                    otherp = getSprite(i)->_yvel;
                    otherx = 0;
                }

                execute(i,getSprite(i)->_yvel,otherx);

                if(ud.multimode > 1)
                    if( getSprite(ps[otherp].i)->_extra > 0 )
                {
                    if( getSprite(i)->_yrepeat > 32 && getSprite(ps[otherp].i)->_yrepeat < 32)
                    {
                        if( otherx < 1400 && p->knee_incs == 0 )
                        {
                            p->knee_incs = 1;
                            p->weapon_pos = -1;
                            p->actorsqu = ps[otherp].i;
                        }
                    }
                }
                if(ud.god)
                {
                    setSprite_extra(i, max_player_health);
                    setSprite_cstat(i, 257);
                    p->jetpack_amount =     1599;
                }


                if( getSprite(i)->_extra > 0 )
                {
                    setHittype_owner(i, i);

                    if(ud.god == 0)
                        if( ceilingspace(getSprite(i)->_sectnum) || floorspace(getSprite(i)->_sectnum) )
                            quickkill(p);
                }
                else
                {

                    p->posx = getSprite(i)->_x;
                    p->posy = getSprite(i)->_y;
                    p->posz = getSprite(i)->_z-(20<<8);

                    p->newowner = -1;

                    if( p->wackedbyactor >= 0 && getSprite(p->wackedbyactor)->_statnum < MAXSTATUS )
                    {
                        p->ang += getincangle(p->ang,getangle(getSprite(p->wackedbyactor)->_x-p->posx,getSprite(p->wackedbyactor)->_y-p->posy))>>1;
                        p->ang &= 2047;
                    }

                }
                setSprite_ang(i, p->ang);
            }
        }
        else
        {
            if(p->holoduke_on == -1)
                KILLIT(i);

            setHittype_bposx(i, getSprite(i)->_x);
            setHittype_bposy(i, getSprite(i)->_y);
            setHittype_bposz(i, getSprite(i)->_z);

            setSprite_cstat(i, 0);

            if(getSprite(i)->_xrepeat < 42)
            {
                setSprite_xrepeat(i, getSprite_xrepeat(i) + 4);
                setSprite_cstat(i, getSprite_cstat(i) | 2);
            }
            else
                setSprite_xrepeat(i, 42);
            if(getSprite(i)->_yrepeat < 36)
                setSprite_yrepeat(i,getSprite(i)->_yrepeat + 4);
            else
            {
                setSprite_yrepeat(i, 36);
                if(getSector(getSprite(i)->_sectnum)->lotag != 2)
                    makeitfall(i);
                if(getSprite(i)->_zvel == 0 && getSector(getSprite(i)->_sectnum)->lotag == 1)
                    setSprite_z(i, getSprite(i)->_z + (32<<8));
            }

            if(getSprite(i)->_extra < 8)
            {
                setSprite_xvel(i, 128);
                setSprite_ang(i, p->ang);
                setSprite_extra(i, getSprite(i)->_extra + 1);
                IFMOVING {}; // <--- next-hack: WTF ?
            }
            else
            {
                setSprite_ang(i, 2047-p->ang);
                setsprite(i, getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
            }
        }

        if (getSector(getSprite(i)->_sectnum)->ceilingstat&1)
            setSprite_shade(i, getSprite(i)->_shade + ((getSector_ceilingshade(getSprite(i)->_sectnum)-getSprite(i)->_shade)>>1));
        else
            setSprite_shade(i, getSprite(i)->_shade + ((getSector_floorshade(getSprite(i)->_sectnum)-getSprite(i)->_shade)>>1));

        BOLT:
        i = nexti;
    }
}


void movefx(void)
{
    short i, j, nexti, p;
    int32_t x, ht;
//    const spritetype *getSprite(i);

    i = headspritestat[11];
    while(i >= 0)
    {

        nexti = spr_add_data[i].nextspritestat;

        switch(getSprite(i)->_picnum)
        {
            case RESPAWN:
                if(getSprite_extra(i) == 66)
                {
                    j = spawn(i,SHT);
                    KILLIT(i);
                }
                else if(getSprite_extra(i) > (66-13))
                    setSprite_extra(i, getSprite_extra(i) + 1);
                break;

            case MUSICANDSFX:

                ht = getSprite(i)->_hitag;

                if(getHittypeTemp(i, 1) != SoundToggle)
                {
                    setHittypeTemp(i, 1, SoundToggle);
                    setHittypeTemp(i, 0, 0);
                }

                if(getSprite(i)->_lotag >= 1000 && getSprite(i)->_lotag < 2000)
                {
                    x = ldist(getSprite(ps[screenpeek].i),getSprite(i));
                    if( x < ht && getHittypeTemp(i, 0) == 0 )
                    {
                        FX_SetReverb( getSprite(i)->_lotag - 1000 );
                        setHittypeTemp(i, 0, 1);
                    }
                    if( x >= ht && getHittypeTemp(i, 0) == 1 )
                    {
                        FX_SetReverb(0);
                        FX_SetReverbDelay(0);
                        setHittypeTemp(i, 0, 0);
                    }
                }
                else if(getSprite(i)->_lotag < 999 && (uint32_t)getSector(getSprite(i)->_sectnum)->lotag < 9 && AmbienceToggle && getSector(SECT)->floorz != getSector(SECT)->ceilingz)
                {
                    if( (soundm[getSprite(i)->_lotag]&2) )
                    {
                        x = dist(getSprite(ps[screenpeek].i),getSprite(i));
                        if( x < ht && getHittypeTemp(i, 0) == 0 && FX_VoiceAvailable(soundpr[getSprite(i)->_lotag]-1) )
                        {
                            if(numenvsnds == MAX_VOICES)
                            {
                                j = headspritestat[11];
                                while(j >= 0)
                                {
                                    if( PN == MUSICANDSFX && j != i && getSprite(j)->_lotag < 999 && getHittypeTemp(j, 0) == 1 && dist(getSprite(j),getSprite(ps[screenpeek].i)) > x )
                                    {
                                        stopenvsound(getSprite(j)->_lotag,j);
                                        break;
                                    }
                                    j = spr_add_data[j].nextspritestat;
                                }
                                if(j == -1) goto BOLT;
                            }
                            spritesound(getSprite(i)->_lotag,i);
                            setHittypeTemp(i, 0, 1);
                        }
                        if( x >= ht && getHittypeTemp(i, 0) == 1 )
                        {
                            setHittypeTemp(i, 0, 0);
                            stopenvsound(getSprite(i)->_lotag,i);
                        }
                    }
                    if( (soundm[getSprite(i)->_lotag]&16) )
                    {
                        if(getHittypeTemp(i, 4) > 0)
                            setHittypeTemp(i, 4, getHittypeTemp(i, 4) - 1);
                        else for(p=connecthead;p>=0;p=connectpoint2[p])
                            if( p == myconnectindex && ps[p].cursectnum == getSprite(i)->_sectnum )
                        {
                            j = getSprite(i)->_lotag+((uint32_t)global_random%(getSprite(i)->_hitag+1));
                            sound(j);
                            setHittypeTemp(i, 4,  26*40 + (global_random%(26*40)));
                        }
                    }
                }
                break;
        }
        BOLT:
        i = nexti;
    }
}



void movefallers(void)
{
    short i, nexti, sect, j;
//    spritetype *getSprite(i);
    int32_t x;

    i = headspritestat[12];
    while(i >= 0)
    {
        nexti = spr_add_data[i].nextspritestat;

        sect = getSprite(i)->_sectnum;

        if( getHittypeTemp(i, 0) == 0 )
        {
            setSprite_z(i, getSprite(i)->_z - (16<<8));
            setHittypeTemp(i, 1, getSprite(i)->_ang);
            x = getSprite(i)->_extra;
            IFHIT
            {
                if( j == FIREEXT || j == RPG || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER )
                {
                    if(getSprite(i)->_extra <= 0)
                    {
                        setHittypeTemp(i, 0, 1);
                        j = headspritestat[12];
                        while(j >= 0)
                        {
                            if(getSprite(j)->_hitag == SHT)
                            {
                                setHittypeTemp(j, 0, 1);
                                setSprite_cstat(j, getSprite(j)->_cstat & (65535-64));
                                if(getSprite(j)->_picnum == CEILINGSTEAM || getSprite(j)->_picnum == STEAM)
                                    setSprite_cstat(j, getSprite(j)->_cstat | SPRITE_INVISIBLE);
                            }
                            j = spr_add_data[j].nextspritestat;
                        }
                    }
                }
                else
                {
                    setHittype_extra(i, 0);
                    setSprite_extra(i, x);
                }
            }
            setSprite_ang(i, getHittypeTemp(i, 1));
            setSprite_z(i, getSprite(i)->_z + (16<<8));
        }
        else if(getHittypeTemp(i, 0) == 1)
        {
            if(getSprite(i)->_lotag > 0)
            {
                setSprite_lotag(i, getSprite(i)->_lotag - 3);
                if(getSprite(i)->_lotag <= 0)
                {
                    setSprite_xvel(i, (32+(TRAND&63)));
                    setSprite_zvel(i, -(1024+(TRAND&1023)));
                }
            }
            else
            {
                if( getSprite(i)->_xvel > 0)
                {
                    setSprite_xvel(i, getSprite(i)->_xvel - 8);
                    ssp(i,CLIPMASK0);
                }

                if( floorspace(getSprite(i)->_sectnum) ) x = 0;
                else
                {
                    if(ceilingspace(getSprite(i)->_sectnum))
                        x = gc/6;
                    else
                        x = gc;
                }

                if( getSprite(i)->_z < (getSector(sect)->floorz-FOURSLEIGHT) )
                {
                    setSprite_zvel(i, getSprite(i)->_zvel + x);
                    if(getSprite(i)->_zvel > 6144)
                        setSprite_zvel(i, 6144);
                    setSprite_z(i, getSprite(i)->_z + getSprite(i)->_zvel);
                }
                if( (getSector(sect)->floorz-getSprite(i)->_z) < (16<<8) )
                {
                    j = 1+(TRAND&7);
                    const spritetype *spritei = getSprite(i);
                    for(x=0;x<j;x++) RANDOMSCRAP(i, spritei);
                    KILLIT(i);
                }
            }
        }

        BOLT:
        i = nexti;
    }
}
void movestandables(void)
{
    short i, j, k, m, nexti, nextj, p, sect;
    int32_t l=0,x; // next-hack: removed *t, because we need setters  ,*t;
    //const int32_t *t;
//    spritetype *getSprite(i);

    i = headspritestat[6];
    while(i >= 0)
    {
        nexti = spr_add_data[i].nextspritestat;

        //t = &getHittypeTempPtr(i)[0];

        sect = getSprite(i)->_sectnum;

        if( sect < 0 ) KILLIT(i);

        setHittype_bposx(i, getSprite(i)->_x);
        setHittype_bposy(i, getSprite(i)->_y);
        setHittype_bposz(i, getSprite(i)->_z);

        IFWITHIN(CRANE,CRANE+3)
        {
            //t[0] = state
            //t[1] = checking getSector number

            if(getSprite(i)->_xvel) getglobalz(i);

            if( getHittypeTemp(i, 0) == 0 ) //Waiting to check the getSector
            {
                j = headspritesect[getHittypeTemp(i, 1)];
                while(j>=0)
                {
                    nextj = spr_add_data[j].nextspritesect;
                    switch( getSprite(j)->_statnum )
                    {
                        case 1:
                        case 2:
                        case 6:
                        case 10:
                            setSprite_ang(i, getangle(msx[getHittypeTemp(i, 4)+1]-getSprite(i)->_x,msy[getHittypeTemp(i, 4)+1]-getSprite(i)->_y));
                            setsprite(j,msx[getHittypeTemp(i, 4)+1],msy[getHittypeTemp(i, 4)+1],getSprite(j)->_z);
                            setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++
                            goto BOLT;
                    }
                    j = nextj;
                }
            }

            else if(getHittypeTemp(i, 0)==1)
            {
                if( getSprite(i)->_xvel < 184 )
                {
                    setSprite_picnum(i, CRANE+1);
                    setSprite_xvel(i, getSprite(i)->_xvel + 8);
                }
                IFMOVING {};  // next-hack: WTF?
                if(sect == getHittypeTemp(i, 1))
                     setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++
            }
            else if(getHittypeTemp(i, 0)==2 || getHittypeTemp(i, 0)==7)
            {
                setSprite_z(i, getSprite(i)->_z + (1024+512));

                if(getHittypeTemp(i, 0)==2)
                {
                    if( (getSector(sect)->floorz - getSprite(i)->_z) < (64<<8) )
                        if(getSprite(i)->_picnum > CRANE)
                            setSprite_picnum(i, getSprite(i)->_picnum - 1);

                    if( (getSector(sect)->floorz - getSprite(i)->_z) < (4096+1024))
                         setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++
                }
                if(getHittypeTemp(i, 0)==7)
                {
                    if( (getSector(sect)->floorz - getSprite(i)->_z) < (64<<8) )
                    {
                        if(getSprite(i)->_picnum > CRANE)
                            setSprite_picnum(i, getSprite(i)->_picnum - 1);
                        else
                        {
                            if(getSprite(i)->_owner==-2)
                            {
                                spritesound(DUKE_GRUNT,ps[p].i);
                                p = findplayer(getSprite(i),&x);
                                if(ps[p].on_crane == i)
                                    ps[p].on_crane = -1;
                            }
                             setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++
                            setSprite_owner(i, -1);
                        }
                    }
                }
            }
            else if(getHittypeTemp(i, 0)==3)
            {
                setSprite_picnum(i, getSprite(i)->_picnum + 1);
                if( getSprite(i)->_picnum == (CRANE+2) )
                {
                    p = checkcursectnums(getHittypeTemp(i, 1));
                    if(p >= 0 && ps[p].on_ground)
                    {
                        setSprite_owner(i, -2);
                        ps[p].on_crane = i;
                        spritesound(DUKE_GRUNT,ps[p].i);
                        ps[p].ang = getSprite(i)->_ang+1024;
                    }
                    else
                    {
                        j = headspritesect[getHittypeTemp(i, 1)];
                        while(j>=0)
                        {
                            switch( getSprite(j)->_statnum )
                            {
                                case 1:
                                case 6:
                                    setSprite_owner(i, j);
                                    break;
                            }
                            j = spr_add_data[j].nextspritesect;
                        }
                    }

                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++//Grabbed the sprite
                    setHittypeTemp(i, 2, 0); //t[2]=0;
                    goto BOLT;
                }
            }
            else if(getHittypeTemp(i, 0)==4) //Delay before going up
            {
                setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 1); // t[2]++;
                if(getHittypeTemp(i, 2) > 10)
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
            }
            else if(getHittypeTemp(i, 0)==5 || getHittypeTemp(i, 0) == 8)
            {
                if(getHittypeTemp(i, 0)==8 && getSprite(i)->_picnum < (CRANE+2))
                    if( (getSector(sect)->floorz-getSprite(i)->_z) > 8192)
                        setSprite_picnum(i, getSprite(i)->_picnum + 1);

                if(getSprite(i)->_z < msx[getHittypeTemp(i, 4)+2])
                {
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                    setSprite_xvel(i, 0);
                }
                else
                    setSprite_z(i, getSprite(i)->_z - (1024+512));
            }
            else if(getHittypeTemp(i, 0)==6)
            {
                if( getSprite(i)->_xvel < 192 )
                    setSprite_xvel(i, getSprite(i)->_xvel + 8);
                setSprite_ang(i, getangle(msx[getHittypeTemp(i, 4)]-getSprite(i)->_x,msy[getHittypeTemp(i, 4)]-getSprite(i)->_y));
                IFMOVING {};  // next-hack: WTF?
                if( ((getSprite(i)->_x-msx[getHittypeTemp(i, 4)])*(getSprite(i)->_x-msx[getHittypeTemp(i, 4)])+(getSprite(i)->_y-msy[getHittypeTemp(i, 4)])*(getSprite(i)->_y-msy[getHittypeTemp(i, 4)]) ) < (128*128) )
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
            }

            else if(getHittypeTemp(i, 0)==9)
                setHittypeTemp(i, 0, 0); //t[0] = 0;

            setsprite(msy[getHittypeTemp(i, 4)+2],getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z-(34<<8));

            if(getSprite(i)->_owner != -1)
            {
                p = findplayer(getSprite(i),&x);

                IFHIT
                {
                    if(getSprite(i)->_owner == -2)
                        if(ps[p].on_crane == i)
                            ps[p].on_crane = -1;
                    setSprite_owner(i, -1);
                    setSprite_picnum(i, CRANE);
                    goto BOLT;
                }

                if(getSprite(i)->_owner >= 0)
                {
                    setsprite(getSprite(i)->_owner,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);

                    setHittype_bposx(getSprite(i)->_owner, getSprite(i)->_x);
                    setHittype_bposy(getSprite(i)->_owner, getSprite(i)->_y);
                    setHittype_bposz(getSprite(i)->_owner, getSprite(i)->_z);

                    setSprite_zvel(i, 0);
                }
                else if(getSprite(i)->_owner == -2)
                {
                    ps[p].oposx = ps[p].posx = getSprite(i)->_x-(sintable[(ps[p].ang+512)&2047]>>6);
                    ps[p].oposy = ps[p].posy = getSprite(i)->_y-(sintable[ps[p].ang&2047]>>6);
                    ps[p].oposz = ps[p].posz = getSprite(i)->_z+(2<<8);
                    setsprite(ps[p].i,ps[p].posx,ps[p].posy,ps[p].posz);
                    ps[p].cursectnum = getSprite(ps[p].i)->_sectnum;
                }
            }

            goto BOLT;
        }

        IFWITHIN(WATERFOUNTAIN,WATERFOUNTAIN+3)
        {
            if(getHittypeTemp(i, 0) > 0)
            {
                if( getHittypeTemp(i, 0) < 20 )
                {
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;

                    setSprite_picnum(i, getSprite(i)->_picnum + 1);

                    if( getSprite(i)->_picnum == ( WATERFOUNTAIN+3 ) )
                        setSprite_picnum(i, WATERFOUNTAIN+1);
                }
                else
                {
                    p = findplayer(getSprite(i),&x);

                    if(x > 512)
                    {
                        setHittypeTemp(i, 0, 0); // t[0] = 0;
                        setSprite_picnum(i, WATERFOUNTAIN);
                    }
                    else
                        setHittypeTemp(i, 0, 1); // t[0] = 1;
                }
            }
            goto BOLT;
        }

        if( AFLAMABLE(getSprite(i)->_picnum) )
        {
            if(getHittypeTemp(i, 0) == 1)
            {
                setHittypeTemp(i, 1, getHittypeTemp(i, 1) + 1); //t[1] ++;
                if( (getHittypeTemp(i, 1)&3) > 0) goto BOLT;

                if( getSprite(i)->_picnum == TIRE && getHittypeTemp(i, 1)  == 32 )
                {
                    setSprite_cstat(i, 0);
                    j = spawn(i,BLOODPOOL);
                    setSprite_shade(j, 127);
                }
                else
                {
                    if(getSprite(i)->_shade < 64)
                        setSprite_shade(i, getSprite(i)->_shade + 1);
                    else KILLIT(i);
                }

                j = getSprite(i)->_xrepeat-(TRAND&7);
                if(j < 10)
                {
                    KILLIT(i);
                }

                setSprite_xrepeat(i, j);

                j = getSprite(i)->_yrepeat-(TRAND&7);
                if(j < 4) { KILLIT(i); }
                setSprite_yrepeat(i, j);
            }
            if(getSprite(i)->_picnum == BOX)
            {
                makeitfall(i);
                setHittype_ceilingz (i, getSector(getSprite(i)->_sectnum)->ceilingz);
            }
            goto BOLT;
        }

        if(getSprite(i)->_picnum == TRIPBOMB)
        {
            if(getHittypeTemp(i, 2) > 0)
            {
                setHittypeTemp(i, 2, getHittypeTemp(i, 2) - 1); //t[2]--;
                if(getHittypeTemp(i, 2) == 8)
                {
                    spritesound(LASERTRIP_EXPLODE,i);
                    const spritetype *spritei = getSprite(i);
                    for(j=0;j<5;j++) RANDOMSCRAP(i, spritei);
                    x = getSprite(i)->_extra;
                    hitradius( i, tripbombblastradius, x>>2,x>>1,x-(x>>2),x);

                    j = spawn(i,EXPLOSION2);
                    setSprite_ang(j, getSprite(i)->_ang);
                    setSprite_xvel(j, 348);
                    ssp(j,CLIPMASK0);

                    j = headspritestat[5];
                    while(j >= 0)
                    {
                        if(getSprite(j)->_picnum == LASERLINE && getSprite(i)->_hitag == getSprite(j)->_hitag)
                        {
                            setSprite_xrepeat(j, 0); setSprite_yrepeat(j, 0);
                        }
                        j = spr_add_data[j].nextspritestat;
                    }
                    KILLIT(i);
                }
                goto BOLT;
            }
            else
            {
                x = getSprite(i)->_extra;
                setSprite_extra(i, 1);
                l = getSprite(i)->_ang;
                IFHIT
                {
                    setHittypeTemp(i, 2, 16); //t[2] = 16;
                }
                setSprite_extra(i, x);
                setSprite_ang(i, l);
            }

            if( getHittypeTemp(i, 0) < 32 )
            {
                p = findplayer(getSprite(i),&x);
                if( x > 768 )
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //t[0]++;
                else if(getHittypeTemp(i, 0) > 16)
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //t[0]++;
            }
            if( getHittypeTemp(i, 0) == 32 )
            {
                l = getSprite(i)->_ang;
                setSprite_ang(i, getHittypeTemp(i, 5));

                setHittypeTemp(i, 3, getSprite(i)->_x); //t[3] = getSprite(i)->_x;
                setHittypeTemp(i, 4, getSprite(i)->_y); //t[4] = getSprite(i)->_y;
                setSprite_x(i, getSprite(i)->_x + (sintable[(getHittypeTemp(i, 5)+512)&2047]>>9));
                setSprite_y(i, getSprite(i)->_y + (sintable[(getHittypeTemp(i, 5))&2047]>>9));
                setSprite_z(i, getSprite(i)->_z - (3<<8));
                setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);

                x = hitasprite(i,&m);

                setHittype_lastvx(i, x);

                setSprite_ang(i, l);

                k = 0;

                while(x > 0)
                {
                    j = spawn(i,LASERLINE);
                    setsprite(j,getSprite(j)->_x,getSprite(j)->_y,getSprite(j)->_z);
                    setSprite_hitag(j, getSprite(i)->_hitag);
                    setHittypeTemp(j, 1, getSprite(j)->_z);

                    setSprite_x(i, getSprite(i)->_x + (sintable[(getHittypeTemp(i, 5)+512)&2047]>>4));
                    setSprite_y(i, getSprite(i)->_y + (sintable[(getHittypeTemp(i, 5))&2047]>>4));

                    if( x < 1024 )
                    {
                        setSprite_xrepeat(j, x>>5);
                        break;
                    }
                    x -= 1024;
                }

                setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //t[0]++;
                setSprite_x(i, getHittypeTemp(i, 3));
                setSprite_y(i, getHittypeTemp(i, 4));
                setSprite_z(i, getSprite(i)->_z + (3<<8));
                setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                setHittypeTemp(i, 3, 0); //t[3] = 0;
                if( m >= 0 )
                {
                    setHittypeTemp(i, 2, 13); //t[2] = 13;
                    spritesound(LASERTRIP_ARMING,i);
                }
                else
                    setHittypeTemp(i, 2, 0); //t[2] = 0;
            }
            if(getHittypeTemp(i, 0) == 33)
            {
                setHittypeTemp(i, 1, getHittypeTemp(i, 1) + 1); //t[1] ++;


                setHittypeTemp(i, 3, getSprite(i)->_x); //t[3] = getSprite(i)->_x;
                setHittypeTemp(i, 4, getSprite(i)->_y); //t[4] = getSprite(i)->_y;
                setSprite_x(i, getSprite(i)->_x + (sintable[(getHittypeTemp(i, 5)+512)&2047]>>9));
                setSprite_y(i, getSprite(i)->_y + (sintable[(getHittypeTemp(i, 5))&2047]>>9));
                setSprite_z(i, getSprite(i)->_z - (3<<8));
                setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);

                x = hitasprite(i,&m);

                setSprite_x(i, getHittypeTemp(i, 3));
                setSprite_y(i, getHittypeTemp(i, 4));
                setSprite_z(i, getSprite(i)->_z + (3<<8));
                setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);

                if( getHittype_lastvx(i) != x )
                {
                    setHittypeTemp(i, 2, 13); //t[2] = 13;
                    spritesound(LASERTRIP_ARMING,i);
                }
            }
            goto BOLT;
        }


        if( getSprite(i)->_picnum >= CRACK1 && getSprite(i)->_picnum <= CRACK4 )
        {
            if(getSprite(i)->_hitag > 0)
            {
                setHittypeTemp(i, 0, getSprite(i)->_cstat); // t[0] = getSprite(i)->_cstat;
                setHittypeTemp(i, 1, getSprite(i)->_ang); //t[1] = getSprite(i)->_ang;
                j = ifhitbyweapon(i);
                if(j == FIREEXT || j == RPG || j == RADIUSEXPLOSION || j == SEENINE || j == OOZFILTER )
                {
                    j = headspritestat[6];
                    while(j >= 0)
                    {
                        if(getSprite(i)->_hitag == getSprite(j)->_hitag && ( getSprite(j)->_picnum == OOZFILTER || getSprite(j)->_picnum == SEENINE ) )
                            if(getSprite(j)->_shade != -32)
                                setSprite_shade(j, -32);
                        j = spr_add_data[j].nextspritestat;
                    }

                    goto DETONATE;
                }
                else
                {
                    setSprite_cstat(i, getHittypeTemp(i, 0));
                    setSprite_ang(i, getHittypeTemp(i, 1));
                    setSprite_extra(i, 0);
                }
            }
            goto BOLT;
        }

        if( getSprite(i)->_picnum == FIREEXT )
        {
            j = ifhitbyweapon(i);
            if( j == -1 ) goto BOLT;

            for(k=0;k<16;k++)
            {
                j = EGS(SECT,SX,SY,SZ-(TRAND%(48<<8)),SCRAP3+(TRAND&3),-8,48,48,TRAND&2047,(TRAND&63)+64,-(TRAND&4095)-(getSprite(i)->_zvel>>2),i,5);
                setSprite_pal(j, 2);
            }

            spawn(i,EXPLOSION2);
            spritesound(PIPEBOMB_EXPLODE,i);
            spritesound(GLASS_HEAVYBREAK,i);

            if(getSprite(i)->_hitag > 0)
            {
                j = headspritestat[6];
                while(j >= 0)
                {
                    if(getSprite(i)->_hitag == getSprite(j)->_hitag && ( getSprite(j)->_picnum == OOZFILTER || getSprite(j)->_picnum == SEENINE ) )
                        if(getSprite(j)->_shade != -32)
                            setSprite_shade(j, -32);
                    j = spr_add_data[j].nextspritestat;
                }

                x = getSprite(i)->_extra;
                spawn(i,EXPLOSION2);
                hitradius( i, pipebombblastradius,x>>2, x-(x>>1),x-(x>>2), x);
                spritesound(PIPEBOMB_EXPLODE,i);

                goto DETONATE;
            }
            else
            {
                hitradius(i,seenineblastradius,10,15,20,25);
                KILLIT(i);
            }
            goto BOLT;
        }

        if(getSprite(i)->_picnum == OOZFILTER || getSprite(i)->_picnum == SEENINE || getSprite(i)->_picnum == SEENINEDEAD || getSprite(i)->_picnum == (SEENINEDEAD+1) )
        {
            if(getSprite(i)->_shade != -32 && getSprite(i)->_shade != -33)
            {
                if(getSprite(i)->_xrepeat)
                    j = (ifhitbyweapon(i) >= 0);
                else
                    j = 0;

                if(( j || getSprite(i)->_shade == -31 )) //  this will slow down
                {
                    if(j)
                        setSprite_lotag(i, 0);

                    setHittypeTemp(i, 3, 1); //t[3] = 1;

                    j = headspritestat[6];
                    while(j >= 0)
                    {
                        // setting shade to -32 will slow down
                        if(getSprite(i)->_hitag == getSprite(j)->_hitag && ( getSprite(j)->_picnum == SEENINE || getSprite(j)->_picnum == OOZFILTER ) )
                            setSprite_shade(j, -32);
                        j = spr_add_data[j].nextspritestat;
                    }
                }
            }
            else
            {
                if(getSprite(i)->_shade == -32)
                {
                    if(getSprite(i)->_lotag > 0)
                    {
                        setSprite_lotag(i, getSprite(i)->_lotag - 3);
                        if(getSprite(i)->_lotag <= 0)
                            setSprite_lotag(i, -99);
                    }
                    else
                        setSprite_shade(i, -33);
                }
                else
                {
                    if( getSprite(i)->_xrepeat > 0 )
                    {
                        setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 1); //t[2]++;
                        if(getHittypeTemp(i, 2) == 3)
                        {
                            if( getSprite(i)->_picnum == OOZFILTER )
                            {
                                setHittypeTemp(i, 2, 0); //t[2] = 0;
                                goto DETONATE;
                            }
                            if( getSprite(i)->_picnum != (SEENINEDEAD+1) )
                            {
                                setHittypeTemp(i, 2, 0); //t[2] = 0;

                                if(getSprite(i)->_picnum == SEENINEDEAD)
                                    setSprite_picnum(i, getSprite(i)->_picnum + 1);
                                else if(getSprite(i)->_picnum == SEENINE)
                                    setSprite_picnum(i, SEENINEDEAD);
                            }
                            else goto DETONATE;
                        }
                        goto BOLT;
                    }

                    DETONATE:
                    earthquaketime = 16;

                    j = headspritestat[3];
                    while((j >= 0))
                    {
                        if( getSprite(i)->_hitag == getSprite(j)->_hitag )
                        {

                            if(getSprite(j)->_lotag == 13)
                            {
                                if( getHittypeTemp(j, 2) == 0 )
                                    setHittypeTemp(j, 2, 1);
                            }
                            else if(getSprite(j)->_lotag == 8)
                                setHittypeTemp(j, 4, 1);
                            else if(getSprite(j)->_lotag == 18)
                            {
                                if(getHittypeTemp(j, 0) == 0)
                                    setHittypeTemp(j, 0, 1);
                            }
                            else if(getSprite(j)->_lotag == 21)
                                setHittypeTemp(j, 0, 1);

                        }
                        j = spr_add_data[j].nextspritestat;
                    }

                    // up to now, no slowdowns;
                   setSprite_z(i, getSprite(i)->_z - (32<<8));

                    if( ( getHittypeTemp(i, 3) == 1 && getSprite(i)->_xrepeat ) || getSprite(i)->_lotag == -99 )
                    {
                        x = getSprite(i)->_extra;
                        spawn(i,EXPLOSION2);
                        hitradius( i,seenineblastradius,x>>2, x-(x>>1),x-(x>>2), x);
                       spritesound(PIPEBOMB_EXPLODE,i);
                    }
                    if(getSprite(i)->_xrepeat)
                    {
                        const spritetype *spritei = getSprite(i);
                        for(x=0;x<8;x++) RANDOMSCRAP(i, spritei);
                    }
                    KILLIT(i);
                }
            }
            goto BOLT;
        }

        if(getSprite(i)->_picnum == MASTERSWITCH)
        {
            if(getSprite(i)->_yvel == 1)
                {
                    setSprite_hitag(i, getSprite(i)->_hitag - 1);
                    if(getSprite(i)->_hitag <= 0)
                    {
                        operatesectors(sect,i);

                        j = headspritesect[sect];
                        while(j >= 0)
                        {
                            if(getSprite(j)->_statnum == 3)
                            {
                                switch(getSprite(j)->_lotag)
                                {
                                    case 2:
                                    case 21:
                                    case 31:
                                    case 32:
                                    case 36:
                                        setHittypeTemp(j, 0, 1);
                                        break;
                                    case 3:
                                        setHittypeTemp(j, 4, 1);
                                        break;
                                }
                            }
                            else if(getSprite(j)->_statnum == 6)
                            {
                                switch(getSprite(j)->_picnum)
                                {
                                    case SEENINE:
                                    case OOZFILTER:
                                        setSprite_shade(j, -31);
                                        break;
                                }
                            }
                            j = spr_add_data[j].nextspritesect;
                        }
                        KILLIT(i);
                    }
                }
                goto BOLT;
        }

        switch(getSprite(i)->_picnum)
        {
            case VIEWSCREEN:
            case VIEWSCREEN2:

                if(getSprite(i)->_xrepeat == 0) KILLIT(i);

                p = findplayer(getSprite(i), &x);

                if( x < 2048 )
                {
                    if( SP == 1 )
                        camsprite = i;
                }
                else if( camsprite != -1 && getHittypeTemp(i, 0) == 1)
                {
                    camsprite = -1;
                    setHittypeTemp(i, 0, 0); //t[0] = 0;
#if !USE_EXT_MEM
                    loadtile(getSprite(i)->_picnum);
#endif
                }

                goto BOLT;

            case TRASH:

                if(getSprite(i)->_xvel == 0)
                    setSprite_xvel(i, 1);
                IFMOVING
                {
                    makeitfall(i);
                    if(TRAND&1)
                        setSprite_zvel(i, getSprite(i)->_zvel - 256);
                    if( klabs(getSprite(i)->_xvel) < 48 )
                        setSprite_xvel(i, getSprite(i)->_xvel + (TRAND&3));
                }
                else KILLIT(i);
                break;

            case SIDEBOLT1:
            case SIDEBOLT1+1:
            case SIDEBOLT1+2:
            case SIDEBOLT1+3:
                p = findplayer(getSprite(i), &x);
                if( x > 20480 ) goto BOLT;

                CLEAR_THE_BOLT2:
                if(getHittypeTemp(i, 2))
                {
                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) - 1); //t[2]--;
                    goto BOLT;
                }
                if( (getSprite(i)->_xrepeat|getSprite(i)->_yrepeat) == 0 )
                {
                    setSprite_xrepeat(i, getHittypeTemp(i, 0));
                    setSprite_yrepeat(i, getHittypeTemp(i, 1));
                }
                if( (TRAND&8) == 0 )
                {
                    setHittypeTemp(i, 0, getSprite(i)->_xrepeat); //t[0]=getSprite(i)->_xrepeat;
                    setHittypeTemp(i, 1, getSprite(i)->_yrepeat); //t[1]=getSprite(i)->_yrepeat;
                    setHittypeTemp(i, 2, global_random & 4); //t[2] = global_random&4;
                    setSprite_xrepeat(i, 0);
                    setSprite_yrepeat(i, 0);
                    goto CLEAR_THE_BOLT2;
                }
                setSprite_picnum(i, getSprite(i)->_picnum + 1);

                if(l&1)
                    setSprite_cstat(i, getSprite(i)->_cstat ^ 2); // l not defined here. Line is met in 2nd demo with l = 0.

                if( (TRAND&1) && getSector(sect)->floorpicnum == HURTRAIL )
                    spritesound(SHORT_CIRCUIT,i);

                if(getSprite(i)->_picnum == SIDEBOLT1+4)
                    setSprite_picnum(i, SIDEBOLT1);

                goto BOLT;

            case BOLT1:
            case BOLT1+1:
            case BOLT1+2:
            case BOLT1+3:
                p = findplayer(getSprite(i), &x);
                if( x > 20480 ) goto BOLT;

                if( getHittypeTemp(i, 3) == 0 )
                    setHittypeTemp(i, 3, getSector_floorshade(sect)); //t[3]=getSector_floorshade(sect);

                CLEAR_THE_BOLT:
                if(getHittypeTemp(i, 2))
                {
                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) - 1); //t[2]--;
                    setSector_floorshade(sect, 20);
                    setSector_ceilingshade(sect, 20);
                    goto BOLT;
                }
                if( (getSprite(i)->_xrepeat|getSprite(i)->_yrepeat) == 0 )
                {
                    setSprite_xrepeat(i, getHittypeTemp(i, 0));
                    setSprite_yrepeat(i, getHittypeTemp(i, 1));                }
                else if( (TRAND&8) == 0 )
                {
                    setHittypeTemp(i, 0, getSprite(i)->_xrepeat); //t[0]=getSprite(i)->_xrepeat;
                    setHittypeTemp(i, 1, getSprite(i)->_yrepeat); //t[1]=getSprite(i)->_yrepeat;
                    setHittypeTemp(i, 2, global_random & 4); //t[2] = global_random&4;
                    setSprite_xrepeat(i, 0);
                    setSprite_yrepeat(i, 0);
                    goto CLEAR_THE_BOLT;
                }
                setSprite_picnum(i, getSprite(i)->_picnum + 1);

                l = global_random&7;
                setSprite_xrepeat(i, l+8);

                if(l&1)
                    setSprite_cstat(i, getSprite(i)->_cstat ^ 2);

                if( getSprite(i)->_picnum == (BOLT1+1) && (TRAND&7) == 0 && getSector(sect)->floorpicnum == HURTRAIL )
                    spritesound(SHORT_CIRCUIT,i);

                if(getSprite(i)->_picnum==BOLT1+4)
                    setSprite_picnum(i, BOLT1);

                if(getSprite(i)->_picnum&1)
                {
                    setSector_floorshade(sect, 0);
                    setSector_ceilingshade(sect, 0);
                }
                else
                {
                    setSector_floorshade(sect, 20);
                    setSector_ceilingshade(sect, 20);
                }
                goto BOLT;

            case WATERDRIP:

                if( getHittypeTemp(i, 1) )
                {
                    setHittypeTemp(i, 1, getHittypeTemp(i, 1) - 1); // t[1]--;
                    if(getHittypeTemp(i, 1) == 0)
                        setSprite_cstat(i, getSprite(i)->_cstat & ~SPRITE_INVISIBLE); //32767;
                }
                else
                {
                    makeitfall(i);
                    ssp(i,CLIPMASK0);
                    if(getSprite(i)->_xvel > 0)
                        setSprite_xvel(i, getSprite(i)->_xvel - 2);

                    if(getSprite(i)->_zvel == 0)
                    {
                        setSprite_cstat(i, getSprite(i)->_cstat | SPRITE_INVISIBLE);

                        if(getSprite(i)->_pal != 2 && getSprite(i)->_hitag == 0)
                            spritesound(SOMETHING_DRIPPING,i);

                        if(getSprite(getSprite(i)->_owner)->_picnum != WATERDRIP)
                        {
                            KILLIT(i);
                        }
                        else
                        {
                            setHittype_bposz(i, getHittypeTemp(i, 0));
                            setSprite_z(i, getHittype(i)->bposz);
                            setHittypeTemp(i, 1, 48 + (TRAND & 31)); //t[1] = 48+(TRAND&31);
                        }
                    }
                }


                goto BOLT;

            case DOORSHOCK:
                j = klabs(getSector(sect)->ceilingz-getSector(sect)->floorz)>>9;
                setSprite_yrepeat(i, j+4);
                setSprite_xrepeat(i, 16);
                setSprite_z(i, getSector(sect)->floorz);
                goto BOLT;

            case TOUCHPLATE:
                if( getHittypeTemp(i, 1) == 1 && getSprite(i)->_hitag >= 0) //Move the getSector floor
                {
                    x = getSector(sect)->floorz;

                    if(getHittypeTemp(i, 3) == 1)
                    {
                        if(x >= getHittypeTemp(i, 2))
                        {
                            setSector_floorz(sect, x);
                            setHittypeTemp(i, 1, 0); //t[1] = 0;
                        }
                        else
                        {
                            setSector_floorz(sect, getSector(sect)->floorz + getSector_extra(sect));
                            p = checkcursectnums(sect);
                            if(p >= 0) ps[p].posz += getSector_extra(sect);
                        }
                    }
                    else
                    {
                        if(x <= getSprite(i)->_z)
                        {
                            setSector_floorz(sect, getSprite(i)->_z);
                            setHittypeTemp(i, 1, 0); ////t[1] = 0;
                        }
                        else
                        {
                            setSector_floorz(sect, getSector(sect)->floorz - getSector_extra(sect));
                            p = checkcursectnums(sect);
                            if(p >= 0)
                                ps[p].posz -= getSector_extra(sect);
                        }
                    }
                    goto BOLT;
                }

                if(getHittypeTemp(i, 5) == 1) goto BOLT;

                p = checkcursectnums(sect);
                if( p >= 0 && ( ps[p].on_ground || getSprite(i)->_ang == 512) )
                {
                    if( getHittypeTemp(i, 0) == 0 && !check_activator_motion(getSprite(i)->_lotag) )
                    {
                        setHittypeTemp(i, 0, 1); //t[0] = 1;
                        setHittypeTemp(i, 0, 1); //t[1] = 1;
                        setHittypeTemp(i, 3, !getHittypeTemp(i, 3)); //t[3] = !getHittypeTemp(i, 3);
                        operatemasterswitches(getSprite(i)->_lotag);
                        operateactivators(getSprite(i)->_lotag,p);
                        if(getSprite(i)->_hitag > 0)
                        {
                            setSprite_hitag(i, getSprite(i)->_hitag - 1);
                            if(getSprite(i)->_hitag == 0)
                                setHittypeTemp(i, 5, 1); //t[5] = 1;
                        }
                    }
                }
                else
                    setHittypeTemp(i, 0, 0); //t[0] = 0;

                if(getHittypeTemp(i, 1) == 1)
                {
                    j = headspritestat[6];
                    while(j >= 0)
                    {
                        if(j != i && getSprite(j)->_picnum == TOUCHPLATE && getSprite(j)->_lotag == getSprite(i)->_lotag)
                        {
                            setHittypeTemp(j, 1, 1);
                            setHittypeTemp(j, 3,  getHittypeTemp(i, 3));
                        }
                        j = spr_add_data[j].nextspritestat;
                    }
                }
                goto BOLT;

            case CANWITHSOMETHING:
            case CANWITHSOMETHING2:
            case CANWITHSOMETHING3:
            case CANWITHSOMETHING4:
                makeitfall(i);
                IFHIT
                {
                    spritesound(VENT_BUST,i);
                    const spritetype *spritei = getSprite(i);
                    for(j=0;j<10;j++)
                        RANDOMSCRAP(i, spritei);

                    if(getSprite(i)->_lotag) spawn(i,getSprite(i)->_lotag);

                    KILLIT(i);
                }
                goto BOLT;

            case EXPLODINGBARREL:
            case WOODENHORSE:
            case HORSEONSIDE:
            case FLOORFLAME:
            case FIREBARREL:
            case FIREVASE:
            case NUKEBARREL:
            case NUKEBARRELDENTED:
            case NUKEBARRELLEAKED:
            case TOILETWATER:
            case RUBBERCAN:
            case STEAM:
            case CEILINGSTEAM:
                p = findplayer(getSprite(i), &x);
                execute(i,p,x);
                goto BOLT;
            case WATERBUBBLEMAKER:
                p = findplayer(getSprite(i), &x);
                execute(i,p,x);
                goto BOLT;
        }

        BOLT:
        i = nexti;
    }
}

void bounce(short i)
{
    int32_t k, l, daang, dax, day, daz, xvect, yvect, zvect;
    short hitsect;
//    spritetype *getSprite(i) = &sprite[i];

    xvect = mulscale10(getSprite(i)->_xvel,sintable[(getSprite(i)->_ang+512)&2047]);
    yvect = mulscale10(getSprite(i)->_xvel,sintable[getSprite(i)->_ang&2047]);
    zvect = getSprite(i)->_zvel;

    hitsect = getSprite(i)->_sectnum;

    k = constsector[hitsect].wallptr; l = getWall_point2(k);
    daang = getangle(getWall(l)->x-getWall(k)->x,getWall(l)->y-getWall(k)->y);

    if ( getSprite(i)->_z < (getHittype(i)->floorz+getHittype(i)->ceilingz)>>1)
        k = constsector[hitsect].ceilingheinum;
    else
        k = getSector(hitsect)->floorheinum;

    dax = mulscale14(k,sintable[(daang)&2047]);
    day = mulscale14(k,sintable[(daang+1536)&2047]);
    daz = 4096;

    k = xvect*dax+yvect*day+zvect*daz;
    l = dax*dax+day*day+daz*daz;
    if ((klabs(k)>>14) < l)
    {
        k = divscale17(k,l);
        xvect -= mulscale16(dax,k);
        yvect -= mulscale16(day,k);
        zvect -= mulscale16(daz,k);
    }

    setSprite_zvel(i, zvect);
    setSprite_xvel(i, ksqrt(dmulscale8(xvect,xvect,yvect,yvect)));
    setSprite_ang(i, getangle(xvect,yvect));
}

void moveweapons(void)
{
    short i, j, k, nexti, p, q;
    int32_t dax,day,daz, x, ll;
    uint32_t qq;

    i = headspritestat[4];
    while(i >= 0)
    {
        nexti = spr_add_data[i].nextspritestat;

        if(getSprite(i)->_sectnum < 0) KILLIT(i);

        setHittype_bposx(i, getSprite(i)->_x);
        setHittype_bposy(i, getSprite(i)->_y);
        setHittype_bposz(i, getSprite(i)->_z);

        switch(getSprite(i)->_picnum)
        {
            case RADIUSEXPLOSION:
            case KNEE:
                KILLIT(i);
            case TONGUE:
                setHittypeTemp(i, 0, sintable[(getHittypeTemp(i, 1))&2047]>>9);
                setHittypeTemp(i, 1, getHittypeTemp(i, 1) + 32);
                if(getHittypeTemp(i, 1) > 2047) KILLIT(i);

                if(getSprite(getSprite(i)->_owner)->_statnum == MAXSTATUS)
                    if(badguy(getSprite(getSprite(i)->_owner)) == 0)
                        KILLIT(i);

                setSprite_ang(i, getSprite(getSprite(i)->_owner)->_ang);
                setSprite_x(i, getSprite(getSprite(i)->_owner)->_x);
                setSprite_y(i, getSprite(getSprite(i)->_owner)->_y);
                if(getSprite(getSprite(i)->_owner)->_picnum == APLAYER)
                    setSprite_z(i, getSprite(getSprite(i)->_owner)->_z-(34<<8));
                for(k=0;k<getHittypeTemp(i, 0);k++)
                {
                    q = EGS(getSprite(i)->_sectnum,
                        getSprite(i)->_x+((k*sintable[(getSprite(i)->_ang+512)&2047])>>9),
                        getSprite(i)->_y+((k*sintable[getSprite(i)->_ang&2047])>>9),
                        getSprite(i)->_z+((k*ksgn(getSprite(i)->_zvel))*klabs(getSprite(i)->_zvel/12)),TONGUE,-40+(k<<1),
                        8,8,0,0,0,i,5);
                    setSprite_cstat(q, 128);
                    setSprite_pal(q,  8);
                }
                q = EGS(getSprite(i)->_sectnum,
                    getSprite(i)->_x+((k*sintable[(getSprite(i)->_ang+512)&2047])>>9),
                    getSprite(i)->_y+((k*sintable[getSprite(i)->_ang&2047])>>9),
                    getSprite(i)->_z+((k*ksgn(getSprite(i)->_zvel))*klabs(getSprite(i)->_zvel/12)),INNERJAW,-40,
                    32,32,0,0,0,i,5);
                setSprite_cstat(q, 128);
                if( getHittypeTemp(i, 1)  > 512 && getHittypeTemp(i, 1)  < (1024) )
                    setSprite_picnum(q, INNERJAW+1);

                goto BOLT;

            case FREEZEBLAST:
                if(getSprite(i)->_yvel < 1 || getSprite(i)->_extra < 2 || (getSprite(i)->_xvel|getSprite(i)->_zvel) == 0)
                {
                    j = spawn(i,TRANSPORTERSTAR);
                    setSprite_pal(j, 1);
                    setSprite_xrepeat(j, 32);
                    setSprite_yrepeat(j, 32);
                    KILLIT(i);
                }
            case SHRINKSPARK:
            case RPG:
            case FIRELASER:
            case SPIT:
            case COOLEXPLOSION1:

                if( getSprite(i)->_picnum == COOLEXPLOSION1 )
//                    if( Sound[WIERDSHOT_FLY].num == 0 )
                      if( getSoundNum(WIERDSHOT_FLY) == 0 )
                        spritesound(WIERDSHOT_FLY,i);

                p = -1;

                if(getSprite(i)->_picnum == RPG && getSector(getSprite(i)->_sectnum)->lotag == 2)
                {
                    k = getSprite(i)->_xvel>>1;
                    ll = getSprite(i)->_zvel>>1;
                }
                else
                {
                    k = getSprite(i)->_xvel;
                    ll = getSprite(i)->_zvel;
                }

                dax = getSprite(i)->_x; day = getSprite(i)->_y; daz = getSprite(i)->_z;

                getglobalz(i);
                qq = CLIPMASK1;

                switch(getSprite(i)->_picnum)
                {
                    case RPG:
                        if(getHittype(i)->picnum != BOSS2 && getSprite(i)->_xrepeat >= 10 && getSector(getSprite(i)->_sectnum)->lotag != 2)
                        {
                            j = spawn(i,SMALLSMOKE);
                            setSprite_z(j, getSprite(j)->_z + (1<<8));
                        }
                        break;
                }

                j = movesprite(i,
                    (k*(sintable[(getSprite(i)->_ang+512)&2047]))>>14,
                    (k*(sintable[getSprite(i)->_ang&2047]))>>14,ll,qq);

                if(getSprite(i)->_picnum == RPG && getSprite(i)->_yvel >= 0)
                    if( FindDistance2D(getSprite(i)->_x-getSprite(getSprite(i)->_yvel)->_x,getSprite(i)->_y-getSprite(getSprite(i)->_yvel)->_y) < 256 )
                        j = 49152|getSprite(i)->_yvel;

                if(getSprite(i)->_sectnum < 0) { KILLIT(i); }

                if( (j&49152) != 49152)
                    if(getSprite(i)->_picnum != FREEZEBLAST)
                {
                    if(getSprite(i)->_z < getHittype(i)->ceilingz)
                    {
                        j = 16384|(getSprite(i)->_sectnum);
                        setSprite_zvel(i, -1);
                    }
                    else
                        if( ( getSprite(i)->_z > getHittype(i)->floorz && getSector(getSprite(i)->_sectnum)->lotag != 1 ) ||
                            ( getSprite(i)->_z > getHittype(i)->floorz+(16<<8) && getSector(getSprite(i)->_sectnum)->lotag == 1 ) )
                    {
                        j = 16384|(getSprite(i)->_sectnum);
                        if(getSector(getSprite(i)->_sectnum)->lotag != 1)
                            setSprite_zvel(i, 1);
                    }
                }

                if(getSprite(i)->_picnum == FIRELASER)
                {
                    const spritetype * spritei = getSprite(i);
                    for(k=-3;k<2;k++)
                    {
                        x = EGS(spritei->_sectnum,
                            spritei->_x+((k*sintable[(spritei->_ang+512)&2047])>>9),
                            spritei->_y+((k*sintable[spritei->_ang&2047])>>9),
                            spritei->_z+((k*ksgn(spritei->_zvel))*klabs(spritei->_zvel/24)),FIRELASER,-40+(k<<2),
                            spritei->_xrepeat,spritei->_yrepeat,0,0,0,spritei->_owner,5);

                        setSprite_cstat(x, 128);
                        setSprite_pal(x, spritei->_pal);
                    }
                }
                else if(getSprite(i)->_picnum == SPIT) if(getSprite(i)->_zvel < 6144)
                    setSprite_zvel(i, getSprite(i)->_zvel + gc-112);

                if( j != 0 )
                {
                    if(getSprite(i)->_picnum == COOLEXPLOSION1)
                    {
#if !NEW_MAXSPRITES
                        if( (j&49152) == 49152 && sprite[j&(MAXSPRITES-1)].picnum != APLAYER)
#else
                        CHECK_SPRITE(j);
                        if( (j&49152) == 49152 && getSprite(j & MAX_SPRITESMASK)->_picnum != APLAYER)
#endif
                            goto BOLT;
                        setSprite_xvel(i, 0);
                        setSprite_zvel(i, 0);
                    }

                    if( (j&49152) == 49152 )
                    {
#if !NEW_MAXSPRITES
                        j &= (MAXSPRITES-1);
#else
                        CHECK_SPRITE(j);
                        j &= MAX_SPRITESMASK;
#endif
                        if(getSprite(i)->_picnum == FREEZEBLAST && getSprite(j)->_pal == 1 )
                            if( badguy(getSprite(j)) || getSprite(j)->_picnum == APLAYER )
                        {
                            j = spawn(i,TRANSPORTERSTAR);
                            setSprite_pal(j, 1);
                            setSprite_xrepeat(j, 32);
                            setSprite_yrepeat(j, 32);

                            KILLIT(i);
                        }

                        checkhitsprite(j,i);

                        if(getSprite(j)->_picnum == APLAYER)
                        {
                            p = getSprite(j)->_yvel;
                            spritesound(PISTOL_BODYHIT,j);

                            if(getSprite(i)->_picnum == SPIT)
                            {
                                ps[p].horiz += 32;
                                ps[p].return_to_center = 8;

                                if(ps[p].loogcnt == 0)
                                {
//                                    if(Sound[DUKE_LONGTERM_PAIN].num < 1)
                                    if(getSoundNum(DUKE_LONGTERM_PAIN) == 0)
                                        spritesound(DUKE_LONGTERM_PAIN,ps[p].i);

                                    j = 3+(TRAND&3);
                                    ps[p].numloogs = j;
                                    ps[p].loogcnt = 24*4;
                                    for(x=0;x < j;x++)
                                    {
                                        ps[p].loogiex[x] = TRAND%xdim;
                                        ps[p].loogiey[x] = TRAND%ydim;
                                    }
                                }
                            }
                        }
                    }
                    else if( (j&49152) == 32768 )
                    {
                       // j &= (MAXWALLS-1);
                        j &= ~0xC000;
                        if(getSprite(i)->_picnum != RPG && getSprite(i)->_picnum != FREEZEBLAST && getSprite(i)->_picnum != SPIT && ( getWall_overpicnum(j) == MIRROR || getWall_picnum(j) == MIRROR ) )
                        {
                            k = getangle(
                                    getWall(getWall_point2(j))->x-getWall(j)->x,
                                    getWall(getWall_point2(j))->y-getWall(j)->y);
                            setSprite_ang(i, ((k<<1) - getSprite(i)->_ang)&2047);
                            setSprite_owner(i, i);
                            spawn(i,TRANSPORTERSTAR);
                            goto BOLT;
                        }
                        else
                        {
                            setsprite(i,dax,day,daz);
                            checkhitwall(i,j,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z,getSprite(i)->_picnum);

                            if(getSprite(i)->_picnum == FREEZEBLAST)
                            {
                                if( getWall_overpicnum(j) != MIRROR && getWall_picnum(j) != MIRROR )
                                {
                                    setSprite_extra(i, getSprite(i)->_extra >> 1);
                                    setSprite_yvel(i, getSprite(i)->_yvel - 1);
                                }

                                k = getangle(
                                    getWall(getWall_point2(j))->x-getWall(j)->x,
                                    getWall(getWall_point2(j))->y-getWall(j)->y);
                                setSprite_ang(i, ((k<<1) - getSprite(i)->_ang)&2047);
                                goto BOLT;
                            }
                        }
                    }
                    else if( (j&49152) == 16384)
                    {
                        setsprite(i,dax,day,daz);

                        if(getSprite(i)->_zvel < 0)
                        {
                            if( getSector(getSprite(i)->_sectnum)->ceilingstat&1 )
                                if(getSector(getSprite(i)->_sectnum)->ceilingpal == 0)
                                    KILLIT(i);

                            checkhitceiling(getSprite(i)->_sectnum);
                        }

                        if(getSprite(i)->_picnum == FREEZEBLAST)
                        {
                            bounce(i);
                            ssp(i,qq);
                            setSprite_extra(i, getSprite(i)->_extra >> 1);
                            if(getSprite(i)->_xrepeat > 8)
                                setSprite_xrepeat(i, getSprite(i)->_xrepeat - 2);
                            if(getSprite(i)->_yrepeat > 8)
                                setSprite_yrepeat(i, getSprite(i)->_yrepeat - 2);
                            setSprite_yvel(i, getSprite(i)->_yvel - 1);
                            goto BOLT;
                        }
                    }

                    if(getSprite(i)->_picnum != SPIT)
                    {
                        if(getSprite(i)->_picnum == RPG)
                        {
                            k = spawn(i,EXPLOSION2);
                            setSprite_x(k, dax);
                            setSprite_y(k, day);
                            setSprite_z(k, daz);

                            if(getSprite(i)->_xrepeat < 10)
                            {
                                setSprite_xrepeat(k, 6);
                                setSprite_yrepeat(k, 6);
                            }
                            else if( (j&49152) == 16384)
                            {
                                if( getSprite(i)->_zvel > 0)
                                    spawn(i,EXPLOSION2BOT);
                                else
                                {
                                    setSprite_cstat(k, getSprite(k)->_cstat | 8);
                                    setSprite_z(k, getSprite(k)->_z + (48<<8));
                                }
                            }
                        }
                        else if(getSprite(i)->_picnum == SHRINKSPARK)
                        {
                            spawn(i,SHRINKEREXPLOSION);
                            spritesound(SHRINKER_HIT,i);
                            hitradius(i,shrinkerblastradius,0,0,0,0);
                        }
                        else if( getSprite(i)->_picnum != COOLEXPLOSION1 && getSprite(i)->_picnum != FREEZEBLAST && getSprite(i)->_picnum != FIRELASER)
                        {
                            k = spawn(i,EXPLOSION2);
                            setSprite_xrepeat(k, getSprite(i)->_xrepeat>>1);
                            setSprite_yrepeat(k, getSprite(k)->_xrepeat);
                            if( (j&49152) == 16384)
                            {
                                if( getSprite(i)->_zvel < 0)
                                {
                                    setSprite_cstat(k, getSprite(k)->_cstat | 8);
                                    setSprite_z(k, getSprite(k)->_z + (72<<8));
                                }
                            }
                        }
                        if( getSprite(i)->_picnum == RPG )
                        {
                            spritesound(RPG_EXPLODE,i);

                            if(getSprite(i)->_xrepeat >= 10)
                            {
                                x = getSprite(i)->_extra;
                                hitradius( i,rpgblastradius, x>>2,x>>1,x-(x>>2),x);
                            }
                            else
                            {
                                x = getSprite(i)->_extra+(global_random&3);
                                hitradius( i,(rpgblastradius>>1),x>>2,x>>1,x-(x>>2),x);
                            }
                        }
                    }
                    if(getSprite(i)->_picnum != COOLEXPLOSION1) KILLIT(i);
                }
                if(getSprite(i)->_picnum == COOLEXPLOSION1)
                {
                    setSprite_shade(i, getSprite(i)->_shade + 1);
                    if(getSprite(i)->_shade >= 40) KILLIT(i);
                }
                else if(getSprite(i)->_picnum == RPG && getSector(getSprite(i)->_sectnum)->lotag == 2 && getSprite(i)->_xrepeat >= 10 && rnd(140))
                    spawn(i,WATERBUBBLE);

                goto BOLT;


            case SHOTSPARK1:
                p = findplayer(getSprite(i),&x);
                execute(i,p,x);
                goto BOLT;
        }
        BOLT:
        i = nexti;
    }
}


void movetransports(void)
{
    uint8_t  warpspriteto;
    short i, j, k, l, p, sect, sectlotag, nexti, nextj;
    int32_t ll,onfloorz,q;

    i = headspritestat[9]; //Transporters

    while(i >= 0)
    {
        sect = SECT;
        sectlotag = getSector(sect)->lotag;

        nexti = spr_add_data[i].nextspritestat;

        if(OW == i)
        {
            i = nexti;
            continue;
        }

        onfloorz = getHittypeTemp(i, 4);

        if(getHittypeTemp(i, 0) > 0)
            setHittypeTemp(i, 0, getHittypeTemp(i, 0) - 1);

        j = headspritesect[sect];
        while(j >= 0)
        {
            nextj = spr_add_data[j].nextspritesect;

            switch(getSprite(j)->_statnum)
            {
                case 10:    // Player

                    if( getSprite(j)->_owner != -1 )
                    {
                        p = getSprite(j)->_yvel;

                        ps[p].on_warping_sector = 1;

                        if( ps[p].transporter_hold == 0 && ps[p].jumping_counter == 0 )
                        {
                            if(ps[p].on_ground && sectlotag == 0 && onfloorz && ps[p].jetpack_on == 0 )
                            {
                                if(getSprite_pal(i) == 0)
                                {
                                    spawn(i,TRANSPORTERBEAM);
                                    spritesound(TELEPORTER,i);
                                }

                                for(k=connecthead;k>=0;k=connectpoint2[k])
                                    if(ps[k].cursectnum == getSprite(OW)->_sectnum)
                                {
                                    ps[k].frag_ps = p;
                                    setSprite_extra(ps[k].i, 0);
                                }

                                ps[p].ang = getSprite(OW)->_ang;

                                if(getSprite(OW)->_owner != OW)
                                {
                                    setHittypeTemp(i, 0, 13);
                                    setHittypeTemp(OW, 0, 13);
                                    ps[p].transporter_hold = 13;
                                }

                                ps[p].bobposx = ps[p].oposx = ps[p].posx = getSprite(OW)->_x;
                                ps[p].bobposy = ps[p].oposy = ps[p].posy = getSprite(OW)->_y;
                                ps[p].oposz = ps[p].posz = getSprite(OW)->_z-PHEIGHT;

                                changespritesect(j,getSprite(OW)->_sectnum);
                                ps[p].cursectnum = getSprite(j)->_sectnum;

                                if(getSprite_pal(i) == 0)
                                {
                                    k = spawn(OW,TRANSPORTERBEAM);
                                    spritesound(TELEPORTER,k);
                                }

                                break;
                            }
                        }
                        else if( !(sectlotag == 1 && ps[p].on_ground == 1)  ) break;

                        if(onfloorz == 0 && klabs(SZ-ps[p].posz) < 6144 )
                            if( (ps[p].jetpack_on == 0 ) || (ps[p].jetpack_on && (sync[p].bits&1) ) ||
                                (ps[p].jetpack_on && (sync[p].bits&2) ) )
                        {
                            ps[p].oposx = ps[p].posx += getSprite(OW)->_x-SX;
                            ps[p].oposy = ps[p].posy += getSprite(OW)->_y-SY;

                            if( ps[p].jetpack_on && ( (sync[p].bits&1) || ps[p].jetpack_on < 11 ) )
                                ps[p].posz = getSprite(OW)->_z-6144;
                            else ps[p].posz = getSprite(OW)->_z+6144;
                            ps[p].oposz = ps[p].posz;

                            setHittype_bposx(ps[p].i, ps[p].posx);
                            setHittype_bposy(ps[p].i, ps[p].posy);
                            setHittype_bposz(ps[p].i, ps[p].posz);

                            changespritesect(j,getSprite(OW)->_sectnum);
                            ps[p].cursectnum = getSprite(OW)->_sectnum;

                            break;
                        }

                        k = 0;

                        if( onfloorz && sectlotag == 1 && ps[p].on_ground && ps[p].posz > (getSector(sect)->floorz-(16<<8)) && ( (sync[p].bits&2) || ps[p].poszv > 2048 ) )
//                        if( onfloorz && sectlotag == 1 && ps[p].posz > (getSector(sect)->floorz-(6<<8)) )
                        {
                            k = 1;
                            if(screenpeek == p)
                            {
                                FX_StopAllSounds();
                                clearsoundlocks();
                            }
                            if(getSprite(ps[p].i)->_extra > 0)
                                spritesound(DUKE_UNDERWATER,j);
                            ps[p].oposz = ps[p].posz =
                                getSector(getSprite(OW)->_sectnum)->ceilingz+(7<<8);

                            ps[p].posxv = 4096-(TRAND&8192);
                            ps[p].posyv = 4096-(TRAND&8192);

                        }

                        if( onfloorz && sectlotag == 2 && ps[p].posz < (getSector(sect)->ceilingz+(6<<8)) )
                        {
                            k = 1;
//                            if( getSprite(j)->_extra <= 0) break;
                            if(screenpeek == p)
                            {
                                FX_StopAllSounds();
                                clearsoundlocks();
                            }
                            spritesound(DUKE_GASP,j);

                            ps[p].oposz = ps[p].posz =
                                getSector(getSprite(OW)->_sectnum)->floorz-(7<<8);

                            ps[p].jumping_toggle = 1;
                            ps[p].jumping_counter = 0;
                        }

                        if(k == 1)
                        {
                            ps[p].oposx = ps[p].posx += getSprite(OW)->_x-SX;
                            ps[p].oposy = ps[p].posy += getSprite(OW)->_y-SY;

                            if(getSprite(OW)->_owner != OW)
                                ps[p].transporter_hold = -2;
                            ps[p].cursectnum = getSprite(OW)->_sectnum;

                            changespritesect(j,getSprite(OW)->_sectnum);
                            setsprite(ps[p].i,ps[p].posx,ps[p].posy,ps[p].posz+PHEIGHT);

                            setpal(&ps[p]);

                            if( (TRAND&255) < 32 )
                                spawn(j,WATERSPLASH2);

                            if(sectlotag == 1)
                                for(l = 0;l < 9;l++)
                            {
                                q = spawn(ps[p].i,WATERBUBBLE);
                                setSprite_z(q, getSprite(q)->_z + (TRAND&16383));
                            }
                        }
                    }
                    break;

                case 1:
                    switch(getSprite(j)->_picnum)
                    {
                        case SHARK:
                        case COMMANDER:
                        case OCTABRAIN:
                        case GREENSLIME:
                        case GREENSLIME+1:
                        case GREENSLIME+2:
                        case GREENSLIME+3:
                        case GREENSLIME+4:
                        case GREENSLIME+5:
                        case GREENSLIME+6:
                        case GREENSLIME+7:
                            if(getSprite(j)->_extra > 0)
                                goto JBOLT;
                    }
                case 4:
                case 5:
                case 12:
                case 13:

                    ll = klabs(getSprite(j)->_zvel);

                    {
                        warpspriteto = 0;
                        if( ll && sectlotag == 2 && getSprite(j)->_z < (getSector(sect)->ceilingz+ll) )
                            warpspriteto = 1;

                        if( ll && sectlotag == 1 && getSprite(j)->_z > (getSector(sect)->floorz-ll) )
                            warpspriteto = 1;

                        if( sectlotag == 0 && ( onfloorz || klabs(getSprite(j)->_z-SZ) < 4096) )
                        {
                            if( getSprite(OW)->_owner != OW && onfloorz && getHittypeTemp(i, 0) > 0 && getSprite(j)->_statnum != 5 )
                            {
                                setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1);
                                goto BOLT;
                            }
                            warpspriteto = 1;
                        }

                        if( warpspriteto ) switch(getSprite(j)->_picnum)
                        {
                            case TRANSPORTERSTAR:
                            case TRANSPORTERBEAM:
                            case TRIPBOMB:
                            case BULLETHOLE:
                            case WATERSPLASH2:
                            case BURNING:
                            case BURNING2:
                            case FIRE:
                            case FIRE2:
                            case TOILETWATER:
                            case LASERLINE:
                                goto JBOLT;
                            case PLAYERONWATER:
                                if(sectlotag == 2)
                                {
                                    setSprite_cstat(j, getSprite(j)->_cstat & ~SPRITE_INVISIBLE); //32767;
                                    break;
                                }
                            default:
                                if(getSprite(j)->_statnum == 5 && !(sectlotag == 1 || sectlotag == 2) )
                                    break;

                            case WATERBUBBLE:
//                                if( rnd(192) && getSprite(j)->_picnum == WATERBUBBLE)
  //                                 break;

                                if(sectlotag > 0)
                                {
                                    k = spawn(j,WATERSPLASH2);
                                    if( sectlotag == 1 && getSprite(j)->_statnum == 4 )
                                    {
                                        setSprite_xvel(k, getSprite(j)->_xvel>>1);
                                        setSprite_ang(k, getSprite(j)->_ang);
                                        ssp(k,CLIPMASK0);
                                    }
                                }

                                switch(sectlotag)
                                {
                                    case 0:
                                        if(onfloorz)
                                        {
                                            if( getSprite(j)->_statnum == 4 || ( checkcursectnums(sect) == -1 && checkcursectnums(getSprite(OW)->_sectnum)  == -1 ) )
                                            {
                                                setSprite_x(j, getSprite(j)->_x + (getSprite(OW)->_x-SX));
                                                setSprite_y(j, getSprite(j)->_y + (getSprite(OW)->_y-SY));
                                                setSprite_z(j, getSprite(j)->_z - (SZ - getSector(getSprite(OW)->_sectnum)->floorz));
                                                setSprite_ang(j, getSprite(OW)->_ang);

                                                setHittype_bposx(j, getSprite(j)->_x);
                                                setHittype_bposy(j, getSprite(j)->_y);
                                                setHittype_bposz(j, getSprite(j)->_z);

                                                if(getSprite_pal(i) == 0)
                                                {
                                                    k = spawn(i,TRANSPORTERBEAM);
                                                    spritesound(TELEPORTER,k);

                                                    k = spawn(OW,TRANSPORTERBEAM);
                                                    spritesound(TELEPORTER,k);
                                                }

                                                if( getSprite(OW)->_owner != OW )
                                                {
                                                    setHittypeTemp(i, 0, 13);
                                                    setHittypeTemp(OW, 0, 13);
                                                }

                                                changespritesect(j,getSprite(OW)->_sectnum);
                                            }
                                        }
                                        else
                                        {
                                            setSprite_x(j, getSprite(j)->_x + (getSprite(OW)->_x-SX));
                                            setSprite_y(j, getSprite(j)->_y + (getSprite(OW)->_y-SY));
                                            setSprite_z(j, getSprite(OW)->_z+4096);

                                            setHittype_bposx(j, getSprite(j)->_x);
                                            setHittype_bposy(j, getSprite(j)->_y);
                                            setHittype_bposz(j, getSprite(j)->_z);

                                            changespritesect(j,getSprite(OW)->_sectnum);
                                        }
                                        break;
                                    case 1:
                                        setSprite_x(j, getSprite(j)->_x + (getSprite(OW)->_x-SX));
                                        setSprite_y(j, getSprite(j)->_y + (getSprite(OW)->_y-SY));
                                        setSprite_z(j, getSector(getSprite(OW)->_sectnum)->ceilingz+ll);

                                        setHittype_bposx(j, getSprite(j)->_x);
                                        setHittype_bposy(j, getSprite(j)->_y);
                                        setHittype_bposz(j, getSprite(j)->_z);

                                        changespritesect(j,getSprite(OW)->_sectnum);

                                        break;
                                    case 2:
                                        setSprite_x(j, getSprite(j)->_x + (getSprite(OW)->_x-SX));
                                        setSprite_y(j, getSprite(j)->_y + (getSprite(OW)->_y-SY));
                                        setSprite_z(j, getSector(getSprite(OW)->_sectnum)->floorz-ll);

                                        setHittype_bposx(j, getSprite(j)->_x);
                                        setHittype_bposy(j, getSprite(j)->_y);
                                        setHittype_bposz(j, getSprite(j)->_z);

                                        changespritesect(j,getSprite(OW)->_sectnum);

                                        break;
                                }

                                break;
                        }
                }
                break;

            }
            JBOLT:
            j = nextj;
        }
        BOLT:
        i = nexti;
    }
}



void moveactors(void)
{
    int32_t x, m, l;
//    const int32_t *t;
    short a, i, j, nexti, nextj, sect, p;
//    const spritetype *s;
    uint16_t k;

    i = headspritestat[1];
    while(i >= 0)
    {
        nexti = spr_add_data[i].nextspritestat;

        //s = &sprite[i];

        sect = getSprite(i)->_sectnum;

        if( getSprite(i)->_xrepeat == 0 || sect < 0 || sect >= MAXSECTORS)
            KILLIT(i);

//        getHittypeTempPtr(i) = &getHittypeTempPtr(i)[0];

        setHittype_bposx(i, getSprite(i)->_x);
        setHittype_bposy(i, getSprite(i)->_y);
        setHittype_bposz(i, getSprite(i)->_z);

        switch(getSprite(i)->_picnum)
        {
            case DUCK:
            case TARGET:
                if(getSprite(i)->_cstat&32)
                {
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                    if(getHittypeTemp(i, 0) > 60)
                    {
                        setHittypeTemp(i, 0, 0); //t[0] = 0;
                        setSprite_cstat(i, 128+257+16);
                        setSprite_extra(i, 1);
                    }
                }
                else
                {
                    j = ifhitbyweapon(i);
                    if( j >= 0 )
                    {
                        setSprite_cstat(i, 32+128);
                        k = 1;

                        j = headspritestat[1];
                        while(j >= 0)
                        {
                            if( getSprite(j)->_lotag == getSprite(i)->_lotag &&
                                getSprite(j)->_picnum == getSprite(i)->_picnum )
                            {
                                if( ( getSprite(j)->_hitag && !(getSprite(j)->_cstat&32) ) ||
                                    ( !getSprite(j)->_hitag && (getSprite(j)->_cstat&32) )
                                  )
                                {
                                    k = 0;
                                    break;
                                }
                            }

                            j = spr_add_data[j].nextspritestat;
                        }

                        if(k == 1)
                        {
                            operateactivators(getSprite(i)->_lotag,-1);
                            operateforcefields(i,getSprite(i)->_lotag);
                            operatemasterswitches(getSprite(i)->_lotag);
                        }
                    }
                }
                goto BOLT;

            case RESPAWNMARKERRED:
            case RESPAWNMARKERYELLOW:
            case RESPAWNMARKERGREEN:
                setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1);
                if(getHittypeTemp(i, 0) > respawnitemtime)
                {
                    KILLIT(i);
                }
                if( getHittypeTemp(i, 0) >= (respawnitemtime>>1) && getHittypeTemp(i, 0) < ((respawnitemtime>>1)+(respawnitemtime>>2)) )
                    setSprite_picnum(i, RESPAWNMARKERYELLOW);
                else if( getHittypeTemp(i, 0) > ((respawnitemtime>>1)+(respawnitemtime>>2)) )
                    setSprite_picnum(i, RESPAWNMARKERGREEN);
                makeitfall(i);
                break;

            case HELECOPT:
            case DUKECAR:

                setSprite_z(i, getSprite(i)->_z + getSprite(i)->_zvel);
                setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;

                if(getHittypeTemp(i, 0) == 4) spritesound(WAR_AMBIENCE2,i);

                if( getHittypeTemp(i, 0) > (26*8) )
                {
                    sound(RPG_EXPLODE);
                    const spritetype *spritei = getSprite(i);
                    for(j=0;j<32;j++) RANDOMSCRAP(i, spritei);
                    earthquaketime = 16;
                    KILLIT(i);
                }
                else if((getHittypeTemp(i, 0)&3) == 0)
                    spawn(i,EXPLOSION2);
                ssp(i,CLIPMASK0);
                break;
            case RAT:
                makeitfall(i);
                IFMOVING
                {
                    if( (TRAND&255) < 3 ) spritesound(RATTY,i);
                    setSprite_ang(i, getSprite(i)->_ang + ((TRAND&31)-15+(sintable[(getHittypeTemp(i, 0)<<8)&2047]>>11)));
                }
                else
                {
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1);
                    if(getHittypeTemp(i, 0) > 1)
                    {
                        KILLIT(i);
                    }
                    else
                        setSprite_ang(i, (TRAND&2047));
                }
                if(getSprite(i)->_xvel < 128)
                    setSprite_xvel(i, getSprite(i)->_xvel + 2);
                setSprite_ang(i, getSprite(i)->_ang + (TRAND&3)-6 );
                break;
            case QUEBALL:
            case STRIPEBALL:
                if(getSprite(i)->_xvel)
                {
                    j = headspritestat[0];
                    while(j >= 0)
                    {
                        nextj = spr_add_data[j].nextspritestat;
                        if( getSprite(j)->_picnum == POCKET && ldist(getSprite(j),getSprite(i)) < 52 ) KILLIT(i);
                        j = nextj;
                    }
                    // next-hack: this to let us use bitfields.
                    int32_t sprx = getSprite(i)->_x;
                    int32_t spry = getSprite(i)->_y;
                    int32_t sprz = getSprite(i)->_z;
                    short ssec = getSprite(i)->_sectnum;
                    //
                    j = clipmove(&sprx,&spry,&sprz,&ssec,
                        (((getSprite(i)->_xvel*(sintable[(getSprite(i)->_ang+512)&2047]))>>14)*TICSPERFRAME)<<11,
                        (((getSprite(i)->_xvel*(sintable[getSprite(i)->_ang&2047]))>>14)*TICSPERFRAME)<<11,
                        24L,(4<<8),(4<<8),CLIPMASK1);
                    //
                    setSprite_x(i, sprx);
                    setSprite_y(i, spry);
                    setSprite_z(i, sprz);
                    setSprite_sectnum(i, ssec);

                    if(j&49152)
                    {
                        if( (j&49152) == 32768 )
                        {
#if !NEW_MAXSPRITES
                            j &= (MAXWALLS-1);
#else
                            j &= ~0xC000;
#endif
                            k = getangle(
                                getWall(getWall_point2(j))->x-getWall(j)->x,
                                getWall(getWall_point2(j))->y-getWall(j)->y);
                            setSprite_ang(i, ((k<<1) - getSprite(i)->_ang)&2047);
                        }
                        else if( (j&49152) == 49152 )
                        {
#if !NEW_MAXSPRITES
                            j &= (MAXSPRITES-1);
#else
                        CHECK_SPRITE(j);
                            j &= MAX_SPRITESMASK;
#endif
                            checkhitsprite(i,j);
                        }
                    }
                    setSprite_xvel(i, getSprite(i)->_xvel - 1);
                    if(getSprite(i)->_xvel < 0)
                        setSprite_xvel(i, 0);
                    if( getSprite(i)->_picnum == STRIPEBALL )
                    {
                        setSprite_cstat(i, 257);
                        setSprite_cstat(i, getSprite(i)->_cstat | (4&getSprite(i)->_xvel));
                        setSprite_cstat(i, getSprite(i)->_cstat | (8&getSprite(i)->_xvel));
                    }
                }
                else
                {
                    p = findplayer(getSprite(i),&x);

                    if( x < 1596)
                    {

//                        if(getSprite(i)->_pal == 12)
                        {
                            j = getincangle(ps[p].ang,getangle(getSprite(i)->_x-ps[p].posx,getSprite(i)->_y-ps[p].posy));
                            if( j > -64 && j < 64 && (sync[p].bits&(1<<29)) )
                                if(ps[p].toggle_key_flag == 1)
                            {
                                a = headspritestat[1];
                                while(a >= 0)
                                {
                                    if(getSprite(a)->_picnum == QUEBALL || getSprite(a)->_picnum == STRIPEBALL)
                                    {
                                        j = getincangle(ps[p].ang,getangle(getSprite(a)->_x-ps[p].posx,getSprite(a)->_y-ps[p].posy));
                                        if( j > -64 && j < 64 )
                                        {
                                            findplayer(getSprite(a),&l);
                                            if(x > l) break;
                                        }
                                    }
                                    a = spr_add_data[a].nextspritestat;
                                }
                                if(a == -1)
                                {
                                    if(getSprite(i)->_pal == 12)
                                        setSprite_xvel(i, 164);
                                    else
                                        setSprite_xvel(i, 140);
                                    setSprite_ang(i, ps[p].ang);
                                    ps[p].toggle_key_flag = 2;
                                }
                            }
                        }
                    }
                    if( x < 512 && getSprite(i)->_sectnum == ps[p].cursectnum )
                    {
                        setSprite_ang(i, getangle(getSprite(i)->_x-ps[p].posx,getSprite(i)->_y-ps[p].posy));
                        setSprite_xvel(i, 48);
                    }
                }

                break;
            case FORCESPHERE:

                if(getSprite(i)->_yvel == 0)
                {
                    setSprite_yvel(i, 1);

                    for(l=512;l<(2048-512);l+= 128)
                        for(j=0;j<2048;j += 128)
                    {
                        k = spawn(i,FORCESPHERE);
                        setSprite_cstat(k, 257+128);
                        spr_add_data[k].clipdist = 64;
                        setSprite_ang(k, j);
                        setSprite_zvel(k, sintable[l&2047]>>5);
                        setSprite_xvel(k, sintable[(l+512)&2047]>>9);
                        setSprite_owner(k, i);
                    }
                }

                if(getHittypeTemp(i, 3) > 0)
                {
                    if(getSprite(i)->_zvel < 6144)
                        setSprite_zvel(i, getSprite(i)->_zvel + 192);
                    setSprite_z(i, getSprite(i)->_z + getSprite(i)->_zvel);
                    if(getSprite(i)->_z > getSector(sect)->floorz)
                        setSprite_z(i, getSector(sect)->floorz);
                    setHittypeTemp(i, 3, getHittypeTemp(i, 3) - 1); //t[3]--;
                    if(getHittypeTemp(i, 3) == 0)
                        KILLIT(i);
                }
                else if(getHittypeTemp(i, 2) > 10)
                {
                    j = headspritestat[5];
                    while(j >= 0)
                    {
                        if(getSprite(j)->_owner == i && getSprite(j)->_picnum == FORCESPHERE)
                            setHittypeTemp(j, 1, 1+(TRAND&63));
                        j = spr_add_data[j].nextspritestat;
                    }
                    setHittypeTemp(i, 3, 64); //t[3] = 64;
                }

                goto BOLT;

            case RECON:

                getglobalz(i);

                if (getSector(getSprite(i)->_sectnum)->ceilingstat&1)
                   setSprite_shade(i, getSprite(i)->_shade + ((getSector_ceilingshade(getSprite(i)->_sectnum)-getSprite(i)->_shade)>>1));
                else
                    setSprite_shade(i, getSprite(i)->_shade + ((getSector_floorshade(getSprite(i)->_sectnum)-getSprite(i)->_shade)>>1));

                if( getSprite(i)->_z < getSector(sect)->ceilingz+(32<<8) )
                    setSprite_z(i, getSector(sect)->ceilingz+(32<<8));

                if( ud.multimode < 2 )
                {
                    if( actor_tog == 1)
                    {
                        setSprite_cstat(i, (short)SPRITE_INVISIBLE);
                        goto BOLT;
                    }
                    else if(actor_tog == 2)
                        setSprite_cstat(i, 257);
                }
                IFHIT
                {
                    if( getSprite(i)->_extra < 0 && getHittypeTemp(i, 0) != -1 )
                    {
                        setHittypeTemp(i, 0, -1); //t[0] = -1;
                        setSprite_extra(i, 0);
                    }
                    spritesound(RECO_PAIN,i);
                    const spritetype *spritei = getSprite(i);
                    RANDOMSCRAP(i, spritei);
                }

                if(getHittypeTemp(i, 0) == -1)
                {
                    setSprite_z(i, getSprite(i)->_z + 1024);
                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 1); //t[2]++;
                    if( (getHittypeTemp(i, 2)&3) == 0) spawn(i,EXPLOSION2);
                    getglobalz(i);
                    setSprite_ang(i, getSprite(i)->_ang + 96);
                    setSprite_xvel(i, 128);
                    j = ssp(i,CLIPMASK0);
                    if(j != 1 || getSprite(i)->_z > getHittype(i)->floorz)
                    {
                        const spritetype *spritei = getSprite(i);
                        for(l=0;l<16;l++)
                            RANDOMSCRAP(i, spritei);
                        spritesound(LASERTRIP_EXPLODE,i);
                        spawn(i,PIGCOP);
                        ps[myconnectindex].actors_killed++;
                        KILLIT(i);
                    }
                    goto BOLT;
                }
                else
                {
                    if( getSprite(i)->_z > getHittype(i)->floorz-(48<<8) )
                        setSprite_z(i, getHittype(i)->floorz-(48<<8));
                }

                p = findplayer(getSprite(i), &x);
                j = getSprite(i)->_owner;

                // 3 = findplayerz, 4 = shoot

                if( getHittypeTemp(i, 0) >= 4 )
                {
                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 1); //t[2]++;
                    if( (getHittypeTemp(i, 2)&15) == 0 )
                    {
                        a = getSprite(i)->_ang;
                        setSprite_ang(i, getHittype_tempang(i));
                        spritesound(RECO_ATTACK,i);
                        shoot(i,FIRELASER);
                        setSprite_ang(i, a);
                    }
                    if( getHittypeTemp(i, 2) > (26*3) || !cansee(getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z-(16<<8),getSprite(i)->_sectnum, ps[p].posx,ps[p].posy,ps[p].posz,ps[p].cursectnum ) )
                    {
                        setHittypeTemp(i, 0, 0); //t[0] = 0;
                        setHittypeTemp(i, 2, 0); //t[2] = 0;
                    }
                    else
                        setHittype_tempang(i, getHittype_tempang(i) + getincangle(getHittype_tempang(i),getangle(ps[p].posx-getSprite(i)->_x,ps[p].posy-getSprite(i)->_y))/3);
                }
                else if(getHittypeTemp(i, 0) == 2 || getHittypeTemp(i, 0) == 3)
                {
                    setHittypeTemp(i, 3, 0); //t[3] = 0;
                    if(getSprite(i)->_xvel > 0)
                        setSprite_xvel(i, getSprite(i)->_xvel - 16);
                    else
                        setSprite_xvel(i, 0);

                    if(getHittypeTemp(i, 0) == 2)
                    {
                        l = ps[p].posz-getSprite(i)->_z;
                        if( klabs(l) < (48<<8) )
                            setHittypeTemp(i, 0, 3); //t[0] = 3;
                        else
                            setSprite_z(i, getSprite(i)->_z + (sgn(ps[p].posz-getSprite(i)->_z)<<10));
                    }
                    else
                    {
                        setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 1); //t[2]++;
                        if( getHittypeTemp(i, 2) > (26*3) || !cansee(getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z-(16<<8),getSprite(i)->_sectnum, ps[p].posx,ps[p].posy,ps[p].posz,ps[p].cursectnum ) )
                        {
                            setHittypeTemp(i, 0, 1); //t[0] = 1;
                            setHittypeTemp(i, 2, 0); //t[2] = 0;
                        }
                        else if( (getHittypeTemp(i, 2)&15) == 0 )
                        {
                            spritesound(RECO_ATTACK,i);
                            shoot(i,FIRELASER);
                        }
                    }
                    setSprite_ang(i, getSprite(i)->_ang + (getincangle(getSprite(i)->_ang,getangle(ps[p].posx-getSprite(i)->_x,ps[p].posy-getSprite(i)->_y))>>2));
                }

                if( getHittypeTemp(i, 0) != 2 && getHittypeTemp(i, 0) != 3 )
                {
                    l = ldist(getSprite(j), getSprite(i));
                    if(l <= 1524)
                    {
                        a = getSprite(i)->_ang;
                        setSprite_xvel(i, getSprite(i)->_xvel >> 1);
                    }
                    else a = getangle(getSprite(j)->_x-getSprite(i)->_x,getSprite(j)->_y-getSprite(i)->_y);

                    if(getHittypeTemp(i, 0) == 1 || getHittypeTemp(i, 0) == 4) // Found a locator and going with it
                    {
                        l = dist(getSprite(j),getSprite(i));

                        if( l <= 1524 )
                        {
                            if(getHittypeTemp(i, 0) == 1)
                                setHittypeTemp(i, 0, 0); // t[0] = 0;
                            else
                                setHittypeTemp(i, 0, 5); //t[0] = 5;
                        }
                        else
                        {
                            // Control speed here
                            if(l > 1524)
                            {
                                if( getSprite(i)->_xvel < 256 )
                                    setSprite_xvel(i, getSprite(i)->_xvel + 32);
                            }
                            else
                            {
                                if(getSprite(i)->_xvel > 0)
                                    setSprite_xvel(i, getSprite(i)->_xvel - 16);
                                else
                                    setSprite_xvel(i, 0);
                            }
                        }

                        if(getHittypeTemp(i, 0) < 2)
                            setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 1); //t[2]++;

                        if( x < 6144 && getHittypeTemp(i, 0) < 2 && getHittypeTemp(i, 2) > (26*4) )
                        {
                            setHittypeTemp(i, 0, 2 + (TRAND & 2)); //t[0] = 2+(TRAND&2);
                            setHittypeTemp(i, 2, 0); //t[2] = 0;
                            setHittype_tempang(i, getSprite(i)->_ang);
                        }
                    }

                    if(getHittypeTemp(i, 0) == 0 || getHittypeTemp(i, 0) == 5)
                    {
                        if(getHittypeTemp(i, 0) == 0)
                            setHittypeTemp(i, 0, 1); //t[0] = 1;
                        else
                            setHittypeTemp(i, 0, 4); //t[0] = 4;
                        j = LocateTheLocator(getSprite(i)->_hitag,-1);
                        setSprite_owner(i, j);
                        if(j == -1)
                        {
                            j = getHittypeTemp(i, 5);
                            setSprite_hitag(i, j);
                            setSprite_owner(i, LocateTheLocator(j,-1));
                            j = getSprite(i)->_owner;
                            if(j == -1) KILLIT(i);
                        }
                        else
                            setSprite_hitag(i, getSprite(i)->_hitag + 1);
                    }

                    setHittypeTemp(i, 3, getincangle(getSprite(i)->_ang, a)); //t[3] = getincangle(getSprite(i)->_ang,a);
                    setSprite_ang(i, getSprite(i)->_ang + (getHittypeTemp(i, 3)>>3));

                    if(getSprite(i)->_z < getSprite(j)->_z)
                        setSprite_z(i, getSprite(i)->_z + 1024);
                    else
                        setSprite_z(i, getSprite(i)->_z - 1024);
                }

//                if(Sound[RECO_ROAM].num == 0 )
                if(getSoundNum(RECO_ROAM) == 0 )
                    spritesound(RECO_ROAM,i);

                ssp(i,CLIPMASK0);

                goto BOLT;

            case OOZ:
            case OOZ2:

                getglobalz(i);

                j = (getHittype(i)->floorz-getHittype(i)->ceilingz)>>9;
                if(j > 255) j = 255;

                x = 25-(j>>1);
                if(x < 8) x = 8;
                else if(x > 48) x = 48;

                setSprite_yrepeat(i, j);
                setSprite_xrepeat(i, x);
                setSprite_z(i, getHittype(i)->floorz);

                goto BOLT;

            case GREENSLIME:
            case GREENSLIME+1:
            case GREENSLIME+2:
            case GREENSLIME+3:
            case GREENSLIME+4:
            case GREENSLIME+5:
            case GREENSLIME+6:
            case GREENSLIME+7:

// #ifndef VOLUMEONE
                if( ud.multimode < 2 )
                {
                    if( actor_tog == 1)
                    {
                        setSprite_cstat(i, (short)SPRITE_INVISIBLE);
                        goto BOLT;
                    }
                    else if(actor_tog == 2)
                        setSprite_cstat(i, 257);
                }
// #endif

                setHittypeTemp(i, 1, getHittypeTemp(i, 1) + 128); //t[1]+=128;

                if(getSector(sect)->floorstat&1)
                    KILLIT(i);

                p = findplayer(getSprite(i),&x);

                if(x > 20480)
                {
                    setHittype_timetosleep(i, getHittype_timetosleep(i) + 1);
                    if( getHittype_timetosleep(i) > SLEEPTIME )
                    {
                        setHittype_timetosleep(i, 0);
                        changespritestat(i,2);
                        goto BOLT;
                    }
                }

                if(getHittypeTemp(i, 0) == -5) // FROZEN
                {
                    setHittypeTemp(i, 3, getHittypeTemp(i, 3) + 1); //t[3]++;
                    if(getHittypeTemp(i, 3) > 280)
                    {
                        setSprite_pal(i, 0);
                        setHittypeTemp(i, 0, 0); //t[0] = 0;
                        goto BOLT;
                    }
                    makeitfall(i);
                    setSprite_cstat(i, 257);
                    setSprite_picnum(i, GREENSLIME+2);
                    setSprite_extra(i, 1);
                    setSprite_pal(i, 1);
                    IFHIT
                    {
                        if(j == FREEZEBLAST) goto BOLT;
                        for(j=16; j >= 0 ;j--)
                        {
                            k = EGS(SECT,SX,SY,SZ,GLASSPIECES+(j%3),-32,36,36,TRAND&2047,32+(TRAND&63),1024-(TRAND&1023),i,5);
                            setSprite_pal(k, 1);
                        }
                        spritesound(GLASS_BREAKING,i);
                        KILLIT(i);
                    }
                    else if(x < 1024 && ps[p].quick_kick == 0)
                    {
                        j = getincangle(ps[p].ang,getangle(SX-ps[p].posx,SY-ps[p].posy));
                        if( j > -128 && j < 128 )
                            ps[p].quick_kick = 14;
                    }

                    goto BOLT;
                }

                if(x < 1596)
                    setSprite_cstat(i, 0);
                else
                    setSprite_cstat(i, 257);

                if(getHittypeTemp(i, 0) == -4) //On the player
                {
                    if( getSprite(ps[p].i)->_extra < 1 )
                    {
                        setHittypeTemp(i, 0, 0); //t[0] = 0;
                        goto BOLT;
                    }

                    setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);

                    setSprite_ang(i, ps[p].ang);

                    if( ( (sync[p].bits&4) || (ps[p].quick_kick > 0) ) && getSprite(ps[p].i)->_extra > 0 )
                        if( ps[p].quick_kick > 0 || ( ps[p].curr_weapon != HANDREMOTE_WEAPON && ps[p].curr_weapon != HANDBOMB_WEAPON && ps[p].curr_weapon != TRIPBOMB_WEAPON && ps[p].ammo_amount[ps[p].curr_weapon] >= 0) )
                    {
                        for(x=0;x<8;x++)
                        {
                            j = EGS(sect,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z-(8<<8),SCRAP3+(TRAND&3),-8,48,48,TRAND&2047,(TRAND&63)+64,-(TRAND&4095)-(getSprite(i)->_zvel>>2),i,5);
                            setSprite_pal(j, 6);
                        }

                        spritesound(SLIM_DYING,i);
                        spritesound(SQUISHED,i);
                        if( (TRAND&255) < 32 )
                        {
                            j = spawn(i,BLOODPOOL);
                            setSprite_pal(j, 0);
                        }
                        ps[p].actors_killed ++;
                        setHittypeTemp(i, 0, -3); //t[0] = -3;
                        if(ps[p].somethingonplayer == i)
                            ps[p].somethingonplayer = -1;
                        KILLIT(i);
                    }

                    setSprite_z(i, ps[p].posz+ps[p].pyoff-getHittypeTemp(i, 2)+(8<<8));

                    setSprite_z(i, getSprite(i)->_z + ((100-ps[p].horiz)<<4));

                    if( getHittypeTemp(i, 2) > 512)
                        setHittypeTemp(i, 2, getHittypeTemp(i, 2) - 128); //t[2] -= 128;

                    if( getHittypeTemp(i, 2) < 348)
                        setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 128); //t[2] += 128;

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

                    if(getHittypeTemp(i, 3)>0)
                    {
                        short frames[] = {5,5,6,6,7,7,6,5};

                        setSprite_picnum(i, GREENSLIME+frames[getHittypeTemp(i, 3)]);

                        if( getHittypeTemp(i, 3) == 5 )
                        {
                            setSprite_extra(ps[p].i, getSprite(ps[p].i)->_extra + -(5+(TRAND&3)));
                            spritesound(SLIM_ATTACK,i);
                        }

                        if(getHittypeTemp(i, 3) < 7)
                            setHittypeTemp(i, 3, getHittypeTemp(i, 3) + 1); //t[3]++;
                        else
                            setHittypeTemp(i, 3, 0); //t[3] = 0;

                    }
                    else
                    {
                        setSprite_picnum(i, GREENSLIME+5);
                        if(rnd(32))
                            setHittypeTemp(i, 3, 1); //t[3] = 1;
                    }

                    setSprite_xrepeat(i, 20+(sintable[getHittypeTemp(i, 1)&2047]>>13));
                    setSprite_yrepeat(i, 15+(sintable[getHittypeTemp(i, 1)&2047]>>13));

                    setSprite_x(i, ps[p].posx + (sintable[(ps[p].ang+512)&2047]>>7));
                    setSprite_y(i, ps[p].posy + (sintable[ps[p].ang&2047]>>7));

                    goto BOLT;
                }

                else if(getSprite(i)->_xvel < 64 && x < 768)
                {
                    if(ps[p].somethingonplayer == -1)
                    {
                        ps[p].somethingonplayer = i;
                        if(getHittypeTemp(i, 0) == 3 || getHittypeTemp(i, 0) == 2) //Falling downward
                            setHittypeTemp(i, 2, 12 << 8); //t[2] = (12<<8);
                        else
                            setHittypeTemp(i, 2, 13 << 8); //t[2] = -(13<<8); //Climbing up duke
                        setHittypeTemp(i, 0, -4); //t[0] = -4;
                    }
                }

                    IFHIT
                    {
                        spritesound(SLIM_DYING,i);

                        ps[p].actors_killed ++;
                        if(ps[p].somethingonplayer == i)
                            ps[p].somethingonplayer = -1;

                        if(j == FREEZEBLAST)
                        {
                            spritesound(SOMETHINGFROZE,i);
                            setHittypeTemp(i, 0, -5); //t[0] = -5 ;
                            setHittypeTemp(i, 3, 0); //t[3] = 0 ;
                            goto BOLT;
                        }

                        if( (TRAND&255) < 32 )
                        {
                            j = spawn(i,BLOODPOOL);
                            setSprite_pal(j, 0);
                        }

                        for(x=0;x<8;x++)
                        {
                            j = EGS(sect,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z-(8<<8),SCRAP3+(TRAND&3),-8,48,48,TRAND&2047,(TRAND&63)+64,-(TRAND&4095)-(getSprite(i)->_zvel>>2),i,5);
                            setSprite_pal(j, 6);
                        }
                        setHittypeTemp(i, 0, -3); //t[0] = -3;
                        KILLIT(i);
                    }
                        // All weap
                if(getHittypeTemp(i, 0) == -1) //Shrinking down
                {
                    makeitfall(i);

                    setSprite_cstat(i, getSprite(i)->_cstat & ( 65535-8));
                    setSprite_picnum(i, GREENSLIME+4);

//                    if(getSprite(i)->_yrepeat > 62)
  //                      guts(getSprite(i),JIBS6,5,myconnectindex);

                    if(getSprite(i)->_xrepeat > 32)
                        setSprite_xrepeat(i, getSprite(i)->_xrepeat - (TRAND&7));
                    if(getSprite(i)->_yrepeat > 16)
                        setSprite_yrepeat(i, getSprite(i)->_yrepeat - (TRAND&7));
                    else
                    {
                        setSprite_xrepeat(i, 40);
                        setSprite_yrepeat(i, 16);
                        setHittypeTemp(i, 5, -1); //t[5] = -1;
                        setHittypeTemp(i, 0, 0); //t[0] = 0;
                    }

                    goto BOLT;
                }
                else if(getHittypeTemp(i, 0) != -2) getglobalz(i);

                if(getHittypeTemp(i, 0) == -2) //On top of somebody
                {
                    makeitfall(i);
                    setSprite_xvel(getHittypeTemp(i, 5), 0);

                    l = getSprite(getHittypeTemp(i, 5))->_ang;

                    setSprite_z(i, getSprite(getHittypeTemp(i, 5))->_z);
                    setSprite_x(i, getSprite(getHittypeTemp(i, 5))->_x+(sintable[(l+512)&2047]>>11));
                    setSprite_y(i, getSprite(getHittypeTemp(i, 5))->_y+(sintable[l&2047]>>11));

                    setSprite_picnum(i,  GREENSLIME+2+(global_random&1) );

                    if(getSprite(i)->_yrepeat < 64)
                        setSprite_yrepeat(i, getSprite(i)->_yrepeat + 2);
                    else
                    {
                        if(getSprite(i)->_xrepeat < 32)
                            setSprite_xrepeat(i, getSprite(i)->_xrepeat + 4);
                        else
                        {
                            setHittypeTemp(i, 0, -1); //t[0] = -1;
                            x = ldist(getSprite(i),getSprite(getHittypeTemp(i, 5)));
                            if(x < 768)
                                setSprite_xrepeat(getHittypeTemp(i, 5), 0);
                        }
                    }

                    goto BOLT;
                }

                //Check randomly to see of there is an actor near
                if(rnd(32))
                {
                    j = headspritesect[sect];
                    while(j>=0)
                    {
                        switch(getSprite(j)->_picnum)
                        {
                            case LIZTROOP:
                            case LIZMAN:
                            case PIGCOP:
                            case NEWBEAST:
                                if( ldist(getSprite(i),getSprite(j)) < 768 && (klabs(getSprite(i)->_z-getSprite(j)->_z)<8192) ) //Gulp them
                                {
                                    setHittypeTemp(i, 5, j); //t[5] = j;
                                    setHittypeTemp(i, 0, -2); //t[0] = -2;
                                    setHittypeTemp(i, 1, 0); //t[1] = 0;
                                    goto BOLT;
                                }
                        }

                        j = spr_add_data[j].nextspritesect;
                    }
                }

                //Moving on the ground or ceiling

                if(getHittypeTemp(i, 0) == 0 || getHittypeTemp(i, 0) == 2)
                {
                    setSprite_picnum(i, GREENSLIME);

                    if( (TRAND&511) == 0 )
                        spritesound(SLIM_ROAM,i);

                    if(getHittypeTemp(i, 0)==2)
                    {
                        setSprite_zvel(i, 0);
                        setSprite_cstat(i, getSprite(i)->_cstat & (65535-8));

                        if( (getSector(sect)->ceilingstat&1) || (getHittype(i)->ceilingz+6144) < getSprite(i)->_z)
                        {
                            setSprite_z(i, getSprite(i)->_z + 2048);
                            setHittypeTemp(i, 0, 3); //t[0] = 3;
                            goto BOLT;
                        }
                    }
                    else
                    {
                        setSprite_cstat(i, getSprite(i)->_cstat | 8);
                        makeitfall(i);
                    }

                    if( everyothertime&1 ) ssp(i,CLIPMASK0);

                    if(getSprite(i)->_xvel > 96)
                    {
                        setSprite_xvel(i, getSprite(i)->_xvel - 2);
                        goto BOLT;
                    }
                    else
                    {
                        if(getSprite(i)->_xvel < 32)
                            setSprite_xvel(i, getSprite(i)->_xvel + 4);
                        setSprite_xvel(i, 64 - (sintable[(getHittypeTemp(i, 1)+512)&2047]>>9));

                        setSprite_ang(i, getSprite(i)->_ang + (getincangle(getSprite(i)->_ang,
                               getangle(ps[p].posx-getSprite(i)->_x,ps[p].posy-getSprite(i)->_y))>>3));
// TJR
                    }

                    setSprite_xrepeat(i, 36 + (sintable[(getHittypeTemp(i, 1)+512)&2047]>>11));
                    setSprite_yrepeat(i, 16 + (sintable[getHittypeTemp(i, 1)&2047]>>13));

                    if(rnd(4) && (getSector(sect)->ceilingstat&1) == 0 &&
                        klabs(getHittype(i)->floorz-getHittype(i)->ceilingz)
                            < (192<<8) )
                            {
                                setSprite_zvel(i, 0);
                                setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                            }

                }

                if(getHittypeTemp(i, 0)==1)
                {
                    setSprite_picnum(i, GREENSLIME);
                    if(getSprite(i)->_yrepeat < 40)
                        setSprite_yrepeat(i, getSprite(i)->_yrepeat + 8);
                    if(getSprite(i)->_xrepeat > 8)
                        setSprite_xrepeat(i, getSprite(i)->_xrepeat - 4);
                    if(getSprite(i)->_zvel > -(2048+1024))
                        setSprite_zvel(i, getSprite(i)->_zvel - 348);
                    setSprite_z(i, getSprite(i)->_z + getSprite(i)->_zvel);
                    if(getSprite(i)->_z < getHittype(i)->ceilingz+4096)
                    {
                        setSprite_z(i, getHittype(i)->ceilingz+4096);
                        setSprite_xvel(i, 0);
                        setHittypeTemp(i, 0, 2); //t[0] = 2;
                    }
                }

                if(getHittypeTemp(i, 0)==3)
                {
                    setSprite_picnum(i, GREENSLIME+1);

                    makeitfall(i);

                    if(getSprite(i)->_z > getHittype(i)->floorz-(8<<8))
                    {
                        setSprite_yrepeat(i, getSprite(i)->_yrepeat - 4);
                        setSprite_xrepeat(i, getSprite(i)->_xrepeat + 2);
                    }
                    else
                    {
                        if(getSprite(i)->_yrepeat < (40-4))
                            setSprite_yrepeat(i, getSprite(i)->_yrepeat + 8);
                        if(getSprite(i)->_xrepeat > 8)
                            setSprite_xrepeat(i, getSprite(i)->_xrepeat - 4);
                    }

                    if(getSprite(i)->_z > getHittype(i)->floorz-2048)
                    {
                        setSprite_z(i, getHittype(i)->floorz-2048);
                        setHittypeTemp(i, 0, 0); //t[0] = 0;
                        setSprite_xvel(i, 0);
                    }
                }
                goto BOLT;

            case BOUNCEMINE:
            case MORTER:
                j = spawn(i,FRAMEEFFECT1);
                setHittypeTemp(j, 0, 3);

            case HEAVYHBOMB:

                if( (getSprite(i)->_cstat&SPRITE_INVISIBLE) )
                {
                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) - 1); //t[2]--;
                    if(getHittypeTemp(i, 2) <= 0)
                    {
                        spritesound(TELEPORTER,i);
                        spawn(i,TRANSPORTERSTAR);
                        setSprite_cstat(i, 257);
                    }
                    goto BOLT;
                }

                p = findplayer(getSprite(i),&x);

                if( x < 1220 )
                    setSprite_cstat(i, getSprite(i)->_cstat & ~257);
                else
                    setSprite_cstat(i, getSprite(i)->_cstat | 257);

                if(getHittypeTemp(i, 3) == 0 )
                {
                    j = ifhitbyweapon(i);
                    if(j >= 0)
                    {
                        setHittypeTemp(i, 3, 1); //t[3] = 1;
                        setHittypeTemp(i, 4, 0); //t[4] = 0;
                        l = 0;
                        setSprite_xvel(i, 0);
                        goto DETONATEB;
                    }
                }

                if( getSprite(i)->_picnum != BOUNCEMINE )
                {
                    makeitfall(i);

                    if( getSector(sect)->lotag != 1 && getSprite(i)->_z >= getHittype(i)->floorz-(FOURSLEIGHT) && getSprite(i)->_yvel < 3 )
                    {
                        if( getSprite(i)->_yvel > 0 || (getSprite(i)->_yvel == 0 && getHittype(i)->floorz == getSector(sect)->floorz ))
                            spritesound(PIPEBOMB_BOUNCE,i);
                        setSprite_zvel(i, -((4-getSprite(i)->_yvel)<<8));
                        if(getSector(getSprite(i)->_sectnum)->lotag== 2)
                            setSprite_zvel(i, getSprite(i)->_zvel >> 2);
                        setSprite_yvel(i, getSprite(i)->_yvel + 1);
                    }
                    if( getSprite(i)->_z < getHittype(i)->ceilingz ) // && getSector(sect)->lotag != 2 )
                    {
                        setSprite_z(i, getHittype(i)->ceilingz+(3<<8));
                        setSprite_zvel(i, 0);
                    }
                }

                j = movesprite(i,
                    (getSprite(i)->_xvel*(sintable[(getSprite(i)->_ang+512)&2047]))>>14,
                    (getSprite(i)->_xvel*(sintable[getSprite(i)->_ang&2047]))>>14,
                    getSprite(i)->_zvel,CLIPMASK0);

                if(getSector(SECT)->lotag == 1 && getSprite(i)->_zvel == 0)
                {
                    setSprite_z(i, getSprite(i)->_z + (32<<8));
                    if(getHittypeTemp(i, 5) == 0)
                    {
                        setHittypeTemp(i, 5, 1); //t[5] = 1;
                        spawn(i,WATERSPLASH2);
                    }
                }
                else
                    setHittypeTemp(i, 5, 0); //t[5] = 0;

                if(getHittypeTemp(i, 3) == 0 && ( getSprite(i)->_picnum == BOUNCEMINE || getSprite(i)->_picnum == MORTER ) && (j || x < 844) )
                {
                    setHittypeTemp(i, 3, 1); //t[3] = 1;
                    setHittypeTemp(i, 4, 0); //t[4] = 0;
                    l = 0;
                    setSprite_xvel(i, 0);
                    goto DETONATEB;
                }

                if(getSprite(getSprite(i)->_owner)->_picnum == APLAYER)
                    l = getSprite(getSprite(i)->_owner)->_yvel;
                else l = -1;

                if(getSprite(i)->_xvel > 0)
                {
                    setSprite_xvel(i, getSprite(i)->_xvel -  5);
                    if(getSector(sect)->lotag == 2)
                        setSprite_xvel(i, getSprite(i)->_xvel - 10);

                    if(getSprite(i)->_xvel < 0)
                        setSprite_xvel(i, 0);
                    if(getSprite(i)->_xvel&8)
                        setSprite_cstat(i, getSprite(i)->_cstat ^ 4);
                }

                if( (j&49152) == 32768 )
                {
                  //  j &= (MAXWALLS-1);
                    j &= ~0xC000;
                    checkhitwall(i,j,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z,getSprite(i)->_picnum);

                    k = getangle(
                        getWall(getWall_point2(j))->x-getWall(j)->x,
                        getWall(getWall_point2(j))->y-getWall(j)->y);

                    setSprite_ang(i, ((k<<1) - getSprite(i)->_ang)&2047);
                    setSprite_xvel(i, getSprite(i)->_xvel >> 1);
                }

                DETONATEB:

                if( ( l >= 0 && ps[l].hbomb_on == 0 ) || getHittypeTemp(i, 3) == 1)
                {
                    setHittypeTemp(i, 4, getHittypeTemp(i, 4) + 1); //t[4]++;

                    if(getHittypeTemp(i, 4) == 2)
                    {
                        x = getSprite(i)->_extra;
                        m = 0;
                        switch(getSprite(i)->_picnum)
                        {
                            case HEAVYHBOMB: m = pipebombblastradius;break;
                            case MORTER: m = morterblastradius;break;
                            case BOUNCEMINE: m = bouncemineblastradius;break;
                        }

                        hitradius( i, m,x>>2,x>>1,x-(x>>2),x);
                        spawn(i,EXPLOSION2);
                        if( getSprite(i)->_zvel == 0 )
                            spawn(i,EXPLOSION2BOT);
                        spritesound(PIPEBOMB_EXPLODE,i);
                        const spritetype *spritei = getSprite(i);
                        for(x=0;x<8;x++)
                            RANDOMSCRAP(i, spritei);
                    }

                    if(getSprite(i)->_yrepeat)
                    {
                        setSprite_yrepeat(i, 0);
                        goto BOLT;
                    }

                    if(getHittypeTemp(i, 4) > 20)
                    {
                        if(getSprite(i)->_owner != i || ud.respawn_items == 0)
                        {
                            KILLIT(i);
                        }
                        else
                        {
                            setHittypeTemp(i, 2, respawnitemtime); //t[2] = respawnitemtime;
                            spawn(i,RESPAWNMARKERRED);
                            setSprite_cstat(i, (short) SPRITE_INVISIBLE);
                            setSprite_yrepeat(i, 9);
                            goto BOLT;
                        }
                    }
                }
                else if(getSprite(i)->_picnum == HEAVYHBOMB && x < 788 && getHittypeTemp(i, 0) > 7 && getSprite(i)->_xvel == 0)
                    if( cansee(getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z-(8<<8),getSprite(i)->_sectnum,ps[p].posx,ps[p].posy,ps[p].posz,ps[p].cursectnum) )
                        if(ps[p].ammo_amount[HANDBOMB_WEAPON] < max_ammo_amount[HANDBOMB_WEAPON] )
                {
                    if(ud.coop >= 1 && getSprite(i)->_owner == i)
                    {
                        for(j=0;j<ps[p].weapreccnt;j++)
                            if(ps[p].weaprecs[j] == getSprite(i)->_picnum)
                                goto BOLT;

                        if(ps[p].weapreccnt < 255)
                            ps[p].weaprecs[ps[p].weapreccnt++] = getSprite(i)->_picnum;
                    }

                    addammo(HANDBOMB_WEAPON,&ps[p],1);
                    spritesound(DUKE_GET,ps[p].i);

                    if( ps[p].gotweapon[HANDBOMB_WEAPON] == 0 || getSprite(i)->_owner == ps[p].i )
                        addweapon(&ps[p],HANDBOMB_WEAPON);

                    if( getSprite(getSprite(i)->_owner)->_picnum != APLAYER )
                    {
                        ps[p].pals[0] = 0;
                        ps[p].pals[1] = 32;
                        ps[p].pals[2] = 0;
                        ps[p].pals_time = 32;
                    }

                    if( getSprite(i)->_owner != i || ud.respawn_items == 0 )
                    {
                        if(getSprite(i)->_owner == i && ud.coop >= 1)
                            goto BOLT;
                        KILLIT(i);
                    }
                    else
                    {
                        setHittypeTemp(i, 2, respawnitemtime); //t[2] = respawnitemtime;
                        spawn(i,RESPAWNMARKERRED);
                        setSprite_cstat(i, (short) SPRITE_INVISIBLE);
                    }
                }

                if(getHittypeTemp(i, 0) < 8)
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                goto BOLT;

            case REACTORBURNT:
            case REACTOR2BURNT:
                goto BOLT;

            case REACTOR:
            case REACTOR2:

                if( getHittypeTemp(i, 4) == 1 )
                {
                    j = headspritesect[sect];
                    while(j >= 0)
                    {
                        switch(getSprite(j)->_picnum)
                        {
                            case SECTOREFFECTOR:
                                if(getSprite(j)->_lotag == 1)
                                {
                                    setSprite_lotag(j, (short) 65535);
                                    setSprite_hitag(j, (short) 65535);
                                }
                                break;
                            case REACTOR:
                                setSprite_picnum(j, REACTORBURNT);
                                break;
                            case REACTOR2:
                                setSprite_picnum(j, REACTOR2BURNT);
                                break;
                            case REACTORSPARK:
                            case REACTOR2SPARK:
                                setSprite_cstat(j, (short) SPRITE_INVISIBLE);
                                break;
                        }
                        j = spr_add_data[j].nextspritesect;
                    }
                    goto BOLT;
                }

                if(getHittypeTemp(i, 1) >= 20)
                {
                    setHittypeTemp(i, 4, 1); //t[4] = 1;
                    goto BOLT;
                }

                p = findplayer(getSprite(i),&x);

                setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 1); //t[2]++;
                if( getHittypeTemp(i, 2) == 4 )
                    setHittypeTemp(i, 2, 0); //t[2]=0;

                if( x < 4096 )
                {
                    if( (TRAND&255) < 16 )
                    {
//                        if(Sound[DUKE_LONGTERM_PAIN].num < 1)
                        if(getSoundNum(DUKE_LONGTERM_PAIN) == 0)
                            spritesound(DUKE_LONGTERM_PAIN,ps[p].i);

                        spritesound(SHORT_CIRCUIT,i);

                        setSprite_extra(ps[p].i, getSprite(ps[p].i)->_extra - 1);
                        ps[p].pals_time = 32;
                        ps[p].pals[0] = 32;
                        ps[p].pals[1] = 0;
                        ps[p].pals[2] = 0;
                    }
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 128); //t[0] += 128;
                    if( getHittypeTemp(i, 3) == 0 )
                        setHittypeTemp(i, 3, 1); //t[3] = 1;
                }
                else
                    setHittypeTemp(i, 3, 0); //t[3] = 0;

                if( getHittypeTemp(i, 1) )
                {
                    setHittypeTemp(i, 1, getHittypeTemp(i, 1) + 1); //t[1]++;

                    setHittypeTemp(i, 4, getSprite(i)->_z); //t[4] = getSprite(i)->_z;
                    setSprite_z(i, getSector(sect)->floorz-(TRAND%(getSector(sect)->floorz-getSector(sect)->ceilingz)));

                    switch( getHittypeTemp(i, 1) )
                    {
                        case 3:
                            //Turn on all of those flashing getSectoreffector.
                            hitradius( i, 4096,
                                       impact_damage<<2,
                                       impact_damage<<2,
                                       impact_damage<<2,
                                       impact_damage<<2 );
/*
                            j = headspritestat[3];
                            while(j>=0)
                            {
                                if( getSprite(j)->_lotag  == 3 )
                                    getHittypeTemp(j, 4)=1;
                                else if(getSprite(j)->_lotag == 12)
                                {
                                    setHittypeTemp(j, 4, 1);
                                    getSprite(j)->_lotag = 3;
                                    getSprite(j)->_owner = 0;
                                    getHittypeTemp(j, 0) = getSprite(i)->_shade;
                                }
                                j = spr_add_data[j].nextspritestat;
                            }
*/
                            j = headspritestat[6];
                            while(j >= 0)
                            {
                                if(getSprite(j)->_picnum == MASTERSWITCH)
                                    if(getSprite(j)->_hitag == getSprite(i)->_hitag)
                                        if(getSprite(j)->_yvel == 0)
                                            setSprite_yvel(j, 1);
                                j = spr_add_data[j].nextspritestat;
                            }
                            break;

                        case 4:
                        case 7:
                        case 10:
                        case 15:
                            j = headspritesect[sect];
                            while(j >= 0)
                            {
                                l = spr_add_data[j].nextspritesect;

                                if(j != i)
                                {
                                    deletesprite(j);
                                    break;
                                }
                                j = l;
                            }
                            break;
                    }
                    const spritetype *spritei = getSprite(i);
                    for(x=0;x<16;x++)
                        RANDOMSCRAP(i, spritei);

                    setSprite_z(i, getHittypeTemp(i, 4));
                    setHittypeTemp(i, 4, 0); //t[4] = 0;

                }
                else
                {
                    IFHIT
                    {
                        const spritetype *spritei = getSprite(i);
                        for(x=0;x<32;x++)
                            RANDOMSCRAP(i, spritei);
                        if(getSprite(i)->_extra < 0)
                            setHittypeTemp(i, 1, 1); //t[1] = 1;
                    }
                }
                goto BOLT;

            case CAMERA1:

                if( getHittypeTemp(i, 0) == 0 )
                {
                    setHittypeTemp(i, 1, getHittypeTemp(i, 1) + 8); //t[1]+=8;
                    if(camerashitable)
                    {
                        IFHIT
                        {
                            setHittypeTemp(i, 0, 1); //t[0] = 1; // static
                            setSprite_cstat(i, (short)SPRITE_INVISIBLE);
                            const spritetype *spritei = getSprite(i);
                            for(x=0;x<5;x++) RANDOMSCRAP(i, spritei);
                            goto BOLT;
                        }
                    }

                    if(getSprite(i)->_hitag > 0)
                    {
                        if(getHittypeTemp(i, 1)<getSprite(i)->_hitag)
                            setSprite_ang(i, getSprite(i)->_ang + 8);
                        else if(getHittypeTemp(i, 1)<(getSprite(i)->_hitag*3))
                            setSprite_ang(i, getSprite(i)->_ang - 8);
                        else if(getHittypeTemp(i, 1) < (getSprite(i)->_hitag<<2) )
                            setSprite_ang(i, getSprite(i)->_ang + 8);
                        else
                        {
                            setHittypeTemp(i, 1, 8); //t[1]=8;
                            setSprite_ang(i, getSprite(i)->_ang + 16);
                        }
                    }
                }
                goto BOLT;
        }


// #ifndef VOLOMEONE
        if( ud.multimode < 2 && badguy(getSprite(i)) )
        {
            if( actor_tog == 1)
            {
                setSprite_cstat(i, (short)SPRITE_INVISIBLE);
                goto BOLT;
            }
            else if(actor_tog == 2)
                setSprite_cstat(i, 257);
        }
// #endif

        p = findplayer(getSprite(i),&x);

        execute(i,p,x);

        BOLT:

        i = nexti;
    }

}


void moveexplosions(void)  // STATNUM 5
{
    short i, j, nexti, sect, p;
    int32_t l, x;
//    const int32_t *t;
//    spritetype *s;

    i = headspritestat[5];
    while(i >= 0)
    {
        nexti = spr_add_data[i].nextspritestat;

//        getHittypeTempPtr(i) = &getHittypeTemp(i, 0);
//        s = &sprite[i];
//        spritetypeoffs *so = &sprite_offsets[i];
        sect = getSprite(i)->_sectnum;

        if( sect < 0 || getSprite(i)->_xrepeat == 0 ) KILLIT(i);

        setHittype_bposx(i, getSprite(i)->_x);
        setHittype_bposy(i, getSprite(i)->_y);
        setHittype_bposz(i, getSprite(i)->_z);

        switch(getSprite(i)->_picnum)
        {
            case NEON1:
            case NEON2:
            case NEON3:
            case NEON4:
            case NEON5:
            case NEON6:

                if( (global_random/(getSprite(i)->_lotag+1)&31) > 4)
                    setSprite_shade(i, -127);
                else
                    setSprite_shade(i, 127);
                goto BOLT;

            case BLOODSPLAT1:
            case BLOODSPLAT2:
            case BLOODSPLAT3:
            case BLOODSPLAT4:

                if( getHittypeTemp(i, 0) == 7*26 )
                    goto BOLT;
                setSprite_z(i, getSprite(i)->_z + 16+(TRAND&15));
                setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                if( (getHittypeTemp(i, 0)%9) == 0 )
                    setSprite_yrepeat(i, getSprite(i)->_yrepeat + 1);
                goto BOLT;

            case NUKEBUTTON:
            case NUKEBUTTON+1:
            case NUKEBUTTON+2:
            case NUKEBUTTON+3:

                if(getHittypeTemp(i, 0))
                {
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                    if(getHittypeTemp(i, 0) == 8)
                        setSprite_picnum(i, NUKEBUTTON+1);
                    else if(getHittypeTemp(i, 0) == 16)
                    {
                        setSprite_picnum(i, NUKEBUTTON+2);
                        ps[getSprite(getSprite(i)->_owner)->_yvel].fist_incs = 1;
                    }
                    if( ps[getSprite(getSprite(i)->_owner)->_yvel].fist_incs == 26 )
                        setSprite_picnum(i, NUKEBUTTON+3);
                }
                goto BOLT;

            case FORCESPHERE:

                l = getSprite(i)->_xrepeat;
                if(getHittypeTemp(i, 1) > 0)
                {
                    setHittypeTemp(i, 1, getHittypeTemp(i, 1) - 1); //t[1]--;
                    if(getHittypeTemp(i, 1) == 0)
                    {
                        KILLIT(i);
                    }
                }
                if(getHittypeTemp(getSprite(i)->_owner, 1) == 0)
                {
                    if(getHittypeTemp(i, 0) < 64)
                    {
                        setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                        l += 3;
                    }
                }
                else
                    if(getHittypeTemp(i, 0) > 64)
                    {
                        setHittypeTemp(i, 0, getHittypeTemp(i, 0) - 1); //t[0]--;
                        l -= 3;
                    }

                setSprite_x(i, getSprite(getSprite(i)->_owner)->_x);
                setSprite_y(i, getSprite(getSprite(i)->_owner)->_y);
                setSprite_z(i, getSprite(getSprite(i)->_owner)->_z);
                setSprite_ang(i, getSprite(i)->_ang + getHittypeTemp(getSprite(i)->_owner, 0));

                if(l > 64) l = 64;
                else if(l < 1) l = 1;

                setSprite_xrepeat(i, l);
                setSprite_yrepeat(i, l);
                setSprite_shade(i, (l>>1)-48);

                for(j=getHittypeTemp(i, 0);j > 0;j--)
                    ssp(i,CLIPMASK0);
                goto BOLT;
            case WATERSPLASH2:

                setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                if(getHittypeTemp(i, 0) == 1 )
                {
                    if(getSector(sect)->lotag != 1 && getSector(sect)->lotag != 2)
                        KILLIT(i);
/*                    else
                    {
                        l = getflorzofslope(sect,getSprite(i)->_x,getSprite(i)->_y)-getSprite(i)->_z;
                        if( l > (16<<8) ) KILLIT(i);
                    }
                    else */
//                        if(Sound[ITEM_SPLASH].num == 0)
                        if(getSoundNum(ITEM_SPLASH) == 0)
                        spritesound(ITEM_SPLASH,i);
                }
                if(getHittypeTemp(i, 0) == 3)
                {
                    setHittypeTemp(i, 0, 0); //t[0] = 0;
                    setHittypeTemp(i, 1, getHittypeTemp(i, 1) + 1); //t[1]++;
                }
                if(getHittypeTemp(i, 1) == 5)
                    deletesprite(i);
                goto BOLT;

            case FRAMEEFFECT1:
			case FRAMEEFFECT1_13CON:
                if(getSprite(i)->_owner >= 0)
                {
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;

                    if( getHittypeTemp(i, 0) > 7 )
                    {
                        KILLIT(i);
                    }
                    else if( getHittypeTemp(i, 0) > 4 )
                        setSprite_cstat(i, getSprite(i)->_cstat | (512+2));
                    else if( getHittypeTemp(i, 0) > 2 )
                        setSprite_cstat(i, getSprite(i)->_cstat | 2);
                    setSprite_xoffset(i, getSprite(getSprite(i)->_owner)->_xoffset);
                    setSprite_yoffset(i, getSprite(getSprite(i)->_owner)->_yoffset);
                }
                goto BOLT;
            case INNERJAW:
            case INNERJAW+1:

                p = findplayer(getSprite(i),&x);
                if(x < 512)
                {
                    ps[p].pals_time = 32;
                    ps[p].pals[0] = 32;
                    ps[p].pals[1] = 0;
                    ps[p].pals[2] = 0;
                    setSprite_extra(ps[p].i, getSprite(ps[p].i)->_extra - 4);
                }

            case FIRELASER:
                if(getSprite(i)->_extra != 999)
                    setSprite_extra(i, 999);
                else KILLIT(i);
                break;
            case TONGUE:
                KILLIT(i);
            case MONEY+1:
            case MAIL+1:
            case PAPER+1:
                setHittype_floorz(i, getflorzofslope(getSprite(i)->_sectnum,getSprite(i)->_x,getSprite(i)->_y));
                setSprite_z(i, getHittype(i)->floorz);
                break;
            case MONEY:
            case MAIL:
            case PAPER:

                setSprite_xvel(i, (TRAND&7)+(sintable[getHittypeTemp(i, 0)&2047]>>9));
                setHittypeTemp(i, 0, getHittypeTemp(i, 0) + (TRAND&63));
                if( (getHittypeTemp(i, 0)&2047) > 512 && (getHittypeTemp(i, 0)&2047) < 1596)
                {
                    if(getSector(sect)->lotag == 2)
                    {
                        if(getSprite(i)->_zvel < 64)
                            setSprite_zvel(i, getSprite(i)->_zvel + (gc>>5)+(TRAND&7));
                    }
                    else
                        if(getSprite(i)->_zvel < 144)
                            setSprite_zvel(i, getSprite(i)->_zvel + (gc>>5)+(TRAND&7));
                }

                ssp(i,CLIPMASK0);

                if( (TRAND&3) == 0 )
                    setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);

                if(getSprite(i)->_sectnum == -1) KILLIT(i);
                l = getflorzofslope(getSprite(i)->_sectnum,getSprite(i)->_x,getSprite(i)->_y);

                if( getSprite(i)->_z > l )
                {
                    setSprite_z(i, l);

                    insertspriteq(i);
                    setSprite_picnum(i, getSprite_picnum(i) + 1); //PN ++;  <--- next-hack: what a crap!

                    j = headspritestat[5];
                    while(j >= 0)
                    {
                        if(getSprite(j)->_picnum == BLOODPOOL)
                            if(ldist(getSprite(i),getSprite(j)) < 348)
                        {
                            setSprite_pal(i, 2);
                            break;
                        }
                        j = spr_add_data[j].nextspritestat;
                    }
                }

                break;

            case JIBS1:
            case JIBS2:
            case JIBS3:
            case JIBS4:
            case JIBS5:
            case JIBS6:
            case HEADJIB1:
            case ARMJIB1:
            case LEGJIB1:
            case LIZMANHEAD1:
            case LIZMANARM1:
            case LIZMANLEG1:
            case DUKETORSO:
            case DUKEGUN:
            case DUKELEG:

                if(getSprite(i)->_xvel > 0)
                    setSprite_xvel(i, getSprite(i)->_xvel - 1);
                else
                    setSprite_xvel(i, 0);

                if( getHittypeTemp(i, 5) < 30*10 )
                    setHittypeTemp(i, 5, getHittypeTemp(i, 5) + 1); //t[5]++;
                else { KILLIT(i); }


                if(getSprite(i)->_zvel > 1024 && getSprite(i)->_zvel < 1280)
                {
                    setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                    sect = getSprite(i)->_sectnum;
                }

                l = getflorzofslope(sect,getSprite(i)->_x,getSprite(i)->_y);
                x = getceilzofslope(sect,getSprite(i)->_x,getSprite(i)->_y);
                if(x == l || sect < 0 || sect >= MAXSECTORS) KILLIT(i);

                if( getSprite(i)->_z < l-(2<<8) )
                {
                    if(getHittypeTemp(i, 1) < 2)
                        setHittypeTemp(i, 1, getHittypeTemp(i, 1) + 1); //t[1]++;
                    else if(getSector(sect)->lotag != 2)
                    {
                        setHittypeTemp(i, 1, 0); //t[1] = 0;
                        if( getSprite(i)->_picnum == DUKELEG || getSprite(i)->_picnum == DUKETORSO || getSprite(i)->_picnum == DUKEGUN )
                        {
                            if(getHittypeTemp(i, 0) > 6)
                            setHittypeTemp(i, 0, 0); //t[0] = 0;
                            else
                                setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                        }
                        else
                        {
                            if(getHittypeTemp(i, 0) > 2)
                                setHittypeTemp(i, 0, 0); //t[0] = 0;
                            else
                                setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //t[0]++;
                        }
                    }

                    if(getSprite(i)->_zvel < 6144)
                    {
                        if(getSector(sect)->lotag == 2)
                        {
                            if(getSprite(i)->_zvel < 1024)
                                setSprite_zvel(i, getSprite(i)->_zvel + 48);
                            else
                                setSprite_zvel(i, 1024);
                        }
                        else
                            setSprite_zvel(i, getSprite(i)->_zvel + gc-50);
                    }

                    setSprite_x(i, getSprite(i)->_x + ( (getSprite(i)->_xvel*sintable[(getSprite(i)->_ang+512)&2047])>>14));
                    setSprite_y(i, getSprite(i)->_y + ( (getSprite(i)->_xvel*sintable[getSprite(i)->_ang&2047])>>14));
                    setSprite_z(i, getSprite(i)->_z + ( getSprite(i)->_zvel));

                }
                else
                {
                    if(getHittypeTemp(i, 2) == 0)
                    {
                        if( getSprite(i)->_sectnum == -1) { KILLIT(i); }
                        if( (getSector(getSprite(i)->_sectnum)->floorstat&2) ) { KILLIT(i); }
                        setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 1); //t[2]++;
                    }
                    l = getflorzofslope(getSprite(i)->_sectnum,getSprite(i)->_x,getSprite(i)->_y);

                    setSprite_z(i, l-(2<<8));
                    setSprite_xvel(i, 0);

                    if(getSprite(i)->_picnum == JIBS6)
                    {
                        setHittypeTemp(i, 1, getHittypeTemp(i, 1) + 1); //t[1]++;
                        if( (getHittypeTemp(i, 1)&3) == 0 && getHittypeTemp(i, 0) < 7)
                            setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); //t[0]++;
                        if(getHittypeTemp(i, 1) > 20) KILLIT(i);
                    }
                    else
                    {
                        setSprite_picnum(i, JIBS6);
                        setHittypeTemp(i, 0, 0); //t[0] = 0;
                        setHittypeTemp(i, 1, 0); //t[1] = 0;
                    }
                }
                goto BOLT;

            case BLOODPOOL:
            case PUKE:

                if(getHittypeTemp(i, 0) == 0)
                {
                    setHittypeTemp(i, 0, 1); //t[0] = 1;
                    if(getSector(sect)->floorstat&2) { KILLIT(i); }
                    else insertspriteq(i);
                }

                makeitfall(i);

                p = findplayer(getSprite(i),&x);

                setSprite_z(i, getHittype(i)->floorz-(FOURSLEIGHT));

                if(getHittypeTemp(i, 2) < 32)
                {
                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 1); //t[2]++;
                    if(getHittype(i)->picnum == TIRE)
                    {
                        if(getSprite(i)->_xrepeat < 64 && getSprite(i)->_yrepeat < 64)
                        {
                            setSprite_xrepeat(i, getSprite(i)->_xrepeat +  (TRAND&3));
                            setSprite_yrepeat(i, getSprite(i)->_yrepeat + (TRAND&3));
                        }
                    }
                    else
                    {
                        if(getSprite(i)->_xrepeat < 32 && getSprite(i)->_yrepeat < 32)
                        {
                            setSprite_xrepeat(i, getSprite(i)->_xrepeat +  (TRAND&3));
                            setSprite_yrepeat(i, getSprite(i)->_yrepeat + (TRAND&3));
                        }
                    }
                }

                if(x < 844 && getSprite(i)->_xrepeat > 6 && getSprite(i)->_yrepeat > 6)
                {
                    if( getSprite(i)->_pal == 0 && (TRAND&255) < 16 && getSprite(i)->_picnum != PUKE)
                    {
                        if(ps[p].boot_amount > 0)
                            ps[p].boot_amount--;
                        else
                        {
//                            if(Sound[DUKE_LONGTERM_PAIN].num < 1)
                            if(getSoundNum(DUKE_LONGTERM_PAIN) == 0)
                                spritesound(DUKE_LONGTERM_PAIN,ps[p].i);
                            setSprite_extra(ps[p].i, getSprite(ps[p].i)->_extra - 1);
                            ps[p].pals_time = 32;
                            ps[p].pals[0] = 16;
                            ps[p].pals[1] = 0;
                            ps[p].pals[2] = 0;
                        }
                    }

                    if(getHittypeTemp(i, 1) == 1) goto BOLT;
                    setHittypeTemp(i, 1, 1); //t[1] = 1;

                    if(getHittype(i)->picnum == TIRE)
                        ps[p].footprintcount = 10;
                    else ps[p].footprintcount = 3;

                    ps[p].footprintpal = getSprite(i)->_pal;
                    ps[p].footprintshade = getSprite(i)->_shade;

                    if(getHittypeTemp(i, 2) == 32)
                    {
                            setSprite_xrepeat(i, getSprite(i)->_xrepeat - 6);
                            setSprite_yrepeat(i, getSprite(i)->_yrepeat - 6);
                    }
                }
                else
                    setHittypeTemp(i, 1, 0); //t[1] = 0;
                goto BOLT;

            case BURNING:
            case BURNING2:
            case FECES:
            case WATERBUBBLE:
            case SMALLSMOKE:
            case EXPLOSION2:
            case SHRINKEREXPLOSION:
            case EXPLOSION2BOT:
            case BLOOD:
            case LASERSITE:
            case FORCERIPPLE:
            case TRANSPORTERSTAR:
            case TRANSPORTERBEAM:
                p = findplayer(getSprite(i),&x);
                execute(i,p,x);
                goto BOLT;

            case SHELL:
            case SHOTGUNSHELL:

                ssp(i,CLIPMASK0);

                if(sect < 0 || ( getSector(sect)->floorz+(24<<8) ) < getSprite(i)->_z ) KILLIT(i);

                if(getSector(sect)->lotag == 2)
                {
                    setHittypeTemp(i, 1, getHittypeTemp(i, 1) + 1); //t[1]++;
                    if(getHittypeTemp(i, 1) > 8)
                    {
                        setHittypeTemp(i, 1, 0); //t[1] = 0;
                        setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                        setHittypeTemp(i, 0, getHittypeTemp(i, 0) & 3); // t[0] &= 3;
                    }
                    if(getSprite(i)->_zvel < 128)
                        setSprite_zvel(i, getSprite(i)->_zvel + (gc/13)); // 8
                    else
                        setSprite_zvel(i, getSprite(i)->_zvel - 64);
                    if(getSprite(i)->_xvel > 0)
                        setSprite_xvel(i, getSprite(i)->_xvel - 4);
                    else
                        setSprite_xvel(i, 0);
                }
                else
                {
                    setHittypeTemp(i, 1, getHittypeTemp(i, 1) + 1); //t[1]++;
                    if(getHittypeTemp(i, 1) > 3)
                    {
                        setHittypeTemp(i, 1, 0); //t[1] = 0;
                        setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                        setHittypeTemp(i, 0, getHittypeTemp(i, 0) & 3); //t[0] &= 3;
                    }
                    if(getSprite(i)->_zvel < 512)
                        setSprite_zvel(i, getSprite(i)->_zvel + (gc/3)); // 52;
                    if(getSprite(i)->_xvel > 0)
                        setSprite_xvel(i, getSprite(i)->_xvel - 1);
                    else KILLIT(i);
                }

                goto BOLT;

            case GLASSPIECES:
            case GLASSPIECES+1:
            case GLASSPIECES+2:
                // next-hack modified.
                // This was using tempData[0], but as we see everything here is screwed up.
                // it look like zVel, yVel and xvel are actually used as temporary variables
                // so, instead of allocating a precious hittype_tempdata (20 bytes),
                // we use yvel.
                makeitfall(i);

                if(getSprite(i)->_zvel > 4096)
                    setSprite_zvel(i, 4096);
                if(sect < 0) KILLIT(i);

//                if( getSprite(i)->_z == getHittype(i)->floorz-(FOURSLEIGHT) && getHittypeTemp(i, 0) < 3)
                if( getSprite(i)->_z == getHittype(i)->floorz-(FOURSLEIGHT) && getSprite(i)->_yvel < 3)
                {
                    //setSprite_zvel(i, -((3-getHittypeTemp(i, 0))<<8)-(TRAND&511));
                    setSprite_zvel(i, -((3-getSprite(i)->_yvel)<<8)-(TRAND&511));

                    if(getSector(sect)->lotag == 2)
                        setSprite_zvel(i, getSprite(i)->_zvel >> 1);
                    setSprite_xrepeat(i, getSprite(i)->_xrepeat >> 1);
                    setSprite_yrepeat(i, getSprite(i)->_yrepeat >> 1);
                    if( rnd(96) )
                      setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                    //setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;//Number of bounces
                    setSprite_yvel(i, getSprite_yvel(i) + 1);   //Number of bounces
                }
//                else if( getHittypeTemp(i, 0) == 3 ) KILLIT(i);
                else if( getSprite_yvel(i) == 3 ) KILLIT(i);

                if(getSprite(i)->_xvel > 0)
                {
                    setSprite_xvel(i, getSprite(i)->_xvel - 2);
                    setSprite_cstat(i, ((getSprite(i)->_xvel&3)<<2));
                }
                else
                    setSprite_xvel(i, 0);

                ssp(i,CLIPMASK0);

                goto BOLT;
        }

        IFWITHIN(SCRAP6,SCRAP5+3)
        {
                if(getSprite(i)->_xvel > 0)
                    setSprite_xvel(i, getSprite(i)->_xvel - 1);
                else
                    setSprite_xvel(i, 0);

                if(getSprite(i)->_zvel > 1024 && getSprite(i)->_zvel < 1280)
                {
                    setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                    sect = getSprite(i)->_sectnum;
                }

                if( getSprite(i)->_z < getSector(sect)->floorz-(2<<8) )
                {
                    if(getHittypeTemp(i, 1) < 1) setHittypeTemp(i, 1, getHittypeTemp(i, 1) + 1); // t[1]++
                    else
                    {
                        setHittypeTemp(i, 1, 0); //t[1] = 0;

                        if(getSprite(i)->_picnum < SCRAP6+8)
                        {
                            if(getHittypeTemp(i, 0) > 6)
                                setHittypeTemp(i, 0, 0); //t[0] = 0;
                            else setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                        }
                        else
                        {
                            if(getHittypeTemp(i, 0) > 2)
                                setHittypeTemp(i, 0, 0); //t[0] = 0;
                            else setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                        }
                    }
                    if(getSprite(i)->_zvel < 4096)
                        setSprite_zvel(i, getSprite(i)->_zvel + gc-50);
                    setSprite_x(i, getSprite(i)->_x + ((getSprite(i)->_xvel*sintable[(getSprite(i)->_ang+512)&2047])>>14));
                    setSprite_y(i, getSprite(i)->_y + ((getSprite(i)->_xvel*sintable[getSprite(i)->_ang&2047])>>14));
                    setSprite_z(i, getSprite(i)->_z + getSprite(i)->_zvel);
                }
                else
                {
                    if(getSprite(i)->_picnum == SCRAP1 && getSprite(i)->_yvel > 0)
                    {
                        j = spawn(i,getSprite(i)->_yvel);
                        setsprite(j,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                        getglobalz(j);
                        setSprite_hitag(j, 0);
                        setSprite_lotag(j, 0);
                    }
                    KILLIT(i);
                }
                goto BOLT;
        }

        BOLT:
        i = nexti;
    }
}
void moveeffectors(void)   //STATNUM 3
{
    int32_t q = 0, l, m, x, st, j;
 //   const int32_t * t;
    short i, k, nexti, nextk, p, sh, nextj;
 //   spritetype *getSprite(i);


    fricxv = fricyv = 0;

    i = headspritestat[3];
    while(i >= 0)
    {
        nexti = spr_add_data[i].nextspritestat;
        //getSprite(i) = &sprite[i];
        sprite_additional_data *s_d = &spr_add_data[i];
        int sscn = getSprite(i)->_sectnum;
        //sc = getSector(sscn);
        st = getSprite(i)->_lotag;
        sh = getSprite(i)->_hitag;

//        getHittypeTempPtr(i) = &getHittypeTemp(i, 0);

        switch(st)
        {
            case 0:  // rotating
            {
                int32_t zchange = 0;

                zchange = 0;

                j = getSprite(i)->_owner;

                if( getSprite(j)->_lotag == (short) 65535 )
                    KILLIT(i);

                q = getSector_extra(sscn)>>3;
                l = 0;

                if(getSector(sscn)->lotag == 30)
                {
                    q >>= 2;

                    if( getSprite_extra(i) == 1 )
                    {
                        if(getHittype_tempang(i) < 256)
                        {
                            setHittype_tempang(i, getHittype_tempang(i) + 4);
                            if(getHittype_tempang(i) >= 256)
                                callsound(getSprite(i)->_sectnum,i);
                            if(s_d->clipdist) l = 1;
                            else l = -1;
                        }
                        else
                            setHittype_tempang(i, 256);

                        if( getSector(sscn)->floorz > getSprite(i)->_z ) //z'getSprite(i) are touching
                        {
                            setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz - 512);
                            zchange = -512;
                            if( getSector(sscn)->floorz < getSprite(i)->_z )
                                setSector_floorz(getSprite(i)->_sectnum, getSprite(i)->_z);
                        }

                        else if( getSector(sscn)->floorz < getSprite(i)->_z ) //z'getSprite(i) are touching
                        {
                            setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz + 512);
                            zchange = 512;
                            if( getSector(sscn)->floorz > getSprite(i)->_z )
                                setSector_floorz(getSprite(i)->_sectnum, getSprite(i)->_z);
                        }
                    }
                    else if(getSprite_extra(i) == 3)
                    {
                        if(getHittype_tempang(i) > 0)
                        {
                            setHittype_tempang(i, getHittype_tempang(i) - 4);
                            if(getHittype_tempang(i) <= 0)
                                callsound(getSprite(i)->_sectnum,i);
                            if( s_d->clipdist ) l = -1;
                            else l = 1;
                        }
                        else
                            setHittype_tempang(i, 0);

                        if( getSector(sscn)->floorz > getHittypeTemp(i, 3) ) //z'getSprite(i) are touching
                        {
                            setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz - 512);
                            zchange = -512;
                            if( getSector(sscn)->floorz < getHittypeTemp(i, 3) )
                                setSector_floorz(getSprite(i)->_sectnum, getHittypeTemp(i, 3));
                        }

                        else if( getSector(sscn)->floorz < getHittypeTemp(i, 3) ) //z'getSprite(i) are touching
                        {
                            setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz + 512);
                            zchange = 512;
                            if( getSector(sscn)->floorz > getHittypeTemp(i, 3) )
                                setSector_floorz(getSprite(i)->_sectnum, getHittypeTemp(i, 3));
                        }
                    }

                    setSprite_ang(i, getSprite(i)->_ang + (l*q));
                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) + l * q); //t[2] += (l*q);
                }
                else
                {
                    if( getHittypeTemp(j, 0) == 0 ) break;
                    if( getHittypeTemp(j, 0) == 2 ) KILLIT(i);

                    if( getSprite(j)->_ang > 1024 )
                        l = -1;
                    else l = 1;
                    if( getHittypeTemp(i, 3) == 0 )
                        setHittypeTemp(i, 3, ldist(getSprite(i), getSprite(j))); //t[3] = ldist(getSprite(i),getSprite(j));
                    setSprite_xvel(i, getHittypeTemp(i, 3));
                    setSprite_x(i, getSprite(j)->_x);
                    setSprite_y(i, getSprite(j)->_y);
                    setSprite_ang(i, getSprite(i)->_ang + (l*q));
                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) + l * q); // t[2] += (l*q);
                }

                if( l && (getSector(sscn)->floorstat&64) )
                {
                    for(p=connecthead;p>=0;p=connectpoint2[p])
                    {
                        if( ps[p].cursectnum == getSprite(i)->_sectnum && ps[p].on_ground == 1)
                        {

                            ps[p].ang += (l*q);
                            ps[p].ang &= 2047;

                            ps[p].posz += zchange;

                            rotatepoint( getSprite(j)->_x,getSprite(j)->_y,
                                ps[p].posx,ps[p].posy,(q*l),
                                &m,&x);

                            ps[p].bobposx += m-ps[p].posx;
                            ps[p].bobposy += x-ps[p].posy;

                            ps[p].posx = m;
                            ps[p].posy = x;

                            if(getSprite(ps[p].i)->_extra <= 0)
                            {
                                setSprite_x(ps[p].i, m);
                                setSprite_y(ps[p].i, x);
                            }
                        }
                    }

                    p = headspritesect[getSprite(i)->_sectnum];
                    while(p >= 0)
                    {
                        if(getSprite(p)->_statnum != 3 && getSprite(p)->_statnum != 4)
                            if( getSprite(p)->_picnum != LASERLINE )
                        {
                            if(getSprite(p)->_picnum == APLAYER && getSprite(p)->_owner >= 0)
                            {
                                p = spr_add_data[p].nextspritesect;
                                continue;
                            }

                            setSprite_ang(p, getSprite(p)->_ang + (l*q));
                            setSprite_ang(p, getSprite(p)->_ang & 2047);

                            setSprite_z(p, getSprite(p)->_z + zchange);

                            // next-hack: this to let us use bitfields.
                            int32_t sprx = getSprite(p)->_x;
                            int32_t spry = getSprite(p)->_y;
                            //
                            rotatepoint(getSprite(j)->_x,getSprite(j)->_y,
                                getSprite(p)->_x,getSprite(p)->_y,(q*l),
                                &sprx,&spry);
                            //
                            setSprite_x(p, sprx);
                            setSprite_y(p, spry);
                        }
                        p = spr_add_data[p].nextspritesect;
                    }

                }

                ms(i);
            }

            break;
            case 1: //Nothing for now used as the pivot
                if(getSprite(i)->_owner == -1) //Init
                {
                    setSprite_owner(i, i);

                    j = headspritestat[3];
                    while(j >= 0)
                    {
                        if( getSprite(j)->_lotag == 19 && getSprite(j)->_hitag == sh )
                        {
                            setHittypeTemp(i, 0, 0); //t[0] = 0;
                            break;
                        }
                        j = spr_add_data[j].nextspritestat;
                    }
                }

                break;
            case 6: // subway
                k = getSector_extra(sscn);

                if(getHittypeTemp(i, 4) > 0)
                {
                    setHittypeTemp(i, 4, getHittypeTemp(i, 4) - 1); //t[4]--;
                    if( getHittypeTemp(i, 4) >= (k-(k>>3)) )
                        setSprite_xvel(i, getSprite(i)->_xvel - (k>>5));
                    if( getHittypeTemp(i, 4) > ((k>>1)-1) && getHittypeTemp(i, 4) < (k-(k>>3)) )
                        setSprite_xvel(i, 0);
                    if( getHittypeTemp(i, 4) < (k>>1) )
                        setSprite_xvel(i, getSprite(i)->_xvel + (k>>5));
                    if( getHittypeTemp(i, 4) < ((k>>1)-(k>>3)) )
                    {
                        setHittypeTemp(i, 4, 0); //t[4] = 0;
                        setSprite_xvel(i, k);
                    }
                }
                else
                    setSprite_xvel(i, k);

                j = headspritestat[3];
                while( j >= 0)
                {
                    if( (getSprite(j)->_lotag == 14) && (sh == getSprite(j)->_hitag) && (getHittypeTemp(j, 0) == getHittypeTemp(i, 0)) )
                    {
                        setSprite_xvel(j, getSprite(i)->_xvel);
//                        if( getHittypeTemp(i, 4) == 1 )
                        {
                            if(getHittypeTemp(j, 5) == 0)
                                setHittypeTemp(j, 5, dist(getSprite(j),getSprite(i)));
                            x = sgn( dist(getSprite(j),getSprite(i))-getHittypeTemp(j, 5) );
                            if(getSprite(j)->_extra)
                                x = -x;
                            setSprite_xvel(i, getSprite(i)->_xvel + x);
                        }
                        setHittypeTemp(j, 4, getHittypeTemp(i, 4));
                    }
                    j = spr_add_data[j].nextspritestat;
                }
                x = 0;


            case 14: // subway car
                if(getSprite(i)->_owner==-1)
                    setSprite_owner(i, LocateTheLocator((short)getHittypeTemp(i, 3),(short)getHittypeTemp(i, 0)));

                if(getSprite(i)->_owner == -1)
                {
                    FIXME("Could not find any locators for SE# 6 and 14 with a hitag of %d.\n",getHittypeTemp(i, 3));
                    gameexit("error");
                }

                j = ldist(getSprite(getSprite(i)->_owner),getSprite(i));

                if( j < 1024L )
                {
                    if(st==6)
                        if(getSprite(getSprite(i)->_owner)->_hitag&1)
                            setHittypeTemp(i, 4, getSector_extra(sscn)); //t[4]=getSector_extra(sscn); //Slow it down
                    setHittypeTemp(i, 3, getHittypeTemp(i, 3) + 1);  // t[3]++;
                    setSprite_owner(i, LocateTheLocator(getHittypeTemp(i, 3),getHittypeTemp(i, 0)));
                    if(getSprite(i)->_owner==-1)
                    {
                        setHittypeTemp(i, 3, 0); //t[3]=0;
                        setSprite_owner(i, LocateTheLocator(0,getHittypeTemp(i, 0)));
                    }
                }

                if(getSprite(i)->_xvel)
                {
                    x = getangle(getSprite(getSprite(i)->_owner)->_x-getSprite(i)->_x,getSprite(getSprite(i)->_owner)->_y-getSprite(i)->_y);
                    q = getincangle(getSprite(i)->_ang,x)>>3;

                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) + q); //t[2] += q;
                    setSprite_ang(i, getSprite(i)->_ang + q);

                    if(getSprite(i)->_xvel == getSector_extra(sscn) )
                    {
                        if( (getSector(sscn)->floorstat&1) == 0 && (getSector(sscn)->ceilingstat&1) == 0 )
                        {
//                            if( Sound[getHittype_lastvx(i)].num == 0 )
                            if( getSoundNum(getHittype_lastvx(i)) == 0 )
                                spritesound(getHittype_lastvx(i),i);
                        }
                        else if( ud.monsters_off == 0 && getSector(sscn)->floorpal == 0 && (getSector(sscn)->floorstat&1) && rnd(8) )
                        {
                            p = findplayer(getSprite(i),&x);
                            if(x < 20480)
                            {
                                j = getSprite(i)->_ang;
                                setSprite_ang(i, getangle(getSprite(i)->_x-ps[p].posx,getSprite(i)->_y-ps[p].posy));
                                shoot(i,RPG);
                                setSprite_ang(i, j);
                            }
                        }
                    }

                    if(getSprite(i)->_xvel <= 64 && (getSector(sscn)->floorstat&1) == 0 && (getSector(sscn)->ceilingstat&1) == 0 )
                        stopsound(getHittype_lastvx(i));

                    if( (getSector(sscn)->floorz-getSector(sscn)->ceilingz) < (108<<8) )
                    {
                        if(ud.clipping == 0 && getSprite(i)->_xvel >= 192)
                            for(p=connecthead;p>=0;p=connectpoint2[p])
                                if(getSprite(ps[p].i)->_extra > 0)
                        {
                            k = ps[p].cursectnum;
                            updatesector(ps[p].posx,ps[p].posy,&k);
                            if( ( k == -1 && ud.clipping == 0 ) || ( k == getSprite(i)->_sectnum && ps[p].cursectnum != getSprite(i)->_sectnum ) )
                            {
                                ps[p].posx = getSprite(i)->_x;
                                ps[p].posy = getSprite(i)->_y;
                                ps[p].cursectnum = getSprite(i)->_sectnum;

                                setsprite(ps[p].i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                                quickkill(&ps[p]);
                            }
                        }
                    }

                    m = (getSprite(i)->_xvel*sintable[(getSprite(i)->_ang+512)&2047])>>14;
                    x = (getSprite(i)->_xvel*sintable[getSprite(i)->_ang&2047])>>14;

                    for(p = connecthead;p >= 0;p=connectpoint2[p])
                       if(getSector(ps[p].cursectnum)->lotag != 2)
                    {
                        if(po[p].os == getSprite(i)->_sectnum)
                        {
                            po[p].ox += m;
                            po[p].oy += x;
                        }

                        if(getSprite(i)->_sectnum == getSprite(ps[p].i)->_sectnum)
                        {
                            rotatepoint(getSprite(i)->_x,getSprite(i)->_y,ps[p].posx,ps[p].posy,q,&ps[p].posx,&ps[p].posy);

                            ps[p].posx += m;
                            ps[p].posy += x;

                            ps[p].bobposx += m;
                            ps[p].bobposy += x;

                            ps[p].ang += q;

                            if(numplayers > 1)
                            {
                                ps[p].oposx = ps[p].posx;
                                ps[p].oposy = ps[p].posy;
                            }
                            if( getSprite(ps[p].i)->_extra <= 0 )
                            {
                                setSprite_x(ps[p].i, ps[p].posx);
                                setSprite_y(ps[p].i, ps[p].posy);
                            }
                        }
                    }
                    j = headspritesect[getSprite(i)->_sectnum];
                    while(j >= 0)
                    {
                        if (getSprite(j)->_statnum != 10 && getSector(getSprite(j)->_sectnum)->lotag != 2 && getSprite(j)->_picnum != SECTOREFFECTOR && getSprite(j)->_picnum != LOCATORS )
                        {
                            // next-hack: done this to be able to use bitfields.
                            int32_t sprx = getSprite(j)->_x;
                            int32_t spry = getSprite(j)->_y;

                            rotatepoint(getSprite(i)->_x,getSprite(i)->_y,
                                getSprite(j)->_x,getSprite(j)->_y,q,
                                &sprx,&spry);

                            setSprite_x(j, sprx);
                            setSprite_y(j, spry);

                            setSprite_x(j, getSprite(j)->_x + m);
                            setSprite_y(j, getSprite(j)->_y + x);

                            setSprite_ang(j, getSprite(j)->_ang + q);

                            if(numplayers > 1)
                            {
                                setHittype_bposx(j, getSprite(j)->_x);
                                setHittype_bposy(j, getSprite(j)->_y);
                            }
                        }
                        j = spr_add_data[j].nextspritesect;
                    }

                    ms(i);
                    setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);

                    if( (getSector(sscn)->floorz-getSector(sscn)->ceilingz) < (108<<8) )
                    {
                        if(ud.clipping == 0 && getSprite(i)->_xvel >= 192)
                            for(p=connecthead;p>=0;p=connectpoint2[p])
                                if(getSprite(ps[p].i)->_extra > 0)
                        {
                            k = ps[p].cursectnum;
                            updatesector(ps[p].posx,ps[p].posy,&k);
                            if( ( k == -1 && ud.clipping == 0 ) || ( k == getSprite(i)->_sectnum && ps[p].cursectnum != getSprite(i)->_sectnum ) )
                            {
                                ps[p].oposx = ps[p].posx = getSprite(i)->_x;
                                ps[p].oposy = ps[p].posy = getSprite(i)->_y;
                                ps[p].cursectnum = getSprite(i)->_sectnum;

                                setsprite(ps[p].i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                                quickkill(&ps[p]);
                            }
                        }

                        j = headspritesect[getSprite(OW)->_sectnum];
                        while(j >= 0)
                        {
                            l = spr_add_data[j].nextspritesect;
                            if (getSprite(j)->_statnum == 1 && badguy(getSprite(j)) && getSprite(j)->_picnum != SECTOREFFECTOR && getSprite(j)->_picnum != LOCATORS )
                            {
                                k = getSprite(j)->_sectnum;
                                updatesector(getSprite(j)->_x,getSprite(j)->_y,&k);
                                if( getSprite(j)->_extra >= 0 && k == getSprite(i)->_sectnum )
                                {
                                    gutsdir(getSprite(j),JIBS6,72,myconnectindex);
                                    spritesound(SQUISHED,i);
                                    deletesprite(j);
                                }
                            }
                            j = l;
                        }
                    }
                }

                break;

            case 30: // two way train
                if(getSprite(i)->_owner == -1)
                {
                    setHittypeTemp(i, 3, !getHittypeTemp(i, 3)); //t[3] = !getHittypeTemp(i, 3);
                    setSprite_owner(i, LocateTheLocator(getHittypeTemp(i, 3),getHittypeTemp(i, 0)));
                }
                else
                {

                    if(getHittypeTemp(i, 4) == 1) // Starting to go
                    {
                        if( ldist( getSprite(getSprite(i)->_owner),getSprite(i) ) < (2048-128) )
                            setHittypeTemp(i, 4, 2); //t[4] = 2;
                        else
                        {
                            if(getSprite(i)->_xvel == 0)
                                operateactivators(getSprite(i)->_hitag+(!getHittypeTemp(i, 3)),-1);
                            if(getSprite(i)->_xvel < 256)
                                setSprite_xvel(i, getSprite(i)->_xvel + 16);
                        }
                    }
                    if(getHittypeTemp(i, 4) == 2)
                    {
                        l = FindDistance2D(getSprite(getSprite(i)->_owner)->_x-getSprite(i)->_x,getSprite(getSprite(i)->_owner)->_y-getSprite(i)->_y);

                        if(l <= 128)
                            setSprite_xvel(i, 0);

                        if( getSprite(i)->_xvel > 0 )
                            setSprite_xvel(i, getSprite(i)->_xvel - 16);
                        else
                        {
                            setSprite_xvel(i, 0);
                            operateactivators(getSprite(i)->_hitag+(short)getHittypeTemp(i, 3),-1);
                            setSprite_owner(i, -1);
                            setSprite_ang(i, getSprite(i)->_ang + 1024);
                            setHittypeTemp(i, 4, 0); //t[4] = 0;
                            operateforcefields(i,getSprite(i)->_hitag);

                            j = headspritesect[getSprite(i)->_sectnum];
                            while(j >= 0)
                            {
                                if(getSprite(j)->_picnum != SECTOREFFECTOR && getSprite(j)->_picnum != LOCATORS )
                                {
                                    setHittype_bposx(j, getSprite(j)->_x);
                                    setHittype_bposy(j, getSprite(j)->_y);
                                }
                                j = spr_add_data[j].nextspritesect;
                            }

                        }
                    }
                }

                if(getSprite(i)->_xvel)
                {
                    l = (getSprite(i)->_xvel*sintable[(getSprite(i)->_ang+512)&2047])>>14;
                    x = (getSprite(i)->_xvel*sintable[getSprite(i)->_ang&2047])>>14;

                    if( (getSector(sscn)->floorz-getSector(sscn)->ceilingz) < (108<<8) )
                        if(ud.clipping == 0)
                            for(p=connecthead;p>=0;p=connectpoint2[p])
                                if(getSprite(ps[p].i)->_extra > 0)
                    {
                        k = ps[p].cursectnum;
                        updatesector(ps[p].posx,ps[p].posy,&k);
                        if( ( k == -1 && ud.clipping == 0 ) || ( k == getSprite(i)->_sectnum && ps[p].cursectnum != getSprite(i)->_sectnum ) )
                        {
                            ps[p].posx = getSprite(i)->_x;
                            ps[p].posy = getSprite(i)->_y;
                            ps[p].cursectnum = getSprite(i)->_sectnum;

                            setsprite(ps[p].i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                            quickkill(&ps[p]);
                        }
                    }

                    for(p = connecthead;p >= 0;p = connectpoint2[p])
                    {
                        if( getSprite(ps[p].i)->_sectnum == getSprite(i)->_sectnum )
                        {
                            ps[p].posx += l;
                            ps[p].posy += x;

                            if(numplayers > 1)
                            {
                                ps[p].oposx = ps[p].posx;
                                ps[p].oposy = ps[p].posy;
                            }

                            ps[p].bobposx += l;
                            ps[p].bobposy += x;
                        }

                        if( po[p].os == getSprite(i)->_sectnum )
                        {
                            po[p].ox += l;
                            po[p].oy += x;
                        }
                    }

                    j = headspritesect[getSprite(i)->_sectnum];
                    while(j >= 0)
                    {
                        if(getSprite(j)->_picnum != SECTOREFFECTOR && getSprite(j)->_picnum != LOCATORS )
                        {
                            if(numplayers < 2)
                            {
                                setHittype_bposx(j, getSprite(j)->_x);
                                setHittype_bposy(j, getSprite(j)->_y);
                            }

                            setSprite_x(j, getSprite(j)->_x + l);
                            setSprite_y(j, getSprite(j)->_y + x);

                            if(numplayers > 1)
                            {
                                setHittype_bposx(j, getSprite(j)->_x);
                                setHittype_bposy(j, getSprite(j)->_y);
                            }
                        }
                        j = spr_add_data[j].nextspritesect;
                    }

                    ms(i);
                    setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);

                    if( (getSector(sscn)->floorz-getSector(sscn)->ceilingz) < (108<<8) )
                    {
                        if(ud.clipping == 0)
                            for(p=connecthead;p>=0;p=connectpoint2[p])
                                if(getSprite(ps[p].i)->_extra > 0)
                        {
                            k = ps[p].cursectnum;
                            updatesector(ps[p].posx,ps[p].posy,&k);
                            if( ( k == -1 && ud.clipping == 0 ) || ( k == getSprite(i)->_sectnum && ps[p].cursectnum != getSprite(i)->_sectnum ) )
                            {
                                ps[p].posx = getSprite(i)->_x;
                                ps[p].posy = getSprite(i)->_y;

                                ps[p].oposx = ps[p].posx;
                                ps[p].oposy = ps[p].posy;

                                ps[p].cursectnum = getSprite(i)->_sectnum;

                                setsprite(ps[p].i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                                quickkill(&ps[p]);
                            }
                        }

                        j = headspritesect[getSprite(OW)->_sectnum];
                        while(j >= 0)
                        {
                            l = spr_add_data[j].nextspritesect;
                            if (getSprite(j)->_statnum == 1 && badguy(getSprite(j)) && getSprite(j)->_picnum != SECTOREFFECTOR && getSprite(j)->_picnum != LOCATORS )
                            {
            //                    if(getSprite(j)->_sectnum != getSprite(i)->_sectnum)
                                {
                                    k = getSprite(j)->_sectnum;
                                    updatesector(getSprite(j)->_x,getSprite(j)->_y,&k);
                                    if( getSprite(j)->_extra >= 0 && k == getSprite(i)->_sectnum )
                                    {
                                        gutsdir(getSprite(j),JIBS6,24,myconnectindex);
                                        spritesound(SQUISHED,j);
                                        deletesprite(j);
                                    }
                                }

                            }
                            j = l;
                        }
                    }
                }

                break;


            case 2://Quakes
                if(getHittypeTemp(i, 4) > 0 && getHittypeTemp(i, 0) == 0 )
                {
                    if( getHittypeTemp(i, 4) < sh )
                        setHittypeTemp(i, 4, getHittypeTemp(i, 4) + 1); //t[4]++;
                    else
                        setHittypeTemp(i, 0, 1); //t[0] = 1;
                }

                if(getHittypeTemp(i, 0) > 0)
                {
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;

                    setSprite_xvel(i, 3);

                    if(getHittypeTemp(i, 0) > 96)
                    {
                        setHittypeTemp(i, 0,  -1); //t[0] = -1; //Stop the quake
                        setHittypeTemp(i, 4, -1); //t[4] = -1;
                        KILLIT(i);
                    }
                    else
                    {
                        if( (getHittypeTemp(i, 0)&31) ==  8 )
                        {
                            earthquaketime = 48;
                            spritesound(EARTHQUAKE,ps[screenpeek].i);
                        }

                        if( klabs( getSector(sscn)->floorheinum-getHittypeTemp(i, 5) ) < 8 )
                            setSector_floorheinum(sscn, getHittypeTemp(i, 5));
                        else
                            setSector_floorheinum(sscn, getSector(sscn)->floorheinum + ( sgn(getHittypeTemp(i, 5)-getSector(sscn)->floorheinum)<<4 ));
                    }

                    m = (getSprite(i)->_xvel*sintable[(getSprite(i)->_ang+512)&2047])>>14;
                    x = (getSprite(i)->_xvel*sintable[getSprite(i)->_ang&2047])>>14;


                    for(p=connecthead;p>=0;p=connectpoint2[p])
                        if(ps[p].cursectnum == getSprite(i)->_sectnum && ps[p].on_ground)
                        {
                            ps[p].posx += m;
                            ps[p].posy += x;

                            ps[p].bobposx += m;
                            ps[p].bobposy += x;
                        }

                    j = headspritesect[getSprite(i)->_sectnum];
                    while(j >= 0)
                    {
                        nextj = spr_add_data[j].nextspritesect;
                        int picnum = getSprite_picnum(j);
                        if (picnum != SECTOREFFECTOR)
                        {
                            setSprite_x(j, getSprite(j)->_x + m);
                            setSprite_y(j, getSprite(j)->_y + x);
                            int res = setsprite(j,getSprite(j)->_x,getSprite(j)->_y,getSprite(j)->_z);
#if FIX_EARTHQUAKE
                            // next-hack: updating sector for sprites outside the map (can be inside walls) is EXTREMELY inefficient. Better kill them.
                            // better kill them if these are randomscrap
                            if (res < 0)
                            {
                                if (picnum >= SCRAP6 && picnum <= SCRAP6 + 15)
                                {
                                    deletesprite(j);
                                }
                            }
#else
                            UNUSED(res);
#endif
                        }
                        j = nextj;
                    }
                    ms(i);
                    setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                }
                break;

            //Flashing getSector lights after reactor EXPLOSION2

            case 3: // random light after shot out

                if( getHittypeTemp(i, 4) == 0 ) break;
                p = findplayer(getSprite(i),&x);

            //    if(getHittypeTemp(i, 5) > 0) { getHittypeTemp(i, 5)--; break; }

                if( (global_random/(sh+1)&31) < 4 && !getHittypeTemp(i, 2))
                {
             //       getHittypeTemp(i, 5) = 4+(global_random&7);
                    setSector_ceilingpal(sscn, getSprite(i)->_owner>>8);
                    setSector_floorpal(sscn, getSprite(i)->_owner&0xff);
                    setHittypeTemp(i, 0, getSprite(i)->_shade + (global_random&15)); // t[0] = getSprite(i)->_shade + (global_random&15);
                }
                else
                {
             //       getHittypeTemp(i, 5) = 4+(global_random&3);
                    setSector_ceilingpal(sscn, getSprite(i)->_pal);
                    setSector_floorpal(sscn, getSprite(i)->_pal);
                    setHittypeTemp(i, 0, getHittypeTemp(i, 3)); // t[0] = getHittypeTemp(i, 3);
                }

                setSector_ceilingshade(sscn, getHittypeTemp(i, 0));
                setSector_floorshade(sscn, getHittypeTemp(i, 0));

                getWall(constsector[getSprite(i)->_sectnum].wallptr);
                int wallIdx = constsector[getSprite(i)->_sectnum].wallptr;
                for(x=constsector[getSprite(i)->_sectnum].wallnum;x > 0;x--, wallIdx++)
                {
                    if( getWall_hitag(wallIdx) != 1 )
                    {
                        setWall_shade(wallIdx, getHittypeTemp(i, 0));
                        if((getWall_cstat(wallIdx)&2) && constwall[wallIdx].nextwall >= 0)
                        {
                            setWall_shade(constwall[wallIdx].nextwall, getWall_shade(wallIdx));
                        }
                    }
                }

                break;

            case 4: // random light

                if((global_random/(sh+1)&31) < 4 )
                {
                    setHittypeTemp(i, 1, getSprite(i)->_shade + (global_random&15)); //  getHittypeTemp(i, 1) = getSprite(i)->_shade + (global_random&15);//Got really bright
                    setHittypeTemp(i, 0, getSprite(i)->_shade + (global_random&15)); // t[0] = getSprite(i)->_shade + (global_random&15);
                    setSector_ceilingpal(sscn, getSprite(i)->_owner>>8);
                    setSector_floorpal(sscn, getSprite(i)->_owner&0xff);
                    j = 1;
                }
                else
                {
                    setHittypeTemp(i, 1, getHittypeTemp(i, 2)); // t[1] = getHittypeTemp(i, 2);
                    setHittypeTemp(i, 0, getHittypeTemp(i, 3)); //t[0] = getHittypeTemp(i, 3);

                    setSector_ceilingpal(sscn, getSprite(i)->_pal);
                    setSector_floorpal(sscn, getSprite(i)->_pal);

                    j = 0;
                }

                setSector_floorshade(sscn, getHittypeTemp(i, 1));
                setSector_ceilingshade(sscn, getHittypeTemp(i, 1));

                getWall(constsector[getSprite(i)->_sectnum].wallptr);
                {
                    int wallIdx = constsector[getSprite(i)->_sectnum].wallptr;
                    for(x=constsector[getSprite(i)->_sectnum].wallnum;x > 0; x--, wallIdx++)
                    {
                        if(j)
                        {
                                setWall_pal(wallIdx, (getSprite(i)->_owner&0xff));
                        }
                        else
                        {
                               setWall_pal(wallIdx, getSprite(i)->_pal);
                        }

                        if( getWall_hitag(wallIdx) != 1 )
                        {
                            setWall_shade(wallIdx, getHittypeTemp(i, 0));
                            if((getWall_cstat(wallIdx)&2) && constwall[wallIdx].nextwall >= 0)
                            {
                                setWall_shade(constwall[wallIdx].nextwall, getWall_shade(wallIdx));

                            }
                        }
                    }
                }
                j = headspritesect[SECT];
                while(j >= 0)
                {
                    if(getSprite(j)->_cstat&16)
                    {
                        if (getSector(sscn)->ceilingstat&1)
                            setSprite_shade(j, getSector_ceilingshade(sscn));
                        else
                            setSprite_shade(j, getSector_floorshade(sscn));
                    }

                    j = spr_add_data[j].nextspritesect;
                }

                if(getHittypeTemp(i, 4)) KILLIT(i);

                break;

            //BOSS
            case 5:
                p = findplayer(getSprite(i),&x);
                if(x < 8192)
                {
                    j = getSprite(i)->_ang;
                    setSprite_ang(i, getangle(getSprite(i)->_x-ps[p].posx,getSprite(i)->_y-ps[p].posy));
                    shoot(i,FIRELASER);
                    setSprite_ang(i, j);
                }

                if(getSprite(i)->_owner==-1) //Start search
                {
                    setHittypeTemp(i, 4, 0); //t[4]=0;
                    l = 0x7fffffff;
                    while(1) //Find the shortest dist
                    {
                        setSprite_owner(i, LocateTheLocator((short)getHittypeTemp(i, 4),-1)); //t[0] hold sectnum

                        if(getSprite(i)->_owner==-1) break;

                        m = ldist(getSprite(ps[p].i),getSprite(getSprite(i)->_owner));

                        if(l > m)
                        {
                            q = getSprite(i)->_owner;
                            l = m;
                        }

                        setHittypeTemp(i, 4, getHittypeTemp(i, 4) + 1); //t[4]++;
                    }

                    setSprite_owner(i, q);
                    setSprite_zvel(i, ksgn(getSprite(q)->_z-getSprite(i)->_z)<<4);
                }

                if(ldist(getSprite(getSprite(i)->_owner),getSprite(i)) < 1024)
                {
                    short ta;
                    ta = getSprite(i)->_ang;
                    // not sure why this...
                    setSprite_ang(i, getangle(ps[p].posx-getSprite(i)->_x,ps[p].posy-getSprite(i)->_y));
                    setSprite_ang(i, ta);
                    setSprite_owner(i, -1);
                    goto BOLT;

                }
                else
                    setSprite_xvel(i, 256);

                x = getangle(getSprite(getSprite(i)->_owner)->_x-getSprite(i)->_x,getSprite(getSprite(i)->_owner)->_y-getSprite(i)->_y);
                q = getincangle(getSprite(i)->_ang,x)>>3;
                setSprite_ang(i, getSprite(i)->_ang + q);

                if(rnd(32))
                {
                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) + q); //t[2]+=q;
                    setSector_ceilingshade(sscn, 127);
                }
                else
                {
                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) + (getincangle(getHittypeTemp(i, 2)+512,getangle(ps[p].posx-getSprite(i)->_x,ps[p].posy-getSprite(i)->_y))>>2)); // t[2] += getincangle(getHittypeTemp(i, 2)+512,getangle(ps[p].posx-getSprite(i)->_x,ps[p].posy-getSprite(i)->_y))>>2;
                    setSector_ceilingshade(sscn, 0);
                }
                IFHIT
                {
                    setHittypeTemp(i, 3, getHittypeTemp(i, 3) + 1);  // t[3]++;
                    if(getHittypeTemp(i, 3) == 5)
                    {
                        setSprite_zvel(i, getSprite(i)->_zvel + 1024);
                        FTA(7,&ps[myconnectindex],0);
                    }
                }

                setSprite_z(i, getSprite(i)->_z + getSprite(i)->_zvel);
                setSector_ceilingz(sscn, getSector(sscn)->ceilingz + getSprite(i)->_zvel);
                setSector_ceilingz(getHittypeTemp(i, 0), getSector(getHittypeTemp(i, 0))->ceilingz + getSprite(i)->_zvel);
                ms(i);
                setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                break;


            case 8://open/door/lights
            case 9:

                // work only if its moving

                j = -1;

                if(getHittypeTemp(i, 4))
                {
                    setHittypeTemp(i, 4, getHittypeTemp(i, 4) + 1);
                    if( getHittypeTemp(i, 4) > 8 ) KILLIT(i);
                    j = 1;
                }
                else
                {
#if INTERPOLATION_USES_POSPTR
                    // note this is a bug
                    j = getanimationgoal(getSector(sscn)->ceilingz);
#else
                    j = getanimationgoal(getSector_ceilingzidx_from_ptr(getSector(sscn)));
#endif
                }

                if( j >= 0 )
                {
                    short sn;

                    if( (getSector(sscn)->lotag&0x8000) || getHittypeTemp(i, 4) )
                        x = -getHittypeTemp(i, 3);
                    else
                        x = getHittypeTemp(i, 3);

                    if ( st == 9 ) x = -x;

                    j = headspritestat[3];
                    while(j >= 0)
                    {
                        if( ((getSprite(j)->_lotag) == st ) && (getSprite(j)->_hitag) == sh )
                        {
                            sn = getSprite(j)->_sectnum;
                            m = getSprite(j)->_shade;

                            getWall(constsector[sn].wallptr);
                            int wallIdx = constsector[sn].wallptr;
                            for(l=constsector[sn].wallnum;l>0;l--, wallIdx++)
                            {
                                if( getWall_hitag(wallIdx) != 1 )
                                {
//                                    wal->shade+=x;
                                    setWall_shade(wallIdx, getWall_shade(wallIdx) + x);
                                    if(getWall_shade(wallIdx)  < m)
                                    {
                                        setWall_shade(wallIdx, m);
                                    }
                                    else if(getWall_shade(wallIdx) > getHittypeTemp(j, 2))
                                    {
                                        setWall_shade(wallIdx, getHittypeTemp(j, 2));
                                    }

                                    if(constwall[wallIdx].nextwall >= 0)
                                        if(getWall_hitag(constwall[wallIdx].nextwall) != 1)
                                        {
                                            setWall_shade(constwall[wallIdx].nextwall, getWall_shade(wallIdx));

                                        }
                                }
                            }

                            setSector_floorshade(sn, getSector_floorshade(sn)   + x);
                            setSector_ceilingshade(sn, getSector_ceilingshade(sn) + x);

                            if(getSector_floorshade(sn) < m)
                                setSector_floorshade(sn, m);
                            else if(getSector_floorshade(sn) > getHittypeTemp(j, 0))
                                setSector_floorshade(sn, getHittypeTemp(j, 0));

                            if(getSector_ceilingshade(sn) < m)
                                setSector_ceilingshade(sn, m);
                            else if(getSector_ceilingshade(sn) > getHittypeTemp(j, 1))
                                setSector_ceilingshade(sn, getHittypeTemp(j, 1));

                        }
                        j = spr_add_data[j].nextspritestat;
                    }
                }
                break;
            case 10: // door auto close

                if( (getSector(sscn)->lotag&0xff) == 27 || ( getSector(sscn)->floorz > getSector(sscn)->ceilingz && (getSector(sscn)->lotag&0xff) != 23 ) || getSector(sscn)->lotag == (short) 32791 )
                {
                    j = 1;

                    if( (getSector(sscn)->lotag&0xff) != 27)
                        for(p=connecthead;p>=0;p=connectpoint2[p])
                            if( getSector(sscn)->lotag != 30 && getSector(sscn)->lotag != 31 && getSector(sscn)->lotag != 0 )
                                if(getSprite(i)->_sectnum == getSprite(ps[p].i)->_sectnum)
                                    j = 0;

                    if(j == 1)
                    {
                        if(getHittypeTemp(i, 0) > sh )
                            switch(getSector(getSprite(i)->_sectnum)->lotag)
                            {
                                case 20:
                                case 21:
                                case 22:
                                case 26:
#if INTERPOLATION_USES_POSPTR
                                  if( getanimationgoal(getSector_ceilingzptr(getSprite(i)->_sectnum)) >= 0 )
#else
                                  if( getanimationgoal(getSector_ceilingzidx(getSprite(i)->_sectnum)) >= 0 )
#endif
                                      break;

                                default:
                                  activatebysector(getSprite(i)->_sectnum,i);
                                  setHittypeTemp(i, 0, 0); //t[0] = 0;
                                  break;
                            }
                        else setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                    }
                }
                else
                    setHittypeTemp(i, 0, 0); //t[0]=0;
                break;
            case 11: //Swingdoor

                if( getHittypeTemp(i, 5) > 0)
                {
                    setHittypeTemp(i, 5, getHittypeTemp(i, 5) - 1); //t[5]--;
                    break;
                }

                if( getHittypeTemp(i, 4) )
                {
                    short startwall,endwall;

                    startwall = constsector[getSprite(i)->_sectnum].wallptr;
                    endwall = startwall+constsector[getSprite(i)->_sectnum].wallnum;

                    for(j=startwall;j<endwall;j++)
                    {
                        k = headspritestat[1];
                        while(k >= 0)
                        {
                            if( getSprite(k)->_extra > 0 && badguy(getSprite(k)) && clipinsidebox(getSprite(k)->_x,getSprite(k)->_y,j,256L) == 1 )
                                goto BOLT;
                            k = spr_add_data[k].nextspritestat;
                        }

                        k = headspritestat[10];
                        while(k >= 0)
                        {
                            if( getSprite(k)->_owner >= 0 && clipinsidebox(getSprite(k)->_x,getSprite(k)->_y,j,144L) == 1 )
                            {
                                setHittypeTemp(i, 5, 8); //t[5] = 8; // Delay
                                k = (SP>>3)*getHittypeTemp(i, 3);
                                setHittypeTemp(i, 2, getHittypeTemp(i, 2) - k); //t[2]-=k;
                                setHittypeTemp(i, 4, getHittypeTemp(i, 4) - k); //t[4]-=k;
                                ms(i);
                                setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                                goto BOLT;
                            }
                            k = spr_add_data[k].nextspritestat;
                        }
                    }

                    k = (SP>>3)*getHittypeTemp(i, 3);
                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) + k); //t[2]+=k;
                    setHittypeTemp(i, 4, getHittypeTemp(i, 4) + k); //t[4]+=k;
                    ms(i);
                    setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);

                    if(getHittypeTemp(i, 4) <= -511 || getHittypeTemp(i, 4) >= 512)
                    {
                        setHittypeTemp(i, 4, 0); //t[4] = 0;
                        setHittypeTemp(i, 2, getHittypeTemp(i, 2) & 0xffffff00); //t[2] &= 0xffffff00;
                        ms(i);
                        setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                        break;
                    }
                }
                break;
            case 12: // light switch
                if( getHittypeTemp(i, 0) == 3 || getHittypeTemp(i, 3) == 1 ) //Lights going off
                {
                    setSector_floorpal(sscn, 0);
                    setSector_ceilingpal(sscn, 0);

                    getWall(constsector[getSprite(i)->_sectnum].wallptr);
                    int wallIdx = constsector[getSprite(i)->_sectnum].wallptr;
                    for(j = constsector[getSprite(i)->_sectnum].wallnum;j > 0; j--, wallIdx++)
                        if(getWall_hitag(wallIdx) != 1)
                        {
                            setWall_shade(wallIdx, getHittypeTemp(i, 1));
                            setWall_pal(wallIdx, 0);
                        }

                    setSector_floorshade(sscn, getHittypeTemp(i, 1));
                    setSector_ceilingshade(sscn, getHittypeTemp(i, 2));
                    setHittypeTemp(i, 0, 0); //t[0]=0;

                    j = headspritesect[SECT];
                    while(j >= 0)
                    {
                        if(getSprite(j)->_cstat&16)
                        {
                            if (getSector(sscn)->ceilingstat&1)
                                setSprite_shade(j, getSector_ceilingshade(sscn));
                            else
                                setSprite_shade(j, getSector_floorshade(sscn));
                        }
                        j = spr_add_data[j].nextspritesect;

                    }

                    if(getHittypeTemp(i, 3) == 1) KILLIT(i);
                }
                if( getHittypeTemp(i, 0) == 1 ) //Lights flickering on
                {
                    if( getSector_floorshade(sscn) > getSprite(i)->_shade )
                    {
                        setSector_floorpal(sscn, getSprite(i)->_pal);
                        setSector_ceilingpal(sscn, getSprite(i)->_pal);

                        setSector_floorshade(sscn, getSector_floorshade(sscn) - 2);
                        setSector_ceilingshade(sscn, getSector_ceilingshade(sscn) - 2);

                        getWall(constsector[getSprite(i)->_sectnum].wallptr);
                        int wallIdx = constsector[getSprite(i)->_sectnum].wallptr;
//                        WARNING("SHADE\r\n");
                        for(j=constsector[getSprite(i)->_sectnum].wallnum;j>0;j--, wallIdx ++)
                            if(getWall_hitag(wallIdx) != 1)
                            {

                                setWall_pal(wallIdx, getSprite(i)->_pal);
                                setWall_shade(wallIdx, getWall_shade(wallIdx) - 2);
                            }
                    }
                    else
                        setHittypeTemp(i, 0, 2); //t[0] = 2;

                    j = headspritesect[SECT];
                    while(j >= 0)
                    {
                        if(getSprite(j)->_cstat&16)
                        {
                            if (getSector(sscn)->ceilingstat&1)
                                setSprite_shade(j, getSector_ceilingshade(sscn));
                            else
                                setSprite_shade(j, getSector_floorshade(sscn));
                        }
                        j = spr_add_data[j].nextspritesect;
                    }
                }
                break;


            case 13: // c-9 explosive
                if( getHittypeTemp(i, 2) )
                {
                    j = (SP<<5)|1;

                    if( getSprite(i)->_ang == 512 )
                    {
                        if( getSprite(i)->_owner )
                        {
                            if( klabs(getHittypeTemp(i, 0)-getSector(sscn)->ceilingz) >= j )
                                setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz + sgn(getHittypeTemp(i, 0)-getSector(sscn)->ceilingz)*j);
                            else
                                setSector_ceilingz(getSprite(i)->_sectnum, getHittypeTemp(i, 0));
                        }
                        else
                        {
                            if( klabs(getHittypeTemp(i, 1)-getSector(sscn)->floorz) >= j )
                                setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz + sgn(getHittypeTemp(i, 1)-getSector(sscn)->floorz)*j );
                            else
                                setSector_floorz(getSprite(i)->_sectnum, getHittypeTemp(i, 1));
                        }
                    }
                    else
                    {
                        if( klabs(getHittypeTemp(i, 1)-getSector(sscn)->floorz) >= j )
                            setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz + sgn(getHittypeTemp(i, 1)-getSector(sscn)->floorz)*j);
                        else
                            setSector_floorz(getSprite(i)->_sectnum, getHittypeTemp(i, 1));
                        if( klabs(getHittypeTemp(i, 0)-getSector(sscn)->ceilingz) >= j )
                            setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz + sgn(getHittypeTemp(i, 0)-getSector(sscn)->ceilingz)*j);
                        setSector_ceilingz(getSprite(i)->_sectnum, getHittypeTemp(i, 0));
                    }

                    if( getHittypeTemp(i, 3) == 1 )
                    {
                        //Change the shades

                        setHittypeTemp(i, 3, getHittypeTemp(i, 3) + 1);  // t[3]++;
                        setSector_ceilingstat(sscn, getSector(sscn)->ceilingstat ^ 1);

                        if(getSprite(i)->_ang == 512)
                        {
                            getWall(constsector[getSprite(i)->_sectnum].wallptr);
                            int wallIdx = constsector[getSprite(i)->_sectnum].wallptr;
                            for(j=constsector[getSprite(i)->_sectnum].wallnum;j>0;j--, wallIdx++)
                                setWall_shade(wallIdx, getSprite(i)->_shade);

                            setSector_floorshade(sscn, getSprite(i)->_shade);

                            if(ps[0].one_parallax_sectnum >= 0)
                            {
                                setSector_ceilingpicnum(sscn, getSector(ps[0].one_parallax_sectnum)->ceilingpicnum);
                                setSector_ceilingshade(sscn, getSector_ceilingshade(ps[0].one_parallax_sectnum));
                            }
                        }
                    }
                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 1); // t[2]++;
                    if(getHittypeTemp(i, 2) > 256)
                        KILLIT(i);
                }


                if( getHittypeTemp(i, 2) == 4 && getSprite(i)->_ang != 512)
                {
                    const spritetype *spritei = getSprite(i);
                    for(x=0;x<7;x++) RANDOMSCRAP(i, spritei);
                }
                break;


            case 15: // slide door

                if(getHittypeTemp(i, 4))
                {
                    setSprite_xvel(i, 16);

                    if(getHittypeTemp(i, 4) == 1) //Opening
                    {
                        if( getHittypeTemp(i, 3) >= (SP>>3) )
                        {
                            setHittypeTemp(i, 4, 0); //t[4] = 0; //Turn off the sliders
                            callsound(getSprite(i)->_sectnum,i);
                            break;
                        }
                        setHittypeTemp(i, 3, getHittypeTemp(i, 3) + 1);  // t[3]++;
                    }
                    else if(getHittypeTemp(i, 4) == 2)
                    {
                        if(getHittypeTemp(i, 3)<1)
                        {
                            setHittypeTemp(i, 4, 0); //t[4] = 0;
                            callsound(getSprite(i)->_sectnum,i);
                            break;
                        }
                        setHittypeTemp(i, 3, getHittypeTemp(i, 3) - 1); //t[3]--;
                    }

                    ms(i);
                    setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);
                }
                break;

            case 16: //Reactor

                setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 32); //t[2]+=32;
                if(getSector(sscn)->floorz<getSector(sscn)->ceilingz)
                    setSprite_shade(i, 0);

                else if( getSector(sscn)->ceilingz < getHittypeTemp(i, 3) )
                {

                    //The following code check to see if
                    //there is any other sprites in the getSector.
                    //If there isn'getHittypeTempPtr(i), then kill this getSectoreffector
                    //itself.....

                    j = headspritesect[getSprite(i)->_sectnum];
                    while(j >= 0)
                    {
                        if(getSprite(j)->_picnum == REACTOR || getSprite(j)->_picnum == REACTOR2)
                            break;
                        j = spr_add_data[j].nextspritesect;
                    }
                    if(j == -1) { KILLIT(i); }
                    else
                        setSprite_shade(i, 1);
                }

                if(getSprite(i)->_shade)
                    setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz + 1024);
                else
                    setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz - 512);

                ms(i);
                setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);

                break;

            case 17: // elevator transport

                q = getHittypeTemp(i, 0)*(SP<<2);

                setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz + q);
                setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz + q);

                j = headspritesect[getSprite(i)->_sectnum];
                while(j >= 0)
                {
                    if(getSprite(j)->_statnum == 10 && getSprite(j)->_owner >= 0)
                    {
                        p = getSprite(j)->_yvel;
                        if(numplayers < 2)
                            ps[p].oposz = ps[p].posz;
                        ps[p].posz += q;
                        ps[p].truefz += q;
                        ps[p].truecz += q;
                        if(numplayers > 1)
                            ps[p].oposz = ps[p].posz;
                    }
                    if( getSprite(j)->_statnum != 3 )
                    {
                        setHittype_bposz(j, getSprite(j)->_z);
                        setSprite_z(j, getSprite(j)->_z + q);
                    }

                    setHittype_floorz(j, getSector(sscn)->floorz);
                    setHittype_ceilingz(j, getSector(sscn)->ceilingz);

                    j = spr_add_data[j].nextspritesect;
                }

                if( getHittypeTemp(i, 0) )                if(getHittypeTemp(i, 0)) //If in motion
                {
                    if( klabs(getSector(sscn)->floorz-getHittypeTemp(i, 2)) <= SP)
                    {
                        activatewarpelevators(i,0);
                        break;
                    }

                    if(getHittypeTemp(i, 0)==-1)
                    {
                        if( getSector(sscn)->floorz > getHittypeTemp(i, 3) )
                            break;
                    }
                    else if( getSector(sscn)->ceilingz < getHittypeTemp(i, 4) ) break;

                    if( getHittypeTemp(i, 1) == 0 ) break;
                    setHittypeTemp(i, 1, 0); //t[1] = 0;

                    j = headspritestat[3];
                    while(j >= 0)
                    {
                                if( i != j && (getSprite(j)->_lotag) == 17)
                                    if( (getSector(sscn)->hitag-getHittypeTemp(i, 0)) ==
                                        (getSector(getSprite(j)->_sectnum)->hitag)
                                        && sh == (getSprite(j)->_hitag))
                                            break;
                                j = spr_add_data[j].nextspritestat;
                    }

                    if(j == -1) break;

                    k = headspritesect[getSprite(i)->_sectnum];
                    while(k >= 0)
                    {
                        nextk = spr_add_data[k].nextspritesect;

                        if(getSprite(k)->_statnum == 10 && getSprite(k)->_owner >= 0)
                        {
                            p = getSprite(k)->_yvel;

                            ps[p].posx += getSprite(j)->_x-getSprite(i)->_x;
                            ps[p].posy += getSprite(j)->_y-getSprite(i)->_y;
                            ps[p].posz = getSector(getSprite(j)->_sectnum)->floorz-(getSector(sscn)->floorz-ps[p].posz);

                            setHittype_floorz(k, getSector(getSprite(j)->_sectnum)->floorz);
                            setHittype_ceilingz(k, getSector(getSprite(j)->_sectnum)->ceilingz);

                            ps[p].bobposx = ps[p].oposx = ps[p].posx;
                            ps[p].bobposy = ps[p].oposy = ps[p].posy;
                            ps[p].oposz = ps[p].posz;

                            ps[p].truefz = getHittype(k)->floorz;
                            ps[p].truecz = getHittype(k)->ceilingz;
                            ps[p].bobcounter = 0;

                            changespritesect(k,getSprite(j)->_sectnum);
                            ps[p].cursectnum = getSprite(j)->_sectnum;
                        }
                        else if( getSprite(k)->_statnum != 3 )
                        {
                            setSprite_x(k, getSprite(k)->_x +
                                getSprite(j)->_x-getSprite(i)->_x);
                            setSprite_y(k, getSprite(k)->_y +
                                getSprite(j)->_y-getSprite(i)->_y);
                            setSprite_z(k, getSector(getSprite(j)->_sectnum)->floorz-
                                (getSector(sscn)->floorz-getSprite(k)->_z));

                            setHittype_bposx(k, getSprite(k)->_x);
                            setHittype_bposy(k, getSprite(k)->_y);
                            setHittype_bposz(k, getSprite(k)->_z);

                            changespritesect(k,getSprite(j)->_sectnum);
                            setsprite(k,getSprite(k)->_x,getSprite(k)->_y,getSprite(k)->_z);

                            setHittype_floorz(k, getSector(getSprite(j)->_sectnum)->floorz);
                            setHittype_ceilingz(k, getSector(getSprite(j)->_sectnum)->ceilingz);

                        }
                        k = nextk;
                    }
                }
                break;

            case 18: // incremental sector rise/fall
                if(getHittypeTemp(i, 0))
                {
                    if(getSprite(i)->_pal)
                    {
                        if(getSprite(i)->_ang == 512)
                        {
                            setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz - getSector_extra(sscn));
                            if(getSector(sscn)->ceilingz <= getHittypeTemp(i, 1))
                            {
                                setSector_ceilingz(getSprite(i)->_sectnum, getHittypeTemp(i, 1));
                                KILLIT(i);
                            }
                        }
                        else
                        {
                            setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz + getSector_extra(sscn));
                            j = headspritesect[getSprite(i)->_sectnum];
                            while(j >= 0)
                            {
                                if(getSprite(j)->_picnum == APLAYER && getSprite(j)->_owner >= 0)
                                    if( ps[getSprite(j)->_yvel].on_ground == 1 )
                                        ps[getSprite(j)->_yvel].posz += getSector_extra(sscn);
                                if( getSprite(j)->_zvel == 0 && getSprite(j)->_statnum != 3 && getSprite(j)->_statnum != 4)
                                {
                                    setSprite_z(j, getSprite(j)->_z + getSector_extra(sscn));
                                    setHittype_bposz(j, getSprite(j)->_z);
                                    setHittype_floorz(j, getSector(sscn)->floorz);
                                }
                                j = spr_add_data[j].nextspritesect;
                            }
                            if(getSector(sscn)->floorz >= getHittypeTemp(i, 1))
                            {
                                setSector_floorz(getSprite(i)->_sectnum, getHittypeTemp(i, 1));
                                KILLIT(i);
                            }
                        }
                    }
                    else
                    {
                        if(getSprite(i)->_ang == 512)
                        {
                            setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz + getSector_extra(sscn));
                            if(getSector(sscn)->ceilingz >= getSprite(i)->_z)
                            {
                                setSector_ceilingz(getSprite(i)->_sectnum, getSprite(i)->_z);
                                KILLIT(i);
                            }
                        }
                        else
                        {
                            setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz - getSector_extra(sscn));
                                j = headspritesect[getSprite(i)->_sectnum];
                                while(j >= 0)
                                {
                                    if(getSprite(j)->_picnum == APLAYER && getSprite(j)->_owner >= 0)
                                        if( ps[getSprite(j)->_yvel].on_ground == 1 )
                                            ps[getSprite(j)->_yvel].posz -= getSector_extra(sscn);
                                    if( getSprite(j)->_zvel == 0 && getSprite(j)->_statnum != 3 && getSprite(j)->_statnum != 4)
                                    {
                                        setSprite_z(j, getSprite(j)->_z - getSector_extra(sscn));
                                        setHittype_bposz(j, getSprite(j)->_z);
                                        setHittype_floorz(j, getSector(sscn)->floorz);
                                    }
                                    j = spr_add_data[j].nextspritesect;
                                }
                            if(getSector(sscn)->floorz <= getSprite(i)->_z)
                            {
                                setSector_floorz(getSprite(i)->_sectnum, getSprite(i)->_z);
                                KILLIT(i);
                            }
                        }
                    }

                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 1); // t[2]++;
                    if(getHittypeTemp(i, 2) >= getSprite(i)->_hitag)
                    {
                        setHittypeTemp(i, 2, 0); //t[2] = 0;
                        setHittypeTemp(i, 0, 0); //t[0] = 0;
                    }
                }
                break;

            case 19: //Battlestar galactica shields (next-hack: I see "explosion lowers ceiling")

                if(getHittypeTemp(i, 0))
                {
                    if(getHittypeTemp(i, 0) == 1)
                    {
                        setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                        x = constsector[getSprite(i)->_sectnum].wallptr;
                        q = x+constsector[getSprite(i)->_sectnum].wallnum;
                        for(j=x;j<q;j++)
                            if(getWall_overpicnum(j) == BIGFORCE)
                            {
                                setWall_cstat(j,  getWall_cstat(j) & (128+32+8+4+2));
                                setWall_overpicnum(j, 0);
                                if(constwall[j].nextwall >= 0)
                                {
                                    setWall_overpicnum(constwall[j].nextwall, 0);
                                    setWall_cstat(constwall[j].nextwall, getWall_cstat(constwall[j].nextwall) & (128+32+8+4+2));
                                }
                            }
                    }

                    if(getSector(sscn)->ceilingz < getSector(sscn)->floorz)
                        setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz + SP);
                    else
                    {
                        setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->floorz);

                        j = headspritestat[3];
                        while(j >= 0)
                        {
                            if(getSprite(j)->_lotag == 0 && getSprite(j)->_hitag==sh)
                            {
                                q = getSprite(getSprite(j)->_owner)->_sectnum;
                                setSector_floorpal(getSprite(j)->_sectnum,getSector(q)->floorpal);
                                setSector_ceilingpal(getSprite(j)->_sectnum, getSector(q)->floorpal);

                                setSector_floorshade(getSprite(j)->_sectnum, getSector_floorshade(q));
                                setSector_ceilingshade(getSprite(j)->_sectnum,getSector_floorshade(q));


                                setHittypeTemp(getSprite(j)->_owner, 0, 2);
                            }
                            j = spr_add_data[j].nextspritestat;
                        }
                        KILLIT(i);
                    }
                }
                else //Not hit yet
                {
                    j=ifhitsectors(getSprite(i)->_sectnum);
                    if(j >= 0)                    //IFHITSECT  // <---- whoever did this crap should be arrested for crimes against humanity.
                    {
                        FTA(8,&ps[myconnectindex],0);

                        l = headspritestat[3];
                        while(l >= 0)
                        {
                            x = getSprite(l)->_lotag&0x7fff;
                            switch( x )
                            {
                                case 0:
                                    if(getSprite(l)->_hitag == sh)
                                    {
                                        q = getSprite(l)->_sectnum;
                                        setSector_floorshade(q, getSprite(getSprite(l)->_owner)->_shade);
                                        setSector_ceilingshade(q, getSprite(getSprite(l)->_owner)->_shade);
                                        setSector_floorpal(q, getSprite(getSprite(l)->_owner)->_pal);
                                        setSector_ceilingpal(q, getSprite(getSprite(l)->_owner)->_pal);
                                    }
                                    break;

                                case 1:
                                case 12:
//                                case 18:
                                case 19:

                                    if( sh == getSprite(l)->_hitag )
                                        if( getHittypeTemp(l, 0) == 0 )
                                        {
                                            setHittypeTemp(l, 0, 1); //Shut them all on
                                            setSprite_owner(l, i);
                                        }

                                    break;
                            }
                            l = spr_add_data[l].nextspritestat;
                        }
                    }
                }

                break;

            case 20: //Extend-o-bridge

                if( getHittypeTemp(i, 0) == 0 ) break;
                if( getHittypeTemp(i, 0) == 1 )
                    setSprite_xvel(i, 8);
                else
                    setSprite_xvel(i, -8);

                if( getSprite(i)->_xvel ) //Moving
                {
                    x = (getSprite(i)->_xvel*sintable[(getSprite(i)->_ang+512)&2047])>>14;
                    l = (getSprite(i)->_xvel*sintable[getSprite(i)->_ang&2047])>>14;

                    setHittypeTemp(i, 3, getHittypeTemp(i, 3) + getSprite(i)->_xvel); //t[3] += getSprite(i)->_xvel;

                    setSprite_x(i, getSprite(i)->_x + x);
                    setSprite_y(i, getSprite(i)->_y + l);

                    if( getHittypeTemp(i, 3) <= 0 || (getHittypeTemp(i, 3)>>6) >= (SP>>6) )
                    {
                        setSprite_x(i, getSprite(i)->_x - x);
                        setSprite_y(i, getSprite(i)->_y - l);
                        setHittypeTemp(i, 0, 0); //t[0] = 0;
                        callsound(getSprite(i)->_sectnum,i);
                        break;
                    }
                    int sectNum = getSprite(i)->_sectnum;
                    j = headspritesect[sectNum];
                    while(j >= 0)
                    {
                        nextj = spr_add_data[j].nextspritesect;

                        if( getSprite(j)->_statnum != 3 && getSprite(j)->_zvel == 0)
                        {
                            setSprite_x(j, getSprite(j)->_x + x);
                            setSprite_y(j, getSprite(j)->_y + l);
                            setsprite(j,getSprite(j)->_x,getSprite(j)->_y,getSprite(j)->_z);
                            if( getSector(getSprite(j)->_sectnum)->floorstat&2 )
                                if(getSprite(j)->_statnum == 2)
                                    makeitfall(j);
                        }
                        j = nextj;
                    }
                    dragpoint((short)getHittypeTemp(i, 1),getWall(getHittypeTemp(i, 1))->x+x,getWall(getHittypeTemp(i, 1))->y+l);
                    dragpoint((short)getHittypeTemp(i, 2),getWall(getHittypeTemp(i, 2))->x+x,getWall(getHittypeTemp(i, 2))->y+l);

                    for(p=connecthead;p>=0;p=connectpoint2[p])
                        if(ps[p].cursectnum == getSprite(i)->_sectnum && ps[p].on_ground)
                        {
                            ps[p].posx += x;
                            ps[p].posy += l;

                            ps[p].oposx = ps[p].posx;
                            ps[p].oposy = ps[p].posy;

                            setsprite(ps[p].i,ps[p].posx,ps[p].posy,ps[p].posz+PHEIGHT);
                        }

                    setSector_floorxpanning(sscn, getSector_floorxpanning(sscn)- (x>>3));
                    setSector_floorypanning(sscn,getSector(sscn)->floorypanning- (l>>3));

                    setSector_ceilingxpanning(sscn, getSector(sscn)->ceilingxpanning - (x>>3));
                    setSector_ceilingypanning(sscn, getSector(sscn)->ceilingypanning - (l>>3));
                }

                break;

            case 21: // Cascading effect. Drop floor
            {
#if INTERPOLATION_USES_POSPTR
                int32_t *lp;

                if (getHittypeTemp(i, 0) == 0) break;

                if (getSprite(i)->_ang == 1536)
                {
                    lp = &getSector(sscn)->ceilingz;
                }
                else
                    lp = &getSector(sscn)->floorz;

                if (getHittypeTemp(i, 0) == 1) //Decide if the getSprite(i)->_sectnum should go up or down
                {
                    getSprite(i)->_zvel = ksgn(getSprite(i)->_z - *lp) * (SP << 4);
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                }

                if (getSector_extra(sscn) == 0)
                {
                    *lp += getSprite(i)->_zvel;

                    if (klabs(*lp - getSprite(i)->_z) < 1024)
                    {
                        *lp = getSprite(i)->_z;
                        KILLIT(i); //All done
                    }
                }
                else getSector_extra(sscn)--;
                break;
#else
                indexedValue_t lpv;
                if (getHittypeTemp(i, 0) == 0) break;

                if (getSprite(i)->_ang == 1536)
                {
                    lpv = getSector_ceilingzidx_from_ptr(getSector(sscn));
                }
                else
                    lpv = getSector_floorzidx_from_ptr(getSector(sscn));

                if (getHittypeTemp(i, 0) == 1) //Decide if the getSprite(i)->_sectnum should go up or down
                {
                    setSprite_zvel(i, ksgn(getSprite(i)->_z - getIndexedValueFromIndexedValue(lpv)) * (SP << 4));
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                }

                if (getSector_extra(sscn) == 0)
                {
                    setIndexedValueFromIndexedValue(lpv, getIndexedValueFromIndexedValue(lpv) + getSprite(i)->_zvel);

                    if (klabs(getIndexedValueFromIndexedValue(lpv) - getSprite(i)->_z) < 1024)
                    {
                        setIndexedValueFromIndexedValue(lpv, getSprite(i)->_z);
                        KILLIT(i); //All done
                    }
                }
                else
                    setSector_extra(sscn, getSector_extra(sscn) - 1);
                break;

#endif
            }

            case 22: // teeth door prong

                if( getHittypeTemp(i, 1) )
                {
#if INTERPOLATION_USES_POSPTR
                    if(getanimationgoal(getSector_ceilingzptr(getHittypeTemp(i, 0))) >= 0)
#else
                    if(getanimationgoal(getSector_ceilingzidx(getHittypeTemp(i, 0))) >= 0)
#endif
                        setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz + getSector_extra(sscn)*9);
                    else
                        setHittypeTemp(i, 1, 0); //t[1] = 0;
                }
                break;

            case 24: // conveyor belt or water current
            case 34:

                if(getHittypeTemp(i, 4)) break;

                x = (SP*sintable[(getSprite(i)->_ang+512)&2047])>>18;
                l = (SP*sintable[getSprite(i)->_ang&2047])>>18;

                k = 0;

                j = headspritesect[getSprite(i)->_sectnum];
                while(j >= 0)
                {
                    nextj = spr_add_data[j].nextspritesect;
                    if(getSprite(j)->_zvel >= 0)
                        switch(getSprite(j)->_statnum)
                    {
                        case 5:
                            switch(getSprite(j)->_picnum)
                            {
                                case BLOODPOOL:
                                case PUKE:
                                case FOOTPRINTS:
                                case FOOTPRINTS2:
                                case FOOTPRINTS3:
                                case FOOTPRINTS4:
                                case BULLETHOLE:
                                case BLOODSPLAT1:
                                case BLOODSPLAT2:
                                case BLOODSPLAT3:
                                case BLOODSPLAT4:
                                    setSprite_xrepeat(j, 0);
                                    setSprite_yrepeat(j, 0);
                                    j = nextj;
                                    continue;
                                case LASERLINE:
                                    j = nextj;
                                    continue;
                            }
                        case 6:
                            if(getSprite(j)->_picnum == TRIPBOMB) break;
                        case 1:
                        case 0:
                            if(
                                getSprite(j)->_picnum == BOLT1 ||
                                getSprite(j)->_picnum == BOLT1+1 ||
                                getSprite(j)->_picnum == BOLT1+2 ||
                                getSprite(j)->_picnum == BOLT1+3 ||
                                getSprite(j)->_picnum == SIDEBOLT1 ||
                                getSprite(j)->_picnum == SIDEBOLT1+1 ||
                                getSprite(j)->_picnum == SIDEBOLT1+2 ||
                                getSprite(j)->_picnum == SIDEBOLT1+3 ||
                                wallswitchcheck(j)
                              )
                              break;

                            if( !(getSprite(j)->_picnum >= CRANE && getSprite(j)->_picnum <= (CRANE+3)))
                            {
                                if( getSprite(j)->_z > (getHittype(j)->floorz-(16<<8)) )
                                {
                                    setHittype_bposx(j, getSprite(j)->_x);
                                    setHittype_bposy(j, getSprite(j)->_y);

                                    setSprite_x(j, getSprite(j)->_x + (x>>2));
                                    setSprite_y(j, getSprite(j)->_y + (l>>2));

                                    setsprite(j,getSprite(j)->_x,getSprite(j)->_y,getSprite(j)->_z);

                                    if( getSector(getSprite(j)->_sectnum)->floorstat&2 )
                                        if(getSprite(j)->_statnum == 2)
                                            makeitfall(j);
                                }
                            }
                            break;
                    }
                    j = nextj;
                }

                p = myconnectindex;
                if(ps[p].cursectnum == getSprite(i)->_sectnum && ps[p].on_ground)
                    if( klabs(ps[p].posz-ps[p].truefz) < PHEIGHT+(9<<8) )
                {
                    fricxv += x<<3;
                    fricyv += l<<3;
                }

                setSector_floorxpanning(sscn, getSector_floorxpanning(sscn) +  (SP>>7));

            break;

            case 35: // undocumented... unfinished implementation...
                if(getSector(sscn)->ceilingz > getSprite(i)->_z)
                    for(j = 0;j < 8;j++)
                {
                    setSprite_ang(i, getSprite(i)->_ang + (TRAND&511));
                    k = spawn(i,SMALLSMOKE);
                    setSprite_xvel(k, 96+(TRAND&127));
                    ssp(k,CLIPMASK0);
                    setsprite(k,getSprite(k)->_x,getSprite(k)->_y,getSprite(k)->_z);
                    if( rnd(16) )
                        spawn(i,EXPLOSION2);
                }

                switch(getHittypeTemp(i, 0))
                {
                    case 0:
                        setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz + getSprite(i)->_yvel);
                        if(getSector(sscn)->ceilingz > getSector(sscn)->floorz)
                            setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz);
                        if(getSector(sscn)->ceilingz > getSprite(i)->_z+(32<<8))
                            setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                        break;
                    case 1:
                        setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz-(getSprite(i)->_yvel<<2));
                        if(getSector(sscn)->ceilingz < getHittypeTemp(i, 4))
                        {
                            setSector_ceilingz(getSprite(i)->_sectnum, getHittypeTemp(i, 4));
                            setHittypeTemp(i, 0, 0); //t[0] = 0;
                        }
                        break;
                }
                break;

            case 25: //PISTONS/engine

                if( getHittypeTemp(i, 4) == 0 ) break;

                if(getSector(sscn)->floorz <= getSector(sscn)->ceilingz)
                    setSprite_shade(i, 0);
                else if( getSector(sscn)->ceilingz <= getHittypeTemp(i, 3))
                    setSprite_shade(i, 1);

                if(getSprite(i)->_shade)
                {
                    setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz + (SP<<4));
                    if(getSector(sscn)->ceilingz > getSector(sscn)->floorz)
                        setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->floorz);
                }
                else
                {
                    setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz -  (SP<<4));
                    if(getSector(sscn)->ceilingz < getHittypeTemp(i, 3))
                        setSector_ceilingz(getSprite(i)->_sectnum, getHittypeTemp(i, 3));
                }

                break;

            case 26: // incomplete step escalator

                setSprite_xvel(i, 32);
                l = (getSprite(i)->_xvel*sintable[(getSprite(i)->_ang+512)&2047])>>14;
                x = (getSprite(i)->_xvel*sintable[getSprite(i)->_ang&2047])>>14;

                setSprite_shade(i, getSprite(i)->_shade + 1);
                if( getSprite(i)->_shade > 7 )
                {
                    setSprite_x(i, getHittypeTemp(i, 3));
                    setSprite_y(i, getHittypeTemp(i, 4));
                    setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz - ((getSprite(i)->_zvel*getSprite(i)->_shade)-getSprite(i)->_zvel));
                    setSprite_shade(i, 0);
                }
                else
                    setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz + getSprite(i)->_zvel);

                j = headspritesect[getSprite(i)->_sectnum];
                while( j >= 0 )
                {
                    nextj = spr_add_data[j].nextspritesect;
                    if(getSprite(j)->_statnum != 3 && getSprite(j)->_statnum != 10)
                    {
                        setHittype_bposx(j, getSprite(j)->_x);
                        setHittype_bposy(j, getSprite(j)->_y);

                        setSprite_x(j, getSprite(j)->_x + l);
                        setSprite_y(j, getSprite(j)->_y + x);

                        setSprite_z(j, getSprite(j)->_z + getSprite(i)->_zvel);
                        setsprite(j,getSprite(j)->_x,getSprite(j)->_y,getSprite(j)->_z);
                    }
                    j = nextj;
                }

                p = myconnectindex;
                if(getSprite(ps[p].i)->_sectnum == getSprite(i)->_sectnum && ps[p].on_ground)
                {
                    fricxv += l<<5;
                    fricyv += x<<5;
                }

                for(p = connecthead;p >= 0;p = connectpoint2[p])
                    if(getSprite(ps[p].i)->_sectnum == getSprite(i)->_sectnum && ps[p].on_ground)
                        ps[p].posz += getSprite(i)->_zvel;

                ms(i);
                setsprite(i,getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z);

                break;


            case 27: // demo camera.
                if(ud.recstat == 0) break;
#if DEMO_SUPPORT

                setHittype_tempang(i, getSprite(i)->_ang);

                p = findplayer(getSprite(i),&x);

				// FIX_00013: 3rd person camera view during demo playback can now be
				// turned off (no need to use hacked no-camera maps anymore)
                if( /*getSprite(ps[p].i)->_extra > 0 && myconnectindex == screenpeek */ 0) // xduke remove camera view for the 1st player (notice myconnectindex is == 1 when replaying, dosent matter who recorded)
                {
                    if( getHittypeTemp(i, 0) < 0 )
                    {
                        ud.camerasprite = i;
                        setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                    }
                    else if(ud.recstat == 2 && ps[p].newowner == -1)
                    {
                        if(cansee(getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z,SECT,ps[p].posx,ps[p].posy,ps[p].posz,ps[p].cursectnum))
                        {
                            if(x < (uint32_t)sh)
                            {
                                ud.camerasprite = i;
                                setHittypeTemp(i, 0, 999); //t[0] = 999;
                                setSprite_ang(i, getSprite(i)->_ang + (getincangle(getSprite(i)->_ang,getangle(ps[p].posx-getSprite(i)->_x,ps[p].posy-getSprite(i)->_y))>>3));
                                setSprite_yvel(i, 100+((getSprite(i)->_z-ps[p].posz)/257));

                            }
                            else if(getHittypeTemp(i, 0) == 999)
                            {
                                if(ud.camerasprite == i)
                                    setHittypeTemp(i, 0, 0); //t[0] = 0;
                                else
                                    setHittypeTemp(i, 0, -10); //t[0] = -10;
                                ud.camerasprite = i;

                            }
                        }
                        else
                        {
                            setSprite_ang(i, getangle(ps[p].posx-getSprite(i)->_x,ps[p].posy-getSprite(i)->_y));

                            if(getHittypeTemp(i, 0) == 999)
                            {
                                if(ud.camerasprite == i)
                                    setHittypeTemp(i, 0, 0); //t[0] = 0;
                                else
                                    setHittypeTemp(i, 0, -20); //t[0] = -20;
                                ud.camerasprite = i;
                            }
                        }
                    }
                }
#else
                FIXME("EFFECTOR 27");
#endif
                break;
            case 28:    // lightning
                if(getHittypeTemp(i, 5) > 0)
                {
                    setHittypeTemp(i, 5, getHittypeTemp(i, 5) - 1); //t[5]--;
                    break;
                }

                if(getHittypeTemp(i, 0) == 0)
                {
                    p = findplayer(getSprite(i),&x);
                    if( x > 15500 )
                        break;
                    setHittypeTemp(i, 0, 1);
                    setHittypeTemp(i, 1,  64 + (TRAND&511));
                    setHittypeTemp(i, 2, 0);
                }
                else
                {
                    setHittypeTemp(i, 2, getHittypeTemp(i, 2) + 1);
                    if(getHittypeTemp(i, 2) > getHittypeTemp(i, 1) )
                    {
                        setHittypeTemp(i, 0, 0);
                        ps[screenpeek].visibility = ud.const_visibility;
                        break;
                    }
                    else if( getHittypeTemp(i, 2) == (getHittypeTemp(i, 1)>>1) )
                        spritesound(THUNDER,i);
                    else if(getHittypeTemp(i, 2) == (getHittypeTemp(i, 1)>>3) )
                        spritesound(LIGHTNING_SLAP,i);
                    else if( getHittypeTemp(i, 2) == (getHittypeTemp(i, 1)>>2) )
                    {
                        j = headspritestat[0];
                        while(j >= 0)
                        {
                            if( getSprite(j)->_picnum == NATURALLIGHTNING && getSprite(j)->_hitag == getSprite(i)->_hitag)
                                setSprite_cstat(j, getSprite(j)->_cstat | SPRITE_INVISIBLE);
                            j = spr_add_data[j].nextspritestat;
                        }
                    }
                    else if(getHittypeTemp(i, 2) > (getHittypeTemp(i, 1)>>3) && getHittypeTemp(i, 2) < (getHittypeTemp(i, 1)>>2) )
                    {
                        if( cansee(getSprite(i)->_x,getSprite(i)->_y,getSprite(i)->_z,getSprite(i)->_sectnum,ps[screenpeek].posx,ps[screenpeek].posy,ps[screenpeek].posz,ps[screenpeek].cursectnum ) )
                            j = 1;
                        else j = 0;

                        if( rnd(192) && (getHittypeTemp(i, 2)&1) )
                        {
                            if(j)
                                ps[screenpeek].visibility = 0;
                        }
                        else if(j)
                            ps[screenpeek].visibility = ud.const_visibility;

                        j = headspritestat[0];
                        while(j >= 0)
                        {
                            if( getSprite(j)->_picnum == NATURALLIGHTNING && getSprite(j)->_hitag == getSprite(i)->_hitag)
                            {
                                if ( rnd(32) && (getHittypeTemp(i, 2)&1) )
                                {
                                    setSprite_cstat(j, getSprite(j)->_cstat & ~SPRITE_INVISIBLE); // was & 32767
                                    spawn(j,SMALLSMOKE);

                                    p = findplayer(getSprite(i),&x);
                                    x = ldist(getSprite(ps[p].i), getSprite(j));
                                    if( x < 768 )
                                    {
//                                        if(Sound[DUKE_LONGTERM_PAIN].num < 1)
                                        if(getSoundNum(DUKE_LONGTERM_PAIN) == 0 )
                                            spritesound(DUKE_LONGTERM_PAIN,ps[p].i);
                                        spritesound(SHORT_CIRCUIT,ps[p].i);
                                        setSprite_extra(ps[p].i, getSprite(ps[p].i)->_extra - (8+(TRAND&7)));
                                        ps[p].pals_time = 32;
                                        ps[p].pals[0] = 16;
                                        ps[p].pals[1] = 0;
                                        ps[p].pals[2] = 0;
                                    }
                                    break;
                                }
                                else
                                    setSprite_cstat(j, getSprite(j)->_cstat | SPRITE_INVISIBLE);
                            }

                            j = spr_add_data[j].nextspritestat;
                        }
                    }
                }
                break;
            case 29:    // float
                setSprite_hitag(i, getSprite(i)->_hitag + 64);
                l = mulscale12((int32_t)getSprite(i)->_yvel,sintable[getSprite(i)->_hitag&2047]);
                setSector_floorz(getSprite(i)->_sectnum, getSprite(i)->_z + l);
                break;
            case 31: // True Drop Floor
                if(getHittypeTemp(i, 0) == 1)
                {
                    // Choose dir

                    if(getHittypeTemp(i, 3) > 0)
                    {
                        setHittypeTemp(i, 3, getHittypeTemp(i, 3) - 1); //t[3]--;
                        break;
                    }

                    if(getHittypeTemp(i, 2) == 1) // Retract
                    {
                        if(SA != 1536)
                        {
                            if( klabs( getSector(sscn)->floorz - getSprite(i)->_z ) < SP )
                            {
                                setSector_floorz(getSprite(i)->_sectnum, getSprite(i)->_z);
                                setHittypeTemp(i, 2, 0); //t[2] = 0;
                                setHittypeTemp(i, 0, 0); //t[0] = 0;
                                setHittypeTemp(i, 3, getSprite(i)->_hitag); //t[3] = getSprite(i)->_hitag;
                                callsound(getSprite(i)->_sectnum,i);
                            }
                            else
                            {
                                l = sgn(getSprite(i)->_z-getSector(sscn)->floorz)*SP;
                                setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz + l);

                                j = headspritesect[getSprite(i)->_sectnum];
                                while(j >= 0)
                                {
                                    if(getSprite(j)->_picnum == APLAYER && getSprite(j)->_owner >= 0)
                                        if( ps[getSprite(j)->_yvel].on_ground == 1 )
                                            ps[getSprite(j)->_yvel].posz += l;
                                    if( getSprite(j)->_zvel == 0 && getSprite(j)->_statnum != 3 && getSprite(j)->_statnum != 4)
                                    {
                                        setSprite_z(j, getSprite(j)->_z + l);
                                        setHittype_bposz(j, getSprite(j)->_z);
                                        setHittype_floorz(j, getSector(sscn)->floorz);
                                    }
                                    j = spr_add_data[j].nextspritesect;
                                }
                            }
                        }
                        else
                        {
                            if( klabs( getSector(sscn)->floorz - getHittypeTemp(i, 1) ) < SP )
                            {
                                setSector_floorz(getSprite(i)->_sectnum, getHittypeTemp(i, 1));
                                callsound(getSprite(i)->_sectnum,i);
                                setHittypeTemp(i, 2, 0); //t[2] = 0;
                                setHittypeTemp(i, 0, 0); //t[0] = 0;
                                setHittypeTemp(i, 3, getSprite(i)->_hitag); //t[3] = getSprite(i)->_hitag;
                            }
                            else
                            {
                                l = sgn(getHittypeTemp(i, 1)-getSector(sscn)->floorz)*SP;
                                setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz + l);

                                j = headspritesect[getSprite(i)->_sectnum];
                                while(j >= 0)
                                {
                                    if(getSprite(j)->_picnum == APLAYER && getSprite(j)->_owner >= 0)
                                        if( ps[getSprite(j)->_yvel].on_ground == 1 )
                                            ps[getSprite(j)->_yvel].posz += l;
                                    if( getSprite(j)->_zvel == 0 && getSprite(j)->_statnum != 3 && getSprite(j)->_statnum != 4 )
                                    {
                                        setSprite_z(j, getSprite(j)->_z + l);
                                        setHittype_bposz(j, getSprite(j)->_z);
                                        setHittype_floorz(j, getSector(sscn)->floorz);
                                    }
                                    j = spr_add_data[j].nextspritesect;
                                }
                            }
                        }
                        break;
                    }

                    if( (getSprite(i)->_ang&2047) == 1536)
                    {
                        if( klabs( getSprite(i)->_z-getSector(sscn)->floorz ) < SP )
                        {
                            callsound(getSprite(i)->_sectnum,i);
                            setHittypeTemp(i, 0, 0); //t[0] = 0;
                            setHittypeTemp(i, 2, 1); //t[2] = 1;
                            setHittypeTemp(i, 3, getSprite(i)->_hitag); //t[3] = getSprite(i)->_hitag;
                        }
                        else
                        {
                            l = sgn(getSprite(i)->_z-getSector(sscn)->floorz)*SP;
                            setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz + l);

                            j = headspritesect[getSprite(i)->_sectnum];
                            while(j >= 0)
                            {
                                if(getSprite(j)->_picnum == APLAYER && getSprite(j)->_owner >= 0)
                                    if( ps[getSprite(j)->_yvel].on_ground == 1 )
                                        ps[getSprite(j)->_yvel].posz += l;
                                if( getSprite(j)->_zvel == 0 && getSprite(j)->_statnum != 3 && getSprite(j)->_statnum != 4 )
                                {
                                    setSprite_z(j, getSprite(j)->_z + l);
                                    setHittype_bposz(j, getSprite(j)->_z);
                                    setHittype_floorz(j, getSector(sscn)->floorz);
                                }
                                j = spr_add_data[j].nextspritesect;
                            }
                        }
                    }
                    else
                    {
                        if( klabs( getSector(sscn)->floorz-getHittypeTemp(i, 1) ) < SP )
                        {
                            setHittypeTemp(i, 0, 0); //t[0] = 0;
                            callsound(getSprite(i)->_sectnum,i);
                            setHittypeTemp(i, 2, 1); //t[2] = 1;
                            setHittypeTemp(i, 3, getSprite(i)->_hitag); //t[3] = getSprite(i)->_hitag;
                        }
                        else
                        {
                            l = sgn(getSprite(i)->_z-getHittypeTemp(i, 1))*SP;
                            setSector_floorz(getSprite(i)->_sectnum, getSector(sscn)->floorz - l);

                            j = headspritesect[getSprite(i)->_sectnum];
                            while(j >= 0)
                            {
                                if(getSprite(j)->_picnum == APLAYER && getSprite(j)->_owner >= 0)
                                    if( ps[getSprite(j)->_yvel].on_ground == 1 )
                                        ps[getSprite(j)->_yvel].posz -= l;
                                if(getSprite(j)->_zvel == 0 && getSprite(j)->_statnum != 3 && getSprite(j)->_statnum != 4 )
                                {
                                    //setHittype_bposz(j, getSprite(j)->_z) -= l;
                                    setSprite_z(j, getSprite(j)->_z - l);
                                    setHittype_bposz(j, getSprite(j)->_z);
                                    setHittype_floorz(j, getSector(sscn)->floorz);
                                }
                                j = spr_add_data[j].nextspritesect;
                            }
                        }
                    }
                }
                break;

           case 32: // True Drop Ceiling
                if(getHittypeTemp(i, 0) == 1)
                {
                    // Choose dir

                    if(getHittypeTemp(i, 2) == 1) // Retract
                    {
                        if(SA != 1536)
                        {
                            if( klabs( getSector(sscn)->ceilingz - getSprite(i)->_z ) <
                                (SP<<1) )
                            {
                                setSector_ceilingz(getSprite(i)->_sectnum, getSprite(i)->_z);
                                callsound(getSprite(i)->_sectnum,i);
                                setHittypeTemp(i, 2, 0); //t[2] = 0;
                                setHittypeTemp(i, 0, 0); //t[0] = 0;
                            }
                            else
                                setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz + sgn(getSprite(i)->_z-getSector(sscn)->ceilingz)*SP);
                        }
                        else
                        {
                            if( klabs( getSector(sscn)->ceilingz - getHittypeTemp(i, 1) ) <
                                (SP<<1) )
                            {
                                setSector_ceilingz(getSprite(i)->_sectnum, getHittypeTemp(i, 1));
                                callsound(getSprite(i)->_sectnum,i);
                                setHittypeTemp(i, 2, 0); //t[2] = 0;
                                setHittypeTemp(i, 0, 0); //t[0] = 0;
                            }
                            else
                                setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz + sgn(getHittypeTemp(i, 1)-getSector(sscn)->ceilingz)*SP);
                        }
                        break;
                    }

                    if( (getSprite(i)->_ang&2047) == 1536)
                    {
                        if( klabs(getSector(sscn)->ceilingz-getSprite(i)->_z ) <
                            (SP<<1) )
                        {
                            setHittypeTemp(i, 0, 0); //t[0] = 0;
                            setHittypeTemp(i, 2, !getHittypeTemp(i, 2)); //t[2] = !getHittypeTemp(i, 2);
                            callsound(getSprite(i)->_sectnum,i);
                            setSector_ceilingz(getSprite(i)->_sectnum, getSprite(i)->_z);
                        }
                        else
                            setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz + sgn(getSprite(i)->_z-getSector(sscn)->ceilingz)*SP);
                    }
                    else
                    {
                        if( klabs(getSector(sscn)->ceilingz-getHittypeTemp(i, 1) ) < (SP<<1) )
                        {
                            setHittypeTemp(i, 0, 0); //t[0] = 0;
                            setHittypeTemp(i, 2, !getHittypeTemp(i, 2)); //t[2] = !getHittypeTemp(i, 2);
                            callsound(getSprite(i)->_sectnum,i);
                        }
                        else
                            setSector_ceilingz(getSprite(i)->_sectnum, getSector(sscn)->ceilingz - sgn(getSprite(i)->_z-getHittypeTemp(i, 1))*SP);
                    }
                }
                break;

            case 33: // earthquake debris.
                if( earthquaketime > 0 && (TRAND&7) == 0 )
                {
                    const spritetype *spritei = getSprite(i);
                    RANDOMSCRAP(i, spritei); // I hate ypu for this: RANDOMSCRAP;
                }
                break;
            case 36: //projectile shooter

                if( getHittypeTemp(i, 0) )
                {
                    if( getHittypeTemp(i, 0) == 1 )
                        shoot(i,getSector_extra(sscn));
                    else if( getHittypeTemp(i, 0) == 26*5 )
                        setHittypeTemp(i, 0, 0); //t[0] = 0;
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                }
                break;

            case 128: //SE to control glass breakage

// FIXME: was a bug here? Shall be & 32?
                //if(wal->cstat|32)   // mh ?
                if (getWall_cstat(getHittypeTemp(i, 2)) & 32)
                {
                    setWall_cstat(getHittypeTemp(i, 2), (getWall_cstat(getHittypeTemp(i, 2)) & (255-32)) | 16);
//                    wal->cstat |= 16;
                    if(constwall[getHittypeTemp(i, 2)].nextwall >= 0)
                    {
//                        wall[wal->nextwall].cstat &= (255-32);
//                        wall[wal->nextwall].cstat |= 16;
                        setWall_cstat(constwall[getHittypeTemp(i, 2)].nextwall, (getWall_cstat(constwall[getHittypeTemp(i, 2)].nextwall) & (255-32)) | 16);
                    }
                }
                else break;
                setWall_overpicnum(getHittypeTemp(i, 2), getWall_overpicnum(getHittypeTemp(i, 2)) + 1); // was wal->overpicnum++;
                if(constwall[getHittypeTemp(i, 2)].nextwall >= 0)
                {
//                    wall[wal->nextwall].overpicnum++;
                     setWall_overpicnum(constwall[getHittypeTemp(i, 2)].nextwall, getWall_overpicnum(constwall[getHittypeTemp(i, 2)].nextwall) + 1);
                }

                if(getHittypeTemp(i, 0) < getHittypeTemp(i, 1))
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;
                else
                {
                    setWall_cstat(getHittypeTemp(i, 2), getWall_cstat(getHittypeTemp(i, 2)) & (128+32+8+4+2));
                    if(constwall[getHittypeTemp(i, 2)].nextwall >= 0)
                    {
                         setWall_cstat(constwall[getHittypeTemp(i, 2)].nextwall, getWall_cstat(constwall[getHittypeTemp(i, 2)].nextwall) & (128+32+8+4+2));
                    }
                    KILLIT(i);
                }
                break;

            case 130:
                if(getHittypeTemp(i, 0) > 80) { KILLIT(i); }
                else setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;

                x = getSector(sscn)->floorz-getSector(sscn)->ceilingz;

                if( rnd(64) )
                {
                    k = spawn(i,EXPLOSION2);
                    setSprite_yrepeat(k, 2+(TRAND&7));
                    setSprite_xrepeat(k, getSprite(k)->_yrepeat);
                    setSprite_z(k, getSector(sscn)->floorz-(TRAND%x));
                    setSprite_ang(k, getSprite(k)->_ang + 256-(TRAND%511));
                    setSprite_xvel(k, TRAND&127);
                    ssp(k,CLIPMASK0);
                }
                break;
            case 131:
                if(getHittypeTemp(i, 0) > 40)
                {
                    KILLIT(i);
                }
                else
                    setHittypeTemp(i, 0, getHittypeTemp(i, 0) + 1); // t[0]++;

                x = getSector(sscn)->floorz-getSector(sscn)->ceilingz;

                if( rnd(32) )
                {
                    k = spawn(i,EXPLOSION2);
                    setSprite_yrepeat(k, 2+(TRAND&3));
                    setSprite_xrepeat(k, getSprite(k)->_yrepeat);
                    setSprite_z(k, getSector(sscn)->floorz-(TRAND%x));
                    setSprite_ang(k, getSprite(k)->_ang + 256-(TRAND%511));
                    setSprite_xvel(k, TRAND&127);
                    ssp(k,CLIPMASK0);
                }
                break;
        }
        BOLT:
        i = nexti;
    }

         //Sloped sin-wave floors!
     for(i=headspritestat[3];i>=0;i=spr_add_data[i].nextspritestat)
     {
          if (getSprite(i)->_lotag != 29) continue;
//          getSector(getSprite(i)->_sectnum);
          if (constsector[getSprite(i)->_sectnum].wallnum != 4) continue;
           const walltype *wal = getWall(constsector[getSprite(i)->_sectnum].wallptr+2);
          alignflorslope(getSprite(i)->_sectnum,wal->x,wal->y,getSector(constwall[constsector[getSprite(i)->_sectnum].wallptr+2].nextsector)->floorz);
     }
}

