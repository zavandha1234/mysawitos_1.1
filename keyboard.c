/* keyboard.c - PS/2 keyboard scancode set 1 (polling), Shift + CapsLock */
#include "mysawit.h"
static const char map[58]={0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']',10,0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '};
static const char smap[58]={0,27,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}',10,0,'A','S','D','F','G','H','J','K','L',':','"','~',0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '};
static int q[32],qh,qt,shift,caps;
void kb_feed(u8 sc){
  if(sc==0x2A||sc==0x36){ shift=1; return; } if(sc==0xAA||sc==0xB6){ shift=0; return; }
  if(sc==0x3A){ caps^=1; return; }
  if(sc&0x80||sc>=58) return;
  char c=map[sc]; if(!c) return;
  if(c>='a'&&c<='z'){ if(shift^caps) c-=32; } else if(shift) c=smap[sc];
  q[qt]=c; qt=(qt+1)&31;
}
int kb_getkey(void){ if(qh==qt) return 0; int c=q[qh]; qh=(qh+1)&31; return c; }
