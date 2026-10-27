/* desktop.c - My Sawit OS v1.1: desktop gaya Windows 10
   Taskbar + search, Start Menu + tiles, File Explorer, Notepad, Calculator, Paint, Settings, Sawit Farm */
#include "mysawit.h"
#include "cursor.h"
int lang=0;

#define TASK_H 32
#define TASK_Y (SCREEN_H-TASK_H)
#define TITLE_H 28
#define BTN_W 38
#define C_TASK 0x14181E
#define C_WHITE 0xFFFFFF
#define MAXW 10
enum { W_EXPLORER,W_FARM,W_MSG,W_ABOUT,W_NOTEPAD,W_CALC,W_PAINT,W_SETTINGS };
typedef struct {
  int id,type,x,y,w,h,min,max,px,py,pw,ph;
  char title[40],path[64],text[96];
  int sel,nent; FsEntry ents[24];
  int farm_n,farm_rp;
  char buf[800]; int len;                       /* notepad */
  int acc,cur,op,fresh,shown,err;               /* calc */
  int pcolor,pbrush;                            /* paint */
} Win;
static Win wins[MAXW]; static int nwin,next_id=1;
static int dirty=1,start_open=0,start_hover=-1,sel_icon=-1;
static int drag=0,drag_dx,drag_dy,drag_right,pl_x,pl_y;   /* 1 move,2 resize-L,3 resize-R,4 paint */
static u32 last_click_t; static int last_click_id=-1;
static u8 exebuf[256*1024];
static u32 accent=0x0078D4;
static u32 wall[SCREEN_W*SCREEN_H];
#define PW 340
#define PH 220
static u8 paint_cv[PW*PH];
static const u32 ppal[12]={0x000000,0xFFFFFF,0xE81123,0xF7630C,0xFFF100,0x16C60C,0x00B7C3,0x0078D4,0x886CE4,0xE3008C,0x8B5A2B,0x7A7574};
static const u32 accents[7]={0x0078D4,0x2E9B3A,0xE8710A,0x7B3FBF,0xC42B1C,0x008B8B,0x3A3F47};

/* ---------------- util gambar ---------------- */
static int inr(int x,int y,int rx,int ry,int rw,int rh){ return x>=rx&&y>=ry&&x<rx+rw&&y<ry+rh; }
static u32 mix(u32 a,u32 b,int t){
  u32 r=(((a>>16)&255)*(255-t)+((b>>16)&255)*t)/255, g=(((a>>8)&255)*(255-t)+((b>>8)&255)*t)/255, bl=((a&255)*(255-t)+(b&255)*t)/255;
  return (r<<16)|(g<<8)|bl;
}
static void button(int x,int y,int w,int h,const char*l,int hover){
  fill_rect(x,y,w,h,hover?mix(accent,0xFFFFFF,200):0xE1E1E1); draw_rect(x,y,w,h,hover?accent:0xADADAD);
  draw_text(x+(w-text_width(l,1))/2,y+(h-8)/2,l,0x000000,1);
}
static void icon_folder(int x,int y){ fill_rect(x,y+1,6,3,0xE8B53A); fill_rect(x,y+3,14,10,0xF4C84B); fill_rect(x,y+5,14,1,0xFBE08A); }
static void icon_file(int x,int y,u32 c){ fill_rect(x+1,y,11,14,0xFFFFFF); draw_rect(x+1,y,11,14,0x707070); fill_rect(x+1,y+3,11,4,c); }
static void ring(int cx,int cy,int r,u32 c,int gap){
  for(int dy=-r;dy<=r;dy++) for(int dx=-r;dx<=r;dx++){ int d=dx*dx+dy*dy; if(d<=r*r&&d>(r-1)*(r-1)){ if(gap==1&&dx>0&&dy<=0&&dy>-r/2) continue; if(gap==2&&dy<0&&dx>=-1&&dx<=1) continue; put_pixel(cx+dx,cy+dy,c); } }
}
/* ikon aplikasi (grid 16, diskala size/16). act: 0 explorer,1 farm,2 install,3 about,6 notepad,7 calc,8 paint,9 settings,10 this pc */
static void app_icon(int act,int ox,int oy,int size){
  int s=size<16?1:size/16;
#define R(x,y,w,h,c) fill_rect(ox+(x)*s,oy+(y)*s,(w)*s,(h)*s,c)
  switch(act){
  case 0: R(1,3,6,3,0xE8B53A); R(1,5,14,9,0xF4C84B); R(1,7,14,1,0xFBE08A); break;
  case 1: draw_logo_size(ox,oy,16*s); break;
  case 2: fill_round(ox,oy,16*s,16*s,3*s,0xFFFFFF); R(7,2,2,7,0x0078D4); for(int i=0;i<4;i++) R(4+i,8+i,8-2*i,1,0x0078D4); R(3,13,10,1,0x2E9B3A); break;
  case 3: fill_round(ox,oy,16*s,16*s,3*s,0x0078D4); R(7,3,2,2,0xFFFFFF); R(7,6,2,7,0xFFFFFF); break;
  case 6: R(3,1,10,14,0xFFFFFF); R(3,1,10,2,accent); for(int i=0;i<4;i++) R(5,5+i*2,6,1,0x6090C0); break;
  case 7: R(2,1,12,14,0x2D2D2D); R(3,2,10,3,0xA8D8A0); for(int i=0;i<3;i++) for(int j=0;j<3;j++) R(3+i*4,6+j*3,3,2,0xE0E0E0); break;
  case 8: R(2,3,12,10,0xFFFFFF); R(3,4,3,3,0xE81123); R(7,4,3,3,0x2E9B3A); R(3,8,3,3,0x0078D4); R(7,8,3,3,0xFFD84D); R(11,5,2,7,0x8B5A2B); break;
  case 9: fill_round(ox,oy,16*s,16*s,3*s,0x4A5560); R(5,5,6,6,0xDDDDDD); R(7,2,2,12,0xDDDDDD); R(2,7,12,2,0xDDDDDD); R(6,6,4,4,0x4A5560); break;
  case 10: R(1,2,14,9,0xDDDDDD); R(2,3,12,7,0x2B88D8); R(6,11,4,2,0xBBBBBB); R(4,13,8,1,0xDDDDDD); break;
  default: R(2,2,12,12,0x888888); }
#undef R
}
static int type_act(int t){ static const int m[8]={0,1,3,3,6,7,8,9}; return m[t]; }
typedef struct { int x,y; const char*l1,*l2; int act; } DIcon;
#define NICONS 7
static const DIcon icons[NICONS]={
  {16,12,"This PC","",10},{16,92,"Sawit Farm","",1},{16,172,"Install My","Sawit OS",2},{16,252,"Notepad","",6},
  {104,12,"Calculator","",7},{104,92,"Paint","",8},{104,172,"Settings","",9}};

