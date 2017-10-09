/*
 *--------------------------------------
 * Program Name: FISHY
 *       Author: Rodger "Iambian" Weisman
 *      License: BSD (See LICENSE file)
 *  Description: Eat the bigger fish and don't get eaten
 *--------------------------------------
*/

#define VERSION_INFO "v0.2"

#define TRANSPARENT_COLOR 0xF8
#define GREETINGS_DIALOG_TEXT_COLOR 0xDF
#define BULLET_TABLE_SIZE 512
#define ENEMY_TABLE_SIZE 32
#define MAX_ENEMY_SIZE 13
#define MAX_PLAYER_SIZE 15
#define MIN_FISH_TNL 2
#define FISH_LEVEL_UP_FACTOR 2

#define GM_TITLE 0
#define GM_OPENANIM 1
#define GM_GAMEMODE 2
#define GM_CLOSINGANIM 3
#define GM_DYING 4
#define GM_GAMEOVER 5
#define GM_OUTPOSTSHOP 6
#define GM_RETURNTOBASE 7
#define GM_VICTORY 8
#define GM_HELP 9

#define GMBOX_X (LCD_WIDTH/4)
#define GMBOX_Y (LCD_HEIGHT/2-LCD_HEIGHT/8)
#define GMBOX_W (LCD_WIDTH/2)
#define GMBOX_H (LCD_HEIGHT/4)

#define ACCELFACTOR ((int)(0.07*256))

/* Standard headers (recommended) */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External library headers */
#include <debug.h>
#include <intce.h>
#include <keypadc.h>
#include <graphx.h>
#include <decompress.h>
#include <fileioc.h>

#include "gfx/sprites_gfx.h"



typedef union fp16_8 {
	int fp;
	struct {
		uint8_t fpart;
		int16_t ipart;
	} p;
} fp16_8;
fp16_8 curx,cury,dx,dy,maxspeed,gravity,thrust;

typedef struct enemy_t {
	uint8_t id;
	fp16_8 x;
	fp16_8 y;
	fp16_8 dx;
	int temp;
} enemy_t;
enemy_t emptyenemy,testenemy,enemytable[ENEMY_TABLE_SIZE];



const char *gameoverdesc[] = {"Burp!","Omnomnom","Tasty!","Delicious!"};
const char *gameoverquit = "You quit the game";
const char *gameovertext = "Game Over";
const char *getready = "Get ready to nom!";
const char *blankstr = "";
const char *title1 = "FISHY";
const char *title3 = "[2nd] = Start game";
const char *title3a = "[DEL] = Show help";
const char *title4 = "[MODE] = Quit game";
const char *title5 = "High score: ";
const char *end1a = "You ate so many fish";
const char *end1b = "that you destroyed";
const char *end1c = "the lake ecosystem.";
const char *end1d = "Nice job breaking everything.";

const char *help1 = "Eat smaller fish to grow";
const char *help2 = "Don't get eaten by bigger fish";
const char *help3 = "Larger fish are worth more points";
const char *help4 = "This game is loosely based of the flash";
const char *help5 = "game \"!Fishy!\" from XGenStudios.com";
const char *help6 = "Press any key to go back to the main menu";

const char *scorefilename = "FISHYDAT";

/* Put your function prototypes here */

void bufferplayersprite();
void movetest(int* v,int* dv,kb_key_t kbneg, kb_key_t kbpos);
void drawbg();
void drawbgempty();
void doplayer();
void doenemies();
//---
void putaway();
void waitanykey();
void keywait();
void drawdebug();
void centerxtext(char* strobj,int y);
void* decompress(void* cdata_in, int out_size);
//---
void expandfish(void);
void setplayerfish(uint8_t size);
void initplayerstate(void);

/* Put all your globals here */
int debugvalue;
uint8_t gamemode;
uint8_t maintimer;
int8_t menuoption;

ti_var_t slot;

int score;
int highscore;
uint8_t playersize;
uint8_t oldplayersize;
bool direction;
int movefactor;
uint8_t fishnommed;
int dragcoeff;
uint8_t debugmode = 0;

gfx_rletsprite_t* leftfish[16];
gfx_rletsprite_t* rightfish[16];
gfx_rletsprite_t* playerfish;
uint8_t scaling[] = {8,12,16,20,24,28,32,40,48,56,64,80,96,102,128,160};  //128,160, 192,224
//-----------------------------------Enemy fish size limit--^
uint8_t coloffsets[] = {2,15,62,31,12,8,37,37};

