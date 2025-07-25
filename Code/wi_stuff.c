// wi_stuff.c :  Intermission screens.

#include "doomdef.h"
#include "wi_stuff.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "m_random.h"
#include "r_local.h"
#include "s_sound.h"
#include "st_stuff.h"
#include "i_video.h"
#include "v_video.h"
#include "z_zone.h"


//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//


//
// Different vetween registered DOOM (1994) and
//  Ultimate DOOM - Final edition (retail, 1995?).
// This is supposedly ignored for commercial
//  release (aka DOOM II), which had 34 maps
//  in one episode. So there.
#define NUMEPISODES     4
#define NUMMAPS         9


// in tics
//U #define PAUSELEN            (TICRATE*2)
//U #define SCORESTEP           100
//U #define ANIMPERIOD          32
// pixel distance from "(YOU)" to "PLAYER N"
//U #define STARDIST            10
//U #define WK 1


// GLOBAL LOCATIONS
#define WI_TITLEY               2
#define WI_SPACINGY             16 //TODO: was 33

// SINGPLE-PLAYER STUFF
#define SP_STATSX               50
#define SP_STATSY               50

#define SP_TIMEX                16
#define SP_TIMEY                (BASEVIDHEIGHT-32)


// NET GAME STUFF
#define NG_STATSY               50
#define NG_STATSX               (32 + SHORT(star->width)/2 + 32*!dofrags)

#define NG_SPACINGX             64


// DEATHMATCH STUFF
#define DM_MATRIXX              16
#define DM_MATRIXY              24

#define DM_SPACINGX             32

#define DM_TOTALSX              269

#define DM_KILLERSX             0
#define DM_KILLERSY             100
#define DM_VICTIMSX             5
#define DM_VICTIMSY             50




typedef enum
{
    ANIM_ALWAYS,
    ANIM_RANDOM,
    ANIM_LEVEL

} animenum_t;

typedef struct
{
    int         x;
    int         y;

} point_t;


//
// Animation.
// There is another anim_t used in p_spec.
//
typedef struct
{
    animenum_t  type;

    // period in tics between animations
    int         period;

    // number of animation frames
    int         nanims;

    // location of animation
    point_t     loc;

    // ALWAYS: n/a,
    // RANDOM: period deviation (<256),
    // LEVEL: level
    int         data1;

    // ALWAYS: n/a,
    // RANDOM: random base period,
    // LEVEL: n/a
    int         data2;

    // actual graphics for frames of animations
    patch_t*    p[3];

    // following must be initialized to zero before use!

    // next value of bcnt (used in conjunction with period)
    int         nexttic;

    // last drawn animation frame
    int         lastdrawn;

    // next frame number to animate
    int         ctr;

    // used by RANDOM and LEVEL when animating
    int         state;

} anim_t;


static point_t lnodes[NUMEPISODES][NUMMAPS] =
{
    // Episode 0 World Map
    {
        { 185, 164 },   // location of level 0 (CJ)
        { 148, 143 },   // location of level 1 (CJ)
        { 69, 122 },    // location of level 2 (CJ)
        { 209, 102 },   // location of level 3 (CJ)
        { 116, 89 },    // location of level 4 (CJ)
        { 166, 55 },    // location of level 5 (CJ)
        { 71, 56 },     // location of level 6 (CJ)
        { 135, 29 },    // location of level 7 (CJ)
        { 71, 24 }      // location of level 8 (CJ)
    },

    // Episode 1 World Map should go here
    {
        { 254, 25 },    // location of level 0 (CJ)
        { 97, 50 },     // location of level 1 (CJ)
        { 188, 64 },    // location of level 2 (CJ)
        { 128, 78 },    // location of level 3 (CJ)
        { 214, 92 },    // location of level 4 (CJ)
        { 133, 130 },   // location of level 5 (CJ)
        { 208, 136 },   // location of level 6 (CJ)
        { 148, 140 },   // location of level 7 (CJ)
        { 235, 158 }    // location of level 8 (CJ)
    },

    // Episode 2 World Map should go here
    {
        { 156, 168 },   // location of level 0 (CJ)
        { 48, 154 },    // location of level 1 (CJ)
        { 174, 95 },    // location of level 2 (CJ)
        { 265, 75 },    // location of level 3 (CJ)
        { 130, 48 },    // location of level 4 (CJ)
        { 279, 23 },    // location of level 5 (CJ)
        { 198, 48 },    // location of level 6 (CJ)
        { 140, 25 },    // location of level 7 (CJ)
        { 281, 136 }    // location of level 8 (CJ)
    }

};


//
// Animation locations for episode 0 (1).
// Using patches saves a lot of space,
//  as they replace 320x200 full screen frames.
//
static anim_t epsd0animinfo[] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 224, 104 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 184, 160 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 112, 136 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 72, 112 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 88, 96 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 48 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 192, 40 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 136, 16 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 80, 16 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 64, 24 } }
};

static anim_t epsd1animinfo[] =
{
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 1 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 2 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 3 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 4 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 5 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 6 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 7 },
    { ANIM_LEVEL, TICRATE/3, 3, { 192, 144 }, 8 },
    { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 8 }
};

static anim_t epsd2animinfo[] =
{
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 168 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 40, 136 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 160, 96 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 104, 80 } },
    { ANIM_ALWAYS, TICRATE/3, 3, { 120, 32 } },
    { ANIM_ALWAYS, TICRATE/4, 3, { 40, 0 } }
};

static int NUMANIMS[NUMEPISODES] =
{
    sizeof(epsd0animinfo)/sizeof(anim_t),
    sizeof(epsd1animinfo)/sizeof(anim_t),
    sizeof(epsd2animinfo)/sizeof(anim_t)
};

static anim_t *anims[NUMEPISODES] =
{
    epsd0animinfo,
    epsd1animinfo,
    epsd2animinfo
};


//
// GENERAL DATA
//

//
// Locally used stuff.
//
#define FB 0


// States for single-player
#define SP_KILLS                0
#define SP_ITEMS                2
#define SP_SECRET               4
#define SP_FRAGS                6
#define SP_TIME                 8
#define SP_PAR                  ST_TIME

#define SP_PAUSE                1

// in seconds
#define SHOWNEXTLOCDELAY        4
//#define SHOWLASTLOCDELAY      SHOWNEXTLOCDELAY


// used to accelerate or skip a stage
static int              acceleratestage;

// wbs->pnum
static int              me;

 // specifies current state
static stateenum_t      state;

// contains information passed into intermission
static wbstartstruct_t* wbs;

static wbplayerstruct_t* plrs;  // wbs->plyr[]

