/*
    CompileCons utility for Duke Nukem 3D port to Silicon Labs MG24 series MCU.
    Copyright (C) 2025, Nicola Wrachien

    This tool is used to compile CON files on the DUKE3D.GRP and generate constant arrays
    and definitions to be included in con_file.h

    This program is based on Duke Nukem 3D source port responsible for compiling such CON files.

    This program is provided just for reference in case you are using a GRP different from Atomic or 1.3d.

    License: same as original one reported below.

    --- Original Copyright and license from 3D Realms.

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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#define MAXSCRIPTSIZE   20460
#define NUMOFFIRSTTIMEACTIVE 128
#define MAX_NUM_SOUNDS   450
#define NUM_SOUNDS maxSoundsFound
#define MAXTILES 5200
#define MAXSPRITES 1200
//
#define KNEE_WEAPON          0
#define PISTOL_WEAPON        1
#define SHOTGUN_WEAPON       2
#define CHAINGUN_WEAPON      3
#define RPG_WEAPON           4
#define HANDBOMB_WEAPON      5
#define SHRINKER_WEAPON      6
#define DEVISTATOR_WEAPON    7
#define TRIPBOMB_WEAPON      8
#define FREEZE_WEAPON        9
#define HANDREMOTE_WEAPON    10
#define GROW_WEAPON          11
#define MAX_WEAPONS          12
//
#define MAX_WORDS_PER_LINE   8
#define MAX_SHORTS_PER_LINE 16
#define MAX_BYTES_PER_LINE 32

#define NUMKEYWORDS     112
#define Error(a, ...) do {printf(__VA_ARGS__); exit(1);}while(0)
#define GRP_SIGNATURE "KenSilverman"
#define PRINT_VARIABLE(type, name) printf("%s %s = %d;\r\n", type, #name, name);

void extract_file(FILE *grp_file, const char *file_name, const char *output_dir);
void passone(int readfromGRP);
void compilecons(void);
static char tempbuf[65536];
int ud_const_visibility;
int32_t maxSoundsFound = 0;
void assert(int x)
{
    if (!x)
    {
        printf("error assert\r\n");
        exit(2);
    }
}
void gameexit(char * reason)
{
    printf("error %s\r\n", reason);
    exit(4);
}
 char  * keyw[NUMKEYWORDS] =
{
    "definelevelname",  // 0
    "actor",            // 1    [#]
    "addammo",          // 2    [#]
    "ifrnd",            // 3    [C]
    "enda",             // 4    [:]
    "ifcansee",         // 5    [C]
    "ifhitweapon",      // 6    [#]
    "action",           // 7    [#]
    "ifpdistl",         // 8    [#]
    "ifpdistg",         // 9    [#]
    "else",             // 10   [#]
    "strength",         // 11   [#]
    "break",            // 12   [#]
    "shoot",            // 13   [#]
    "palfrom",          // 14   [#]
    "sound",            // 15   [filename.voc]
    "fall",             // 16   []
    "state",            // 17
    "ends",             // 18
    "define",           // 19
    "//",               // 20
    "ifai",             // 21
    "killit",           // 22
    "addweapon",        // 23
    "ai",               // 24
    "addphealth",       // 25
    "ifdead",           // 26
    "ifsquished",       // 27
    "sizeto",           // 28
    "{",                // 29
    "}",                // 30
    "spawn",            // 31
    "move",             // 32
    "ifwasweapon",      // 33
    "ifaction",         // 34
    "ifactioncount",    // 35
    "resetactioncount", // 36
    "debris",           // 37
    "pstomp",           // 38
    "/*",               // 39
    "cstat",            // 40
    "ifmove",           // 41
    "resetplayer",      // 42
    "ifonwater",        // 43
    "ifinwater",        // 44
    "ifcanshoottarget", // 45
    "ifcount",          // 46
    "resetcount",       // 47
    "addinventory",     // 48
    "ifactornotstayput",// 49
    "hitradius",        // 50
    "ifp",              // 51
    "count",            // 52
    "ifactor",          // 53
    "music",            // 54
    "include",          // 55
    "ifstrength",       // 56
    "definesound",      // 57
    "guts",             // 58
    "ifspawnedby",      // 59
    "gamestartup",      // 60
    "wackplayer",       // 61
    "ifgapzl",          // 62
    "ifhitspace",       // 63
    "ifoutside",        // 64
    "ifmultiplayer",    // 65
    "operate",          // 66
    "ifinspace",        // 67
    "debug",            // 68
    "endofgame",        // 69
    "ifbulletnear",     // 70
    "ifrespawn",        // 71
    "iffloordistl",     // 72
    "ifceilingdistl",   // 73
    "spritepal",        // 74
    "ifpinventory",     // 75
    "betaname",         // 76
    "cactor",           // 77
    "ifphealthl",       // 78
    "definequote",      // 79
    "quote",            // 80
    "ifinouterspace",   // 81
    "ifnotmoving",      // 82
    "respawnhitag",     // 83
    "tip",              // 84
    "ifspritepal",      // 85
    "money",            // 86
    "soundonce",        // 87
    "addkills",         // 88
    "stopsound",        // 89
    "ifawayfromwall",   // 90
    "ifcanseetarget",   // 91
    "globalsound",      // 92
    "lotsofglass",      // 93
    "ifgotweaponce",    // 94
    "getlastpal",       // 95
    "pkick",            // 96
    "mikesnd",          // 97
    "useractor",        // 98
    "sizeat",           // 99
    "addstrength",      // 100   [#]
    "cstator",          // 101
    "mail",             // 102
    "paper",            // 103
    "tossweapon",       // 104
    "sleeptime",        // 105
    "nullop",           // 106
    "definevolumename", // 107
    "defineskillname",  // 108
    "ifnosounds",       // 109
    "clipdist",         // 110
    "ifangdiffl"        // 111
};
uint8_t  ispecial(uint8_t  c);

int spriteqamount = 64;
uint8_t lasermode;
int32_t gc,max_player_health,max_armour_amount,max_ammo_amount[MAX_WEAPONS];
int conVersion = 0;
int32_t *parsing_actor;
char  fta_quotes[NUMOFFIRSTTIMEACTIVE][64];
char level_names[44][33];
char level_file_names[44][128];
int32_t partime[44],designertime[44];
char  volume_names[4][33] = { "L.A. MELTDOWN", "LUNAR APOCALYPSE", "SHRAPNEL CITY", "" }; // Names are not in 1.3 con files. MUST be in code.
char  skill_names[5][33] = { "PIECE OF CAKE", "LET'S ROCK", "COME GET SOME", "DAMN I'M GOOD", "" };
char confilename[128] = {"GAME.CON"};
short soundps[MAX_NUM_SOUNDS],soundpe[MAX_NUM_SOUNDS],soundvo[MAX_NUM_SOUNDS];
uint8_t  soundm[MAX_NUM_SOUNDS],soundpr[MAX_NUM_SOUNDS];
char  sounds[MAX_NUM_SOUNDS][14];
int32_t respawnactortime=768, respawnitemtime=768;
int32_t script[MAXSCRIPTSIZE];
int16_t actorscridx[MAXTILES];
uint8_t  actortype[MAXTILES];
char  music_fn[4][11][13];
char  env_music_fn[4][13];
uint8_t *mymembuf;
int32_t impact_damage;
int32_t dukefriction = 0xcc00;
uint8_t  camerashitable,freezerhurtowner=0;
int32_t numfreezebounces=3,rpgblastradius,pipebombblastradius,tripbombblastradius,shrinkerblastradius,morterblastradius,bouncemineblastradius,seenineblastradius;

static short total_lines,line_number;
static uint8_t  checking_ifelse,parsing_state;
char *last_used_text;
static short num_squigilly_brackets;
static int32_t last_used_size;
int32_t *scriptptr,*insptr,*labelcode,labelcnt;
char  *label,*textptr,error,warning;

char  fta_quotes[NUMOFFIRSTTIMEACTIVE][64];
char  sounds[MAX_NUM_SOUNDS][14];
int32_t script[MAXSCRIPTSIZE];
char  *defaultcons[3] =
{
     "GAME.CON",
     "USER.CON",
     "DEFS.CON"
};
int main()
{
    printf("Sizeof void * %d\r\n", sizeof(void *));
    FILE *grp = fopen("DUKE3D.GRP", "rb");
    if (grp)
    {
        for (int i = 0; i < sizeof(defaultcons)/ sizeof(defaultcons[0]); i++)
        {
           extract_file(grp, defaultcons[i], ".");
        }
        compilecons();
    }
    else
    {
        Error(0, "DUKE3D.GRP not found!");
    }
}



typedef struct {
    char name[12];
    unsigned int size;
} FileEntry;

void extract_file(FILE *grp_file, const char *file_name, const char *output_dir)
{
    char signature[12];
    unsigned int num_files;
    FileEntry *file_entries;
    int found = 0;
    unsigned int i;
    fseek(grp_file, 0, SEEK_SET);
    // Read the GRP file signature
    fread(signature, sizeof(char), 12, grp_file);
    if (strncmp(signature, GRP_SIGNATURE, 12) != 0)
    {
        fprintf(stderr, "Not a valid GRP file.\n");
        return;
    }

    // Read the number of files
    fread(&num_files, sizeof(unsigned int), 1, grp_file);

    // Allocate memory for file entries
    file_entries = (FileEntry *)malloc(num_files * sizeof(FileEntry));
    if (!file_entries) {
        fprintf(stderr, "Memory allocation failed.\n");
        return;
    }

    // Read the file entries
    for (i = 0; i < num_files; i++) {
        fread(file_entries[i].name, sizeof(char), 12, grp_file);
        fread(&file_entries[i].size, sizeof(unsigned int), 1, grp_file);
    }

    // Search for the desired file
    for (i = 0; i < num_files; i++) {
        if (strncmp(file_entries[i].name, file_name, 12) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        fprintf(stderr, "File %s not found in the GRP file.\n", file_name);
        free(file_entries);
        return;
    }

    // Extract the file
    char *file_data = (char *)malloc(file_entries[i].size);
    if (!file_data) {
        fprintf(stderr, "Memory allocation failed.\n");
        free(file_entries);
        return;
    }

    fseek(grp_file, 16 + num_files * 16, SEEK_SET); // Move to the start of the file data
    for (unsigned int j = 0; j < i; j++) {
        fseek(grp_file, file_entries[j].size, SEEK_CUR); // Skip the previous files
    }

    fread(file_data, sizeof(char), file_entries[i].size, grp_file);

    // Create the output file path
    char output_path[256];
    snprintf(output_path, sizeof(output_path), "%s/%s", output_dir, file_entries[i].name);

    // Write the extracted file to the output directory
    FILE *output_file = fopen(output_path, "wb");
    if (!output_file) {
        fprintf(stderr, "Failed to open output file %s.\n", output_path);
        free(file_data);
        free(file_entries);
        return;
    }

    fwrite(file_data, sizeof(char), file_entries[i].size, output_file);
    fclose(output_file);

    printf("Extracted %s to %s\n", file_entries[i].name, output_path);

    // Clean up
    free(file_data);
    free(file_entries);
}

FILE * TCkopen4load( char *name , int dummy)
{
    return fopen(name, "rb");
}
int kread(FILE *f, void *dest, int len)
{
    return fread(dest, 1, len, f);
}
void kclose(FILE *f)
{
    fclose(f);
}
long kfilelength(FILE* fp) {
    long size;

    // Save the current position of the file pointer
    long current_pos = ftell(fp);

    // Seek to the end of the file
    fseek(fp, 0, SEEK_END);

    // Get the position of the file pointer, which is the size of the file
    size = ftell(fp);

    // Restore the file pointer to its original position
    fseek(fp, current_pos, SEEK_SET);

    return size;
}
/**
 * Encode a scriptptr into a form suitable for portably
 * inserting into bytecode. We store the pointer as minus
 * the offset from the start of the script buffer, just
 * to make it perhaps a little more obvious what is happening.
 */
