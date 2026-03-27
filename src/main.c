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


#define LCD_WIDTH GFX_LCD_WIDTH
#define LCD_HEIGHT GFX_LCD_HEIGHT

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
#include <stdlib.h>
#include <string.h>
#include <sys/rtc.h>


/* External library headers */
#include <keypadc.h>
#include <graphx.h>
#include <compression.h>
#include <fileioc.h>

#include "gfx/sprites_gfx.h"

#define DBG_KEYCOMBO (kb_2nd | kb_Left | kb_Right)

typedef int fixed_t;

#define FIXED_SHIFT 8

//The check below is for determining if shifting uses arithmetic right shifts.
//If it does (as is the case on the ez80), then we have a cheaper path to do
//these operations.
#if ((-1 >> 1) == -1)
	#define FIXED_INT_PART_USES_SHIFT 1
	#define FIXED_ONE (1 << FIXED_SHIFT)
	//NOTE: fixed_from_int was split but it turns out that LLVM actually optimizes
	//this so the split is unneeded. I'm just too lazy to revert it back at this point.
	#define fixed_from_int(value) ((value) << FIXED_SHIFT)
	#define fixed_int_part(value) ((value) >> FIXED_SHIFT)
#else
	#define FIXED_ONE (1 << FIXED_SHIFT)
	#define FIXED_INT_PART_USES_SHIFT 0
	static inline fixed_t fixed_from_int(int value) {
		return value * FIXED_ONE;
	}
	static inline int fixed_int_part(fixed_t value) {
		if (value >= 0) {
			return value / FIXED_ONE;
		}
		return -(((-value) + (FIXED_ONE - 1)) / FIXED_ONE);
	}
#endif

fixed_t curx,cury,dx,dy,maxspeed;

typedef struct enemy_t {
	uint8_t id;
	fixed_t x;
	fixed_t y;
	fixed_t dx;
} enemy_t;
enemy_t enemytable[ENEMY_TABLE_SIZE];



const char *gameoverdesc[] = {"Burp!","Omnomnom","Tasty!","Delicious!"};
const char *gameoverquit = "You quit the game";
const char *gameovertext = "Game Over";
const char *getready = "Get ready to nom!";
const char *blankstr = "";
const char *title1 = "FISHY";
const char *title5 = "High score: ";

const char *titleblock[] = {
	"[2nd] = Start game",
	"[DEL] = Show help",
	"[MODE] = Quit game",
	NULL
};

const char *endblock[] = {
	"You ate so many fish",
	"that you destroyed",
	"the lake ecosystem.",
	"Nice job breaking everything.",
	NULL
};

const char *helpblock[] = {
	"Eat smaller fish to grow",
	"Don't get eaten by bigger fish",
	"Larger fish are worth more points",
	"This game is loosely based on the flash",
	"game \"!Fishy!\" from XGenStudios.com",
	"Press any key to go back to the main menu",
	NULL
};

const char *scorefilename = "FISHYDAT";

/* Put your function prototypes here */