/* ---------------- wallpaper gaya Windows 10 ---------------- */
static void gen_wall(void){
  u32 dark=mix(accent,0,175), lite=mix(accent,0xFFFFFF,70);
  for(int y=0;y<SCREEN_H;y++) for(int x=0;x<SCREEN_W;x++){
    int dx=x-430,dy=y-200,d2=dx*dx+dy*dy,f=255-d2/1000; if(f<0)f=0; wall[y*SCREEN_W+x]=mix(dark,lite,f); }
  static const int pr[4][4]={{300,100,126,104},{436,100,150,104},{300,212,126,104},{436,212,150,104}};
  for(int k=0;k<4;k++) for(int y=pr[k][1];y<pr[k][1]+pr[k][3];y++) for(int x=pr[k][0];x<pr[k][0]+pr[k][2];x++){
    u32*p=&wall[y*SCREEN_W+x]; *p=mix(*p,0xFFFFFF,38+(x-pr[k][0])/10); }
}

/* ---------------- window mgmt ---------------- */
static Win *win_open(int type,const char*title,int x,int y,int w,int h){
  if(nwin==MAXW) return NULL;
  Win*n=&wins[nwin++]; memset(n,0,sizeof *n);
  n->id=next_id++; n->type=type; n->x=x;n->y=y;n->w=w;n->h=h; strncpy0(n->title,title,39); dirty=1; return n;
}
static int bring_front(int i){ Win t=wins[i]; for(int k=i;k<nwin-1;k++) wins[k]=wins[k+1]; wins[nwin-1]=t; dirty=1; return nwin-1; }
static void close_win(int i){ for(int k=i;k<nwin-1;k++) wins[k]=wins[k+1]; nwin--; dirty=1; }
static int active_idx(void){ for(int i=nwin-1;i>=0;i--) if(!wins[i].min) return i; return -1; }
static int win_at(int x,int y){ for(int i=nwin-1;i>=0;i--) if(!wins[i].min&&inr(x,y,wins[i].x,wins[i].y,wins[i].w,wins[i].h)) return i; return -1; }
static int edge_hit(int x,int y){
  for(int i=nwin-1;i>=0;i--){ Win*w=&wins[i]; if(w->min) continue;
    if(inr(x,y,w->x-3,w->y,w->w+6,w->h)){ if(w->max) return -1; if(x<=w->x+2) return 1; if(x>=w->x+w->w-3) return 2; return -1; } }
  return -1;
}
static Win *open_single(int type,const char*title,int x,int y,int w,int h){
  for(int i=0;i<nwin;i++) if(wins[i].type==type){ wins[i].min=0; return &wins[bring_front(i)]; }
  return win_open(type,title,x,y,w,h);
}
static void msgbox(const char*title,const char*text){
  Win*w=win_open(W_MSG,title,170+(nwin%4)*20,130+(nwin%4)*20,300,140); if(!w) return; strncpy0(w->text,text,95);
}
static u32 api_ticks(void){ return millis(); }
static const SawitAPI api={1,msgbox,api_ticks};