int32_t encodescriptptr(int32_t* scptr)
{
    int32_t offs = (int32_t)(scptr - script);

	assert(offs >= 0);
    assert(offs < MAXSCRIPTSIZE);

	return -offs;
}
/* next-hack: added these functions to use indexes instead of pointers.
*/

int32_t getscriptptridx(int32_t* scptr)
{
    int32_t offs = (int32_t)(scptr - script);

	assert(offs >= 0);
    assert(offs < MAXSCRIPTSIZE);
    // add 1 so that we can use idx 0 as null pointer
	return offs + 1;
}
int32_t* getscriptptr(int16_t idx)
{
    if (idx == 0)
        return NULL;
    return &script[idx - 1];
}
void printCstring(char *str)
{
    while(*str > 0)
    {
        if (*str == '\r')
        {
            printf("\\r");
        }
        else  if (*str == '\n')
        {
            printf("\\n");
        }
        else  if (*str == '\"')
        {
            printf("\\\"");
        }
        else
            printf("%c", *str);
        str++;
    }
}

/**
 * Decode an encoded representation of a scriptptr
 */
int32_t* decodescriptptr(int32_t scptr)
{
    assert(scptr <= 0);

	int32_t offs = -scptr;

	assert(offs >= 0);
    assert(offs < MAXSCRIPTSIZE);

	return script + offs;
}

