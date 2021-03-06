/*
   Copyright (C) 2013 by L.Sebilleau <l.sebilleau@free.fr>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/*
    Yet Another map generator
    Implementation file.
*/

#include "random.hpp"
#include "yamg_params.hpp"

#ifdef YAMG_STANDALONE
#include <ctype.h>
#include <string.h>
#endif


//yamg_params::yamg_params(config& /*cfg*/)
//{
//	yamg_params::yamg_params();
//}


yamg_params::yamg_params()
{
    //ctor set defaults values
#ifdef YAMG_STANDALONE
    strcpy(path,"wesmap.map");           ///< pathname of the file
#endif
    //TODO
    seed = get_random();
    		//148737975;       ///< random seed
    larg = 65;              ///< map width
    haut = 65;              ///< map height
    rough = 12;             ///< map roughness
    type = 't';             ///< landscape type
    season = 's';           ///< season to use
    snowlev = M_NUMLEVEL * 10;   ///< snow level 0-M_NUMLEVEL
    wRatio = 100;           ///< water ratio for rivers and lakes
    altMid = 0;             ///< map global altitudes
    altNW = 0;              ///< map altitude NW
    altNE = 0;              ///< map altitude NE
    altSE = 0;              ///< map altitude SE
    altSW = 0;              ///< map altitude SW
    bridges = 50;
    roads = true;
    roRoad = 0;
    vill = 10;              ///< number of isolated houses
    burgs = 4;              ///< number of villages
//    towns = 0;              ///< number of towns
    forests = 20;           ///< forests rate of terrain
    players = 0;            ///< number of players / castles
    casthexes = 6;          ///< number of hexes in castles

    thickness[YAMG_DEEPSEA] = 10;      ///< layers thickness
    thickness[YAMG_SHALLSEA] = 10;
    thickness[YAMG_BEACH] = 5;
    thickness[YAMG_SWAMPS] = 5;
    thickness[YAMG_GROUND] = 35;
    thickness[YAMG_HILLS] = 15;
    thickness[YAMG_MOUNTAINS] = 15;
    thickness[YAMG_IMPMOUNTS] = 10;

    int i;
    for(i=0; i < M_NUMLEVEL; i++)
        baseCust[i] = NULL;
    for(i=0; i < 12; i++)
        forestCust[i] = NULL;
    for(i=0; i < 14; i++)
        housesCust[i] = NULL;
    for(i=0; i < 10; i++)
        keepsCastlesC[i] = NULL;
    for(i=0; i < 10; i++)
        hexesCastlesC[i] = NULL;
    for(i=0; i < M_NUMLEVEL; i++)
        baseSnowC[i] = NULL;
    for(i=0; i < 4; i++)
        roadsC[i] = NULL;
    liliesC = NULL;
    bridgesC = NULL;
    fieldsC = NULL;
}

yamg_params::~yamg_params()
{
    int i;
    for(i=0; i < M_NUMLEVEL; i++)
        if(baseCust[i] != NULL)
            delete baseCust[i];
    for(i=0; i < 12; i++)
        if(forestCust[i] != NULL)
            delete forestCust[i];
    for(i=0; i < 14; i++)
        if(housesCust[i] != NULL)
            delete housesCust[i];
    for(i=0; i < 10; i++)
        if(keepsCastlesC[i] != NULL)
            delete keepsCastlesC[i];
    for(i=0; i < 10; i++)
        if(hexesCastlesC[i] != NULL)
            delete hexesCastlesC[i];
    for(i=0; i < M_NUMLEVEL; i++)
        if(baseSnowC[i] != NULL)
            delete baseSnowC[i];
    for(i=0; i < 4; i++)
        if(roadsC[i] != NULL)
            delete roadsC[i];
    if(liliesC != NULL)
        delete liliesC;
    if(bridgesC != NULL)
        delete bridgesC;
    if(fieldsC != NULL)
        delete fieldsC;
}