void bufferplayersprite();
void movetest(fixed_t* v,fixed_t* dv,kb_key_t kbneg, kb_key_t kbpos);
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
void centeredblock(const char *strsobj[], int startheight, int lineheight);
void fatalerror(const char* message);
bool consumeenemy(enemy_t* e, bool stop_after_eat);
void updatehighscore(void);
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
				centeredblock(titleblock, 105, 25);
				gfx_SetTextScale(1, 1);
				gfx_SetTextXY(5, 230);
				printtext(title5);
				gfx_PrintInt(highscore, 6);
				gfx_SetTextXY(290, 230);
				printtext(VERSION_INFO);
				//drawdebug();	//Comment out when done.
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
				updatehighscore();
				break;
				
			case GM_VICTORY:
				emptyfishtank();
				allocplayerfish(PLAYERFISH_VICTORY_BUFFER_SIZE);
				for (i=scaling[MAX_PLAYER_SIZE];i<240;i+=2) {
					setplayerfish(i);
					curx -= 128;
					cury -= 128;
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

				centeredblock(endblock, 88, 20);
				gfx_SwapDraw();
				allocplayerfish(PLAYERFISH_GAMEPLAY_BUFFER_SIZE);
				fillfishtank();
				setplayerfish(0);
				waitanykey();
				gamemode = GM_TITLE;
				updatehighscore();
				break;
				
			case GM_HELP:
				keywait();
				drawbgempty();
				gfx_SetTextFGColor(TITLE_TEXT_COLOR);
				gfx_SetTextScale(5, 5);
				centerxtext(title1, 5);
				gfx_SetTextScale(1, 1);
				centeredblock(helpblock, 80, 20);
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
	movetest(&curx,&dx,kb_Left,kb_Right);
	t = LCD_WIDTH-((scaling[playersize]*fish_width)>>6);
	if (fixed_int_part(curx)>t) {
		curx = fixed_from_int(t);
		dx = 0;
	}
	
	movetest(&cury,&dy,kb_Up,kb_Down);
	t = LCD_HEIGHT-((scaling[playersize]*fish_height)>>6);
	if (fixed_int_part(cury)>t) {
		cury = fixed_from_int(t);
		dy = 0;
	}
	
	return;
}
//old 6431 bytes
//new 6084 bytes
void movetest(fixed_t* v, fixed_t* dv, kb_key_t kbneg, kb_key_t kbpos) {
	kb_key_t k;
	int t;
	
	k = kb_Data[7];
	if (k & kbneg) *dv -= MOVE_FACTOR;
	if (k & kbpos) *dv += MOVE_FACTOR;
	t = *dv;
	t = (t<0) ? t+DRAG_COEFFICIENT : t-DRAG_COEFFICIENT;
	if ((t^(*dv))<0) t = 0;
	if (t<-maxspeed)	t = -maxspeed;
	if (t>maxspeed) t = maxspeed;
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
	gfx_RLETSprite(playerfish,fixed_int_part(curx),fixed_int_part(cury));
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
			if (e->dx>0) {
				ptr = rightfish[(e->id)-1];
			} else {
				ptr = leftfish[(e->id)-1];
			}
			gfx_RLETSprite(ptr,fixed_int_part(e->x),fixed_int_part(e->y));
		}
	}
	//Collide with enemies and perform object thingies.
	scale = (int) scaling[playersize];
	x = fixed_int_part(curx);
	y = fixed_int_part(cury);
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
			x = fixed_int_part(e->x);
			y = fixed_int_part(e->y);
			for(j=0;j<8;j+=2) {
				eb[j+0] = x+((((int)coloffsets[j+0])*scale)>>6);
				eb[j+1] = y+((((int)coloffsets[j+1])*scale)>>6);
			}
			if (((pb[0]<eb[2]) && (pb[2]>eb[0]) && (pb[1]<eb[3]) && (pb[3]>eb[1])) ||
				((pb[4]<eb[6]) && (pb[6]>eb[4]) && (pb[5]<eb[7]) && (pb[7]>eb[5]))) {
				if (consumeenemy(e, ((playersize < ((e->id)-1)) || (playersize == ((e->id)-1) && randInt(0,1))) && !debugmode)) {
					return;
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
				e->x = fixed_from_int(x);
				e->y = fixed_from_int(randInt(0,239-((scaling[id]*fish_height)>>6)));
				e->dx = dx;
				break;
			}
		}
	}
	
	//Move enemies
	for (i=0;i<ENEMY_TABLE_SIZE;i++) {
		e = &enemytable[i];
		if (e->id) {
			e->x += e->dx;
			x = fixed_int_part(e->x);
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

void centeredblock(const char *strsobj[], int startheight, int lineheight) {
	uint8_t i;
	for (i=0; strsobj[i]; ++i) {
		centerxtext(strsobj[i], startheight);
		startheight += lineheight;
	}
}



void centerxtext(const char* strobj,int y) {
	gfx_PrintStringXY((char*)strobj,(LCD_WIDTH-gfx_GetStringWidth((char*)strobj))/2,y);
}

void fatalerror(const char* message) {
	gfx_End();
	asm_ClrLCDFull();
	os_NewLine();
	os_PutStrFull((char*)message);
	while (true) {
	}
}

bool consumeenemy(enemy_t* e, bool stop_after_eat) {
	if (stop_after_eat) {
		//If the player is smaller than the enemy, or if they are the same size
		//but the random number says the enemy wins, then the player dies.
		gamemode = GM_DYING;
		return true;
	}
	score += 7 * e->id;
	e->id = 0;
	++fishnommed;
	if ((fishnommed > (MIN_FISH_TNL + (playersize * FISH_LEVEL_UP_FACTOR))) && !debugmode) {
		++playersize;
		if (playersize > MAX_PLAYER_SIZE) {
			gamemode = GM_VICTORY;
			return true;
		}
		fishnommed = 0;
	}

	return stop_after_eat;
}

void updatehighscore(void) {
	if (score > highscore) {
		highscore = score;
	}
}

//----------------------------------------------------------------------------------

uint8_t *gettempbuffer(void) {
	return (uint8_t*) gfx_vbuffer;
}


void allocplayerfish(size_t size) {
	freeplayerfish();
	playerfish = malloc(size);	//We have bigger problems if this fails.
}


void freeplayerfish(void) {
	free(playerfish);
	playerfish = NULL;
}


void *addtofishtank(size_t size) {
	void *ptr;
	ptr = fishtankptr;
	fishtankptr += size;
	/* Do not remove the stuff below in case the user changes what the fish
	   looks like since this affects conversion to RLETSprite. If you *must*
	   recover the space this takes, comment it out for future use. Doing this
	   programmatically would consume even more space so we don't do that.
	*/
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
	gfx_sprite_t* expaimg;	//Expanded (or shrunk) version of sprite object
	gfx_sprite_t* flipimg;	//Expanded sprite, but flipped across Y axis.
	uint8_t i;

	emptyfishtank();
	fishtank = malloc(FISHTANK_SIZE);

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
}

void emptyfishtank(void) {
	free(fishtank);
	fishtank = NULL;
	fishtankptr = NULL;
	memset(leftfish, 0, sizeof leftfish);
	memset(rightfish, 0, sizeof rightfish);
}

/*  During gameplay, set size to 0 in order to use the current player size.
	The size parameter is only really used in a nonzero capacity to freely
	resize the player sprite during victory animation.
*/
void setplayerfish(uint8_t size) {
	uint8_t *tempbuffer;
	gfx_sprite_t *initial_sprite;
	gfx_sprite_t *expanded_sprite;	//Also used as scratch space for flipping.

	tempbuffer = gettempbuffer();
	initial_sprite = (void*) tempbuffer;
	expanded_sprite = (void*) (tempbuffer+fish_size);

	if (debugmode) {
		zx0_Decompress(initial_sprite, devblock_compressed);
		expanded_sprite->width = initial_sprite->width;
		expanded_sprite->height = initial_sprite->height;
	} else {
		zx0_Decompress(initial_sprite, player_compressed);
		if (direction) {
			gfx_FlipSpriteY(initial_sprite, expanded_sprite);
			memcpy(initial_sprite, expanded_sprite, fish_size);	//Flip back since the scaling function modifies the source sprite.
		}
		//Set scaling size based on player size or victory animation size.
		if (!size) {
			expanded_sprite->width = (scaling[playersize]*fish_width)>>6;
			expanded_sprite->height = (scaling[playersize]*fish_height)>>6;
		} else {
			direction = 0;
			expanded_sprite->width = (size*fish_width)>>6;
			expanded_sprite->height = (size*fish_height)>>6;
		}
	}
	gfx_ScaleSprite(initial_sprite, expanded_sprite);
	gfx_ConvertToRLETSprite(expanded_sprite, playerfish);
}


void initplayerstate(void) {
	oldplayersize = playersize = 1;
	dx = dy = 0;
	direction = 0;
	score = 0;
	fishnommed = 0;
	curx = fixed_from_int((LCD_WIDTH+8)/2);
	cury = fixed_from_int((LCD_HEIGHT+6)/2);
	maxspeed = fixed_from_int(2);
	setplayerfish(0);
	memset(enemytable,0,sizeof enemytable);

}