/* ---------------- Explorer ---------------- */
static void exp_load(Win*w){
  w->sel=-1;
  if(!w->path[0]){ strcpy(w->ents[0].name,"My Sawit (C:)"); w->ents[0].isdir=1; w->ents[0].size=0; w->nent=1; return; }
  int n=vfs_list(w->path,w->ents,24); w->nent=n<0?0:n;
}
static void launch_explorer(const char*path){
  Win*w=win_open(W_EXPLORER,"File Explorer",50+(nwin%5)*24,30+(nwin%5)*24,440,310); if(!w) return;
  strcpy(w->path,path); exp_load(w);
}
static void launch_farm(void){ open_single(W_FARM,"Sawit Farm",150,30,340,300); }
static void launch_about(void){ open_single(W_ABOUT,"About My Sawit OS",170,90,300,210); }
static void launch_notepad(void){ Win*w=win_open(W_NOTEPAD,"Untitled - Notepad",90+(nwin%4)*20,40+(nwin%4)*20,400,280); (void)w; }
static void launch_calc(void){ Win*w=open_single(W_CALC,"Calculator",200,40,240,320); if(w&&w->id==next_id-1){ w->fresh=1; } }
static void launch_paint(void){ Win*w=open_single(W_PAINT,"Paint",60,30,360,TITLE_H+34+PH+10); if(w&&w->id==next_id-1){ w->pcolor=0; w->pbrush=2; } }
static void launch_settings(void){ open_single(W_SETTINGS,"Settings",120,60,330,250); }
static void run_exe(const char*path){
  set_cursor_type(CURSOR_LOADING); u32 t=millis(); while(millis()-t<900) ui_pump();
  int n=vfs_read(path,exebuf,sizeof exebuf); const char*err="";
  if(n<=0) msgbox("Error","Tidak bisa membaca file."); else if(pe_run(exebuf,(u32)n,&api,&err)) msgbox("Cannot run",err);
  set_cursor_type(CURSOR_ARROW); dirty=1;
}
static void exp_up(Win*w){
  int n=strlen(w->path); if(!n) return; int i=n-1; while(i>0&&w->path[i]!='/') i--;
  if(i==0){ if(n==1) w->path[0]=0; else strcpy(w->path,"/"); } else w->path[i]=0;
  exp_load(w);
}
static void exp_open(Win*w,int i){
  FsEntry*e=&w->ents[i]; char p[64];
  if(!w->path[0]){ strcpy(w->path,"/"); exp_load(w); return; }
  strcpy(p,w->path); if(p[strlen(p)-1]!='/') strcpy(p+strlen(p),"/"); strcpy(p+strlen(p),e->name);
  if(e->isdir){ strcpy(w->path,p); exp_load(w); }
  else { int l=strlen(e->name); if(l>4&&!stricmp(e->name+l-4,".EXE")) run_exe(p); else msgbox("File Explorer","Tidak ada aplikasi untuk membuka file ini."); }
}
static void fmt_size(char*o,u32 s){ int n=u2s(s/1024+(s?1:0),o); strcpy(o+n," KB"); }
static void exp_draw(Win*w){
  int cx=w->x+1,cy=w->y+TITLE_H,cw=w->w-2,ch=w->h-TITLE_H-1; u32 hl=mix(accent,0xFFFFFF,205);
  fill_rect(cx,cy,cw,28,0xF3F3F3); button(cx+4,cy+5,22,18,"<",inr(mouse_x,mouse_y,cx+4,cy+5,22,18));
  fill_rect(cx+32,cy+5,cw-38,18,0xFFFFFF); draw_rect(cx+32,cy+5,cw-38,18,0xA0A0A0);
  char addr[80]; if(!w->path[0]) strcpy(addr,"This PC"); else { strcpy(addr,"C:"); strcpy(addr+2,w->path); for(char*c=addr;*c;c++) if(*c=='/')*c='\\'; if(!w->path[1]) strcpy(addr,"C:\\"); }
  draw_text(cx+38,cy+10,addr,0x000000,1);
  fill_rect(cx,cy+28,100,ch-28,0xFFFFFF); fill_rect(cx+100,cy+28,1,ch-28,0xE0E0E0);
  const char*nav[3]={"This PC","My Sawit (C:)","Apps"};
  for(int i=0;i<3;i++){ int ny=cy+34+i*22; if(inr(mouse_x,mouse_y,cx,ny,100,20)) fill_rect(cx+2,ny,96,20,0xE5F3FF);
    if(i) icon_folder(cx+6,ny+3); else fill_rect(cx+6,ny+3,14,10,0x2B88D8); draw_text(cx+24,ny+6,nav[i],0x000000,1); }
  int lx=cx+101,lw=cw-101; fill_rect(lx,cy+28,lw,ch-28,0xFFFFFF);
  fill_rect(lx,cy+43,lw,1,0xE0E0E0);
  draw_text(lx+26,cy+32,TR("Name","Nama"),0x606060,1); draw_text(lx+lw-70,cy+32,"Size",0x606060,1);
  for(int i=0;i<w->nent&&i<14;i++){ FsEntry*e=&w->ents[i]; int ry=cy+46+i*18;
    if(i==w->sel) fill_rect(lx,ry,lw,18,hl); else if(inr(mouse_x,mouse_y,lx,ry,lw,18)) fill_rect(lx,ry,lw,18,0xE5F3FF);
    int l=strlen(e->name); if(e->isdir) icon_folder(lx+6,ry+2); else icon_file(lx+6,ry+1,(l>4&&!stricmp(e->name+l-4,".EXE"))?0x2E9B3A:0x0078D4);
    draw_text(lx+26,ry+5,e->name,0x000000,1);
    if(!e->isdir){ char s[20]; fmt_size(s,e->size); draw_text(lx+lw-70,ry+5,s,0x404040,1); } }
  if(!w->nent) draw_text(lx+10,cy+62,TR("This folder is empty.","Folder ini kosong."),0x808080,1);
  char st[40]; int n=u2s(w->nent,st); strcpy(st+n,TR(" items"," item")); fill_rect(cx,cy+ch-14,cw,14,0xF3F3F3); draw_text(cx+6,cy+ch-11,st,0x404040,1);
}
static void exp_click(Win*w,int mx,int my,int dbl){
  int cx=w->x+1,cy=w->y+TITLE_H;
  if(inr(mx,my,cx+4,cy+5,22,18)){ exp_up(w); dirty=1; return; }
  for(int i=0;i<3;i++) if(inr(mx,my,cx,cy+34+i*22,100,20)){ strcpy(w->path,i==0?"":(i==1?"/":"/APPS")); exp_load(w); dirty=1; return; }
  int lx=cx+101; if(mx>=lx&&my>=cy+46){ int i=(my-(cy+46))/18; if(i<w->nent&&i<14){ if(dbl&&w->sel==i) exp_open(w,i); else w->sel=i; } else w->sel=-1; dirty=1; }
}