/**
    Check parameters limits and consistency
    TODO: finish this
*/
unsigned int yamg_params::verify()
{

    unsigned int result = YAMG_OK;

    if((larg > YAMG_MAPMAXSIZE) || (haut > YAMG_MAPMAXSIZE) )
        result |= YAMG_LIMITS;

    if((rough > YAMG_ROUGHMAX) || (rough == 0))
        result |= YAMG_ROUGHOFF;

    if(!findInString(YAMG_TYPES,type))
        result |= YAMG_BADTYPE;

    if(!findInString(YAMG_SEASONS,season))
        result |= YAMG_BADSEASON;

    //TODO this line causes a warning
    if((snowlev > ((M_NUMLEVEL+1) * 10))) // || (snowlev < 0))
        result |= YAMG_SNOW;

    if((altMid > YAMG_ALTMAX) || (altMid < -YAMG_ALTMAX))
        result |= YAMG_ALTOFF;

    if(vill > YAMG_VILLMAX)
        result |= YAMG_VILLOFF;

    if(burgs > YAMG_BURGMAX)
        result |= YAMG_BURGOFF;

//    if(towns > YAMG_TOWNSMAX)
//        result |= YAMG_TOWNSOFF;

    if(forests > YAMG_FORESTMAX)
        result |= YAMG_FORESTOFF;

    if(players > YAMG_PLAYERSMAX)
        result |= YAMG_PLAYERSOFF;

    return result;
}

#ifdef YAMG_STANDALONE
/**
    This part is needed only if parameters are read from a file
*/
const char *parmlist[] =
{
    "mapname",
    "seed",
    "width",
    "height",
    "rough",
    "type",
    "season",
    "snowlevel",
    "altmiddle",
    "roads",
    "bridges",
    "castlesHexes",
    "villages",
    "burgs",
    "towns",
    "forests",
    "castles",
    "evaporation",
    "deepsea",
    "shallowsea",
    "beach",
    "swamps",
    "fields",
    "hills",
    "mountains",
    "peaks",
    "straightness",
    "altbases",
    "altforests",
    "althouses",
    "altkeeps",
    "altcastles",
    "altsnow",
    "altroads",
    "altlilies",
    "altfields",
    "altbridges",
    "altNW",
    "altNE",
    "altSE",
    "altSW",
};

/**
    Storing custom terrain codes from parameters file
*/
void yamg_params::storeTerrainCodes(const char *input, const char **table, unsigned int cnt)
{
    unsigned int i,l,nt;
    const char *pt;
    char *w;

    for(i = 0; i < cnt; i++)
    {
        pt = input;
        while((*pt != ',') && (*pt != '\0'))
            pt++;
        l = pt - input;
        if((l > 1) && (l <= 6))
        {
            table[i] = new char[10];
            for(nt = 0, w = (char *)table[i]; nt < l; nt++, w++, input++)
            {
                *w = *input;
            }
            *w = '\0';
        }
        if( *pt == '\0')
            return;
        else
            input = pt + 1;
    }
}

