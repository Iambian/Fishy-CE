/*
 *--------------------------------------
 * Program Name: FISHY
 *       Author: Rodger "Iambian" Weisman
 *      License: MIT (See LICENSE file)
 *  Description: Eat the bigger fish and don't get eaten
 *--------------------------------------
*/

#define VERSION_INFO "v0.3"

#define CUSTOM_PALETTE
#ifndef CUSTOM_PALETTE
//If not using custom palette, use XLIBC defaults.
#define TRANSPARENT_COLOR 0xF8
#define GREETINGS_DIALOG_TEXT_COLOR 0xDF
#define DARK_BLUE_COLOR 0x08
#define TITLE_TEXT_COLOR 0xEF
#define SCORE_TEXT_COLOR 0xFE
#define BACKGROUND_COLOR 0x12
#else
//Custom palette entry offsets. Use mappings in convimg.yaml to set these.
#define TRANSPARENT_COLOR 0
#define GREETINGS_DIALOG_TEXT_COLOR 1
#define DARK_BLUE_COLOR 2
#define TITLE_TEXT_COLOR 3
#define SCORE_TEXT_COLOR 4
#define BACKGROUND_COLOR 5
#endif


#define LCD_WIDTH 320
#define LCD_HEIGHT 240

#define BULLET_TABLE_SIZE 512
#define ENEMY_TABLE_SIZE 32
#define MAX_ENEMY_SIZE 13
#define MAX_PLAYER_SIZE 15
#define MIN_FISH_TNL 2
#define FISH_LEVEL_UP_FACTOR 2
#define DRAG_COEFFICIENT 6
#define MOVE_FACTOR 14

#define FISHTANK_SIZE 24576  //Measured usage is 23669 bytes with current assets.
#define PLAYERFISH_GAMEPLAY_BUFFER_SIZE ((160*120)+2)
#define PLAYERFISH_VICTORY_BUFFER_SIZE ((240*180)+2)

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

/* Keep these headers */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>

/* Standard headers (recommended) */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/rtc.h>
#include <assert.h>


/* External library headers */
#include <debug.h>
#include <intce.h>
#include <keypadc.h>
#include <graphx.h>
#include <compression.h>
#include <fileioc.h>

#include "gfx/sprites_gfx.h"

#define DBG_KEYCOMBO (kb_2nd | kb_Left | kb_Right)

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
void waitanykey();
void keywait();
void drawdebug();
void printtext(const char* strobj);
void centerxtext(const char* strobj,int y);
void* decompress(void* cdata_in, int out_size);
//---
uint8_t *gettempbuffer(void);	//Returns current graph buffer for temp use. This buffer is assumed to be 76800 bytes large.
void allocplayerfish(size_t size);
void freeplayerfish(void);
void fillfishtank(void);	//Init leftfish and rightfish tables.
void emptyfishtank(void);	//Deallocates buffer for this.
void setplayerfish(uint8_t size);
void initplayerstate(void);

/* Put all your globals here */
int debugvalue;
uint8_t gamemode;
uint8_t maintimer;

int score;
int highscore;
uint8_t playersize;
uint8_t oldplayersize;
bool direction;
uint8_t fishnommed;
uint8_t debugmode;


uint8_t *fishtank;
uint8_t *fishtankptr;
gfx_rletsprite_t* leftfish[16];
gfx_rletsprite_t* rightfish[16];
gfx_rletsprite_t* playerfish;
uint8_t scaling[] = {8,12,16,20,24,28,32,40,48,56,64,80,96,102,128,160};  //128,160, 192,224
//-----------------------------------Enemy fish size limit--^
uint8_t coloffsets[] = {2,15,62,31,12,8,37,37};