/* ---------------- Notepad ---------------- */
static int np_pass(Win*w,int cols,int first,int draw,int cx,int cy){
  int r=0,c=0;
  for(int i=0;i<w->len;i++){ char ch=w->buf[i]; if(ch=='\n'){ r++; c=0; continue; } if(c>=cols){ r++; c=0; }
    if(draw&&r>=first) draw_char(cx+c*8,cy+(r-first)*10,ch,0x000000,1); c++; }
  if(c>=cols){ r++; c=0; }
  if(draw&&r>=first) draw_char(cx+c*8,cy+(r-first)*10,'_',accent,1);
  return r+1;
}
static void np_draw(Win*w,int cx,int cy,int cw,int ch){
  fill_rect(cx,cy,cw,20,0xF3F3F3); draw_text(cx+8,cy+6,"File   Edit   Format   View   Help",0x202020,1);
  fill_rect(cx,cy+20,cw,ch-20,0xFFFFFF);
  int cols=(cw-12)/8, rows=(ch-20-8)/10; if(cols<1)cols=1; if(rows<1)rows=1;
  int total=np_pass(w,cols,0,0,0,0); int first=total>rows?total-rows:0;
  np_pass(w,cols,first,1,cx+6,cy+26);
}
static void np_key(Win*w,int k){
  if(k==8){ if(w->len) w->len--; } else if(k==10){ if(w->len<790) w->buf[w->len++]='\n'; } else if(k>=32&&k<127){ if(w->len<790) w->buf[w->len++]=(char)k; } else return;
  w->buf[w->len]=0; dirty=1;
}

/* ---------------- Calculator ---------------- */
static const char*calc_lbl[16]={"7","8","9","/","4","5","6","*","1","2","3","-","C","0","=","+"};
static int calc_rect(Win*w,int i,int*x,int*y,int*bw,int*bh){
  int cx=w->x+1,cy=w->y+TITLE_H,cw=w->w-2,ch=w->h-TITLE_H-1; int W=(cw-30)/4,H=(ch-84)/4; if(H<18)H=18;
  *bw=W;*bh=H;*x=cx+6+(i%4)*(W+6);*y=cy+60+(i/4)*(H+6); return 0;
}
static int calc_eval(Win*w,int a,int b,int op){ if(op=='+')return a+b; if(op=='-')return a-b; if(op=='*')return a*b; if(op=='/'){ if(!b){ w->err=1; return 0; } return a/b; } return b; }
static void calc_press(Win*w,int c){
  if(c>='0'&&c<='9'){ if(w->err){ w->err=0; w->acc=0; w->op=0; w->cur=0; } if(w->fresh){ w->cur=0; w->fresh=0; } if(w->cur<100000000) w->cur=w->cur*10+(c-'0'); w->shown=w->cur; }
  else if(c=='C'){ w->acc=w->cur=w->op=w->shown=w->err=0; w->fresh=1; }
  else if(c=='+'||c=='-'||c=='*'||c=='/'){ if(w->op&&!w->fresh) w->acc=calc_eval(w,w->acc,w->cur,w->op); else if(!w->op) w->acc=w->cur; w->op=c; w->fresh=1; w->shown=w->acc; }
  else if(c=='='){ if(w->op){ w->cur=calc_eval(w,w->acc,w->cur,w->op); w->acc=w->cur; w->op=0; w->fresh=1; w->shown=w->cur; } }
  else if(c==8){ if(!w->fresh){ w->cur/=10; w->shown=w->cur; } }
  dirty=1;
}
static void calc_draw(Win*w,int cx,int cy,int cw,int ch){
  fill_rect(cx,cy,cw,ch,0xE6E6E6); fill_rect(cx+6,cy+6,cw-12,46,0xF8F8F8); draw_rect(cx+6,cy+6,cw-12,46,0xC0C0C0);
  char s[16]; if(w->err) strcpy(s,"Error"); else { int v=w->shown; int neg=v<0; if(neg) v=-v; int n=0; if(neg) s[n++]='-'; u2s((u32)v,s+n); }
  draw_text(cx+cw-12-text_width(s,2),cy+22,s,0x000000,2);
  for(int i=0;i<16;i++){ int x,y,bw,bh; calc_rect(w,i,&x,&y,&bw,&bh); int hv=inr(mouse_x,mouse_y,x,y,bw,bh); int op=(i%4==3)||i==14;
    fill_rect(x,y,bw,bh,i==14?accent:(hv?0xD6E8F8:(op?0xDADADA:0xFAFAFA))); draw_rect(x,y,bw,bh,hv?accent:0xC8C8C8);
    draw_text(x+(bw-16)/2,y+(bh-16)/2,calc_lbl[i],i==14?0xFFFFFF:0x000000,2); }
}

/* ---------------- Paint ---------------- */
static void paint_dot(Win*w,int x,int y){
  int b=w->pbrush; for(int j=0;j<b;j++) for(int i=0;i<b;i++){ int px=x-b/2+i,py=y-b/2+j; if(px>=0&&py>=0&&px<PW&&py<PH) paint_cv[py*PW+px]=(u8)w->pcolor; }
}
static void paint_line(Win*w,int x0,int y0,int x1,int y1){
  int dx=x1>x0?x1-x0:x0-x1, dy=y1>y0?y0-y1:y1-y0, sx=x0<x1?1:-1, sy=y0<y1?1:-1, e=dx+dy;
  for(;;){ paint_dot(w,x0,y0); if(x0==x1&&y0==y1) break; int e2=2*e; if(e2>=dy){e+=dy;x0+=sx;} if(e2<=dx){e+=dx;y0+=sy;} }
}
static void paint_draw(Win*w,int cx,int cy,int cw,int ch){
  fill_rect(cx,cy,cw,ch,0xF0F0F0);
  for(int i=0;i<12;i++){ fill_rect(cx+6+i*20,cy+6,18,18,ppal[i]); draw_rect(cx+6+i*20,cy+6,18,18,i==w->pcolor?accent:0x808080); if(i==w->pcolor) draw_rect(cx+5+i*20,cy+5,20,20,accent); }
  button(cx+cw-130,cy+5,56,22,w->pbrush>3?"Big":"Small",inr(mouse_x,mouse_y,cx+cw-130,cy+5,56,22));
  button(cx+cw-68,cy+5,60,22,TR("Clear","Hapus"),inr(mouse_x,mouse_y,cx+cw-68,cy+5,60,22));
  int ox=cx+6,oy=cy+34;
  for(int y=0;y<PH;y++){ int sy=oy+y; if(sy<0||sy>=SCREEN_H) continue; for(int x=0;x<PW;x++){ int sx=ox+x; if(sx<0||sx>=SCREEN_W) continue; backbuf[sy*SCREEN_W+sx]=ppal[paint_cv[y*PW+x]]; } }
  draw_rect(ox-1,oy-1,PW+2,PH+2,0x606060);
}