void main(void) {
    int x,y,i,j,temp,subtimer;
	kb_key_t k;
	char* tmp_str;
	void* ptr1;
	void* ptr2;
	
	/* Initialize system */
	malloc(0);  //for linking purposes
	gfx_Begin(gfx_8bpp);
	gfx_SetDrawBuffer();
	gfx_SetTransparentColor(TRANSPARENT_COLOR);
	gfx_SetClipRegion(0,0,320,240);
	/* Initialize variables */
	expandfish();  //playerfish initialized in this routine too.
	initplayerstate();
	
	highscore = 0;
	ti_CloseAll();
	slot = ti_Open(scorefilename,"r");
	if (slot) ti_Read(&highscore,sizeof highscore, 1, slot);
	ti_CloseAll();
	
	/* Initiate main game loop */
	gamemode = GM_TITLE;
	maintimer = 0;
	menuoption = 0;
	while (1) {
		kb_Scan();
		switch (gamemode) {
			case GM_TITLE:
				k = kb_Data[1];
				i = randInt(0,255);  //keep picking rands to cycle
				debugmode = 0;
				if ((k == kb_2nd)&&(kb_Data[7]==(kb_Up|kb_Left))) debugmode = 1;
				if (k == kb_2nd) {
					gamemode = GM_OPENANIM;
					maintimer = 0;
					initplayerstate();
					break;
				}
				if (k == kb_Mode) putaway();
				if (k == kb_Del) gamemode = GM_HELP;
				
				drawbgempty();
				
				gfx_SetTextFGColor(0xEF);
				gfx_SetTextScale(5,5);
				centerxtext(title1,5);
				gfx_SetTextScale(2,2);
				centerxtext(title3,105);
				centerxtext(title3a,130);
				centerxtext(title4,155);
				gfx_SetTextScale(1,1);
				gfx_SetTextXY(5,230);
				gfx_PrintString(title5);
				gfx_PrintInt(highscore,6);
				gfx_SetTextXY(290,230);
				gfx_PrintString(VERSION_INFO);
				gfx_SwapDraw();
				
				break;
			case GM_OPENANIM:
				//game gfx
				drawbg();
				doplayer();
				gfx_SetColor(0x08);  //xlibc dark blue
				gfx_FillRectangle(GMBOX_X,GMBOX_Y,GMBOX_W,GMBOX_H);
				gfx_SetTextFGColor(GREETINGS_DIALOG_TEXT_COLOR);
				tmp_str = (maintimer & 0x08) ? getready : blankstr;
				gfx_GetStringWidth(tmp_str);
				centerxtext(tmp_str,GMBOX_Y+25);
				gfx_SwapDraw();
				if (maintimer>64) gamemode = GM_GAMEMODE;
				break;
			case GM_GAMEMODE:
				k = kb_Data[1];
				if (k & kb_Mode) {
					gamemode = GM_GAMEOVER;
					break;
				}
				bufferplayersprite();
				drawbg();
				doplayer();
				doenemies();
				gfx_SwapDraw();
				break;
			case GM_DYING:
				gamemode = GM_GAMEOVER;
				break;
			case GM_GAMEOVER:
				gfx_SetColor(0x08);  //xlibc dark blue
				gfx_FillRectangle(GMBOX_X,GMBOX_Y,GMBOX_W,GMBOX_H);
				tmp_str = gameoverdesc[randInt(0,3)];
				if (kb_Data[1] == kb_Mode) tmp_str = gameoverquit;
				gfx_GetStringWidth(tmp_str);
				gfx_SetTextFGColor(GREETINGS_DIALOG_TEXT_COLOR);
				centerxtext(tmp_str,GMBOX_Y+15);
				centerxtext(gameovertext,GMBOX_Y+35);
				gfx_SwapDraw();
				waitanykey();
				gamemode = GM_TITLE;
				if (score>highscore) {
					highscore = score;
				}
				break;
			case GM_CLOSINGANIM:
				//This game doesn't actually close. So... yeah. Start
				//replacing these endgame events pronto.
				break;
				
			case GM_VICTORY:
				for (i=0;i<MAX_ENEMY_SIZE;i++) {
					free(leftfish[i]);
					free(rightfish[i]);
				}
				free(playerfish);
				playerfish = malloc((224*168)+2);
				for (i=scaling[MAX_PLAYER_SIZE];i<240;i+=2) {
					setplayerfish(i);
					curx.fp -= 128;
					cury.fp -= 128;
					drawbg();
					doplayer();
					gfx_SwapDraw();
				}
				for (i=0;i<32;i++){
					drawbg();
					doplayer();
					gfx_SwapDraw();
				}
				free(playerfish);
				gfx_FillScreen(0x08);
				gfx_SetColor(0x08);  //xlibc dark blue
				centerxtext(end1a,88);
				centerxtext(end1b,108);
				centerxtext(end1c,128);
				centerxtext(end1d,148);
				gfx_SwapDraw();
				expandfish();
				waitanykey();
				gamemode = GM_TITLE;
				if (score>highscore) {
					highscore = score;
				}
				break;
				
			case GM_HELP:
				keywait();
				drawbgempty();
				gfx_SetTextFGColor(0xEF);
				gfx_SetTextScale(5,5);
				centerxtext(title1,5);
				gfx_SetTextScale(1,1);
				centerxtext(help1,80);
				centerxtext(help2,100);
				centerxtext(help3,120);
				centerxtext(help4,140);
				centerxtext(help5,160);
				centerxtext(help6,180);
				
				gfx_SetTextXY(5,230);
				gfx_PrintString(title5);
				gfx_PrintInt(highscore,6);
				gfx_SetTextXY(290,230);
				gfx_PrintString(VERSION_INFO);
				gfx_SwapDraw();
				waitanykey();
				gamemode = GM_TITLE;
				break;
				
			default:
				putaway();
				break;
			
		}
		maintimer++;
	}
	
	

//	for (i=0;i<32;i++) enemies[i] = emptyenemy; //NOT NEEDED ANYMORE BUT KEPT FOR REFERENCE
//	ti_CloseAll();
//	slot = ti_Open("LLOONDAT","r");
//	if (slot) ti_Read(&highscore,sizeof highscore, 1, slot);
//	ti_CloseAll();
//	loonsub_sprite = gfx_MallocSprite(32,32);
//	dzx7_Turbo(loonsub_compressed,loonsub_sprite);

}

