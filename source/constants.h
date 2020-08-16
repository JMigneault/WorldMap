
// AABBGGRR: NOTE: alpha is "opaqueness" so must be set to 255 to see colors
static const int dark = (0xFF << 24) | (0x00 << 16) | (0x11 << 8) | (0x30 << 0);
static const int light = (0xFF << 24) | (0x00 << 16) | (0x30 << 8) | (0xC0 << 0);
static const int cutscene = (0xFF << 24) | (0xFF << 16) | (0xFF << 8) | (0xFF << 0);
static const int out = (0xFF << 24) | (0x33 << 16) | (0x99 << 8) | (0x44 << 0);

static const int highlightColor = (0x55 << 24) | (0xFF << 16) | (0xFF << 8) | (0xFF << 0);

// todo: will need object representation for player eventually (to manage sprites for example)
static const int playerColor = (0xFF << 24) | (0xA0 << 16) | (0x00 << 8) | (0xA0 << 0);

// TEMP: eventually should be determined by art assets themselves
// in the meantime set at resolution of a tile for a 16 x 9 tilemap on 1920x1080 display
static const int TILERES = 120;

const float TOWNWIDTH = 2.5;
const float TOWNHEIGHT = 2.5;

const float PLAYERWIDTH = 1.7;
// TEMP: not currently used
const float PLAYERHEIGHT = 2.6;
const int PLAYERSPEED = 7;
const int PLAYERPOSX = 5;
const int PLAYERPOSY = 50;

const float WORLDCAMERAWIDTH = 48.0;
const float WORLDCAMERAHEIGHT = 27.0;

const float PUZZLECAMERAWIDTH = 16.0;
const float PUZZLECAMERAHEIGHT = 9.0;

const int SCREENWIDTH = 1920;
const int SCREENHEIGHT = 1080;

const float ALLTOWNPOSX[] = {5.0, 90.0, 70.0, 2.5};
const float ALLTOWNPOSY[] = {50.0, 40.0, 5.0, 22.5};

// NOTE: filepaths cannot be longer than 100 chars
const int MAXFILEPATHLENGTH = 100;
// Top level path
char *TLPATH = ".\\";

const int NUMPLAYERANIMATIONS = 4;
const int PLAYERANIMATIONSIZES[NUMPLAYERANIMATIONS] = {4, 4, 4, 4};
const char *PLAYERBMPS[] = {"assets\\walkleft1.bmp", "assets\\walkleft2.bmp", "assets\\walkleft3.bmp", "assets\\walkleft4.bmp",
"assets\\walkup1.bmp", "assets\\walkup2.bmp", "assets\\walkup3.bmp", "assets\\walkup4.bmp",
"assets\\walkright1.bmp", "assets\\walkright2.bmp", "assets\\walkright3.bmp", "assets\\walkright4.bmp",
"assets\\walkdown1.bmp", "assets\\walkdown2.bmp", "assets\\walkdown3.bmp", "assets\\walkdown4.bmp",
};
const int PLAYERPIXELCENTERSX[] = {326, 483, 323, 274,
322, 274, 322, 289,
281, 281, 281, 284,
411, 404, 412, 366
};
const int PLAYERPIXELCENTERSY[] = {430, 425, 431, 424,
348, 348, 348, 348,
430, 424, 431, 425,
421, 430, 421, 429
};

const float PLAYERPIXELSPERUNIT = 300;
const int PLAYERFRAMESPERSPRITE = 15;