/* ---------------- Settings ---------------- */
static void set_draw(Win*w,int cx,int cy,int cw,int ch){
  (void)w; fill_rect(cx,cy,cw,ch,0xFFFFFF);
  draw_text(cx+16,cy+14,TR("Personalization","Personalisasi"),0x000000,2);
  draw_text(cx+16,cy+42,TR("Accent color / wallpaper:","Warna aksen / wallpaper:"),0x404040,1);
  for(int i=0;i<7;i++){ int x=cx+16+i*42; fill_rect(x,cy+60,34,34,accents[i]); if(accents[i]==accent){ draw_rect(x-2,cy+58,38,38,0x000000); } }
  draw_text(cx+16,cy+116,TR("Language:","Bahasa:"),0x404040,1);
  button(cx+16,cy+130,150,26,lang?"Bahasa Indonesia":"English",inr(mouse_x,mouse_y,cx+16,cy+130,150,26));
  draw_text(cx+16,cy+172,"My Sawit OS v1.1",0x000000,1); draw_text(cx+16,cy+186,"By My Sawit Studio",0x606060,1);
}

/* ---------------- window drawing ---------------- */
static void draw_window(Win*w,int active){
  int x=w->x,y=w->y,ww=w->w,hh=w->h;
  draw_rect(x-1,y-1,ww+2,hh+2,active?accent:0x8A8A8A);
  fill_rect(x,y,ww,hh,0xFFFFFF);
  fill_rect(x,y,ww,TITLE_H,active?0xFFFFFF:0xEBEBEB);
  app_icon(type_act(w->type),x+8,y+6,16);
  char t[30]; int mc=(ww-8*BTN_W/3-60)/8; if(mc>29)mc=29; if(mc<3)mc=3; strncpy0(t,w->title,mc); draw_text(x+30,y+10,t,active?0x000000:0x808080,1);
  for(int k=0;k<3;k++){ int bx=x+ww-BTN_W*(k+1),by=y,hv=inr(mouse_x,mouse_y,bx,by,BTN_W,TITLE_H-1); u32 fg=0x000000;
    if(hv){ fill_rect(bx,by,BTN_W,TITLE_H-1,k==0?0xE81123:0xE5E5E5); if(k==0) fg=0xFFFFFF; }
    if(k==0){ draw_line(bx+14,by+9,bx+23,by+18,fg); draw_line(bx+23,by+9,bx+14,by+18,fg); }
    else if(k==1){ if(w->max){ draw_rect(bx+16,by+8,8,8,fg); draw_rect(bx+14,by+10,8,8,fg); } else draw_rect(bx+14,by+9,10,10,fg); }
    else fill_rect(bx+14,by+14,10,1,fg); }
  int cx=x+1,cy=y+TITLE_H,cw=ww-2,ch=hh-TITLE_H-1;
  if(w->type==W_EXPLORER) exp_draw(w);
  else if(w->type==W_NOTEPAD) np_draw(w,cx,cy,cw,ch);
  else if(w->type==W_CALC) calc_draw(w,cx,cy,cw,ch);
  else if(w->type==W_PAINT) paint_draw(w,cx,cy,cw,ch);
  else if(w->type==W_SETTINGS) set_draw(w,cx,cy,cw,ch);
  else if(w->type==W_FARM){
    fill_rect(cx,cy,cw,ch,0x87CEEB); fill_rect(cx,cy+ch-70,cw,70,0x5DAA3A);
    draw_logo(cx+cw/2-80,cy+10,2);
    char s[48]; strcpy(s,"Tandan: "); u2s(w->farm_n,s+8); fill_rect(cx+8,cy+ch-62,150,16,0xFFFFFF); draw_text(cx+12,cy+ch-58,s,0x000000,1);
    strcpy(s,"Rp "); u2s(w->farm_rp,s+3); fill_rect(cx+8,cy+ch-42,150,16,0xFFFFFF); draw_text(cx+12,cy+ch-38,s,0x000000,1);
    button(cx+cw-110,cy+ch-50,100,26,TR("Sell","Jual"),inr(mouse_x,mouse_y,cx+cw-110,cy+ch-50,100,26));
    draw_text_c(cx+cw/2,cy+ch-14,TR("Click the tree to harvest!","Klik pohon untuk panen!"),0x103010,1);
  } else if(w->type==W_MSG){
    for(int i=0;i<3;i++){ char ln[32]; int o=i*30; if((int)strlen(w->text)<=o) break; strncpy0(ln,w->text+o,30); draw_text(cx+12,cy+14+i*14,ln,0x000000,1); }
    button(cx+cw-80,cy+ch-34,64,24,"OK",inr(mouse_x,mouse_y,cx+cw-80,cy+ch-34,64,24));
  } else if(w->type==W_ABOUT){
    draw_logo(cx+10,cy+12,1); draw_text(cx+100,cy+20,"My Sawit OS",0x1B5E20,2); draw_text(cx+100,cy+44,"Version 1.1",0x000000,1);
    draw_text(cx+100,cy+58,"By My Sawit Studio",0x000000,1); draw_text(cx+10,cy+108,TR("Lightweight OS, familiar UI.","OS ringan dengan tampilan akrab."),0x303030,1);
    draw_text(cx+10,cy+124,"Windows-like. Made in Indonesia.",0x303030,1);
  }
}