void bufferplayersprite() {
	kb_key_t k = kb_Data[7];
	int t = 0;
	if (((k & kb_Right) && !direction) || ((k & kb_Left) && direction)) {
		direction = direction^1;
		setplayerfish(0);
	}
	if (oldplayersize != playersize) {
		setplayerfish(0);
		oldplayersize = playersize;
	}

	//Accelerate if moving in direction, else decelerate (less)
	movetest((int*) &curx.fp,(int*) &dx.fp,kb_Left,kb_Right);
	t = LCD_WIDTH-((scaling[playersize]*fish_width)>>6);
	if (curx.p.ipart>t) { curx.p.ipart = t;	dx.fp = 0; }
	
	movetest((int*) &cury.fp,(int*) &dy.fp,kb_Up,kb_Down);
	t = LCD_HEIGHT-((scaling[playersize]*fish_height)>>6);
	if (cury.p.ipart>t) { cury.p.ipart = t; dy.fp = 0; }
	
	return;
}
//old 6431 bytes
//new 6084 bytes
void movetest(int* v,int* dv,kb_key_t kbneg, kb_key_t kbpos) {
	kb_key_t k = kb_Data[7];
	int t;
	
	if (k&kbneg) (*dv) -= movefactor;
	if (k&kbpos) (*dv) += movefactor;
	t = *dv;
	t = (t<0) ? t+dragcoeff : t-dragcoeff;
	if ((t^(*dv))<0) t = 0;
	if (t<-maxspeed.fp)	t = -maxspeed.fp;
	if (t>maxspeed.fp) t = maxspeed.fp;
	(*dv) = t;
	(*v) += t;
	// First half of bounds checking. The second half is in the caller since it
	// uses different bounds depending on which axis is being used.
	if (*v<0) *v = *dv = 0;
	return;
	
}
void drawbgempty() {
	gfx_FillScreen(0x12);
}

void drawbg() {
	uint8_t y,i;
	int x;
	drawbgempty();
	gfx_SetTextFGColor(0xFE);
	gfx_SetTextXY(3,3);
	gfx_PrintString("SCORE: ");
	gfx_PrintInt(score,6);
	//gfx_SetTextXY(3,13); for(i=0;i<ENEMY_TABLE_SIZE;i++) gfx_PrintChar(enemytable[i].id+'A');
}

void doplayer() {
	gfx_RLETSprite(playerfish,curx.p.ipart,cury.p.ipart);
	return;
}

