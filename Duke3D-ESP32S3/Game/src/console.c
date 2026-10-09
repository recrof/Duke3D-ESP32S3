#include <stdarg.h>
#ifdef WIN32
#include <stdio.h>
#else
#include <printf.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "duke3d.h"
#include "console.h"
#include "control.h"
#include "cvars.h"
#include "cvar_defs.h"
#include "engine.h"
#include "names.h"
#include "function.h"
#include <strings.h>
// next-hack this shall be rewritten to support few bytes heap.
#define MAX_STRING_LENGTH 32
#define MAX_CONSOLE_STRING_LENGTH       60 // was 70
#define MAX_CONSOLE_ARGV_STRING_LENGTH  24 // was as MAX_CONSOLE_STRING_LENGTH
#define MAX_CVAR_ARGS 4 // was 10
#define MAX_CONSOLE_VIEWABLE 10

#define BUILD_NUMBER "Build 19"
#define DYNAMIC_CONSOLE 1
#define PRE_WRITTEN_CONSOLE_CHEATS 1

static const char* const cheatCommands[] =
{
    "",
    "DNKROZ",
    "DNITEMS",
    "DNWEAPONS",
    "DNKEYS",
    "DNSTUFF",
    "DNUNLOCK",
    "DNSHOWMAP",
    "DNCASHMAN",
//    "DNCLIP"   // removing this one as it is prone to crashes.
};
static uint8_t cheatPos = 0;
// TODO: remove this.
typedef struct console_element
{
#if DYNAMIC_CONSOLE
    void* prev;
    void* next;
#endif
    char  text[];   // next-hack: variable length element
}CONSOLEELEMENT;
// we can additionally decide to show at most n-lines despite if we have enough heap.
#if FREE_IF_TOO_MANY_LINES
int numLines = 0;
#endif
// Private member functions
void CONSOLE_InsertUsedCommand(char * szUsedCommand);   // next-hack: removed const, because we want to make sure it's null terminated.
void CONSOLE_ClearUsedCommandList();
void CONSOLE_RecalculateDirtyBuffer();

// console argument tracker
int argc;
#define STACK_ARGV 1
#if STACK_ARGV
char  (*argv) [MAX_CVAR_ARGS][MAX_CONSOLE_ARGV_STRING_LENGTH];
#else
char  argv[MAX_CVAR_ARGS][MAX_CONSOLE_ARGV_STRING_LENGTH];
#endif // STACK_ARGV
// Console entries, prepending linked list
CONSOLEELEMENT *console_buffer = NULL;
// Current viewed section of the console
CONSOLEELEMENT *p_console_current_view = NULL;
// Previously entered commands:
CONSOLEELEMENT *console_used_command_list = NULL;
// Previously entered commands:
CONSOLEELEMENT *console_used_command_list_current = NULL;

// dirty buffer
char  dirty_buffer[MAX_CONSOLE_STRING_LENGTH];

// dirty buffer control vars
int console_cursor_pos = 0; //without spaces
int console_num_spaces = 0; //number of spaces

// Toggle for the console state
int nConsole_Active = 0;

void CVAR_RegisterDefaultCvarBindings(void);
// Initialize the console
void CONSOLE_Init()
{
    CONSOLE_Reset();

    // Console Variable init
    CVAR_RegisterDefaultCvarBindings();

    // read in our startup.txt
    CONSOLE_ParseStartupScript();

    CONSOLE_Printf("Type \'HELP\' for help with console Commands.");
}

void CONSOLE_Reset()
{
    CONSOLEELEMENT *pElement;
    CONSOLEELEMENT *pDelElement;

    // Free the list if it exists
    pElement = console_buffer;
    while(pElement)
    {
        pDelElement = pElement;
        pElement = (CONSOLEELEMENT*)pElement->next;

        free(pDelElement);
    }
#if FREE_IF_TOO_MANY_LINES
    numLines = 0;
#endif
    console_buffer = NULL;
    p_console_current_view = NULL;
}

void CONSOLE_Term()
{
    // Do any clean up releasing to the console
    CONSOLE_Reset();
    CONSOLE_ClearUsedCommandList();
}

void CONSOLE_ParseStartupScript()
{
#if WIN32
    // FIX_00017: heavy autoexec.cfg not needed anymore.
    char  *sStartupScript = "startup.cfg";

    FILE* fp = fopen(sStartupScript, "r");

    // If the file exists
    if(NULL != fp)
    {
        char  line[128];
        memset(line, 0, 128);

        while(fgets(line ,128-1, fp) != NULL)
        {
            CONSOLE_ParseCommand(line);

            memset(line, 0, 128);
        }
        fclose(fp);
    }
#endif
}