int main(void) {
	uint8_t i;
	kb_key_t k;
	const char* tmp_str;
	ti_var_t file;
	bool isrunning;
	
	/* Initialize system */
	gfx_Begin();
	gfx_SetDrawBuffer();
	memcpy(gfx_palette, game_pal, sizeof game_pal);
	gfx_SetColor(DARK_BLUE_COLOR);
	gfx_SetTransparentColor(TRANSPARENT_COLOR);
	drawbgempty();
	gfx_SwapDraw();	//Show something before doing fish loading.
	
	/* Initialize variables */
	fillfishtank();
	allocplayerfish(PLAYERFISH_GAMEPLAY_BUFFER_SIZE);
	initplayerstate();
	
	file = ti_Open(scorefilename,"r");
	if (file) {
		ti_Read(&highscore,sizeof highscore, 1, file);
		ti_Close(file);
	} else {
		highscore = 0;
	}
	
	/* Initiate main game loop */
	gamemode = GM_TITLE;
	isrunning = true;
	while (isrunning) {
		kb_Scan();
		k = kb_Data[1] | kb_Data[7];
		switch (gamemode) {
			case GM_TITLE:
				srandom(rtc_Time());	//Always a brand new game each time.
				debugmode = (k == DBG_KEYCOMBO);
				if (k & kb_2nd) {
					gamemode = GM_OPENANIM;
					maintimer = 0;
					initplayerstate();
					break;
				}
				if (k & kb_Mode) {
					isrunning = false;
					break;
				}
				if (k & kb_Del) gamemode = GM_HELP;
				
				drawbgempty();
				
				gfx_SetTextFGColor(TITLE_TEXT_COLOR);
				gfx_SetTextScale(5, 5);
				centerxtext(title1, 5);
				gfx_SetTextScale(2, 2);
				centerxtext(title3, 105);
				centerxtext(title3a, 130);
				centerxtext(title4, 155);
				gfx_SetTextScale(1, 1);
				gfx_SetTextXY(5, 230);
				printtext(title5);
				gfx_PrintInt(highscore, 6);
				gfx_SetTextXY(290, 230);
				printtext(VERSION_INFO);
				drawdebug();	//Comment out when done.
				gfx_SwapDraw();
				
				break;

			case GM_OPENANIM:
				//game gfx
				drawbg();
				doplayer();
				gfx_FillRectangle(GMBOX_X, GMBOX_Y, GMBOX_W, GMBOX_H);
				gfx_SetTextFGColor(GREETINGS_DIALOG_TEXT_COLOR);
				tmp_str = (maintimer & 0x08) ? getready : blankstr;
				centerxtext(tmp_str ,GMBOX_Y+25);
				gfx_SwapDraw();
				if (maintimer>64) gamemode = GM_GAMEMODE;
				break;
			case GM_GAMEMODE:
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
				gfx_FillRectangle(GMBOX_X,GMBOX_Y,GMBOX_W,GMBOX_H);
				if (k & kb_Mode) {
					tmp_str = gameoverquit;
				}  else {
					tmp_str = gameoverdesc[randInt(0,3)];
				}
				gfx_SetTextFGColor(GREETINGS_DIALOG_TEXT_COLOR);
				centerxtext(tmp_str, GMBOX_Y+15);
				centerxtext(gameovertext, GMBOX_Y+35);
				gfx_SwapDraw();
				waitanykey();
				gamemode = GM_TITLE;
				if (score > highscore) {
					highscore = score;
				}
				break;
			case GM_CLOSINGANIM:
				//This game doesn't actually close. So... yeah. Start
				//replacing these endgame events pronto.
				break;
				
			case GM_VICTORY:
				emptyfishtank();
				allocplayerfish(PLAYERFISH_VICTORY_BUFFER_SIZE);
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
				gfx_FillScreen(DARK_BLUE_COLOR);
				centerxtext(end1a, 88);
				centerxtext(end1b, 108);
				centerxtext(end1c, 128);
				centerxtext(end1d, 148);
				gfx_SwapDraw();
				allocplayerfish(PLAYERFISH_GAMEPLAY_BUFFER_SIZE);
				fillfishtank();
				setplayerfish(0);
				waitanykey();
				gamemode = GM_TITLE;
				if (score>highscore) {
					highscore = score;
				}
				break;
				
			case GM_HELP:
				keywait();
				drawbgempty();
				gfx_SetTextFGColor(TITLE_TEXT_COLOR);
				gfx_SetTextScale(5, 5);
				centerxtext(title1, 5);
				gfx_SetTextScale(1, 1);
				centerxtext(help1, 80);
				centerxtext(help2, 100);
				centerxtext(help3, 120);
				centerxtext(help4, 140);
				centerxtext(help5, 160);
				centerxtext(help6, 180);
				
				gfx_SetTextXY(5, 230);
				printtext(title5);
				gfx_PrintInt(highscore, 6);
				gfx_SetTextXY(290, 230);
				printtext(VERSION_INFO);
				gfx_SwapDraw();
				waitanykey();
				gamemode = GM_TITLE;
				break;
				
			default:
				isrunning = false;
				break;
			
		}
		maintimer++;
	}
	freeplayerfish();
	emptyfishtank();
	gfx_End();
	file = ti_Open(scorefilename,"w");
	if (file) {
		ti_Write(&highscore,sizeof highscore, 1, file);
		ti_Close(file);
	} else {
		asm_ClrLCDFull();
		os_NewLine();
		os_PutStrFull("Could not save game data.");
	}
}