/*	Collision detection. Too lazy to do oval-type detection, so fudging
	it to using two overlapping bounding boxes:
	Long bounding box on basic 64x48 sprites: [2,15][62,31]
	Tall bounding box on basic 64x48 sprites: [12,18][37,37]
*/
void doenemies() {
	void *ptr;
	uint8_t i,j,id;
	int x,y,dx;
	int spd,w,scale;
	int pb[8];
	int eb[8];
	enemy_t* e;
	
	//Draw enemies
	for(i=0;i<ENEMY_TABLE_SIZE;i++) {
		e = &enemytable[i];
		if (e->id) {
			if ((e->dx.fp)>0) {
				ptr = rightfish[(e->id)-1];
			} else {
				ptr = leftfish[(e->id)-1];
			}
			gfx_RLETSprite(ptr,e->x.p.ipart,e->y.p.ipart);
		}
	}
	//Collide with enemies and perform object thingies.
	scale = (int) scaling[playersize];
	x = curx.p.ipart;
	y = cury.p.ipart;
	if (debugmode) {
		for(i=0;i<8;i+=4) {
			pb[i+0] = x+0;
			pb[i+1] = y+0;
			pb[i+2] = x+32;
			pb[i+3] = y+32;
		}
	} else {
		for(i=0;i<8;i+=2) {
			pb[i+0] = x+((((int)coloffsets[i+0])*scale)>>6);
			pb[i+1] = y+((((int)coloffsets[i+1])*scale)>>6);
		}
	}
	
	for(i=0;i<ENEMY_TABLE_SIZE;i++) {
		e = &enemytable[i];
		if (e->id) {
			scale = (int) scaling[(e->id)-1];
			x = e->x.p.ipart;
			y = e->y.p.ipart;
			for(j=0;j<8;j+=2) {
				eb[j+0] = x+((((int)coloffsets[j+0])*scale)>>6);
				eb[j+1] = y+((((int)coloffsets[j+1])*scale)>>6);
			}
			if (((pb[0]<eb[2]) && (pb[2]>eb[0]) && (pb[1]<eb[3]) && (pb[3]>eb[1])) ||
				((pb[4]<eb[6]) && (pb[6]>eb[4]) && (pb[5]<eb[7]) && (pb[7]>eb[5]))) {
				if (((playersize < ((e->id)-1)) || ( playersize == ((e->id)-1) && randInt(0,1)))&&!debugmode) {
					gamemode = GM_DYING;
					//dbg_sprintf(dbgout,"COLLIDED : %i\n",i);
					//dbg_sprintf(dbgout,"PLOBJ : [%i,%i] [%i,%i] [%i,%i] [%i,%i] \n",pb[0],pb[1],pb[2],pb[3],pb[4],pb[5],pb[6],pb[7]);
					//dbg_sprintf(dbgout,"ENOBJ : [%i,%i] [%i,%i] [%i,%i] [%i,%i] \n",eb[0],eb[1],eb[2],eb[3],eb[4],eb[5],eb[6],eb[7]);
					return;
				} else {
					score += 7*(e->id);
					e->id = 0;
					fishnommed++;
					if ((fishnommed>(MIN_FISH_TNL+(playersize*FISH_LEVEL_UP_FACTOR)))&&!debugmode) {
						playersize++;
						if (playersize>MAX_PLAYER_SIZE) {
							gamemode = GM_VICTORY;
							return;
						}
						fishnommed = 0;
					}
				}
			
			}
		}
	}
	
	//Make new enemies
	if (!randInt(0,15)) {
		for(i=0;i<ENEMY_TABLE_SIZE;i++) {
			e = &enemytable[i];
			if (!(e->id)) {
				spd = randInt(3,8)<<6;
				id = randInt(0,MAX_ENEMY_SIZE-1);
				x = (scaling[id]*fish_width)>>6;
				if (!randInt(0,1)) {
					//start on left side of screen
					x = -x;
					dx = spd;
				} else {
					x = x+320;
					dx = -spd;
				}
				e->id = id+1;
				e->x.fp = x<<8;
				e->y.fp = randInt(0,239-((scaling[id]*fish_height)>>6))<<8;
				e->dx.fp = dx;
				e->temp = 0;
				break;
			}
		}
	}
	
	//Move enemies
	for (i=0;i<ENEMY_TABLE_SIZE;i++) {
		e = &enemytable[i];
		if (e->id) {
			e->x.fp += e->dx.fp;
			x = e->x.p.ipart;
			w = (scaling[(e->id)-1]*fish_width)>>6;
			if ((x<(-w)) || (x>(LCD_WIDTH+w))) {
				e->id = 0;
				continue;
			}
		}
	}
}