/* ---------------- Start menu (Windows 10) ---------------- */
#define MENU_W 380
#define MENU_H 400
#define MENU_Y (TASK_Y-MENU_H)
static const struct { const char*n; int act; } mlist[8]={{"File Explorer",0},{"Notepad",6},{"Calculator",7},{"Paint",8},{"Sawit Farm",1},{"Settings",9},{"Install My Sawit OS",2},{"About My Sawit OS",3}};
static const struct { const char*n; int act; } mtile[6]={{"Explorer",0},{"Notepad",6},{"Calculator",7},{"Paint",8},{"Sawit Farm",1},{"Settings",9}};
static const u32 tilec[6]={0x0063B1,0x0078D4,0x4C5A6A,0x7B3FBF,0x2E9B3A,0x3A3F47};
/* return act, 100+n untuk rail (9 settings,4 restart,5 shutdown), -1 dalam menu tanpa target, -2 di luar */
static int menu_hit(int x,int y){
  if(!inr(x,y,0,MENU_Y,MENU_W,MENU_H)) return -2;
  if(x<44){ int y0=MENU_Y+MENU_H-3*40; for(int k=0;k<3;k++) if(inr(x,y,0,y0+k*40,44,40)) return k==0?9:(k==1?4:5); return -1; }
  for(int i=0;i<8;i++) if(inr(x,y,48,MENU_Y+44+i*38,170,36)) return mlist[i].act;
  for(int i=0;i<6;i++) if(inr(x,y,228+(i%2)*74,MENU_Y+44+(i/2)*74,70,70)) return mtile[i].act;
  return -1;
}
static void draw_start_menu(void){
  fill_rect(0,MENU_Y,MENU_W,MENU_H,0x1F2329); fill_rect(0,MENU_Y,44,MENU_H,0x181B20); draw_rect(0,MENU_Y,MENU_W,MENU_H,0x3A3F47);
  draw_logo_size(10,MENU_Y+10,24);
  draw_text(56,MENU_Y+16,TR("Apps","Aplikasi"),0xA0A8B0,1); draw_text(230,MENU_Y+16,TR("My Sawit OS","My Sawit OS"),0xA0A8B0,1);
  for(int i=0;i<8;i++){ int y=MENU_Y+44+i*38; if(mlist[i].act==start_hover) fill_rect(48,y,170,36,0x3A3F47);
    app_icon(mlist[i].act,56,y+10,16); draw_text(82,y+14,mlist[i].n,C_WHITE,1); }
  for(int i=0;i<6;i++){ int x=228+(i%2)*74,y=MENU_Y+44+(i/2)*74; fill_rect(x,y,70,70,mtile[i].act==start_hover?mix(tilec[i],C_WHITE,50):tilec[i]);
    app_icon(mtile[i].act,x+19,y+8,32); draw_text_c(x+35,y+54,mtile[i].n,C_WHITE,1); }
  int y0=MENU_Y+MENU_H-3*40;
  for(int k=0;k<3;k++){ int act=k==0?9:(k==1?4:5); if(act==start_hover) fill_rect(0,y0+k*40,44,40,0x3A3F47); int cx=22,cy=y0+k*40+20;
    if(k==0) app_icon(9,cx-8,cy-8,16); else if(k==1){ ring(cx,cy,7,C_WHITE,1); fill_rect(cx+2,cy-8,4,2,C_WHITE); } else { ring(cx,cy,7,C_WHITE,2); fill_rect(cx,cy-8,2,8,C_WHITE); } }
}

/* ---------------- taskbar ---------------- */
static int cmos(int r){ outb(0x70,r); return inb(0x71); }
static int bcd(int v){ return (cmos(0x0B)&4)?v:((v&15)+(v>>4)*10); }
static void tb_order(int*o){ int n=nwin; for(int i=0;i<n;i++) o[i]=i; for(int i=1;i<n;i++){ int k=o[i],j=i-1; while(j>=0&&wins[o[j]].id>wins[k].id){ o[j+1]=o[j]; j--; } o[j+1]=k; } }
static int tb_bw(void){ int bw=nwin?(SCREEN_W-90-244)/nwin:84; if(bw>84)bw=84; if(bw<30)bw=30; return bw; }
static void p2(char*d,int v){ d[0]='0'+v/10; d[1]='0'+v%10; }
static void draw_taskbar(int act){
  fill_rect(0,TASK_Y,SCREEN_W,TASK_H,C_TASK);
  int sh=inr(mouse_x,mouse_y,0,TASK_Y,44,TASK_H); if(start_open||sh) fill_rect(0,TASK_Y,44,TASK_H,0x2E343C);
  draw_logo_size(10,TASK_Y+4,24);
  fill_rect(48,TASK_Y+4,190,24,0xF2F2F2); ring(60,TASK_Y+14,4,0x505050,0); draw_line(63,TASK_Y+17,66,TASK_Y+20,0x505050);
  draw_text(74,TASK_Y+12,TR("Type here to search","Ketik di sini untuk cari"),0x606060,1);
  int o[MAXW]; tb_order(o); int bw=tb_bw();
  for(int k=0;k<nwin;k++){ Win*w=&wins[o[k]]; int bx=244+k*bw; int hv=inr(mouse_x,mouse_y,bx,TASK_Y,bw-2,TASK_H);
    fill_rect(bx,TASK_Y,bw-2,TASK_H,o[k]==act?0x3A414A:(hv?0x2A3038:C_TASK)); fill_rect(bx,TASK_Y+TASK_H-3,bw-2,3,w->min?0x707880:accent);
    app_icon(type_act(w->type),bx+(bw-2-16)/2,TASK_Y+6,16); }
  int h=bcd(cmos(4))%24,m=bcd(cmos(2)),d=bcd(cmos(7)),mo=bcd(cmos(8)),yr=bcd(cmos(9));
  char tm[8],dt[12]; p2(tm,h);tm[2]=':';p2(tm+3,m);tm[5]=0; p2(dt,d);dt[2]='/';p2(dt+3,mo);dt[5]='/';dt[6]='2';dt[7]='0';p2(dt+8,yr);dt[10]=0;
  draw_text(SCREEN_W-8-text_width(tm,1),TASK_Y+5,tm,C_WHITE,1); draw_text(SCREEN_W-8-text_width(dt,1),TASK_Y+17,dt,C_WHITE,1);
}
static void render(void){
  memcpy(backbuf,wall,sizeof wall);
  for(int i=0;i<NICONS;i++){ const DIcon*d=&icons[i]; if(i==sel_icon) fill_rect(d->x-8,d->y-6,80,74,mix(accent,C_WHITE,70));
    app_icon(d->act,d->x+4,d->y,48); draw_text_c(d->x+28,d->y+52,d->l1,C_WHITE,1); if(d->l2[0]) draw_text_c(d->x+28,d->y+61,d->l2,C_WHITE,1); }
  int act=active_idx();
  for(int i=0;i<nwin;i++) if(!wins[i].min) draw_window(&wins[i],i==act);
  draw_taskbar(act);
  if(start_open) draw_start_menu();
}