void CONSOLE_HandleInput()
{
    char * lastKey;
    int tmp;

    if(g_CV_classic)
    {
        nConsole_Active = 0;
        return;
    }

    // See if we should toggle the console
    if(ACTION(gamefunc_Console))
    {
        nConsole_Active = !nConsole_Active;

        CONTROL_ClearAction(gamefunc_Console);
        //KB_ClearKeyDown(sc_Tilde);


        if(nConsole_Active)
        {
            // If this is a singleplayer game, let's pause the game when the console is active
            if (numplayers == 1)
            {
                if (!ud.recstat) //check if we are not playing a Demo and console is active
                {
                    ud.pause_on = 1;
                }else
                {
                    ud.pause_on = 0;
                }
            }
        }
        else
        // Bug fix: make sure the frag bar displays after console
        // is hidden
        //if(!nConsole_Active)
        {
            if ( ud.multimode > 1 && ud.coop != 1 )
            {
                displayfragbar();
            }
            if(numplayers<2)
                ud.pause_on = 0;
            // FIX_00078: Out Of Synch error (multiplayer) when using console in pause mode
        }

        return;
    }

    // If the console is not active, there's no need to process input
    if(!nConsole_Active)
    {
        return;
    }

    switch(KB_GetLastScanCode())
    {
    case sc_Space:
        {
            if(strlen(dirty_buffer) < MAX_CONSOLE_STRING_LENGTH-2)
            {
                strncat(dirty_buffer, " ", 2);
                console_num_spaces++;
            }
        }
        break;
    case sc_Delete:
    case sc_BackSpace:
        {
            tmp = strlen(dirty_buffer);
            if(tmp > 0)
            {
                if( dirty_buffer[tmp - 1] == ' ' )
                {
                    dirty_buffer[tmp - 1] = '\0';
                    console_num_spaces--;
                }
                else
                {
                    dirty_buffer[tmp - 1] = '\0';
                    console_cursor_pos--;
                }
            }
        }
        break;
        //Ignore list
    case sc_LeftShift:
    case sc_RightShift:
    case sc_PrintScreen:
    case sc_Tab:
    case sc_NumLock:
    case sc_LeftAlt:
    case sc_LeftControl:
    case sc_CapsLock:
    case sc_Bad:
    case sc_LeftArrow:
    case sc_RightArrow:
    case sc_Insert:
    case sc_Home: //this should take us to the top of the list
    case sc_RightAlt:
    case sc_RightControl:
    case sc_Tilde: //ignore
        {
        }break;
    case sc_kpad_Enter:
    case sc_Enter:
        {
            //If console_buffer[0] strlen() != 0
            //1. Push the dirty_buffer unto the console_buffer
            //2. parse the text

            CONSOLE_Printf("%s", dirty_buffer);
            console_cursor_pos = 0;
            console_num_spaces = 0;
            #if !MG24
            // we don't have enough memory for history. We could share the same element as printf though.
            CONSOLE_InsertUsedCommand(dirty_buffer);
            #endif
            dirty_buffer[sizeof(dirty_buffer) - 1] = 0;
            // check for cheats.
            if(dirty_buffer[0]  == 'D' && dirty_buffer[1] == 'N')
            {
                cheats(true, &dirty_buffer[2]);
            }
            else
            {
            CONSOLE_ParseCommand(dirty_buffer);
            }
            memset(dirty_buffer, 0, MAX_CONSOLE_STRING_LENGTH);

        }break;
    case sc_UpArrow:
        {
            /*
            if(p_console_current_view->next != NULL)
            {
                p_console_current_view = p_console_current_view->next;
            }
            */
#if !PRE_WRITTEN_CONSOLE_CHEATS
            if(console_used_command_list_current == NULL)
            {
                if(NULL == console_used_command_list)
                {
                    break;
                }
                console_used_command_list_current = console_used_command_list;
                sprintf(dirty_buffer, "%s", console_used_command_list_current->text);
                CONSOLE_RecalculateDirtyBuffer();
                break;
            }

            if(console_used_command_list_current->next != NULL)
            {
                console_used_command_list_current = console_used_command_list_current->next;
                sprintf(dirty_buffer, "%s", console_used_command_list_current->text);
                CONSOLE_RecalculateDirtyBuffer();
            }else
            if(console_used_command_list_current != NULL)
            {
                sprintf(dirty_buffer, "%s", console_used_command_list_current->text);
                CONSOLE_RecalculateDirtyBuffer();
            }
#else
    cheatPos++;
    if (cheatPos >= sizeof(cheatCommands)/sizeof(cheatCommands[0]))
    {
        cheatPos = 0;
    }
    sprintf(dirty_buffer, "%s", cheatCommands[cheatPos]);
    CONSOLE_RecalculateDirtyBuffer();
#endif

        }break;
    case sc_PgUp:
        {
            int i;
            for(i = 0; i < MAX_CONSOLE_VIEWABLE; i++)
            {
                if(p_console_current_view->next != NULL)
                {
                    p_console_current_view = p_console_current_view->next;
                }
                else
                {
                    break;
                }
            }
        }break;
    case sc_DownArrow:
        {
            /*
            if(p_console_current_view->prev != NULL)
            {
                p_console_current_view = p_console_current_view->prev;
            }
            */
#if !PRE_WRITTEN_CONSOLE_CHEATS

            if(console_used_command_list_current != NULL)
            {
                if(console_used_command_list_current->prev != NULL)
                {
                    console_used_command_list_current = console_used_command_list_current->prev;
                    sprintf(dirty_buffer, "%s", console_used_command_list_current->text);
                    CONSOLE_RecalculateDirtyBuffer();
                }
            }
#else
    if (cheatPos == 0)
    {
        cheatPos = sizeof(cheatCommands)/sizeof(cheatCommands[0]) - 1;
    }
    else
    {
        cheatPos--;
    }
    sprintf(dirty_buffer, "%s", cheatCommands[cheatPos]);
    CONSOLE_RecalculateDirtyBuffer();

#endif
        }break;
    case sc_PgDn:
        {
            int i;
            for(i = 0; i < MAX_CONSOLE_VIEWABLE; i++)
            {
                if(p_console_current_view->prev != NULL)
                {
                    p_console_current_view = p_console_current_view->prev;
                }
                else
                {
                    break;
                }
            }
        }break;
    case sc_End:
        {
            p_console_current_view = console_buffer;
        }break;
    case sc_Escape:
        {
            nConsole_Active = 0;
            KB_ClearKeyDown(sc_Escape);
            // FIX_00057: Using ESC to get out of the console mode wouldn't take pause mode off
            if(numplayers<2)
                ud.pause_on = 0;
            // FIX_00078: Out Of Synch error (multiplayer) when using console in pause mode
        }break;
    default:
        {
            if(strlen(dirty_buffer) < MAX_CONSOLE_STRING_LENGTH-2)
            {
                lastKey = KB_ScanCodeToString(KB_GetLastScanCode());

                if(lastKey)
                {
                    strncat(dirty_buffer, lastKey, 1);
                    console_cursor_pos++;
                    //printf("Key %s : %s\n", lastKey, console_buffer[0]);
                }
            }

        }
        break;
    }


    KB_ClearLastScanCode();
}