void loadefs(char  *filenam, char  *mptr, int readfromGRP)
{
    int32_t fs;
    FILE *fp;
	uint8_t  kbdKey;

	memset(script, 0, sizeof(script));

	// FIX_00071: do not ask for internal default con if
	// external con or internal con is buggy

    fp = TCkopen4load(filenam,readfromGRP);
    if( fp == NULL )
    {
		Error(EXIT_SUCCESS, "ERROR: CON(%s) not found.\n", filenam);
    }
    else
    {
        printf("Compiling: '%s'.\n",filenam);

        fs = kfilelength(fp);

        last_used_text = textptr = (char  *) mptr;
        last_used_size = fs;

        kread(fp,(uint8_t  *)textptr,fs);
        kclose(fp);
    }

    textptr[fs - 2] = 0;

    memset(actorscridx, 0, sizeof(actorscridx));
    memset(actortype,0, MAXTILES);

    labelcnt = 0;
    scriptptr = script+1;
    warning = 0;
    error = 0;
    line_number = 1;
    total_lines = 0;
    passone(readfromGRP); //Tokenize
    *script = encodescriptptr(scriptptr);

    if(warning|error)
        printf("Found %hhd warning(s), '%c' error(s).\n",warning,error);

    if(error)
    {
		Error(EXIT_SUCCESS, "ERROR in CON(%s)\n", filenam);
    }
    else
    {
        total_lines += line_number;
        printf("Code Size:%d bytes(%d labels).\n",(int32_t)((scriptptr-script)<<2)-4,labelcnt);
    }
}