// used for general timing
static int              cnt;

// used for timing of background animation
static int              bcnt;

// signals to refresh everything for one frame
static int              firstrefresh;

static int              cnt_kills[MAXPLAYERS];
static int              cnt_items[MAXPLAYERS];
static int              cnt_secret[MAXPLAYERS];
static int              cnt_time;
static int              cnt_par;
static int              cnt_pause;

// # of commercial levels
static int              NUMCMAPS;


//
//      GRAPHICS
//

// background (map of levels).
//static patch_t*       bg;
static char             bgname[9];

// You Are Here graphic
static patch_t*         yah[2];

// splat
static patch_t*         splat;

// %, : graphics
static patch_t* percent;
static patch_t* percentxmas;
static patch_t* colon;
static patch_t* colonxmas;

// 0-9 graphic
static patch_t* num[10];
static patch_t* numxmas[10];

// minus sign
static patch_t* wiminus;
static patch_t* wiminusxmas;

// "Finished!" graphics
static patch_t*         finished;

// "Entering" graphic
static patch_t*         entering;

// "secret"
static patch_t*         sp_secret;

 // "Kills", "Scrt", "Items", "Frags"
static patch_t* kills;
static patch_t* secret;
static patch_t* items;
static patch_t* frags;

static patch_t* killsxmas;
static patch_t* secretxmas;
static patch_t* itemsxmas;
static patch_t* fragsxmas;

// Time sucks.
static patch_t* time;
static patch_t* timexmas;
static patch_t* par;
static patch_t* parxmas;
static patch_t*         sucks;

// "killers", "victims"
static patch_t*         killers;
static patch_t*         victims;

// "Total", your face, your dead face
static patch_t*         total;
static patch_t*         star;
static patch_t*         bstar;

// "red P[1..MAXPLAYERS]"
//static patch_t*         p[MAXPLAYERS]; //UNUSED now check WI_loaddata

//added:08-02-98: use STPB0 for all players, but translate the colors
static patch_t*         stpb;
//static char             wibp[MAXPLAYERS][4];    //P1,P2,...

// "gray P[1..MAXPLAYERS]"
//static patch_t*         bp[MAXPLAYERS];

 // Name graphics of each level (centered)
static patch_t**        lnames;

//
// CODE
//

// slam background
// UNUSED static unsigned char *background=0;

void WI_slamBackground(void)
{
    if (rendermode==render_soft) {
        memcpy(screens[0], screens[1], vid.width * vid.height);
        V_MarkRect (0, 0, vid.width, vid.height);
    } else {
        V_DrawScaledPatch(0, 0, 1+V_NOSCALESTART, W_CachePatchName(bgname, PU_CACHE));
    }

}

// The ticker is used to detect keys
//  because of timing issues in netgames.
boolean WI_Responder(event_t* ev)
{
    return false;
}


// Draws "<Levelname> Finished!"
void WI_drawLF(void)
{
    int y = WI_TITLEY;

    // draw <LevelName>
    V_DrawScaledPatch ((BASEVIDWIDTH - SHORT(lnames[wbs->last]->width))/2,
                       y, FB, lnames[wbs->last]);

    // draw "Finished!"
    y += (5*SHORT(lnames[wbs->last]->height))/4;

    V_DrawScaledPatch ((BASEVIDWIDTH - SHORT(finished->width))/2,
                       y, FB, finished);
}



// Draws "Entering <LevelName>"
void WI_drawEL(void)
{
    int y = WI_TITLEY;

    // draw "Entering"
    V_DrawScaledPatch((BASEVIDWIDTH - SHORT(entering->width))/2,
                y, FB, entering);

    // draw level
    y += (5*SHORT(lnames[wbs->next]->height))/4;

    V_DrawScaledPatch((BASEVIDWIDTH - SHORT(lnames[wbs->next]->width))/2,
                y, FB, lnames[wbs->next]);

}

void
WI_drawOnLnode
( int           n,
  patch_t*      c[] )
{

    int         i;
    int         left;
    int         top;
    int         right;
    int         bottom;
    boolean     fits = false;

    i = 0;
    do
    {
        left = lnodes[wbs->epsd][n].x - SHORT(c[i]->leftoffset);
        top = lnodes[wbs->epsd][n].y - SHORT(c[i]->topoffset);
        right = left + SHORT(c[i]->width);
        bottom = top + SHORT(c[i]->height);

        if (left >= 0
            && right < BASEVIDWIDTH
            && top >= 0
            && bottom < BASEVIDHEIGHT)
        {
            fits = true;
        }
        else
        {
            i++;
        }
    } while (!fits && i!=2);

    if (fits && i<2)
    {
        V_DrawScaledPatch(lnodes[wbs->epsd][n].x, lnodes[wbs->epsd][n].y,
                    FB, c[i]);
    }
    else
    {
        // DEBUG
        printf("Could not place patch on level %d", n+1);
    }
}



void WI_initAnimatedBack(void)
{
    int         i;
    anim_t*     a;

    if (gamemode == commercial)
        return;

    if (wbs->epsd > 2)
        return;

    for (i=0;i<NUMANIMS[wbs->epsd];i++)
    {
        a = &anims[wbs->epsd][i];

        // init variables
        a->ctr = -1;

        // specify the next time to draw it
        if (a->type == ANIM_ALWAYS)
            a->nexttic = bcnt + 1 + (M_Random()%a->period);
        else if (a->type == ANIM_RANDOM)
            a->nexttic = bcnt + 1 + a->data2+(M_Random()%a->data1);
        else if (a->type == ANIM_LEVEL)
            a->nexttic = bcnt + 1;
    }

}

void WI_updateAnimatedBack(void)
{
    int         i;
    anim_t*     a;

    if (gamemode == commercial)
        return;

    if (wbs->epsd > 2)
        return;

    for (i=0;i<NUMANIMS[wbs->epsd];i++)
    {
        a = &anims[wbs->epsd][i];

        if (bcnt == a->nexttic)
        {
            switch (a->type)
            {
              case ANIM_ALWAYS:
                if (++a->ctr >= a->nanims) a->ctr = 0;
                a->nexttic = bcnt + a->period;
                break;

              case ANIM_RANDOM:
                a->ctr++;
                if (a->ctr == a->nanims)
                {
                    a->ctr = -1;
                    a->nexttic = bcnt+a->data2+(M_Random()%a->data1);
                }
                else a->nexttic = bcnt + a->period;
                break;

              case ANIM_LEVEL:
                // gawd-awful hack for level anims
                if (!(state == StatCount && i == 7)
                    && wbs->next == a->data1)
                {
                    a->ctr++;
                    if (a->ctr == a->nanims) a->ctr--;
                    a->nexttic = bcnt + a->period;
                }
                break;
            }
        }

    }

}