void CONSOLE_Render()
{
    if(g_CV_classic)
    {
        return;
    }
    // Let the Cvars draw something if they need to.
    CVAR_Render();
    uint8_t tempStack[MAX_RAM_FLAT_SIZE];
    setTempRamTileData(tempStack);
    if(nConsole_Active)
    {
       clearBufferedTileImagePtr();
        int x,y, y1, y2;
        int i, iCurHeight = 0, iCurWidth = 0;
        int iYOffset = 3; //offset for the console text
        CONSOLEELEMENT *pElement;

        y1 = 0;
        y2 = (ydim / 2) - ((ydim / 2)/12);

        // Draw console background
        for(y=y1;y<y2;y+=128)
        {
            for(x=0;x<xdim;x+=128)
            {
                rotatesprite(x<<16,y<<16,65536L,0,BIGHOLE,8,0,(g_CV_TransConsole ? 1:0)+8+16+64+128,0,y1,xdim-1,y2-1);
            }
        }

        // Draw bottom egde of console
        rotatesprite(78<<16,94<<16,65536L,512,WINDOWBORDER1,24,0,2+8,0,0,xdim-1,ydim-1);
        rotatesprite(238<<16,94<<16,65536L,512,WINDOWBORDER1,24,0,2+8,0,0,xdim-1,ydim-1);

        // Draw the contents of the console buffer
        pElement = p_console_current_view;//console_buffer;
        for(i = 0; i < MAX_CONSOLE_VIEWABLE; i++)
        {
            if(!pElement)
            {
                break;
            }

            minitext(5,(8*(9-i)) + iYOffset,
                     pElement->text,
                     g_CV_console_text_color,
                     10+16);

             pElement = (CONSOLEELEMENT*)pElement->next;

        }

        // Set the height of the new line
        iCurHeight = (8*10) + iYOffset;

        // Draw dirty buffer
        minitext(5, iCurHeight, dirty_buffer, 0,10+16);

        // Calculate the location of the cursor
        iCurWidth =  (console_cursor_pos*4) + (console_num_spaces*5) +5;

        //Draw the version number
        minitext(283, iCurHeight, BUILD_NUMBER, 17,10+16);

        // Draw the cursor //Change the color every second
#if WIN32
        int seconds = time(NULL);
#else
        int seconds = (I_GetTimeMicrosecs() >> 20); // more or less
#endif
        minitext(iCurWidth, iCurHeight,"_",(seconds%2)+1,10+16);
    }
    else
    if(g_CV_num_console_lines > 0)
    {
        int i, iYOffset = 3; //offset for the console text
        CONSOLEELEMENT *pElement;
        // Draw the contents of the console buffer
        pElement = console_buffer;
        for(i = 0; i < g_CV_num_console_lines; i++)
        {
            if(!pElement)
            {
                break;
            }

            minitext(5,(8*((g_CV_num_console_lines-1)-i)) + iYOffset,
                     pElement->text,
                     g_CV_console_text_color,
                     10+16);

             pElement = (CONSOLEELEMENT*)pElement->next;
        }
    }
}
char *strtok_noheap(char *str, const char *delim)
{
    static char *last;
    if (str == NULL)
    {
        str = last;
    }
    if (str == NULL)
    {
        return NULL;
    }

    // Skip leading delimiters
    str += strspn(str, delim);
    if (*str == '\0')
    {
        last = NULL;
        return NULL;
    }

    // Find the end of the token
    char *end = str + strcspn(str, delim);
    if (*end == '\0')
    {
        last = NULL;
    } else
    {
        *end = '\0';
        last = end + 1;
    }
    return str;
}

