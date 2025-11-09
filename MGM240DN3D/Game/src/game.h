#ifndef _GAME_H_
#define _GAME_H_

//extern uint8_t  game_dir[512];
char * getgamedir();
int gametextpal(int x,int y,const char  *t,uint8_t  s,uint8_t  p);
int dn3dmain(int argc,char  **argv);
char *get_fta_quotes(int q);
#endif  // include-once header.