void WI_drawAnimatedBack(void)
{
    int                 i;
    anim_t*             a;

    if (commercial)
        return;

    if (wbs->epsd > 2)
        return;

    for (i=0 ; i<NUMANIMS[wbs->epsd] ; i++)
    {
        a = &anims[wbs->epsd][i];

        if (a->ctr >= 0)
            V_DrawScaledPatch(a->loc.x, a->loc.y, FB, a->p[a->ctr]);
    }

}

//
// Draws a number.
// If digits > 0, then use that many digits minimum,
//  otherwise only use as many as necessary.
// Returns new x position.
//

int WI_drawNum(int x, int y, int n, int digits) {
    int fontwidth;
    int neg;
    int temp;

    if (cv_fonttype.value == FONT_MAR2K) {
        fontwidth = SHORT(num[0]->width);
    }
    else {
        fontwidth = SHORT(numxmas[0]->width);
    }

    if (digits < 0)
    {
        if (!n)
        {
            // make variable-length zeros 1 digit long
            digits = 1;
        }
        else
        {
            // figure out # of digits in #
            digits = 0;
            temp = n;

            while (temp)
            {
                temp /= 10;
                digits++;
            }
        }
    }

    neg = n < 0;
    if (neg)
        n = -n;

    // if non-number, do not draw it
    if (n == 1994)
        return 0;

    // draw the new number
    while (digits--)
    {
        x -= fontwidth;
        if (cv_fonttype.value == FONT_MAR2K) {
            V_DrawScaledPatch(x, y, FB, num[n % 10]);
        }
        else {
            V_DrawScaledPatch(x, y, FB, numxmas[n % 10]);
        }
        n /= 10;
    }

    // draw a minus sign if necessary
    if (neg) {
        if (cv_fonttype.value == FONT_XMAS)
            V_DrawScaledPatch(x -= 8, y, FB, wiminusxmas);
        else
            V_DrawScaledPatch(x -= 8, y, FB, wiminus);
    }

    return x;

}

void
WI_drawPercent
( int           x,
  int           y,
  int           p )
{
    if (p < 0)
        return;
    if (cv_fonttype.value == FONT_MAR2K)
        V_DrawScaledPatch(x, y, FB, percent);
    else
        V_DrawScaledPatch(x, y, FB, percentxmas);
    WI_drawNum(x, y, p, -1);
}



//
// Display level completion time and par,
//  or "sucks" message if overflow.
//
void
WI_drawTime
( int           x,
  int           y,
  int           t )
{

    int         div;
    int         n;

    if (t<0)
        return;

    if (cv_fonttype.value == FONT_MAR2K) {
        if (t <= 61 * 59)
        {
            div = 1;

            do
            {
                n = (t / div) % 60;
                x = WI_drawNum(x, y, n, 2) - SHORT(colon->width);
                div *= 60;

                // draw
                if (div == 60 || t / div)
                    V_DrawScaledPatch(x, y, FB, colon);

            } while (t / div);
        }
        else
        {
            // "sucks"
            V_DrawScaledPatch(x - SHORT(sucks->width), y, FB, sucks);
        }
    }
    else {
        if (t <= 61 * 59)
        {
            div = 1;

            do
            {
                n = (t / div) % 60;
                x = WI_drawNum(x, y, n, 2) - SHORT(colonxmas->width);
                div *= 60;

                // draw
                if (div == 60 || t / div)
                    V_DrawScaledPatch(x, y, FB, colonxmas);

            } while (t / div);
        }
        else
        {
            // "sucks"
            V_DrawScaledPatch(x - SHORT(sucks->width), y, FB, sucks);
        }
    }
}


void WI_End(void)
{
    void WI_unloadData(void);
    WI_unloadData();
}

// used for write introduce next level
void WI_initNoState(void)
{
    state = NoState;
    acceleratestage = 0;
    cnt = 10;
}

void WI_updateNoState(void) {

    WI_updateAnimatedBack();

    if (--cnt==0)
    {
        WI_End();
        G_WorldDone();
    }

}

static boolean          snl_pointeron = false;


void WI_initShowNextLoc(void)
{
    state = ShowNextLoc;
    acceleratestage = 0;
    cnt = SHOWNEXTLOCDELAY * TICRATE;

    WI_initAnimatedBack();
}

void WI_updateShowNextLoc(void)
{
    WI_updateAnimatedBack();

    if (!--cnt || acceleratestage)
        WI_initNoState();
    else
        snl_pointeron = (cnt & 31) < 20;
}

void WI_drawShowNextLoc(void)
{

    int         i;
    int         last;

    if (cnt<=0)  // all removed no draw !!!
        return;

    WI_slamBackground();

    // draw animated background
    WI_drawAnimatedBack();

    if ( gamemode != commercial)
    {
        if (wbs->epsd > 2)
        {
            WI_drawEL();
            return;
        }

        last = (wbs->last == 8) ? wbs->next - 1 : wbs->last;

        // draw a splat on taken cities.
        for (i=0 ; i<=last ; i++)
            WI_drawOnLnode(i, &splat);

        // splat the secret level?
        if (wbs->didsecret)
            WI_drawOnLnode(8, &splat);

        // draw flashing ptr
        if (snl_pointeron)
            WI_drawOnLnode(wbs->next, yah);
    }

    // draws which level you are entering..
    if ( (gamemode != commercial)
         || wbs->next != 30)
        WI_drawEL();

}

void WI_drawNoState(void)
{
    snl_pointeron = true;
    WI_drawShowNextLoc();
}


static int              dm_state;
static int              dm_frags[MAXPLAYERS][MAXPLAYERS];
static int              dm_totals[MAXPLAYERS];
// boris added wait 10 second in the detahmatch intermission
static int              waittic;

void WI_initDeathmatchStats(void)
{

    int         i;
    int         j;

    state = StatCount;
    acceleratestage = 0;
    dm_state = 1;

    cnt_pause = TICRATE;
    waittic=TICRATE*10;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        for (j=0 ; j<MAXPLAYERS ; j++)
            dm_frags[i][j] = 0;

        dm_totals[i] = 0;
    }

    WI_initAnimatedBack();
}