void CONSOLE_ParseCommand(char * command)
{
    char  *cvar;
    char  *token;
    int i, numCvars;
    argc = 0;
#if STACK_ARGV
    char  stackargv[MAX_CVAR_ARGS][MAX_CONSOLE_ARGV_STRING_LENGTH];
    argv = &stackargv;
#endif
    if( strlen(command) < 1)
    {
        #if STACK_ARGV
            argv = NULL;    // to prevent compiler warning
        #endif // STACK_ARGV
        return;
    }

    // Split the command into cvar and args
    cvar = strtok_noheap(command, " \r\n");
    //
    if (cvar == NULL)
    {
        #if STACK_ARGV
            argv = NULL;    // to prevent compiler warning
        #endif // STACK_ARGV
        return;
    }
    for(i = 0; i < MAX_CVAR_ARGS; i++)
    {
        token = strtok_noheap(NULL, " ");

        if(!token)
        {
            break;
        }
#if STACK_ARGV
        sprintf((*argv)[i], "%s", token);
#else
        sprintf(argv[i], "%s", token);
#endif
        argc++;
    }

    // Cycle through our cvar list and look for this keyword
    numCvars = CVAR_GetNumCvarBindings();
    for(i = 0; i < numCvars; i++)
    {
        const cvar_binding* binding = CVAR_GetCvarBinding(i);

        // Did we find it?
        if ( strcasecmp(cvar, binding->name) == 0 )
        {
            binding->function(binding);
            break;
        }
    }
    #if STACK_ARGV
        argv = NULL;    // to prevent compiler warning
    #endif // STACK_ARGV
}


void CONSOLE_InsertUsedCommand(char * szUsedCommand)
{
    //create a new element in the list, and add it to the front
    szUsedCommand[MAX_CONSOLE_STRING_LENGTH - 1] = 0;   // next-hack: make sure it's null terminated.
    int n = strlen(szUsedCommand);
    CONSOLEELEMENT *pElement = (CONSOLEELEMENT*)malloc(sizeof(CONSOLEELEMENT) + n + 1);
    if(pElement)
    {
        //Store our newly created member as the prev address
        if(NULL != console_used_command_list)
        {
            console_used_command_list->prev = pElement;
        }
        // Set the next pointer to the front of the list
        pElement->next = console_used_command_list;

        console_used_command_list_current = NULL;//pElement;

        // Prepend the entry. This entry is now the head of the list.
        console_used_command_list = pElement;

        // Make sure we NULL out the prev for our top level element
        pElement->prev = NULL;

        //sprintf(console_buffer->text, "%s", msg);
        memset(console_used_command_list->text, 0, n + 1);
        strncpy(console_used_command_list->text, szUsedCommand, n + 1);
    }
}