/* ---------------- input ---------------- */
void ui_pump(void){ if(mouse_poll()) draw_cursor(mouse_x,mouse_y); cursor_tick(millis()); }
static void start_action(int a){
  start_open=0; dirty=1;
  switch(a){
    case 0: launch_explorer("/"); break; case 1: launch_farm(); break;
    case 2: installer_run(); dirty=1; set_cursor_type(CURSOR_ARROW); break;
    case 3: launch_about(); break; case 4: reboot(); break; case 5: shutdown(); break;
    case 6: launch_notepad(); break; case 7: launch_calc(); break; case 8: launch_paint(); break;
    case 9: launch_settings(); break; case 10: launch_explorer(""); break; }
}
static void toggle_max(Win*w){
  if(!w->max){ w->px=w->x;w->py=w->y;w->pw=w->w;w->ph=w->h; w->x=0;w->y=0;w->w=SCREEN_W;w->h=TASK_Y; w->max=1; }
  else { w->x=w->px;w->y=w->py;w->w=w->pw;w->h=w->ph; w->max=0; }
  dirty=1;
}
static void content_click(int i,int mx,int my,int dbl){
  Win*w=&wins[i]; int cx=w->x+1,cy=w->y+TITLE_H,cw=w->w-2,ch=w->h-TITLE_H-1; dirty=1;
  if(w->type==W_EXPLORER) exp_click(w,mx,my,dbl);
  else if(w->type==W_MSG){ if(inr(mx,my,cx+cw-80,cy+ch-34,64,24)) close_win(i); }
  else if(w->type==W_FARM){ if(inr(mx,my,cx+cw-110,cy+ch-50,100,26)){ w->farm_rp+=w->farm_n*5000; w->farm_n=0; } else if(inr(mx,my,cx+cw/2-80,cy+10,160,160)) w->farm_n++; }
  else if(w->type==W_CALC){ for(int k=0;k<16;k++){ int x,y,bw,bh; calc_rect(w,k,&x,&y,&bw,&bh); if(inr(mx,my,x,y,bw,bh)){ calc_press(w,calc_lbl[k][0]); break; } } }
  else if(w->type==W_PAINT){
    for(int k=0;k<12;k++) if(inr(mx,my,cx+6+k*20,cy+6,18,18)) w->pcolor=k;
    if(inr(mx,my,cx+cw-130,cy+5,56,22)) w->pbrush=w->pbrush>3?2:6;
    if(inr(mx,my,cx+cw-68,cy+5,60,22)) memset(paint_cv,1,sizeof paint_cv);
    if(inr(mx,my,cx+6,cy+34,PW,PH)){ drag=4; pl_x=mx-(cx+6); pl_y=my-(cy+34); paint_dot(w,pl_x,pl_y); }
  } else if(w->type==W_SETTINGS){
    for(int k=0;k<7;k++) if(inr(mx,my,cx+16+k*42,cy+60,34,34)){ accent=accents[k]; gen_wall(); }
    if(inr(mx,my,cx+16,cy+130,150,26)) lang=!lang;
  }
}
static void on_down(void){
  int mx=mouse_x,my=mouse_y; u32 now=millis();
  if(start_open){ int h=menu_hit(mx,my);
    if(h>=0){ start_action(h); return; } if(h==-1) return;
    start_open=0; dirty=1; if(inr(mx,my,0,TASK_Y,244,TASK_H)) return; }
  if(my>=TASK_Y){
    if(inr(mx,my,0,TASK_Y,244,TASK_H)){ start_open=1; start_hover=-1; dirty=1; return; }
    int o[MAXW]; tb_order(o); int act=active_idx(),bw=tb_bw();
    for(int k=0;k<nwin;k++) if(inr(mx,my,244+k*bw,TASK_Y,bw-2,TASK_H)){ int i=o[k];
      if(wins[i].min){ wins[i].min=0; bring_front(i); } else if(i==act) wins[i].min=1; else bring_front(i); dirty=1; return; }
    return; }
  int e=edge_hit(mx,my);
  if(e>0){ int i; for(i=nwin-1;i>=0;i--) if(!wins[i].min&&inr(mx,my,wins[i].x-3,wins[i].y,wins[i].w+6,wins[i].h)) break;
    if(i>=0){ i=bring_front(i); drag=e==1?2:3; drag_right=wins[i].x+wins[i].w; return; } }
  int i=win_at(mx,my);
  if(i>=0){ i=bring_front(i); Win*w=&wins[i];
    for(int k=0;k<3;k++){ int bx=w->x+w->w-BTN_W*(k+1); if(inr(mx,my,bx,w->y,BTN_W,TITLE_H-1)){ if(k==0) close_win(i); else if(k==1) toggle_max(w); else { w->min=1; dirty=1; } return; } }
    if(my<w->y+TITLE_H){ if(last_click_id==w->id&&now-last_click_t<400&&w->type!=W_MSG){ toggle_max(w); last_click_id=-1; return; }
      last_click_id=w->id; last_click_t=now; if(!w->max){ drag=1; drag_dx=mx-w->x; drag_dy=my-w->y; } return; }
    int dbl=(last_click_id==w->id+1000&&now-last_click_t<450); last_click_id=w->id+1000; last_click_t=now;
    content_click(i,mx,my,dbl); return; }
  for(int k=0;k<NICONS;k++){ const DIcon*d=&icons[k]; if(inr(mx,my,d->x-8,d->y-6,80,74)){ int dbl=(last_click_id==-2-k&&now-last_click_t<450);
      last_click_id=-2-k; last_click_t=now; sel_icon=k; dirty=1;
      if(dbl){ sel_icon=-1; last_click_id=-1; start_action(d->act); } return; } }
  if(sel_icon>=0){ sel_icon=-1; dirty=1; }
}
static void on_move(void){
  if(drag&&nwin){ Win*w=&wins[nwin-1];
    if(drag==1){ w->x=mouse_x-drag_dx; w->y=mouse_y-drag_dy; if(w->y<0)w->y=0; if(w->y>TASK_Y-TITLE_H)w->y=TASK_Y-TITLE_H; }
    else if(drag==2){ int nx=mouse_x; if(drag_right-nx<200) nx=drag_right-200; w->x=nx; w->w=drag_right-nx; }
    else if(drag==3){ int nw=mouse_x-w->x; w->w=nw<200?200:nw; }
    else if(drag==4&&w->type==W_PAINT){ int px=mouse_x-(w->x+1+6),py=mouse_y-(w->y+TITLE_H+34); paint_line(w,pl_x,pl_y,px,py); pl_x=px; pl_y=py; }
    dirty=1; return; }
  if(start_open){ int h=menu_hit(mouse_x,mouse_y); if(h<0) h=-1; if(h!=start_hover){ start_hover=h; dirty=1; } }
}
static int hover_token(void){
  int t=win_at(mouse_x,mouse_y),tok=(t+1)*1000;
  if(t>=0){ for(int k=0;k<3;k++) if(inr(mouse_x,mouse_y,wins[t].x+wins[t].w-BTN_W*(k+1),wins[t].y,BTN_W,TITLE_H-1)) tok+=k+1;
    if(wins[t].type==W_EXPLORER||wins[t].type==W_CALC||wins[t].type==W_SETTINGS||wins[t].type==W_MSG||wins[t].type==W_FARM) tok+=((mouse_x/24)*37+(mouse_y/12))*8; }
  if(mouse_y>=TASK_Y) tok+=500000+(mouse_x/20);
  return tok;
}
static void update_cursor(void){
  if(get_cursor_type()==CURSOR_LOADING) return;
  int t=CURSOR_ARROW;
  if(drag>=2&&drag<=3) t=CURSOR_RESIZE;
  else if(!drag&&!start_open&&mouse_y<TASK_Y&&edge_hit(mouse_x,mouse_y)>0) t=CURSOR_RESIZE;
  else if(!drag){ int i=win_at(mouse_x,mouse_y);
    if(i>=0){ Win*w=&wins[i];
      if(w->type==W_EXPLORER&&inr(mouse_x,mouse_y,w->x+33,w->y+TITLE_H+5,w->w-40,18)) t=CURSOR_TEXT;
      else if(w->type==W_NOTEPAD&&mouse_y>w->y+TITLE_H+20) t=CURSOR_TEXT; } }
  set_cursor_type(t);
}
void desktop_run(void){
  int pb=0,lx=-1,ly=-1,hov=-1; u32 lastsec=0; dirty=1;
  memset(paint_cv,1,sizeof paint_cv); gen_wall();
  for(;;){
    u32 now=millis(); int moved=mouse_poll(); int k;
    while((k=kb_getkey())){
      if(k==KEY_ESC&&start_open){ start_open=0; dirty=1; continue; }
      int a=active_idx(); if(a<0) continue; Win*w=&wins[a];
      if(w->type==W_NOTEPAD) np_key(w,k);
      else if(w->type==W_CALC){ if((k>='0'&&k<='9')||k=='+'||k=='-'||k=='*'||k=='/'||k=='C'||k=='c'||k==8) calc_press(w,k=='c'?'C':k); else if(k==10||k=='=') calc_press(w,'='); } }
    if(moved){
      if(mouse_x!=lx||mouse_y!=ly){ lx=mouse_x;ly=mouse_y; draw_cursor(lx,ly); on_move(); }
      int nb=mouse_btn&1; if(nb&&!pb) on_down(); if(!nb&&pb) drag=0; pb=nb;
      int tk=hover_token(); if(tk!=hov){ hov=tk; dirty=1; }
    }
    update_cursor();
    if(now/1000!=lastsec){ lastsec=now/1000; if((lastsec%5)==0) dirty=1; }
    if(dirty){ dirty=0; render(); vga_present(); cursor_after_present(); }
    cursor_tick(now);
  }
}