void WI_updateDeathmatchStats(void)
{

    int         i;
    int         j;

    boolean     stillticking;

    WI_updateAnimatedBack();

    if (acceleratestage && dm_state != 4)
    {
        acceleratestage = 0;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (playeringame[i])
            {
                for (j=0 ; j<MAXPLAYERS ; j++)
                    if (playeringame[j])
                        dm_frags[i][j] = plrs[i].frags[j];

                dm_totals[i] = ST_PlayerFrags(i);
            }
        }

        waittic=TICRATE*10;
        S_StartSound(0, sfx_barexp);
        dm_state = 4;
    }


    if (dm_state == 2)
    {
        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (playeringame[i])
            {
                for (j=0 ; j<MAXPLAYERS ; j++)
                {
                    if (playeringame[j]
                        && dm_frags[i][j] != plrs[i].frags[j])
                    {
                        if (plrs[i].frags[j] < 0)
                            dm_frags[i][j]--;
                        else
                            dm_frags[i][j]++;

                        stillticking = true;
                    }
                }
                dm_totals[i] = ST_PlayerFrags(i);
            }

        }
        if (!stillticking)
        {
            S_StartSound(0, sfx_barexp);
            dm_state++;
        }
    }
    else if (dm_state == 4)
    {
        if(waittic>0) waittic--;
        if (waittic==0 && acceleratestage)
        {
            S_StartSound(0, sfx_slop);

            if ( gamemode == commercial)
                WI_initNoState();
            else
                WI_initShowNextLoc();
        }
    }
    else if (dm_state & 1)
    {
        if (!--cnt_pause)
        {
            dm_state++;
            cnt_pause = TICRATE;
        }
    }
}


//  Quick-patch for the Cave party 19-04-1998 !!
//
extern byte*       whitemap;  // color translation table for red->white font

void WI_drawRancking(char *title,int x,int y,fragsort_t *fragtable
                         , int scorelines, boolean large, int white)
{
    int   i,j;
    int   color;
    char  num[12];
    int   plnum;
    int   frags;
    fragsort_t temp;

    // sort the frags count
    for (i=0; i<scorelines; i++)
        for(j=0; j<scorelines-1-i; j++)
            if( fragtable[j].count < fragtable[j+1].count )
            {
                temp = fragtable[j];
                fragtable[j] = fragtable[j+1];
                fragtable[j+1] = temp;
            }

    if(title)
        V_DrawString (x, y-14, title);
    // draw rankings
    for (i=0; i<scorelines; i++)
    {
        frags = fragtable[i].count;
        plnum = fragtable[i].num;

        // draw color background
        color = fragtable[i].color;
        if (!color)
            color = *( (byte *)colormaps + 0x78 );
        else
            color = *( (byte *)translationtables - 256 + (color<<8) + 0x78 );
        V_DrawFill (x-1,y-1,large ? 40 : 26,9,color);

        // draw frags count
        sprintf(num,"%3i", frags );
        V_DrawString (x+(large ? 32 : 24)-V_StringWidth(num), y, num);

        // draw name
        if (plnum == white)
            V_DrawStringWhite (x+(large ? 64 : 29), y, fragtable[i].name);
        else
            V_DrawString (x+(large ? 64 : 29), y, fragtable[i].name);

        y += 12;
        if (y>=BASEVIDHEIGHT)
            break;            // dont draw past bottom of screen
    }
}

void WI_drawDeathmatchStats(void)
{
    int          i,j;
    int          scorelines;
    int          whiteplayer;
    fragsort_t   fragtab[MAXPLAYERS];

    WI_slamBackground();

    // draw animated background
    WI_drawAnimatedBack();
    WI_drawLF();

    //Fab:25-04-98: when you play, you quickly see your frags because your
    //  name is displayed white, when playback demo, you quicly see who's the
    //  view.
    whiteplayer = demoplayback ? displayplayer : consoleplayer;

    // count frags for each present player
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
        if (playeringame[i])
        {
            fragtab[scorelines].count = dm_totals[i];
            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];
            scorelines++;
        }
    WI_drawRancking("Frags",5,80,fragtab,scorelines,false,whiteplayer);

    // count buchholz
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
        if (playeringame[i])
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
                if (playeringame[j] && i!=j)
                     fragtab[scorelines].count+= dm_frags[i][j]*dm_totals[j];

            fragtab[scorelines].num = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];
            scorelines++;
        }
    WI_drawRancking("Buchholz",85,80,fragtab,scorelines,false,whiteplayer);

    // count individuel
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
        if (playeringame[i])
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
                if (playeringame[j] && i!=j)
                {
                     if(dm_frags[i][j]>dm_frags[j][i])
                         fragtab[scorelines].count+=3;
                     else
                         if(dm_frags[i][j]==dm_frags[j][i])
                              fragtab[scorelines].count+=1;
                }

            fragtab[scorelines].num = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];
            scorelines++;
        }
    WI_drawRancking("indiv.",165,80,fragtab,scorelines,false,whiteplayer);

    // count deads
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
        if (playeringame[i])
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
                if (playeringame[j])
                     fragtab[scorelines].count+=dm_frags[j][i];
            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = players[i].skincolor;
            fragtab[scorelines].name  = player_names[i];

            scorelines++;
        }
    WI_drawRancking("deads",245,80,fragtab,scorelines,false,whiteplayer);
}

boolean teamingame(int teamnum)
{
   int i;

   if (cv_teamplay.value == 1)
   {
       for(i=0;i<MAXPLAYERS;i++)
          if(playeringame[i] && players[i].skincolor==teamnum)
              return true;
   }
   else
   if (cv_teamplay.value == 2)
   {
       for(i=0;i<MAXPLAYERS;i++)
          if(playeringame[i] && players[i].skin==teamnum)
              return true;
   }
   return false;
}