void compilecons(void)
{

    char  userconfilename[512];
    char mymembuf_stack[262144] = {0}, labelcode_stack[262144] = {0}, label_stack[262144] = {0};
    mymembuf = mymembuf_stack;
    labelcode = (int32_t*)labelcode_stack;
    label = (char*)label_stack;
    sprintf(userconfilename, "%s", confilename);

    loadefs(userconfilename, mymembuf, 0);

    printf("Code Size:%d bytes(%d labels).\n",(int32_t)((scriptptr-script)<<2)-4,labelcnt);
    printf("Conversion %d\r\n", conVersion);
    int size = (scriptptr-script);
    printf("Volume Name[3] = %s\r\n", volume_names[3]);
    printf("#define MAXSCRIPTSIZE %d\r\n", size);
    printf("#define LABEL_CNT %d\r\n", labelcnt);
    printf("#define NUM_SOUNDS %d\r\n", NUM_SOUNDS);
    printf("const int32_t script[MAXSCRIPTSIZE] = \r\n{\r\n    ");
    for (int i = 0; i < size; i++)
    {
        printf("0x%08X, ", script[i]);
        if (i % MAX_WORDS_PER_LINE == MAX_WORDS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    // char  music_fn[4][11][13];
    printf("const char  music_fn[4][11][13] =\r\n{\r\n");
    for (int i = 0; i < 4; i++)
    {
        printf("    {\r\n        ");
        for (int j = 0; j < 11; j++)
        {
            printf("\"%s\", ", music_fn[i][j]);
        }
        printf("\r\n    },\r\n");
    }
    printf("};\r\n");
    // char  env_music_fn[4][13];
    printf("const char  env_music_fn[4][13] =\r\n{\r\n");
    for (int i = 0; i < 4; i++)
    {
            printf("\"%s\", ", env_music_fn[i]);
    }
    printf("\r\n};\r\n");
    #warning to be reduced.

    //int16_t  actorsscridx[MAXTILES]
    printf("const int16_t  actorscridx[MAXTILES] =\r\n{\r\n");
    for (int i = 0; i < MAXTILES; i++)
    {
        printf("0x%04X, ", actorscridx[i]);
        if (i % MAX_SHORTS_PER_LINE == MAX_SHORTS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    // uint8_t  actortype[MAXTILES];
    printf("#if USE_ACTORTYPE_ARRAY\r\n");
    printf("const uint8_t  actortype[MAXTILES] =\r\n{\r\n");
    for (int i = 0; i < MAXTILES; i++)
    {
        printf("0x%02X, ", actortype[i]);
        if (i % MAX_BYTES_PER_LINE == MAX_BYTES_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    //
    printf("#else\r\n");
    printf("uint8_t getActorType(int idx)\r\n");
    printf("{\r\n");
    printf("  switch(idx)\r\n");
    printf("  {\r\n");
    for (int i = 0; i < MAXTILES; i++)
    {
        if(actortype[i])
            printf("    case %d: return %d;\r\n", i, actortype[i]);
    }
    printf("  }\r\n");
    printf("  return 0;\r\n");
    printf("}\r\n");

    printf("#endif\r\n");

    //
    // skill_names: NOT NEEDED, ALREADY PRESENT (unless we use > 1.3)
    // volume_names: NOT NEEDED, ALREADY PRESENT (unless we use >1.3. Todo anyway.
    // betaname  // not used.

    // char level_names[44][33];
    printf("const char level_names[44][20] =\r\n{\r\n");
    for (int i = 0; i < 44; i++)
    {
            printf("    \"%s\",\r\n", level_names[i]);
    }
    printf("};\r\n");
    // char level_file_names[44][128];
    printf("const char level_file_names[44][10] =\r\n{\r\n");
    for (int i = 0; i < 44; i++)
    {
            printf("    \"%s\",\r\n", level_file_names[i]);
    }
    printf("};\r\n");
    // int32_t partime[44];
    printf("const int32_t partime[44] = \r\n{\r\n    ");
    for (int i = 0; i < 44; i++)
    {
        printf("0x%08X, ", partime[i]);
        if (i % MAX_WORDS_PER_LINE == MAX_WORDS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    // int32_t designertime[44];
    printf("const int32_t designertime[44] = \r\n{\r\n    ");
    for (int i = 0; i < 44; i++)
    {
        printf("0x%08X, ", designertime[i]);
        if (i % MAX_WORDS_PER_LINE == MAX_WORDS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    // char fta_quotes[NUMOFFIRSTTIMEACTIVE][64];
    printf("const char fta_quotes[NUMOFFIRSTTIMEACTIVE][38]=\r\n{\r\n");
    for (int i = 0; i < NUMOFFIRSTTIMEACTIVE; i++)
    {
            printf("    \"");
            printCstring(fta_quotes[i]);
            printf("\",\r\n");
    }
    printf("};\r\n");
    // char  sounds[NUM_SOUNDS][14];
    printf("const char sounds[NUM_SOUNDS][14] = \r\n{\r\n");
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
            printf("    \"%s\",\r\n", sounds[i]);
    }
    printf("\r\n};\r\n");
    //short soundps[NUM_SOUNDS]
    printf("const short soundps[NUM_SOUNDS] =\r\n{\r\n");
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
        printf("0x%04X, ", soundps[i] & 0xFFFF);
        if (i % MAX_SHORTS_PER_LINE == MAX_SHORTS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    //short soundpe[NUM_SOUNDS]
    printf("const short soundpe[NUM_SOUNDS] =\r\n{\r\n");
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
        printf("0x%04X, ", soundpe[i] & 0xFFFF);
        if (i % MAX_SHORTS_PER_LINE == MAX_SHORTS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    //short soundvo[NUM_SOUNDS]
    printf("const short soundvo[NUM_SOUNDS] =\r\n{\r\n");
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
        printf("0x%04X, ", soundvo[i] & 0xFFFF);
        if (i % MAX_SHORTS_PER_LINE == MAX_SHORTS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    //uint8_t soundm[NUM_SOUNDS]
    printf("const uint8_t soundm[NUM_SOUNDS] =\r\n{\r\n");
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
        printf("0x%02X, ", soundm[i] & 0xFF);
        if (i % MAX_SHORTS_PER_LINE == MAX_SHORTS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    //uint8_t soundpr[NUM_SOUNDS]
    printf("const uint8_t soundpr[NUM_SOUNDS] =\r\n{\r\n");
    for (int i = 0; i < NUM_SOUNDS; i++)
    {
        printf("0x%02X, ", soundpr[i] & 0xFF);
        if (i % MAX_BYTES_PER_LINE == MAX_BYTES_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    // int32_t max_ammo_amount[MAX_WEAPONS]
    printf("const int32_t max_ammo_amount[MAX_WEAPONS] =\r\n{\r\n    ");
    for (int i = 0; i < MAX_WEAPONS; i++)
    {
        printf("0x%08X, ", max_ammo_amount[i]);
        if (i % MAX_WORDS_PER_LINE == MAX_WORDS_PER_LINE - 1)
        {
            printf("\r\n    ");
        }
    }
    printf("\r\n};\r\n");
    PRINT_VARIABLE("const int32_t", impact_damage);
    PRINT_VARIABLE("const int32_t", max_armour_amount);
    PRINT_VARIABLE("const int32_t", max_player_health);
    PRINT_VARIABLE("const int32_t", gc);
    PRINT_VARIABLE("const int32_t", dukefriction);
    PRINT_VARIABLE("const int32_t", respawnitemtime);
    PRINT_VARIABLE("const int32_t", respawnactortime);

    PRINT_VARIABLE("const int32_t", rpgblastradius);
    PRINT_VARIABLE("const int32_t", pipebombblastradius);
    PRINT_VARIABLE("const int32_t", shrinkerblastradius);
    PRINT_VARIABLE("const int32_t", tripbombblastradius);
    PRINT_VARIABLE("const int32_t", morterblastradius);
    PRINT_VARIABLE("const int32_t", bouncemineblastradius);
    PRINT_VARIABLE("const int32_t", seenineblastradius);
    PRINT_VARIABLE("const int32_t", numfreezebounces);
    PRINT_VARIABLE("const uint8_t", camerashitable);
    PRINT_VARIABLE("const uint8_t", freezerhurtowner);
    system("pause");
}

void getlabel(void)
{
    int32_t i;

    while( isalnum(*textptr) == 0 )
    {
        if(*textptr == 0x0a) line_number++;
        textptr++;
        if( *textptr == 0)
            return;
    }

    i = 0;
    while( ispecial(*textptr) == 0 )
        label[(labelcnt<<6)+i++] = *(textptr++);

    label[(labelcnt<<6)+i] = 0;
}
uint8_t  ispecial(uint8_t  c)
{
    if(c == 0x0a)
    {
        line_number++;
        return 1;
    }

    if(c == ' ' || c == 0x0d)
        return 1;

    return 0;
}

uint8_t  isaltok(uint8_t  c)
{
    return ( isalnum(c) || c == '{' || c == '}' || c == '/' || c == '*' || c == '-' || c == '_' || c == '.');
}
int32_t keyword(void)
{
    int32_t i;
    char  *temptextptr;

    temptextptr = textptr;

    while( isaltok(*temptextptr) == 0 )
    {
        temptextptr++;
        if( *temptextptr == 0 )
            return 0;
    }

    i = 0;
    while( isaltok(*temptextptr) )
    {
        tempbuf[i] = *(temptextptr++);
        i++;
    }
    tempbuf[i] = 0;

    for(i=0;i<NUMKEYWORDS;i++)
        if( strcmp( (const char *)tempbuf,keyw[i]) == 0 )
            return i;

    return -1;
}

int32_t transword(void) //Returns its code #
{
    int32_t i, l;

    while( isaltok(*textptr) == 0 )
    {
        if(*textptr == 0x0a) line_number++;
        if( *textptr == 0 )
            return -1;
        textptr++;
    }

    l = 0;
    while( isaltok(*(textptr+l)) )
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    for(i=0;i<NUMKEYWORDS;i++)
    {
        if( strcmp( (const char *)tempbuf,keyw[i]) == 0 )
        {
            *scriptptr = i;
            textptr += l;
            scriptptr++;
            return i;
        }
    }

    textptr += l;

    if( tempbuf[0] == '{' && tempbuf[1] != 0)
        printf("  * ERROR!(L%hd) Expecting a SPACE or CR between '{' and '%s'.\n",line_number,tempbuf+1);
    else if( tempbuf[0] == '}' && tempbuf[1] != 0)
        printf("  * ERROR!(L%hd) Expecting a SPACE or CR between '}' and '%s'.\n",line_number,tempbuf+1);
    else if( tempbuf[0] == '/' && tempbuf[1] == '/' && tempbuf[2] != 0 )
        printf("  * ERROR!(L%hd) Expecting a SPACE between '//' and '%s'.\n",line_number,tempbuf+2);
    else if( tempbuf[0] == '/' && tempbuf[1] == '*' && tempbuf[2] != 0 )
        printf("  * ERROR!(L%hd) Expecting a SPACE between '/*' and '%s'.\n",line_number,tempbuf+2);
    else if( tempbuf[0] == '*' && tempbuf[1] == '/' && tempbuf[2] != 0 )
        printf("  * ERROR!(L%hd) Expecting a SPACE between '*/' and '%s'.\n",line_number,tempbuf+2);
    else printf("  * ERROR!(L%hd) Expecting key word, but found '%s'.\n",line_number,tempbuf);

    error++;
    return -1;
}

void transnum(void)
{
    int32_t i, l;

    while( isaltok(*textptr) == 0 )
    {
        if(*textptr == 0x0a) line_number++;
        textptr++;
        if( *textptr == 0 )
            return;
    }


    l = 0;
    while( isaltok(*(textptr+l)) )
    {
        tempbuf[l] = textptr[l];
        l++;
    }
    tempbuf[l] = 0;

    for(i=0;i<NUMKEYWORDS;i++)
        if( strcmp( label+(labelcnt<<6),keyw[i]) == 0 )
    {
        error++;
        printf("  * ERROR!(L%hd) Symbol '%s' is a key word.\n",line_number,label+(labelcnt<<6));
        textptr+=l;
    }


    for(i=0;i<labelcnt;i++)
    {
        if( strcmp((const char *)tempbuf,label+(i<<6)) == 0 )
        {
            *scriptptr = labelcode[i];
            scriptptr++;
            textptr += l;
            return;
        }
    }

    if( isdigit(*textptr) == 0 && *textptr != '-')
    {
        printf("  * ERROR!(L%hd) Parameter '%s' is undefined.\n",line_number,tempbuf);
        error++;
        textptr+=l;
        return;
    }

    *scriptptr = atol(textptr);
    scriptptr++;

    textptr += l;
}
void FixFilePath(char  *filename)
{
    // windows only!
}
uint8_t parsecommand(int readfromGRP)
{
    #ifdef WIN32
        static int maxActorIndex;
    #endif // WIN32
    int32_t i, j, k, *tempscrptr;
    uint8_t  done, temp_ifelse_check;
    int32_t tw;
    char *origtptr;
    short temp_line_number;
    FILE * fp;

    if( error > 12 || ( *textptr == '\0' ) || ( *(textptr+1) == '\0' ) ) return 1;

    tw = transword();

    switch(tw)
    {
        default:
        case -1:
            return 0; //End
        case 39:      //Rem endrem
            scriptptr--;
            j = line_number;
            do
            {
                textptr++;
                if(*textptr == 0x0a) line_number++;
                if( *textptr == 0 )
                {
                    printf("  * ERROR!(L%d) Found '/*' with no '*/'.\n",j);
                    error++;
                    return 0;
                }
            }
            while( *textptr != '*' || *(textptr+1) != '/' );
            textptr+=2;
            return 0;
        case 17:
            if( parsing_actor == 0 && parsing_state == 0 )
            {
                getlabel();
                scriptptr--;
                labelcode[labelcnt] = encodescriptptr(scriptptr);
                labelcnt++;

                parsing_state = 1;

                return 0;
            }

            getlabel();

            for(i=0;i<NUMKEYWORDS;i++)
                if( strcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                {
                    error++;
                    printf("  * ERROR!(L%hd) Symbol '%s' is a key word.\n",line_number,label+(labelcnt<<6));
                    return 0;
                }

            for(j=0;j<labelcnt;j++)
            {
                if( strcmp(label+(j<<6),label+(labelcnt<<6)) == 0 )
                {
                    *scriptptr = labelcode[j];
                    break;
                }
            }

            if(j==labelcnt)
            {
                printf("  * ERROR!(L%hd) State '%s' not found.\n",line_number,label+(labelcnt<<6));
                error++;
            }
            scriptptr++;
            return 0;

        case 15:
        case 92:
        case 87:
        case 89:
        case 93:
            transnum();
            return 0;

        case 18:
            if( parsing_state == 0 )
            {
                printf("  * ERROR!(L%hd) Found 'ends' with no 'state'.\n",line_number);
                error++;
            }
//            else
            {
                if( num_squigilly_brackets > 0 )
                {
                    printf("  * ERROR!(L%hd) Found more '{' than '}' before 'ends'.\n",line_number);
                    error++;
                }
                if( num_squigilly_brackets < 0 )
                {
                    printf("  * ERROR!(L%hd) Found more '}' than '{' before 'ends'.\n",line_number);
                    error++;
                }
                parsing_state = 0;
            }
            return 0;
        case 19:
            getlabel();
            // Check to see it's already defined

            for(i=0;i<NUMKEYWORDS;i++)
                if( strcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                {
                    error++;
                    printf("  * ERROR!(L%hd) Symbol '%s' is a key word.\n",line_number,label+(labelcnt<<6));
                    return 0;
                }

            for(i=0;i<labelcnt;i++)
            {
                if( strcmp(label+(labelcnt<<6),label+(i<<6)) == 0 )
                {
                    warning++;
                    printf("  * WARNING.(L%hd) Duplicate definition '%s' ignored.\n",line_number,label+(labelcnt<<6));
                    break;
                }
            }

            transnum();
            if(i == labelcnt)
                labelcode[labelcnt++] = *(scriptptr-1);
            scriptptr -= 2;
            return 0;
        case 14:

            for(j = 0;j < 4;j++)
            {
                if( keyword() == -1 )
                    transnum();
                else break;
            }

            while(j < 4)
            {
                *scriptptr = 0;
                scriptptr++;
                j++;
            }
            return 0;

        case 32:
            if( parsing_actor || parsing_state )
            {
                transnum();

                j = 0;
                while(keyword() == -1)
                {
                    transnum();
                    scriptptr--;
                    j |= *scriptptr;
                }
                *scriptptr = j;
                scriptptr++;
            }
            else
            {
                scriptptr--;
                getlabel();
                // Check to see it's already defined

                for(i=0;i<NUMKEYWORDS;i++)
                    if( strcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                {
                    error++;
                    printf("  * ERROR!(L%hd) Symbol '%s' is a key word.\n",line_number,label+(labelcnt<<6));
                    return 0;
                }

                for(i=0;i<labelcnt;i++)
                    if( strcmp(label+(labelcnt<<6),label+(i<<6)) == 0 )
                    {
                        warning++;
                        printf("  * WARNING.(L%hd) Duplicate move '%s' ignored.\n",line_number,label+(labelcnt<<6));
                        break;
                    }
                if(i == labelcnt)
                    labelcode[labelcnt++] = encodescriptptr(scriptptr);
                for(j=0;j<2;j++)
                {
                    if(keyword() >= 0) break;
                    transnum();
                }
                for(k=j;k<2;k++)
                {
                    *scriptptr = 0;
                    scriptptr++;
                }
            }
            return 0;

        case 54:
            {
                scriptptr--;
                transnum(); // Volume Number (0/4)
                scriptptr--;

                k = *scriptptr-1;

                if(k >= 0) // if it's background music
                {
                    i = 0;
                    while(keyword() == -1)
                    {
                        while( isaltok(*textptr) == 0 )
                        {
                            if(*textptr == 0x0a) line_number++;
                            textptr++;
                            if( *textptr == 0 ) break;
                        }
                        j = 0;
                        while( isaltok(*(textptr+j)) )
                        {
                            music_fn[k][i][j] = textptr[j];
                            j++;
                        }
                        music_fn[k][i][j] = '\0';
                        textptr += j;
                        if(i > 9) break;
                        i++;
                    }
                }
                else
                {
                    i = 0;
                    while(keyword() == -1)
                    {
                        while( isaltok(*textptr) == 0 )
                        {
                            if(*textptr == 0x0a) line_number++;
                            textptr++;
                            if( *textptr == 0 ) break;
                        }
                        j = 0;
                        while( isaltok(*(textptr+j)) )
                        {
                            env_music_fn[i][j] = textptr[j];
                            j++;
                        }
                        env_music_fn[i][j] = '\0';

                        textptr += j;
                        if(i > 9) break;
                        i++;
                    }
                }
            }
            return 0;
        case 55: // include other con files.
			{
				char  includedconfile[512];
				scriptptr--;
				while( isaltok(*textptr) == 0 )
				{
					if(*textptr == 0x0a) line_number++;
					textptr++;
					if( *textptr == 0 ) break;
				}
				j = 0;
				while( isaltok(*textptr) )
				{
					tempbuf[j] = *(textptr++);
					j++;
				}
				tempbuf[j] = '\0';

				// fix path for unix. (doesn't really matter...)
				FixFilePath((char *)tempbuf);

				sprintf(includedconfile, "%s", tempbuf);

				fp = TCkopen4load(includedconfile,readfromGRP);
				if(fp == NULL)
				{
					error++;
					printf("  * ERROR!(ln%hd) Could not find '%s'.\n",line_number,label+(labelcnt<<6));

					printf("ERROR: could not open (%s)\n", includedconfile);
					gameexit("");
					return 0;
				}

				j = kfilelength(fp);

				printf("Including: '%s'.\n", includedconfile);

				temp_line_number = line_number;
				line_number = 1;
				temp_ifelse_check = checking_ifelse;
				checking_ifelse = 0;
				origtptr = textptr;
				textptr = last_used_text+last_used_size;

				*(textptr+j) = 0;

				kread(fp,(uint8_t  *)textptr,j);
				kclose(fp);
//				ud.conCRC[0] = crc32_update((uint8_t  *)textptr, j, ud.conCRC[0]);

				do
					done = parsecommand(readfromGRP);
				while( done == 0 );

				textptr = origtptr;
				total_lines += line_number;
				line_number = temp_line_number;
				checking_ifelse = temp_ifelse_check;

				return 0;
			}
        case 24:
            if( parsing_actor || parsing_state )
                transnum();
            else
            {
                scriptptr--;
                getlabel();

                for(i=0;i<NUMKEYWORDS;i++)
                    if( strcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                    {
                        error++;
                        printf("  * ERROR!(L%hd) Symbol '%s' is a key word.\n",line_number,label+(labelcnt<<6));
                        return 0;
                    }

                for(i=0;i<labelcnt;i++)
                    if( strcmp(label+(labelcnt<<6),label+(i<<6)) == 0 )
                    {
                        warning++;
                        printf("  * WARNING.(L%hd) Duplicate ai '%s' ignored.\n",line_number,label+(labelcnt<<6));
                        break;
                    }

                if(i == labelcnt)
                    labelcode[labelcnt++] = encodescriptptr(scriptptr);

                for(j=0;j<3;j++)
                {
                    if(keyword() >= 0) break;
                    if(j == 2)
                    {
                        k = 0;
                        while(keyword() == -1)
                        {
                            transnum();
                            scriptptr--;
                            k |= *scriptptr;
                        }
                        *scriptptr = k;
                        scriptptr++;
                        return 0;
                    }
                    else transnum();
                }
                for(k=j;k<3;k++)
                {
                    *scriptptr = 0;
                    scriptptr++;
                }
            }
            return 0;

        case 7:
            if( parsing_actor || parsing_state )
                transnum();
            else
            {
                scriptptr--;
                getlabel();
                // Check to see it's already defined

                for(i=0;i<NUMKEYWORDS;i++)
                    if( strcmp( label+(labelcnt<<6),keyw[i]) == 0 )
                    {
                        error++;
                        printf("  * ERROR!(L%hd) Symbol '%s' is a key word.\n",line_number,label+(labelcnt<<6));
                        return 0;
                    }

                for(i=0;i<labelcnt;i++)
                    if( strcmp(label+(labelcnt<<6),label+(i<<6)) == 0 )
                    {
                        warning++;
                        printf("  * WARNING.(L%hd) Duplicate action '%s' ignored.\n",line_number,label+(labelcnt<<6));
                        break;
                    }

                if(i == labelcnt)
                    labelcode[labelcnt++] = encodescriptptr(scriptptr);

                for(j=0;j<5;j++)
                {
                    if(keyword() >= 0) break;
                    transnum();
                }
                for(k=j;k<5;k++)
                {
                    *scriptptr = 0;
                    scriptptr++;
                }
            }
            return 0;

        case 1:
            if( parsing_state )
            {
                printf("  * ERROR!(L%hd) Found 'actor' within 'state'.\n",line_number);
                error++;
            }

            if( parsing_actor )
            {
                printf("  * ERROR!(L%hd) Found 'actor' within 'actor'.\n",line_number);
                error++;
            }

            num_squigilly_brackets = 0;
            scriptptr--;
            parsing_actor = scriptptr;

            transnum();
            scriptptr--;
#if USE_ACTORSCRPTR
            actorscrptr[*scriptptr] = parsing_actor;
#else
            actorscridx[*scriptptr] = getscriptptridx(parsing_actor);
#endif
#ifdef WIN32
            if (*scriptptr > maxActorIndex)
            {
                maxActorIndex = *scriptptr;
                printf("Max Actor Index: %d\r\n", maxActorIndex);
            }
#endif // WIN32
            for(j=0;j<4;j++)
            {
                *(parsing_actor+j) = 0;
                if(j == 3)
                {
                    j = 0;
                    while(keyword() == -1)
                    {
                        transnum();
                        scriptptr--;
                        j |= *scriptptr;
                    }
                    *scriptptr = j;
                    scriptptr++;
                    break;
                }
                else
                {
                    if(keyword() >= 0)
                    {
                        scriptptr += (4-j);
                        break;
                    }
                    transnum();

                    *(parsing_actor+j) = *(scriptptr-1);
                }
            }

            checking_ifelse = 0;

            return 0;

        case 98:

            if( parsing_state )
            {
                printf("  * ERROR!(L%hd) Found 'useritem' within 'state'.\n",line_number);
                error++;
            }

            if( parsing_actor )
            {
                printf("  * ERROR!(L%hd) Found 'useritem' within 'actor'.\n",line_number);
                error++;
            }

            num_squigilly_brackets = 0;
            scriptptr--;
            parsing_actor = scriptptr;

            transnum();
            scriptptr--;
            j = *scriptptr;

            transnum();
            scriptptr--;
#if USE_ACTORSCRPTR
            actorscrptr[*scriptptr] = parsing_actor;
#else
            actorscridx[*scriptptr] = getscriptptridx(parsing_actor);
#endif
            actortype[*scriptptr] = j;
#ifdef WIN32
            if (*scriptptr > maxActorIndex)
            {
                maxActorIndex = *scriptptr;
                printf("Max Actor Index: %d\r\n", maxActorIndex);
            }
#endif // WIN32

            for(j=0;j<4;j++)
            {
                *(parsing_actor+j) = 0;
                if(j == 3)
                {
                    j = 0;
                    while(keyword() == -1)
                    {
                        transnum();
                        scriptptr--;
                        j |= *scriptptr;
                    }
                    *scriptptr = j;
                    scriptptr++;
                    break;
                }
                else
                {
                    if(keyword() >= 0)
                    {
                        scriptptr += (4-j);
                        break;
                    }
                    transnum();

                    *(parsing_actor+j) = *(scriptptr-1);
                }
            }

            checking_ifelse = 0;

            return 0;



        case 11:
        case 13:
        case 25:
        case 31:
        case 40:
        case 52:
        case 69:
        case 74:
        case 77:
        case 80:
        case 86:
        case 88:
        case 68:
        case 100:
        case 101:
        case 102:
        case 103:
        case 105:
        case 110:
            transnum();
            return 0;

        case 2:
        case 23:
        case 28:
        case 99:
        case 37:
        case 48:
        case 58:
            transnum();
            transnum();
            break;
        case 50:
            transnum();
            transnum();
            transnum();
            transnum();
            transnum();
            break;
        case 10:
            if( checking_ifelse )
            {
                checking_ifelse--;
                tempscrptr = scriptptr;
                scriptptr++; //Leave a spot for the fail location
                parsecommand(readfromGRP);
                *tempscrptr = encodescriptptr(scriptptr);
            }
            else
            {
                scriptptr--;
                error++;
                printf("  * ERROR!(L%hd) Found 'else' with no 'if'.\n",line_number);
            }

            return 0;

        case 75:
            transnum();
        case 3:
        case 8:
        case 9:
        case 21:
        case 33:
        case 34:
        case 35:
        case 41:
        case 46:
        case 53:
        case 56:
        case 59:
        case 62:
        case 72:
        case 73:
//        case 74:
        case 78:
        case 85:
        case 94:
        case 111:
            transnum();
        case 43:
        case 44:
        case 49:
        case 5:
        case 6:
        case 27:
        case 26:
        case 45:
        case 51:
        case 63:
        case 64:
        case 65:
        case 67:
        case 70:
        case 71:
        case 81:
        case 82:
        case 90:
        case 91:
        case 109:

            if(tw == 51)
            {
                j = 0;
                do
                {
                    transnum();
                    scriptptr--;
                    j |= *scriptptr;
                }
                while(keyword() == -1);
                *scriptptr = j;
                scriptptr++;
            }

            tempscrptr = scriptptr;
            scriptptr++; //Leave a spot for the fail location

            do
            {
                j = keyword();
                if(j == 20 || j == 39)
                    parsecommand(readfromGRP);
            } while(j == 20 || j == 39);

            parsecommand(readfromGRP);

            *tempscrptr = encodescriptptr(scriptptr);

            checking_ifelse++;
            return 0;
        case 29:
            num_squigilly_brackets++;
            do
                done = parsecommand(readfromGRP);
            while( done == 0 );
            return 0;
        case 30:
            num_squigilly_brackets--;
            if( num_squigilly_brackets < 0 )
            {
                printf("  * ERROR!(L%hd) Found more '}' than '{'.\n",line_number);
                error++;
            }
            return 1;
        case 76:
            scriptptr--;
            j = 0;
            while( *textptr != 0x0a )
            {
                #if USE_BETANAME
                    betaname[j] = *textptr;
                #endif
                j++; textptr++;
            }
            #if USE_BETANAME
                betaname[j] = 0;
            #endif
            return 0;
        case 20:
            scriptptr--; //Negate the rem
            while( *textptr != 0x0a )
                textptr++;

            // line_number++;
            return 0;

        case 107:
            scriptptr--;
            transnum();
            scriptptr--;
            j = *scriptptr;
            while( *textptr == ' ' ) textptr++;

            i = 0;

            while( *textptr != 0x0a )
            {
                volume_names[j][i] = toupper(*textptr);
                textptr++,i++;
                if(i >= 32)
                {
                    printf("  * ERROR!(L%hd) Volume name exceeds character size limit of 32.\n",line_number);
                    error++;
                    while( *textptr != 0x0a ) textptr++;
                    break;
                }
            }
#ifdef UNIX
            volume_names[j][i] = '\0';
#else
            volume_names[j][i-1] = '\0';
#endif
            return 0;
        case 108:
            scriptptr--;
            transnum();
            scriptptr--;
            j = *scriptptr;
            while( *textptr == ' ' ) textptr++;

            i = 0;

            while( *textptr != 0x0a )
            {
                skill_names[j][i] = toupper(*textptr);
                textptr++,i++;
                if(i >= 32)
                {
                    printf("  * ERROR!(L%hd) Skill name exceeds character size limit of 32.\n",line_number);
                    error++;
                    while( *textptr != 0x0a ) textptr++;
                    break;
                }
            }
#ifdef UNIX
            skill_names[j][i] = '\0';
#else
            skill_names[j][i-1] = '\0';
#endif
            return 0;

        case 0:
            scriptptr--;
            transnum();
            scriptptr--;
            j = *scriptptr;
            transnum();
            scriptptr--;
            k = *scriptptr;
            while( *textptr == ' ' ) textptr++;

            i = 0;
            while( *textptr != ' ' && *textptr != 0x0a )
            {
                level_file_names[j*11+k][i] = *textptr;
                textptr++,i++;
                if(i > 127)
                {
                    printf("  * ERROR!(L%hd) Level file name exceeds character size limit of 128.\n",line_number);
                    error++;
                    while( *textptr != ' ') textptr++;
                    break;
                }
            }
#ifdef UNIX
            level_names[j*11+k][i] = '\0';
#else
            level_names[j*11+k][i-1] = '\0';
#endif

            while( *textptr == ' ' ) textptr++;

            partime[j*11+k] =
                (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*26*60)+
                (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*26);

            textptr += 5;
            while( *textptr == ' ' ) textptr++;

            designertime[j*11+k] =
                (((*(textptr+0)-'0')*10+(*(textptr+1)-'0'))*26*60)+
                (((*(textptr+3)-'0')*10+(*(textptr+4)-'0'))*26);

            textptr += 5;
            while( *textptr == ' ' ) textptr++;

            i = 0;

            while( *textptr != 0x0a )
            {
                level_names[j*11+k][i] = toupper(*textptr);
                textptr++,i++;
                if(i >= 32)
                {
                    printf("  * ERROR!(L%hd) Level name exceeds character size limit of 32.\n",line_number);
                    error++;
                    while( *textptr != 0x0a ) textptr++;
                    break;
                }
            }
#ifdef UNIX
            level_names[j*11+k][i] = '\0';
#else
            level_names[j*11+k][i-1] = '\0';
#endif
            return 0;

        case 79:
            scriptptr--;
            transnum();
            k = *(scriptptr-1);
            if(k >= NUMOFFIRSTTIMEACTIVE)
            {
                printf("  * ERROR!(L%hd) Quote amount exceeds limit of %d characters.\n",line_number,NUMOFFIRSTTIMEACTIVE);
                error++;
            }
            scriptptr--;
            i = 0;
            while( *textptr == ' ' )
                textptr++;

            while( *textptr != 0x0a )
            {
                fta_quotes[k][i] = *textptr;
                textptr++,i++;
                if(i >= 64)
                {
                    printf("  * ERROR!(L%hd) Quote exceeds character size limit of 64.\n",line_number);
                    error++;
                    while( *textptr != 0x0a ) textptr++;
                    break;
                }
            }
            fta_quotes[k][i] = '\0';
            return 0;
        case 57:
            scriptptr--;
            transnum();
            k = *(scriptptr-1);
            if(k >= MAX_NUM_SOUNDS)
            {
                printf("  * ERROR!(L%hd) Exceeded sound limit of %d.\n",line_number,NUM_SOUNDS);
                error++;
            }
            scriptptr--;
            i = 0;
            while( *textptr == ' ')
                textptr++;

            while( *textptr != ' ' )
            {
                sounds[k][i] = *textptr;
                textptr++,i++;
                if(i >= 13)
                {
                    puts(sounds[k]);
                    printf("  * ERROR!(L%hd) Sound filename exceeded limit of 13 characters.\n",line_number);
                    error++;
                    while( *textptr != ' ' ) textptr++;
                    break;
                }
            }
            sounds[k][i] = '\0';

            transnum();
            soundps[k] = *(scriptptr-1);
            scriptptr--;
            transnum();
            soundpe[k] = *(scriptptr-1);
            scriptptr--;
            transnum();
            soundpr[k] = *(scriptptr-1);
            scriptptr--;
            transnum();
            soundm[k] = *(scriptptr-1);
            scriptptr--;
            transnum();
            soundvo[k] = *(scriptptr-1);
            if (k > maxSoundsFound)
            {
                maxSoundsFound = k;
            }
            scriptptr--;
            return 0;

        case 4:
            if( parsing_actor == 0 )
            {
                printf("  * ERROR!(L%hd) Found 'enda' without defining 'actor'.\n",line_number);
                error++;
            }
//            else
            {
                if( num_squigilly_brackets > 0 )
                {
                    printf("  * ERROR!(L%hd) Found more '{' than '}' before 'enda'.\n",line_number);
                    error++;
                }
                parsing_actor = 0;
            }

            return 0;
        case 12:
        case 16:
        case 84:
//        case 21:
        case 22:    //KILLIT
        case 36:
        case 38:
        case 42:
        case 47:
        case 61:
        case 66:
        case 83:
        case 95:
        case 96:
        case 97:
        case 104:
        case 106:
            return 0;
        case 60:
		{
			int32_t params[30];

			scriptptr--;
			for(j = 0; j < 30; j++)
        	{
				transnum();
				scriptptr--;
				params[j] = *scriptptr;

				if (j != 25) continue; // we try to guess if we are using 1.3/1.3d or 1.4/1.5 con files

				if (keyword() != -1) // Is the 26th variable set? If so then it's probably a 1.4/1.5 con file
				{
					break;
				}
				else
				{
					conVersion = 15;
				}
			}

			/* From Jonathon's code --mk
			v1.3d					v1.5

			DEFAULTVISIBILITY		DEFAULTVISIBILITY
			GENERICIMPACTDAMAGE		GENERICIMPACTDAMAGE
			MAXPLAYERHEALTH			MAXPLAYERHEALTH
			STARTARMORHEALTH		STARTARMORHEALTH
			RESPAWNACTORTIME		RESPAWNACTORTIME
			RESPAWNITEMTIME			RESPAWNITEMTIME
			RUNNINGSPEED			RUNNINGSPEED
									GRAVITATIONALCONSTANT
			RPGBLASTRADIUS			RPGBLASTRADIUS
			PIPEBOMBRADIUS			PIPEBOMBRADIUS
			SHRINKERBLASTRADIUS		SHRINKERBLASTRADIUS
			TRIPBOMBBLASTRADIUS		TRIPBOMBBLASTRADIUS
			MORTERBLASTRADIUS		MORTERBLASTRADIUS
			BOUNCEMINEBLASTRADIUS	BOUNCEMINEBLASTRADIUS
			SEENINEBLASTRADIUS		SEENINEBLASTRADIUS
			MAXPISTOLAMMO			MAXPISTOLAMMO
			MAXSHOTGUNAMMO			MAXSHOTGUNAMMO
			MAXCHAINGUNAMMO			MAXCHAINGUNAMMO
			MAXRPGAMMO				MAXRPGAMMO
			MAXHANDBOMBAMMO			MAXHANDBOMBAMMO
			MAXSHRINKERAMMO			MAXSHRINKERAMMO
			MAXDEVISTATORAMMO		MAXDEVISTATORAMMO
			MAXTRIPBOMBAMMO			MAXTRIPBOMBAMMO
			MAXFREEZEAMMO			MAXFREEZEAMMO
									MAXGROWAMMO
			CAMERASDESTRUCTABLE		CAMERASDESTRUCTABLE
			NUMFREEZEBOUNCES		NUMFREEZEBOUNCES
			FREEZERHURTOWNER		FREEZERHURTOWNER
									QSIZE
									TRIPBOMBLASERMODE
			*/

			// Used Jonathon Fowler's parser. Cool to make the code
			// robust to 1.3 con files --mk

			j = 0;

			ud_const_visibility		= params[j++];
			impact_damage			= params[j++];
			max_player_health		= params[j++];
			max_armour_amount		= params[j++];
			respawnactortime		= params[j++];
			respawnitemtime			= params[j++];
			dukefriction			= params[j++];
			if (conVersion == 15)
				gc					= params[j++];
			else
				gc					= 176; // default (guess) when using 1.3d CONs
			rpgblastradius			= params[j++];
			pipebombblastradius		= params[j++];
			shrinkerblastradius		= params[j++];
			tripbombblastradius		= params[j++];
			morterblastradius		= params[j++];
			bouncemineblastradius	= params[j++];
			seenineblastradius		= params[j++];
			max_ammo_amount[PISTOL_WEAPON]		= params[j++];
			max_ammo_amount[SHOTGUN_WEAPON]		= params[j++];
			max_ammo_amount[CHAINGUN_WEAPON]	= params[j++];
			max_ammo_amount[RPG_WEAPON]			= params[j++];
			max_ammo_amount[HANDBOMB_WEAPON]	= params[j++];
			max_ammo_amount[SHRINKER_WEAPON]	= params[j++];
			max_ammo_amount[DEVISTATOR_WEAPON]	= params[j++];
			max_ammo_amount[TRIPBOMB_WEAPON]	= params[j++];
			max_ammo_amount[FREEZE_WEAPON]		= params[j++];
			if (conVersion == 15)
				max_ammo_amount[GROW_WEAPON]	= params[j++];
			else
				max_ammo_amount[GROW_WEAPON]	= 50; // default (guess) when using 1.3d CONs
			camerashitable			= params[j++];
			numfreezebounces		= params[j++];
			freezerhurtowner		= params[j++];
			if (conVersion == 15)
			{
				spriteqamount		= params[j++];

				if(spriteqamount > 1024)
					spriteqamount	= 1024;
				else if(spriteqamount < 0)
					spriteqamount	= 0;
				lasermode			= params[j++];
			}
			else
			{
				// spriteqamount = 64 is the default
				lasermode = 0; // default (guess) when using 1.3d CONs
			}
		}
		return 0;

	} // end of switch(tw)

	return 0;
}

void passone(int readfromGRP)
{

    while( parsecommand(readfromGRP) == 0 );

    if( (error+warning) > 12)
        puts(  "  * ERROR! Too many warnings or errors.");

}
