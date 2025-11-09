#ifndef _CVARS_H_
#define _CVARS_H_

#if USE_OLD_VARIABLE
#define REGCONVAR(varname, varhelp, variable, function) CVAR_RegisterCvar(varname, varhelp, &variable, &function)
#define REGCONFUNC(varname, varhelp, function) CVAR_RegisterCvar(varname, varhelp, NULL, &function)
#else
#define CVAR_RegisterCvarMacro(varname, varhelp, variableaddr, func) {.name = varname, .help = varhelp, .variable = variableaddr,  .function = func}
#define REGCONVAR(varname, varhelp, variable, function) CVAR_RegisterCvarMacro(varname, varhelp, &variable, (function_t) &function)
#define REGCONFUNC(varname, varhelp, function) CVAR_RegisterCvarMacro(varname, varhelp, NULL, (function_t) &function)

#endif
#include <inttypes.h>

typedef void (*function_t) (const void* binding);

typedef struct t_cvar_binding
{
  char           name[64];
  char           help[64];
  void*         variable;
  function_t	function;
} cvar_binding;


void            CVAR_RegisterCvar(const char * varname, const char * varhelp, void* variable, function_t function);
int             CVAR_GetNumCvarBindings();
const cvar_binding*   CVAR_GetCvarBinding(int nBinding);
void            CVAR_Render();

#endif