void WI_drawTeamsStats(void)
{
    int          i,j;
    int          scorelines;
    int          whiteplayer;
    fragsort_t   fragtab[MAXPLAYERS];

    WI_slamBackground();

    // draw animated background
    WI_drawAnimatedBack();
    WI_drawLF();

    //Fab:25-04-98: when you play, you quickly see your frags because your
    //  name is displayed white, when playback demo, you quicly see who's the
    //  view.
    if(cv_teamplay.value==1)
        whiteplayer = demoplayback ? players[displayplayer].skincolor
                                   : players[consoleplayer].skincolor;
    else
        whiteplayer = demoplayback ? players[displayplayer].skin
                                   : players[consoleplayer].skin;

    // count frags for each present player
    scorelines = HU_CreateTeamFragTbl(fragtab,dm_totals,dm_frags);

    WI_drawRancking("Frags",5,80,fragtab,scorelines,false,whiteplayer);

    // count buchholz
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
        if (teamingame(i))
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
                if (teamingame(j) && i!=j)
                    fragtab[scorelines].count+= dm_frags[i][j]*dm_totals[j];

            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = i;
            fragtab[scorelines].name  = team_names[i];
            scorelines++;
        }
    WI_drawRancking("Buchholz",85,80,fragtab,scorelines,false,whiteplayer);

    // count individuel
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
        if (teamingame(i))
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
                if (teamingame(j) && i!=j)
                {
                     if(dm_frags[i][j]>dm_frags[j][i])
                         fragtab[scorelines].count+=3;
                     else
                         if(dm_frags[i][j]==dm_frags[j][i])
                              fragtab[scorelines].count+=1;
                }

            fragtab[scorelines].num = i;
            fragtab[scorelines].color = i;
            fragtab[scorelines].name  = team_names[i];
            scorelines++;
        }
    WI_drawRancking("indiv.",165,80,fragtab,scorelines,false,whiteplayer);

    // count deads
    scorelines = 0;
    for (i=0; i<MAXPLAYERS; i++)
        if (teamingame(i))
        {
            fragtab[scorelines].count = 0;
            for (j=0; j<MAXPLAYERS; j++)
                if (teamingame(j))
                     fragtab[scorelines].count+=dm_frags[j][i];
            fragtab[scorelines].num   = i;
            fragtab[scorelines].color = i;
            fragtab[scorelines].name  = team_names[i];

            scorelines++;
        }
    WI_drawRancking("deads",245,80,fragtab,scorelines,false,whiteplayer);
}



void WI_ddrawDeathmatchStats(void)
{

    int         i;
    int         j;
    int         x;
    int         y;
    int         w;

    int         lh;     // line height

    byte*       colormap;       //added:08-02-98:see below

    lh = WI_SPACINGY;

    WI_slamBackground();

    // draw animated background
    WI_drawAnimatedBack();
    WI_drawLF();

    // draw stat titles (top line)
    V_DrawScaledPatch(DM_TOTALSX-SHORT(total->width)/2,
                DM_MATRIXY-WI_SPACINGY+10,
                FB,
                total);

    V_DrawScaledPatch(DM_KILLERSX, DM_KILLERSY, FB, killers);
    V_DrawScaledPatch(DM_VICTIMSX, DM_VICTIMSY, FB, victims);

    // draw P?
    x = DM_MATRIXX + DM_SPACINGX;
    y = DM_MATRIXY;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (playeringame[i])
        {
            //added:08-02-98: use V_DrawMappedPatch instead of
            //                    V_DrawScaledPatch, so that the
            // graphics are 'colormapped' to the player's colors!
            if (players[i].skincolor==0)
                colormap = colormaps;
            else
                colormap = (byte *) translationtables - 256 + (players[i].skincolor<<8);

            V_DrawMappedPatch(x-SHORT(stpb->width)/2,
                        DM_MATRIXY - WI_SPACINGY,
                        FB,
                        stpb,      //p[i], now uses a common STPB0 translated
                        colormap); //      to the right colors

            V_DrawMappedPatch(DM_MATRIXX-SHORT(stpb->width)/2,
                        y,
                        FB,
                        stpb,      //p[i]
                        colormap);

            if (i == me)
            {
                V_DrawScaledPatch(x-SHORT(stpb->width)/2,
                            DM_MATRIXY - WI_SPACINGY,
                            FB,
                            bstar);

                V_DrawScaledPatch(DM_MATRIXX-SHORT(stpb->width)/2,
                            y,
                            FB,
                            star);
            }
        }
        else
        {
            // V_DrawPatch(x-SHORT(bp[i]->width)/2,
            //   DM_MATRIXY - WI_SPACINGY, FB, bp[i]);
            // V_DrawPatch(DM_MATRIXX-SHORT(bp[i]->width)/2,
            //   y, FB, bp[i]);
        }
        x += DM_SPACINGX;
        y += WI_SPACINGY;
    }

    // draw stats
    y = DM_MATRIXY+10;
    w = SHORT(num[0]->width);

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        x = DM_MATRIXX + DM_SPACINGX;

        if (playeringame[i])
        {
            for (j=0 ; j<MAXPLAYERS ; j++)
            {
                if (playeringame[j])
                    WI_drawNum(x+w, y, dm_frags[i][j], 2);

                x += DM_SPACINGX;
            }
            WI_drawNum(DM_TOTALSX+w, y, dm_totals[i], 2);
        }
        y += WI_SPACINGY;
    }
}

static int      cnt_frags[MAXPLAYERS];
static int      dofrags;
static int      ng_state;

void WI_initNetgameStats(void)
{

    int i;

    state = StatCount;
    acceleratestage = 0;
    ng_state = 1;

    cnt_pause = TICRATE;

    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (!playeringame[i])
            continue;

        cnt_kills[i] = cnt_items[i] = cnt_secret[i] = cnt_frags[i] = 0;

        dofrags += ST_PlayerFrags(i);
    }

    dofrags = !!dofrags;

    WI_initAnimatedBack();
}



void WI_updateNetgameStats(void)
{

    int         i;
    int         fsum;

    boolean     stillticking;

    WI_updateAnimatedBack();

    if (acceleratestage && ng_state != 10)
    {
        acceleratestage = 0;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_kills[i] = (plrs[i].skills * 100) / wbs->maxkills;
            cnt_items[i] = (plrs[i].sitems * 100) / wbs->maxitems;
            cnt_secret[i] = (plrs[i].ssecret * 100) / wbs->maxsecret;

            if (dofrags)
                cnt_frags[i] = ST_PlayerFrags(i);
        }
        S_StartSound(0, sfx_barexp);
        ng_state = 10;
    }

    if (ng_state == 2)
    {
        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_kills[i] += 2;

            if (cnt_kills[i] >= (plrs[i].skills * 100) / wbs->maxkills)
                cnt_kills[i] = (plrs[i].skills * 100) / wbs->maxkills;
            else
                stillticking = true;
        }

        if (!stillticking)
        {
            S_StartSound(0, sfx_barexp);
            ng_state++;
        }
    }
    else if (ng_state == 4)
    {
        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_items[i] += 2;
            if (cnt_items[i] >= (plrs[i].sitems * 100) / wbs->maxitems)
                cnt_items[i] = (plrs[i].sitems * 100) / wbs->maxitems;
            else
                stillticking = true;
        }
        if (!stillticking)
        {
            S_StartSound(0, sfx_barexp);
            ng_state++;
        }
    }
    else if (ng_state == 6)
    {
        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_secret[i] += 2;

            if (cnt_secret[i] >= (plrs[i].ssecret * 100) / wbs->maxsecret)
                cnt_secret[i] = (plrs[i].ssecret * 100) / wbs->maxsecret;
            else
                stillticking = true;
        }

        if (!stillticking)
        {
            S_StartSound(0, sfx_barexp);
            ng_state += 1 + 2*!dofrags;
        }
    }
    else if (ng_state == 8)
    {
        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        stillticking = false;

        for (i=0 ; i<MAXPLAYERS ; i++)
        {
            if (!playeringame[i])
                continue;

            cnt_frags[i] += 1;

            if (cnt_frags[i] >= (fsum = ST_PlayerFrags(i)))
                cnt_frags[i] = fsum;
            else
                stillticking = true;
        }

        if (!stillticking)
        {
            S_StartSound(0, sfx_pldeth);
            ng_state++;
        }
    }
    else if (ng_state == 10)
    {
        if (acceleratestage)
        {
            S_StartSound(0, sfx_sgcock);
            if ( gamemode == commercial )
                WI_initNoState();
            else
                WI_initShowNextLoc();
        }
    }
    else if (ng_state & 1)
    {
        if (!--cnt_pause)
        {
            ng_state++;
            cnt_pause = TICRATE;
        }
    }
}