//---------------------------------------------------------------------------

void putaway() {
//	int_Reset();
	gfx_End();
	slot = ti_Open(scorefilename,"w");
	ti_Write(&highscore,sizeof highscore, 1, slot);
	ti_CloseAll();
	exit(0);
}

void waitanykey() {
	keywait();            //wait until all keys are released
	while (!kb_AnyKey()); //wait until a key has been pressed.
	while (kb_AnyKey());  //make sure key is released before advancing
}	

void keywait() {
	while (kb_AnyKey());  //wait until all keys are released
}

void drawdebug() {
	static int i=0;
	i++;
	gfx_SetTextFGColor(0xFE);
	gfx_SetTextXY(10,15);
	gfx_PrintInt(debugvalue,4);
}

void centerxtext(char* strobj,int y) {
	gfx_PrintStringXY(strobj,(LCD_WIDTH-gfx_GetStringWidth(strobj))/2,y);
}

void* decompress(void *cdata_in,int out_size) {
	void *ptr = malloc(out_size);
	dzx7_Turbo(cdata_in,ptr);
	return ptr;
}

//----------------------------------------------------------------------------------

void expandfish(void) {
	gfx_sprite_t* baseimg;
	gfx_sprite_t* expaimg;
	gfx_sprite_t* flipimg;
	uint8_t i;

	baseimg = (void*) gfx_vbuffer;
	expaimg = (void*) (*gfx_vbuffer+fish_size);
	flipimg = (void*) (*gfx_vbuffer+fish_size+32768+2);
	
	dzx7_Turbo(fish_compressed,baseimg);
	
	for(i=0;i<MAX_ENEMY_SIZE;i++) {
		expaimg->width = (scaling[i]*fish_width)/64;
		expaimg->height = (scaling[i]*fish_height)/64;
		
		gfx_ScaleSprite(baseimg,expaimg);
		gfx_FlipSpriteY(expaimg,flipimg);
		leftfish[i] = gfx_ConvertMallocRLETSprite(expaimg);
		rightfish[i] = gfx_ConvertMallocRLETSprite(flipimg);
	}
	playerfish = malloc((160*120)+2);  //fish_width*3*fish_height*3+2
}



void setplayerfish(uint8_t size) {
	gfx_sprite_t* baseimg;
	gfx_sprite_t* expaimg;
	gfx_sprite_t* flipimg;
	uint8_t i;

	baseimg = (void*) gfx_vbuffer;
	expaimg = (void*) (*gfx_vbuffer+fish_size);
	flipimg = (void*) (*gfx_vbuffer+fish_size+32768+2);
	
	if (debugmode) {
		dzx7_Turbo(devblock_compressed,expaimg);
	} else {
		dzx7_Turbo(player_compressed,baseimg);
		if (!size) {
			expaimg->width = (scaling[playersize]*fish_width)>>6;
			expaimg->height = (scaling[playersize]*fish_height)>>6;
		} else {
			direction = 0;
			expaimg->width = (size*fish_width)>>6;
			expaimg->height = (size*fish_height)>>6;
		}
		gfx_ScaleSprite(baseimg,expaimg);
	}
	/* WARNING: THE CONVERSION ASSUMES THERE IS ENOUGH TRANSPARENCY IN THE
	   IMAGE TO OVERCOME THE OVERHEAD OF THE RLET SPRITE FORMAT. DO NOT USE
	   IMAGES THAT ARE TOO LARGE OR TOO NOISY */
	if (direction) {
		gfx_FlipSpriteY(expaimg,flipimg);
		gfx_ConvertToRLETSprite(flipimg,playerfish);
	} else {
		gfx_ConvertToRLETSprite(expaimg,playerfish);
	}
}

void initplayerstate(void) {
	oldplayersize = playersize = 1;
	dx.fp = dy.fp = 0;
	direction = 0;
	score = 0;
	fishnommed = 0;
	curx.p.ipart = (LCD_WIDTH+8)/2;
	cury.p.ipart = (LCD_HEIGHT+6)/2;
	movefactor = 14;
	dragcoeff = 6;
	maxspeed.fp = 2*256;
	setplayerfish(0);
	memset(&emptyenemy,0,sizeof emptyenemy);
	memset(enemytable,0,ENEMY_TABLE_SIZE*sizeof(emptyenemy));

}