void bufferplayersprite() {
	kb_key_t k;
	k = kb_Data[7];
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
void movetest(int* v, int* dv, kb_key_t kbneg, kb_key_t kbpos) {
	kb_key_t k;
	int t;
	
	k = kb_Data[7];
	if (k & kbneg) *dv -= MOVE_FACTOR;
	if (k & kbpos) *dv += MOVE_FACTOR;
	t = *dv;
	t = (t<0) ? t+DRAG_COEFFICIENT : t-DRAG_COEFFICIENT;
	if ((t^(*dv))<0) t = 0;
	if (t<-maxspeed.fp)	t = -maxspeed.fp;
	if (t>maxspeed.fp) t = maxspeed.fp;
	*dv = t;
	*v += t;
	// First half of bounds checking. The second half is in the caller since it
	// uses different bounds depending on which axis is being used.
	if (*v<0) *v = *dv = 0;
	return;
	
}
void drawbgempty() {
	gfx_FillScreen(BACKGROUND_COLOR);
}

void drawbg() {
	//uint8_t y,i;
	//int x;
	drawbgempty();
	gfx_SetTextFGColor(SCORE_TEXT_COLOR);
	gfx_SetTextXY(3,3);
	printtext("SCORE: ");
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
					//gamemode = GM_DYING;

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


void waitanykey() {
	keywait();            //wait until all keys are released
	while (!kb_AnyKey()); //wait until a key has been pressed.
	while (kb_AnyKey());  //make sure key is released before advancing
}	

void keywait() {
	while (kb_AnyKey());  //wait until all keys are released
}

void drawdebug() {
	//static int i=0;
	//i++;
	gfx_SetTextFGColor(SCORE_TEXT_COLOR);
	gfx_SetTextXY(10,15);
	gfx_PrintInt(debugvalue,6);
}

void printtext(const char* strobj) {
	gfx_PrintString((char*)strobj);
}

void centerxtext(const char* strobj,int y) {
	gfx_PrintStringXY((char*)strobj,(LCD_WIDTH-gfx_GetStringWidth((char*)strobj))/2,y);
}

void* decompress(void *cdata_in,int out_size) {
	void *ptr = malloc(out_size);
	zx0_Decompress(ptr,cdata_in);
	return ptr;
}

//----------------------------------------------------------------------------------

uint8_t *gettempbuffer(void) {
	return (uint8_t*) gfx_vbuffer;
}


void allocplayerfish(size_t size) {
	freeplayerfish();
	playerfish = malloc(size);
	assert(playerfish);
}


void freeplayerfish(void) {
	free(playerfish);
	playerfish = NULL;
}


void *addtofishtank(size_t size) {
	void *ptr;
	ptr = fishtankptr;
	fishtankptr += size;
	if ((fishtankptr-fishtank)>FISHTANK_SIZE) {
		sprintf((char*)0xFC0000,"Fishtank overflow! Used %i bytes.",fishtankptr-fishtank);
		gfx_End();
		exit(1);
	}
	return ptr;
}

//The fish tank is a large area of memory used to store the decompressed and
//expanded fish sprites. Fill it when the game starts. Empty it when it ends.
void fillfishtank(void) {
	uint8_t *tempbuffer;
	gfx_sprite_t* baseimg;	//Sprite object. Initial size.
	gfx_sprite_t* expaimg;	//Expaneded (or shrunk) version of sprite object
	gfx_sprite_t* flipimg;	//Expanded sprite, but flipped across Y axis.
	uint8_t i;

	emptyfishtank();
	fishtank = malloc(FISHTANK_SIZE);
	assert(fishtank);
	fishtankptr = fishtank;

	tempbuffer = gettempbuffer();
	baseimg = (void*) tempbuffer;
	expaimg = (void*) (tempbuffer+fish_size);
	flipimg	= (void*) (tempbuffer+fish_size+((LCD_WIDTH*LCD_HEIGHT)/2)+2);

	zx0_Decompress(baseimg,fish_compressed);

	for (i=0; i<MAX_ENEMY_SIZE; ++i) {
		//Scale fish by setting destination size parameters first.
		expaimg->width = (scaling[i]*fish_width)/64;
		expaimg->height = (scaling[i]*fish_height)/64;
		//Scale base sprite then flip it.
		gfx_ScaleSprite(baseimg,expaimg);
		gfx_FlipSpriteY(expaimg,flipimg);
		leftfish[i] = gfx_ConvertToNewRLETSprite(expaimg, addtofishtank);
		rightfish[i] = gfx_ConvertToNewRLETSprite(flipimg, addtofishtank);
	}
	sprintf((char*)0xFB0000,"Fishtank used %i bytes.",fishtankptr-fishtank);
}

void emptyfishtank(void) {
	free(fishtank);
	fishtank = NULL;
	fishtankptr = NULL;
	memset(leftfish, 0, sizeof leftfish);
	memset(rightfish, 0, sizeof rightfish);
	//NOTE: Please refrain from using the fish tables (leftfish/rightfish)
	//after this function is called. No checks are made to ensure they aren't.
	//Also, the player sprite is not stored in the fish tank, so it is unaffected
	//by this function.
	//NOTE: Or do try to use them if you want to know what use-after-free feels like. I won't stop you.
}

//TODO: Figure out how all these calls are being used and see if any changes
//need to be made to the way the player sprite is stored. The main concern
//at the moment is how I've changed how and where I've used malloc for everything.
//Keep in mind that if you win the game, a lot of freeing is done around that
//time to make room for the giant player sprite used in the victory animation. 
//So be sure to test that if you change anything in this area.


/*  During gameplay, set size to 0 in order to use the current player size.
	The size parameter is only really used in a nonzero capacity to freely
	resize the player sprite during victory animation.
*/
void setplayerfish(uint8_t size) {
	uint8_t *tempbuffer;
	gfx_sprite_t* baseimg;
	gfx_sprite_t* expaimg;
	gfx_sprite_t* flipimg;

	assert(playerfish);
	tempbuffer = gettempbuffer();
	baseimg = (void*) tempbuffer;
	expaimg = (void*) (tempbuffer+fish_size);
	flipimg = (void*) (tempbuffer+fish_size+((LCD_WIDTH*LCD_HEIGHT)/2)+2);
	
	if (debugmode) {
		zx0_Decompress(expaimg,devblock_compressed);
	} else {
		zx0_Decompress(baseimg,player_compressed);
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
	maxspeed.fp = 2*256;
	setplayerfish(0);
	memset(&emptyenemy,0,sizeof emptyenemy);
	memset(enemytable,0,ENEMY_TABLE_SIZE*sizeof(emptyenemy));

}