/**
    Read a parameter file
    -> path to file
*/
unsigned int yamg_params::readParams(const char *ficnom)
{
    FILE *f;
    char buf[20000], instr[100], value[2048], *wr = NULL, *ptr, *end;
    int n,i;

    f = fopen(ficnom,"r");
    if(f == NULL)
        return YAMG_FILENOTFOUND;

    n = fread(buf,1,20000,f);
    ptr = buf;
    end = buf + n;
    n = 3;
    // ---- read loop ----
    while(ptr < end)
    {
        switch(n)
        {
        case 0:
            // catch parameter name.
            while( (*ptr != '=') && (*ptr != '#') && (ptr < end))
                *wr++ = *ptr++;
            if(*ptr == '#')
                n = 2; // skip the line
            else
            {
                *wr++ = '\0';
                wr = value;
                ptr++;
                n = 1;
            }
            break;

        case 1:
            // get the value
            while( (!isspace(*ptr)) && (*ptr != '#') && (ptr < end))
                *wr++ = *ptr++;

            *wr++ = '\0';
            for(i=0; i < 41; i++)
            {
                if(strcmp(instr,parmlist[i]) == 0)
                    break;
            }

            switch(i)
            {
            case 0: // mapname
                strcpy(path,value);
                break;
            case 1:
                sscanf(value,"%u",&seed);
                break;
            case 2:
                sscanf(value,"%u",&larg);
                break;
            case 3:
                sscanf(value,"%u",&haut);
                break;
            case 4:
                sscanf(value,"%u",&rough);
                break;
            case 5:
                type = value[0];
                break;
            case 6:
                season = value[0];
                break;
            case 7:
                sscanf(value,"%u",&snowlev);
                snowlev -= 10;
                break;
            case 8:
                sscanf(value,"%i",&altMid);
                break;
            case 9:
                roads = ((*value == 'n') || (*value == 'N')) ? false:true;
                break;
            case 10:
                sscanf(value,"%i",&bridges);
                break;
            case 11:
                sscanf(value,"%u",&casthexes);
                break;
            case 12:
                sscanf(value,"%u",&vill);
                break;
            case 13:
                sscanf(value,"%u",&burgs);
                break;
            case 14:
                //sscanf(value,"%u",&towns);
                break;
            case 15:
                sscanf(value,"%u",&forests);
                break;
            case 16:
                sscanf(value,"%u",&players);
                break;
            case 17:
                sscanf(value,"%u",&wRatio);
                break;
            case 18:
                sscanf(value,"%u",&thickness[0]);
                break;
            case 19:
                sscanf(value,"%u",&thickness[1]);
                break;
            case 20:
                sscanf(value,"%u",&thickness[2]);
                break;
            case 21:
                sscanf(value,"%u",&thickness[3]);
                break;
            case 22:
                sscanf(value,"%u",&thickness[4]);
                break;
            case 23:
                sscanf(value,"%u",&thickness[5]);
                break;
            case 24:
                sscanf(value,"%u",&thickness[6]);
                break;
            case 25:
                sscanf(value,"%u",&thickness[7]);
                break;
            case 26:
                sscanf(value,"%u",&roRoad);
                break;
            case 27:
                storeTerrainCodes(value, baseCust, M_NUMLEVEL);
                break;
            case 28:
                storeTerrainCodes(value, forestCust, 12);
                break;
            case 29:
                storeTerrainCodes(value, housesCust, 14);
                break;
            case 30:
                storeTerrainCodes(value, keepsCastlesC, 10);
                break;
            case 31:
                storeTerrainCodes(value, hexesCastlesC, 10);
                break;
            case 32:
                storeTerrainCodes(value, baseSnowC, M_NUMLEVEL);
                break;
            case 33:
                storeTerrainCodes(value, roadsC, 4);
                break;
            case 34:
                liliesC = new char[10];
                strncpy(liliesC,value,5);
                break;
            case 35:
                fieldsC = new char[10];
                strncpy(fieldsC,value,5);
                break;
            case 36:
                bridgesC = new char[10];
                strncpy(bridgesC,value,5);
                break;
            case 37:
                sscanf(value,"%u",&altNW);
                break;
            case 38:
                sscanf(value,"%u",&altNE);
                break;
            case 39:
                sscanf(value,"%u",&altSE);
                break;
            case 40:
                sscanf(value,"%u",&altSW);
                break;

            default: // unknow parameter
                break;
            }

        case 2:
            // go to end of line
            while( (*ptr != '\n') && (ptr < end))
                ptr++;
            ptr++;
            if((ptr >= end) || (*ptr == '\n'))
                break;
            // no break if we are only at the end of a line.
        case 3:
            wr = instr;
            n = 0;
            break;
        }
    }
    fclose(f);
    return verify();
}
#endif

//----------------- Utils ----------------
bool findInString(const char *str, char c)
{
    const char *ptr = str;

    while( *ptr != '\0')
        if(*ptr == c)
            return true;
        else
            ptr++;
    return false;
}