void WI_drawNetgameStats(void)
{
    int         i;
    int         x;
    int         y;
    int         pwidth = SHORT(percent->width);

    byte*       colormap;   //added:08-02-98: remap STBP0 to player color

    WI_slamBackground();

    // draw animated background
    WI_drawAnimatedBack();

    WI_drawLF();

    if (cv_fonttype.value == FONT_MAR2K) {
        // draw stat titles (top line)
        V_DrawScaledPatch(NG_STATSX + NG_SPACINGX - SHORT(kills->width),
            NG_STATSY, FB, kills);

        V_DrawScaledPatch(NG_STATSX + 2 * NG_SPACINGX - SHORT(items->width),
            NG_STATSY, FB, items);

        V_DrawScaledPatch(NG_STATSX + 3 * NG_SPACINGX - SHORT(secret->width),
            NG_STATSY, FB, secret);

        if (dofrags)
            V_DrawScaledPatch(NG_STATSX + 4 * NG_SPACINGX - SHORT(frags->width),
                NG_STATSY, FB, frags);

        // draw stats
        y = NG_STATSY + SHORT(kills->height);
    }
    else {
        // draw stat titles (top line)
        V_DrawScaledPatch(NG_STATSX + NG_SPACINGX - SHORT(killsxmas->width),
            NG_STATSY, FB, killsxmas);

        V_DrawScaledPatch(NG_STATSX + 2 * NG_SPACINGX - SHORT(itemsxmas->width),
            NG_STATSY, FB, itemsxmas);

        V_DrawScaledPatch(NG_STATSX + 3 * NG_SPACINGX - SHORT(secretxmas->width),
            NG_STATSY, FB, secretxmas);

        if (dofrags)
            V_DrawScaledPatch(NG_STATSX + 4 * NG_SPACINGX - SHORT(fragsxmas->width),
                NG_STATSY, FB, fragsxmas);

        // draw stats
        y = NG_STATSY + SHORT(killsxmas->height);
    }

    //added:08-02-98: p[i] replaced by stpb (see WI_loadData for more)
    for (i=0 ; i<MAXPLAYERS ; i++)
    {
        if (!playeringame[i])
            continue;

        x = NG_STATSX;
        if (players[i].skincolor==0)
            colormap = colormaps;       //no translation table for green guy
        else
            colormap = (byte *) translationtables - 256 + (players[i].skincolor<<8);

        V_DrawMappedPatch(x-SHORT(stpb->width), y, FB, stpb, colormap);

        if (i == me)
            V_DrawScaledPatch(x-SHORT(stpb->width), y, FB, star);

        x += NG_SPACINGX;
        WI_drawPercent(x-pwidth, y+10, cnt_kills[i]);   x += NG_SPACINGX;
        WI_drawPercent(x-pwidth, y+10, cnt_items[i]);   x += NG_SPACINGX;
        WI_drawPercent(x-pwidth, y+10, cnt_secret[i]);  x += NG_SPACINGX;

        if (dofrags)
            WI_drawNum(x, y+10, cnt_frags[i], -1);

        y += WI_SPACINGY;
    }

}

static int      sp_state;

void WI_initStats(void)
{
    state = StatCount;
    acceleratestage = 0;
    sp_state = 1;
    cnt_kills[0] = cnt_items[0] = cnt_secret[0] = -1;
    cnt_time = cnt_par = -1;
    cnt_pause = TICRATE;

    WI_initAnimatedBack();
}

void WI_updateStats(void)
{

    WI_updateAnimatedBack();

    if (acceleratestage && sp_state != 10)
    {
        acceleratestage = 0;
        cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
        cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
        cnt_secret[0] = (plrs[me].ssecret * 100) / wbs->maxsecret;
        cnt_time = plrs[me].stime / TICRATE;
        cnt_par = wbs->partime / TICRATE;
        S_StartSound(0, sfx_barexp);
        sp_state = 10;
    }

    if (sp_state == 2)
    {
        cnt_kills[0] += 2;

        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        if (cnt_kills[0] >= (plrs[me].skills * 100) / wbs->maxkills)
        {
            cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
            S_StartSound(0, sfx_barexp);
            sp_state++;
        }
    }
    else if (sp_state == 4)
    {
        cnt_items[0] += 2;

        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        if (cnt_items[0] >= (plrs[me].sitems * 100) / wbs->maxitems)
        {
            cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
            S_StartSound(0, sfx_barexp);
            sp_state++;
        }
    }
    else if (sp_state == 6)
    {
        cnt_secret[0] += 2;

        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        if (cnt_secret[0] >= (plrs[me].ssecret * 100) / wbs->maxsecret)
        {
            cnt_secret[0] = (plrs[me].ssecret * 100) / wbs->maxsecret;
            S_StartSound(0, sfx_barexp);
            sp_state++;
        }
    }

    else if (sp_state == 8)
    {
        if (!(bcnt&3))
            S_StartSound(0, sfx_pistol);

        cnt_time += 3;

        if (cnt_time >= plrs[me].stime / TICRATE)
            cnt_time = plrs[me].stime / TICRATE;

        cnt_par += 3;

        if (cnt_par >= wbs->partime / TICRATE)
        {
            cnt_par = wbs->partime / TICRATE;

            if (cnt_time >= plrs[me].stime / TICRATE)
            {
                S_StartSound(0, sfx_barexp);
                sp_state++;
            }
        }
    }
    else if (sp_state == 10)
    {
        if (acceleratestage)
        {
            S_StartSound(0, sfx_sgcock);

            if (gamemode == commercial)
                WI_initNoState();
            else
                WI_initShowNextLoc();
        }
    }
    else if (sp_state & 1)
    {
        if (!--cnt_pause)
        {
            sp_state++;
            cnt_pause = TICRATE;
        }
    }

}