void CONSOLE_ClearUsedCommandList()
{
    CONSOLEELEMENT *pElement;
    CONSOLEELEMENT *pDelElement;

    // Free the list if it exists
    pElement = console_used_command_list;
    while(pElement)
    {
        pDelElement = pElement;
        pElement = (CONSOLEELEMENT*)pElement->next;

        free(pDelElement);
    }

    console_used_command_list = NULL;
    console_used_command_list_current = NULL;
}


void CONSOLE_RecalculateDirtyBuffer()
{
    int len;
    int l;

    console_cursor_pos = 0; //without spaces
    console_num_spaces = 0; //number of spac

    len = strlen(dirty_buffer);

    for(l = 0; l < len; ++l)
    {
        if(dirty_buffer[l] == ' ')
        {
            ++console_num_spaces;
        }
        else
        {
            ++console_cursor_pos;
        }
    }
}


void CONSOLE_Printf(const char  *newmsg, ...)
{
    CONSOLEELEMENT *pElement;
    va_list     argptr;
    char        msg[MAX_CONSOLE_STRING_LENGTH];//[MAX_CONSOLE_STRING_LENGTH];
    va_start (argptr,newmsg);
    int n = vsnprintf (msg, MAX_CONSOLE_STRING_LENGTH, newmsg, argptr);
    va_end (argptr);
    if (n <= 0)
        return;
    // allocate memory. Free if not enough.
    do
    {
        pElement = (CONSOLEELEMENT*)malloc(sizeof(CONSOLEELEMENT) + n + 1);
        // did we get a buffer?
        if (!pElement)
        {
            // no. Was there any line printed ?
            if (console_buffer == NULL)
                return; // no, simply, there is no mem. We can't do anything
            // if where is some line printed, walk down to the first line and remove it
            CONSOLEELEMENT *ptElement = console_buffer;
            // the first line is the one whose next is NULL
            while (ptElement->next != NULL)
            {
                // get next line.
                ptElement = (CONSOLEELEMENT *) ptElement->next;
            }
            // note: this might be the first line, so it might not have prev.
            if (ptElement->prev)
            {
                ((CONSOLEELEMENT*)ptElement->prev)->next = NULL;    // pElement->prev shall always be non null.
            }
            else
            {
                // if it does not have prev, this is the first line, so it is console buffer, which need to be
                // nulled.
                console_buffer = NULL;
            }
            free(ptElement);
#if FREE_IF_TOO_MANY_LINES
            if (numLines > 0)
                numLines --;
#endif
        }
    } while(pElement == NULL);
    //create a new element in the list, and add it to the front
    // next-hack removed check for NULL, as we either exit the function or the memory has been allocated.
    // if(pElement)
    {
        //Store our newly created member as the prev address
        if(NULL != console_buffer)
        {
            console_buffer->prev = pElement;
        }
        // Set the next pointer to the front of the list
        pElement->next = console_buffer;

        // Set our view, if they are at the bottom of the list.
        // Otherwise, if we set it everytime, Folks might lose
        // what they were looking for in the output, if they
        // were using pgup and pgdn to scroll through the entries.
        if(p_console_current_view == console_buffer)
        {
            p_console_current_view = pElement;
        }

        // Prepend the entry. This entry is now the head of the list.
        console_buffer = pElement;

        // Make sure we NULL out the prev for our top level element
        pElement->prev = NULL;

        //sprintf(console_buffer->text, "%s", msg);
        memset(console_buffer->text, 0, n + 1);
        strncpy(console_buffer->text, msg, n + 1);
#if FREE_IF_TOO_MANY_LINES
        numLines++;
        if (numLines > MAX_CONSOLE_VIEWABLE)
        {
            numLines = MAX_CONSOLE_VIEWABLE;
            while (pElement->next != NULL)
            {
                pElement = pElement->next;
            }
//            printf("too many lines, freeing\r\n");
            ((CONSOLEELEMENT*)pElement->prev)->next = NULL;    // pElement->prev shall always be non null.
            free(pElement);
        }
#endif
    }

}

// Get the current number of args for this keyword
int CONSOLE_GetArgc()
{
    return argc;
}

// Get the current list of args for this keyword
char * CONSOLE_GetArgv(unsigned int var)
{
#if STACK_ARGV
    return (*argv)[var];
#else
    return argv[var];
#endif
}

// Is our console showing?
int CONSOLE_IsActive()
{

    return nConsole_Active;
}

// Set our consople active or not.
void CONSOLE_SetActive(int i)
{
    nConsole_Active = (i == 0) ? 0 : 1;
}
