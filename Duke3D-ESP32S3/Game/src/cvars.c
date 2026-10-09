#include "cvars.h"
#include "duke3d.h" // added to get extern variables
#include "cvar_defs.h"
#include <stdlib.h>

#include <string.h>

#define MAX_CVARS 32

#if USE_OLD_CVAR
cvar_binding cvar_binding_list[MAX_CVARS];
int num_cvar_bindings = 0;

#else
const cvar_binding cvar_binding_list[] =
{
    REGCONVAR("SetConsoleColor", " - Change console color.",g_CV_console_text_color, CVARDEFS_DefaultFunction),
    REGCONVAR("NumConsoleLines", " - Number of visible console lines.", g_CV_num_console_lines, CVARDEFS_DefaultFunction),
    REGCONVAR("Classic", " - Enable Classic Mode.", g_CV_classic, CVARDEFS_DefaultFunction),
    REGCONVAR("EnableCubic", " - Turn on/off Cubic Interpolation for VOCs.", g_CV_CubicInterpolation, CVARDEFS_DefaultFunction),
    REGCONVAR("TransConsole", " - Toggle the transparency of the console", g_CV_TransConsole, CVARDEFS_DefaultFunction),
    REGCONVAR("DebugJoystick", " - Displays info on the active Joystick", g_CV_DebugJoystick, CVARDEFS_DefaultFunction),
    REGCONVAR("DebugSound", " - Displays info on the active Sounds", g_CV_DebugSound, CVARDEFS_DefaultFunction),
    REGCONVAR("DebugFileAccess", " - Displays info on file access", g_CV_DebugFileAccess, CVARDEFS_DefaultFunction),
    REGCONVAR("TickRate", " - Changes the tick rate", g_iTickRate, CVARDEFS_DefaultFunction),
    REGCONVAR("TicksPerFrame", " - Changes the ticks per frame", g_iTicksPerFrame, CVARDEFS_DefaultFunction),
    REGCONFUNC("Quit", " - Quit game.", CVARDEFS_FunctionQuit),
    REGCONFUNC("Clear", " - Clear the console.", CVARDEFS_FunctionClear),
	REGCONFUNC("Name", " - Change player name.", CVARDEFS_FunctionName),
    REGCONFUNC("Level", " - Change level. Args: Level <episode> <mission>", CVARDEFS_FunctionLevel),
    REGCONFUNC("PlayMidi"," - Plays a MIDI file", CVARDEFS_FunctionPlayMidi),
    REGCONFUNC("Help"," - Print out help commands for console", CVARDEFS_FunctionHelp)
};
#define num_cvar_bindings ((int)(sizeof(cvar_binding_list) / sizeof(cvar_binding_list[0])))
#endif
#if WIN32
void CVAR_RegisterCvar(const char * varname, const char * varhelp, void* variable, function_t function)
{
#if USE_OLD_CVAR
    if(NULL == function)
    {
        return;
    }

    cvar_binding_list[num_cvar_bindings].variable = variable;
    cvar_binding_list[num_cvar_bindings].function = function;
    memset(cvar_binding_list[num_cvar_bindings].name, 0, 64);
    strncpy(cvar_binding_list[num_cvar_bindings].name, varname, 63);
	memset(cvar_binding_list[num_cvar_bindings].help, 0, 64);
    strncpy(cvar_binding_list[num_cvar_bindings].help, varhelp, 63);
    num_cvar_bindings++;
#else
    FIXME("attempting to register a Cvar!");
#endif
}
#endif
int CVAR_GetNumCvarBindings()
{
    return num_cvar_bindings;
}

const cvar_binding* CVAR_GetCvarBinding(int nBinding)
{
    if(nBinding > num_cvar_bindings -1)
    {
        return NULL;
    }

    return &cvar_binding_list[nBinding];
}

// Bind all standard CVars here
void CVAR_RegisterDefaultCvarBindings(void)
{
    CVARDEFS_Init();
}

void CVAR_Render(void)
{
    CVARDEFS_Render();
}