void WI_drawStats(void)
{
    // line height
    int lh;

    lh = (3*SHORT(num[0]->height))/2;

    WI_slamBackground();

    // draw animated background
    WI_drawAnimatedBack();

    WI_drawLF();

    if (cv_fonttype.value == FONT_MAR2K) {
        V_DrawScaledPatch(SP_STATSX, SP_STATSY, FB, kills);
        WI_drawPercent(BASEVIDWIDTH - SP_STATSX, SP_STATSY, cnt_kills[0]);

        V_DrawScaledPatch(SP_STATSX, SP_STATSY + lh, FB, items);
        WI_drawPercent(BASEVIDWIDTH - SP_STATSX, SP_STATSY + lh, cnt_items[0]);

        V_DrawScaledPatch(SP_STATSX, SP_STATSY + 2 * lh, FB, sp_secret);
        WI_drawPercent(BASEVIDWIDTH - SP_STATSX, SP_STATSY + 2 * lh, cnt_secret[0]);

        V_DrawScaledPatch(SP_TIMEX, SP_TIMEY, FB, time);
        WI_drawTime(BASEVIDWIDTH / 2 - SP_TIMEX, SP_TIMEY, cnt_time);
    }
    else {
        V_DrawScaledPatch(SP_STATSX, SP_STATSY, FB, killsxmas);
        WI_drawPercent(BASEVIDWIDTH - SP_STATSX, SP_STATSY, cnt_kills[0]);

        V_DrawScaledPatch(SP_STATSX, SP_STATSY + lh, FB, itemsxmas);
        WI_drawPercent(BASEVIDWIDTH - SP_STATSX, SP_STATSY + lh, cnt_items[0]);

        V_DrawScaledPatch(SP_STATSX, SP_STATSY + 2 * lh, FB, secretxmas);
        WI_drawPercent(BASEVIDWIDTH - SP_STATSX, SP_STATSY + 2 * lh, cnt_secret[0]);

        V_DrawScaledPatch(SP_TIMEX, SP_TIMEY, FB, timexmas);
        WI_drawTime(BASEVIDWIDTH / 2 - SP_TIMEX, SP_TIMEY, cnt_time);
    }

    if (wbs->epsd < 3)
    {
        if (cv_fonttype.value == FONT_MAR2K) {
            V_DrawScaledPatch(BASEVIDWIDTH / 2 + SP_TIMEX, SP_TIMEY, FB, par);
        }
        else {
            V_DrawScaledPatch(BASEVIDWIDTH / 2 + SP_TIMEX, SP_TIMEY, FB, parxmas);
        }
        WI_drawTime(BASEVIDWIDTH - SP_TIMEX, SP_TIMEY, cnt_par);
    }

}

void WI_checkForAccelerate(void)
{
    int   i;
    player_t  *player;

    // check for button presses to skip delays
    for (i=0, player = players ; i<MAXPLAYERS ; i++, player++)
    {
        if (playeringame[i])
        {
            if (player->cmd.buttons & BT_ATTACK)
            {
                if (!player->attackdown)
                    acceleratestage = 1;
                player->attackdown = true;
            }
            else
                player->attackdown = false;
            if (player->cmd.buttons & BT_USE)
            {
                if (!player->usedown)
                    acceleratestage = 1;
                player->usedown = true;
            }
            else
                player->usedown = false;
        }
    }
}



// Updates stuff each tick
void WI_Ticker(void)
{
    // counter for general background animation
    bcnt++;

    if (bcnt == 1)
    {
        // intermission music
        if ( gamemode == commercial )
          S_ChangeMusic(mus_dm2int, true);
        else
          S_ChangeMusic(mus_inter, true);
    }

    WI_checkForAccelerate();

    switch (state)
    {
      case StatCount:
        if (cv_deathmatch.value) WI_updateDeathmatchStats();
        else if (multiplayer) WI_updateNetgameStats();
        else WI_updateStats();
        break;

      case ShowNextLoc:
        WI_updateShowNextLoc();
        break;

      case NoState:
        WI_updateNoState();
        break;
    }

}


void WI_loadData(void)
{
    int         i;
    int         j;
    anim_t*     a;
    char        name[9];

    // choose the background of the intermission
       if (gamemap == 1) { // Tails 12-02-99
            strcpy(bgname, "TITLEPIC"); 
       } else {
           strcpy(bgname, "INTERPIC");
       }

    // background stored in backbuffer
    V_DrawScaledPatch(0, 0, 1, W_CachePatchName(bgname, PU_CACHE));

        NUMCMAPS = 32;
        lnames = (patch_t **) Z_Malloc(sizeof(patch_t*) * NUMCMAPS,
                                       PU_STATIC, 0);
        for (i=0 ; i<NUMCMAPS ; i++)
        {
            sprintf(name, "CWILV%2.2d", i);
            lnames[i] = W_CachePatchName(name, PU_STATIC);
        }

    // Load font (both March2K and Xmas)
    wiminus = W_CachePatchName("WIMINUS", PU_STATIC);
    wiminusxmas = W_CachePatchName("XIMINUS", PU_STATIC);

    for (i=0;i<10;i++)
    {
         // numbers 0-9
        sprintf(name, "WINUM%d", i);
        num[i] = W_CachePatchName(name, PU_STATIC);
    }


    for (i = 0; i < 10; i++)
    {
        // numbers 0-9
        sprintf(name, "XINUM%d", i);
        numxmas[i] = W_CachePatchName(name, PU_STATIC);
    }

    percent = W_CachePatchName("WIPCNT", PU_STATIC);
    percentxmas = W_CachePatchName("XIPCNT", PU_STATIC);
    finished = W_CachePatchName("WIF", PU_STATIC);
    entering = W_CachePatchName("WIENTER", PU_STATIC);

    kills = W_CachePatchName("WIOSTK", PU_STATIC);
    secret = W_CachePatchName("WIOSTS", PU_STATIC);
    sp_secret = W_CachePatchName("WISCRT2", PU_STATIC);
    items = W_CachePatchName("WIOSTI", PU_STATIC);
    frags = W_CachePatchName("WIFRGS", PU_STATIC);

    killsxmas = W_CachePatchName("XIOSTK", PU_STATIC);
    secretxmas = W_CachePatchName("XIOSTS", PU_STATIC);
    itemsxmas = W_CachePatchName("XIOSTI", PU_STATIC);
    fragsxmas = W_CachePatchName("XIFRGS", PU_STATIC);

    colon = W_CachePatchName("WICOLON", PU_STATIC);
    colonxmas = W_CachePatchName("XICOLON", PU_STATIC);

    time = W_CachePatchName("WITIME", PU_STATIC);
    timexmas = W_CachePatchName("XITIME", PU_STATIC);

    sucks = W_CachePatchName("WISUCKS", PU_STATIC);

    par = W_CachePatchName("WIPAR", PU_STATIC);
    parxmas = W_CachePatchName("XIPAR", PU_STATIC);

    killers = W_CachePatchName("WIKILRS", PU_STATIC);
    victims = W_CachePatchName("WIVCTMS", PU_STATIC);
    total = W_CachePatchName("WIMSTT", PU_STATIC);


    star = W_CachePatchName("STFST01", PU_STATIC);
    bstar = W_CachePatchName("STFDEAD0", PU_STATIC);
    stpb = W_CachePatchName("STPB0", PU_STATIC);
}

void WI_unloadData(void)
{
    int         i;
    int         j;

    //faB: never Z_ChangeTag() a pointer returned by W_CachePatchxxx()
    //     it doesn't work and is unecessary
    if (rendermode==render_soft)
    {
    Z_ChangeTag(wiminus, PU_CACHE);
    Z_ChangeTag(wiminusxmas, PU_CACHE);

    for (i=0 ; i<10 ; i++)
        Z_ChangeTag(num[i], PU_CACHE);

    for (i = 0; i < 10; i++)
        Z_ChangeTag(numxmas[i], PU_CACHE);

    if (gamemode == commercial)
    {
        for (i=0 ; i<NUMCMAPS ; i++)
            Z_ChangeTag(lnames[i], PU_CACHE);
    }
    else
    {
        Z_ChangeTag(yah[0], PU_CACHE);
        Z_ChangeTag(yah[1], PU_CACHE);

        Z_ChangeTag(splat, PU_CACHE);

        for (i=0 ; i<NUMMAPS ; i++)
            Z_ChangeTag(lnames[i], PU_CACHE);

        if (wbs->epsd < 3)
        {
            for (j=0;j<NUMANIMS[wbs->epsd];j++)
            {
                if (wbs->epsd != 1 || j != 8)
                    for (i=0;i<anims[wbs->epsd][j].nanims;i++)
                        Z_ChangeTag(anims[wbs->epsd][j].p[i], PU_CACHE);
            }
        }
    }
    }

    Z_Free(lnames);

    if (rendermode==render_soft)
    {
    Z_ChangeTag(percent, PU_CACHE);
    Z_ChangeTag(percentxmas, PU_CACHE);
    Z_ChangeTag(colon, PU_CACHE);
    Z_ChangeTag(colonxmas, PU_CACHE);
    Z_ChangeTag(finished, PU_CACHE);
    Z_ChangeTag(entering, PU_CACHE);

    Z_ChangeTag(kills, PU_CACHE);
    Z_ChangeTag(secret, PU_CACHE);
    Z_ChangeTag(sp_secret, PU_CACHE);
    Z_ChangeTag(items, PU_CACHE);
    Z_ChangeTag(frags, PU_CACHE);
    Z_ChangeTag(time, PU_CACHE);
   
    Z_ChangeTag(killsxmas, PU_CACHE);
    Z_ChangeTag(secretxmas, PU_CACHE);
    Z_ChangeTag(itemsxmas, PU_CACHE);
    Z_ChangeTag(fragsxmas, PU_CACHE);
    Z_ChangeTag(timexmas, PU_CACHE);

    Z_ChangeTag(sucks, PU_CACHE);
    Z_ChangeTag(par, PU_CACHE);
    Z_ChangeTag(parxmas, PU_CACHE);

    Z_ChangeTag(victims, PU_CACHE);
    Z_ChangeTag(killers, PU_CACHE);
    Z_ChangeTag(total, PU_CACHE);
        //      Z_ChangeTag(star, PU_CACHE);    //faces
    //  Z_ChangeTag(bstar, PU_CACHE);

    //added:08-02-98: unused, see WI_loadData
    //for (i=0 ; i<MAXPLAYERS ; i++)
    //    Z_ChangeTag(p[i], PU_CACHE);
    //for (i=0 ; i<MAXPLAYERS ; i++)
    //    Z_ChangeTag(bp[i], PU_CACHE);
    }
}

void WI_Drawer (void)
{
    switch (state)
    {
      case StatCount:
        if (cv_deathmatch.value)
        {
            if(cv_teamplay.value)
                WI_drawTeamsStats();
            else
                WI_drawDeathmatchStats();

        }
        else if (multiplayer)
            WI_drawNetgameStats();
        else
            WI_drawStats();
        break;

      case ShowNextLoc:
        WI_drawShowNextLoc();
        break;

      case NoState:
        WI_drawNoState();
        break;
    }
}


void WI_initVariables(wbstartstruct_t* wbstartstruct)
{

    wbs = wbstartstruct;

#ifdef RANGECHECKING
    if (gamemode != commercial)
    {
      if ( gamemode == retail )
        RNGCHECK(wbs->epsd, 0, 3);
      else
        RNGCHECK(wbs->epsd, 0, 2);
    }
    else
    {
        RNGCHECK(wbs->last, 0, 8);
        RNGCHECK(wbs->next, 0, 8);
    }
    RNGCHECK(wbs->pnum, 0, MAXPLAYERS);
    RNGCHECK(wbs->pnum, 0, MAXPLAYERS);
#endif

    acceleratestage = 0;
    cnt = bcnt = 0;
    firstrefresh = 1;
    me = wbs->pnum;
    plrs = wbs->plyr;

    if (!wbs->maxkills)
        wbs->maxkills = 1;

    if (!wbs->maxitems)
        wbs->maxitems = 1;

    if (!wbs->maxsecret)
        wbs->maxsecret = 1;

    if ( gamemode != retail )
      if (wbs->epsd > 2)
        wbs->epsd -= 3;
}

void WI_Start(wbstartstruct_t* wbstartstruct)
{

    WI_initVariables(wbstartstruct);
    WI_loadData();

    if (cv_deathmatch.value)
        WI_initDeathmatchStats();
    else if (multiplayer)
        WI_initNetgameStats();
    else
        WI_initStats();
}
