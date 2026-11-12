#pragma once
// PokeKnomiGochi V2 scenery sprites. Drawn by the V2 scenes at 3x or 4x (see layout_v2.h).
// Two kinds:
//  * ROLE sprites use digits, recoloured per zone and time of day from palettes_v2.h:
//      '1' outline  '2' light  '3' mid  '4' dark  '5' trunk     ('.' is transparent)
//    light/mid/dark are mixed from the layer colour the sprite stands on (see v2Shade() in the brief).
//  * FIXED sprites use the usual species.h palette letters (spriteColor), or their own table below.

// ROLE  round tree (meadow, field)
static const char* const SPR_V2_TREE[10] = {  // 10x10
  "...1111...",
  ".11222231.",
  "1222233331",
  "1222333331",
  "1223333341",
  "1333333441",
  ".13333441.",
  "..144441..",
  "....55....",
  "....55....",
};

// ROLE  pine (forest, mountain; snow: light -> snow white)
static const char* const SPR_V2_PINE[12] = {  // 10x12
  "....11....",
  "...1231...",
  "..122331..",
  "...1231...",
  "..122331..",
  ".12223331.",
  "..122331..",
  ".12233341.",
  "1222233341",
  ".11111111.",
  "....55....",
  "....55....",
};

// ROLE  palm (beach), fronds sway: shift rows 0-4 one cell left on alternate beats
static const char* const SPR_V2_PALM[12] = {  // 12x12
  "..11....11..",
  ".1221..1221.",
  "122221112221",
  "1..1222221.1",
  "...1122211..",
  "..121.5.121.",
  "..1...5...1.",
  "......5.....",
  "......5.....",
  ".....5......",
  ".....5......",
  "....555.....",
};

// ROLE  rock (mountain, volcano, cave)
static const char* const SPR_V2_ROCK[5] = {  // 8x5
  "..1111..",
  ".122331.",
  "12233341",
  "12333441",
  "11111111",
};

// ROLE  bare tree (mansion), drawn all in '1'
static const char* const SPR_V2_BARE[11] = {  // 10x11
  "....1.....",
  ".1..1..1..",
  "..1.1.1...",
  "...111..1.",
  "1...11.1..",
  ".1..1111..",
  "..1.11....",
  "...111....",
  "....11....",
  "....11....",
  "...1111...",
};

// two-tone tall grass: 'G' = dark, 'g' = near layer mixed 35% toward dark (snow: G #b9c9de, g white)
static const char* const SPR_V2_TALLGRASS[5] = {  // 12x5
  "..G...G...G.",
  ".Gg..Gg..Gg.",
  ".gG.GgG.GgG.",
  "GgGGgGgGgGgG",
  "gGgGgGgGgGgG",
};

// cloud on the 4 px grid: 'x' = cloud, 's' = cloud_sh
static const char* const SPR_V2_CLOUD[5] = {  // 14x5
  "....xxxx......",
  "..xxxxxxxx.xx.",
  ".xxxxxxxxxxxxx",
  "xxxxxxxxxxxxxx",
  ".ssssssssssss.",
};

// FIXED (species.h letters)  the little house on the field hill
static const char* const SPR_V2_HOUSE[12] = {  // 16x12
  "....kkkkkkkk....",
  "...krrrrrrrrk...",
  "..krrrrrrrrrrk..",
  ".krrrrrrrrrrrrk.",
  "kRRRRRRRRRRRRRRk",
  "kkkkkkkkkkkkkkkk",
  ".kwwwwwwwwwwwwk.",
  ".kwbbwwkkkwbbwk.",
  ".kwbBwwkCkwbBwk.",
  ".kwwwwwkCkwwwwk.",
  ".kyyyyykCkyyyyk.",
  ".kkkkkkkkkkkkkk.",
};

// cave crystal: 't' #8ff0e0, 'T' #3fb8a8
static const char* const SPR_V2_CRYSTAL[6] = {  // 5x6
  "..t..",
  ".tTt.",
  ".tTt.",
  "ttTtt",
  "tTTTt",
  ".TTT.",
};

// beach crab: 'r' #e86f5f, 'w' white, 'k' ink
static const char* const SPR_V2_CRAB[6] = {  // 9x6
  ".r.....r.",
  "rr.....rr",
  ".r.rrr.r.",
  "..rrrrr..",
  ".rwkrkwr.",
  "r.r...r.r",
};

// field bird, wings up: 'k' #18181f
static const char* const SPR_V2_BIRD_UP[3] = {  // 5x3
  "k...k",
  ".k.k.",
  "..k..",
};

// field bird, wings down (flap every 5 frames)
static const char* const SPR_V2_BIRD_DOWN[3] = {  // 5x3
  ".....",
  "k.k.k",
  ".k.k.",
};

// graveyard stone: 'o' #0d0b20, 's' #5a5f7c
static const char* const SPR_V2_TOMB[7] = {  // 6x7
  ".oooo.",
  "osssso",
  "osooso",
  "osssso",
  "osssso",
  "osssso",
  "oooooo",
};

// graveyard cross, same colours
static const char* const SPR_V2_CROSS[7] = {  // 5x7
  "..o..",
  ".oso.",
  "ossso",
  ".oso.",
  ".oso.",
  ".oso.",
  ".ooo.",
};

// forest mushroom: 'r' #e8503a, 'w' white
static const char* const SPR_V2_MUSH[3] = {  // 5x3
  ".rrr.",
  "rwrwr",
  ".www.",
};

// beach shell: 'p' #f08aa4, 'w' white
static const char* const SPR_V2_SHELL[3] = {  // 4x3
  ".pp.",
  "pwwp",
  "pppp",
};

// beach starfish: 'o' #f5863d
static const char* const SPR_V2_STARFISH[4] = {  // 5x4
  "..o..",
  "ooooo",
  ".ooo.",
  "o...o",
};

// FIXED  the moon icon with orange Zz ('v' -> 'o'); a drop-in for SPR_HOME_MOON in V1 too
static const char* const SPR_HOME_MOON_V2[18] = {  // 18x18
  ".............ooooo",
  "......kk.......oo.",
  "....kkffk.....oo..",
  "...kfffyk....ooooo",
  "..kffyyyk.ooo.....",
  ".kffyyyk....o.....",
  ".kfyyyfk...o......",
  "kffyyfffk.ooo.....",
  "kfyyffffk.........",
  "kfywfffffkk..kkk..",
  "kffffffffffkkYYYk.",
  "kfffffffffYYYYYYk.",
  ".kfffffffYYYYYYk..",
  ".kfYffffYYYYYYYk..",
  "..kYYffYYYYYYYk...",
  "...kYYYYYYYYYk....",
  "....kkYYYYYkk.....",
  "......kkkkk.......",
};

// The Mansion zone's mansion, drawn at 4x, outlined by v2Outline() (every empty cell touching it gets ink #0d0b20),
// then moonlight #5a4c8a on the left edge of wall cells. Letters index MANSION_PAL.
// FIXED (MANSION_PAL)  58x34
static const char* const SPR_V2_MANSION[34] = {  // 58x34
  ".........F................................................",
  "........EDD...............................................",
  ".......EDDDD..............................................",
  ".......EDDDD..............................................",
  ".......EDDDD....GGGG.......EDD............................",
  "......EDDDDDD....GG........EDD........GGGG................",
  ".....EDDDDDDDD...GG.......EDDDD........GG.................",
  ".....EDDDDDDDD...GG.......EDDDD........GG.................",
  ".....EDDDDDDDD...GG......EKKKKKD.......GG.................",
  "....EDDDDDDDDDD..GG.....EDKPPPKDD......GG.................",
  "...EDDDDDDDDDDDEEGGEEEEEEDKPPPKDDEEEEEEGGEE...............",
  "...EDDDDDDDDDDDDDDKKKKDEDDKPPPKDDDDKKKKDDDD...............",
  ".....FFFFFFFFFFDDDKNLKDEDDKKKKKDDDDKMMKDDDDD...EDD........",
  ".....AAKKKKKAADDDDKLLKEDDDDDDDDDDDDKMMKDDDDD..EDDDD.......",
  ".....AAKNLLKADDDDDKKKKEDDDDDDDDDDDDKKKKDDDDDDEDDDDDD......",
  ".....AAKLLLKADDDDDDDDEDDDDDDDDDDDDDDDDDDDDDDEDDDDDDDD.....",
  ".....AAKLLLKABFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFDDDDDDDDDD....",
  ".....AAKLLLKABBAAAAAAAAAAAAAAAAAAAAAAAAAAAEDDDDDDDDDDDD...",
  ".....CCKKKKKCBBCKKKKKKKKCCCCCCCCCCKKKKKKKKDDDDDDDDDDDDDD..",
  ".....AAAAAAAABBAKNLKKNLKAAAAAAAAAAKNLKKNLKAFFFFFFFFFFFF...",
  ".....AAKKKKKABBAKLLKKLLKAAAAAAAAAAKLLKKLLKABBAAAAAAAAAA...",
  ".....CCKMMMKCBBCKLLKKLLKCCCCCCCCCCKLLKKLLKCBBKKKKKKKKCC...",
  ".....AAKMMMKABBAKKKKKKKKAAAAAAAAAAKKKKKKKKABBKNLKKMMKAA...",
  ".....AAKMMMKABBAAAAAADDDDDDDDDDDDDDDAAAAAAABBKLLKKMMKAA...",
  ".....CCKMMMKCBBCKKKKKKFFFFFFFFFFFFFKKKKKKKCBBKLLKKMMKCC...",
  ".....AAKKKKKABBAKNLKKNFKAAFAAAFAAAFMMKKNLKABBKKKKKKKKAA...",
  ".....AAKKKKKABBAKLLKKLFKAAFHKHFAAAFMMKKLLKABBAAAAAAAAAA...",
  ".....CCKNLLKCBBCKLLKKLFKCCFHKHFCCCFMMKKLLKCBBKKKKKKKKCC...",
  ".....AAKLLLKABBAKKKKKKFKAAFHKHFAAAFKKKKKKKABBKNLKKNLKAA...",
  ".....AAKLLLKABBAAAAAAAFAAAFHKHFAAAFAAAAAAAABBKLLKKLLKAA...",
  ".....CCKKKKKCBBCCCCCCCFCCCFHKHFCCCFCCCCCCCCBBKLLKKLLKCC...",
  ".....AAAAAAAABBAAAAAAAFAAAFHKHFAAAFAAAAAAAABBKKKKKKKKAA...",
  ".....AAAAAAAABBAAAAAAAFAAAFHKHFAAAFAAAAAAAABBAAAAAAAAAA...",
  ".....CCCCCCCCBBCCCCCFFFFFFFFFFFFFFFFFCCCCCCBBCCCCCCCCCC...",
};
static const uint16_t MANSION_PAL[] = {  // A..P
  /* 'A' */ 0x398C,  // #3d3262
  /* 'B' */ 0x2929,  // #2c2448
  /* 'C' */ 0x314A,  // #342a55
  /* 'D' */ 0x18C7,  // #1e1838
  /* 'E' */ 0x4A10,  // #4e4280
  /* 'F' */ 0x7B55,  // #7a6aa8
  /* 'G' */ 0x2908,  // #2a2244
  /* 'H' */ 0x4963,  // #4a2e1a
  /* 'K' */ 0x0844,  // #0d0b20
  /* 'L' */ 0xF609,  // #f0c04a
  /* 'M' */ 0x1084,  // #141024
  /* 'N' */ 0xFF96,  // #fff0b0
  /* 'P' */ 0xB39E,  // #b070f0
};

// bath time's rubber ducky, drawn at 2x beside the pet while it bathes, bobbing with the bubbles:
// 'k' #1b1b25, 'y' #ffd95e, 'Y' #e0b860, 'o' #f5863d, 'w' white
static const char* const SPR_V2_DUCK[12] = {  // 14x12
  "....kkkk......",
  "...kyyyyk.....",
  "..kywkyyyk....",
  "..kyyyyyyk....",
  "..kyyyyykookk.",
  "...kyyyykoook.",
  "kk..kyyyykkk..",
  "kykkyyyyyyyk..",
  "kyyyyyyyyyyyk.",
  ".kyyyyyyyyyYk.",
  "..kYYYYYYYYk..",
  "...kkkkkkkk...",
};

// FIXED  the home band's games icon (Fedor: "Change the games icon's color from blue to the iconic gray with green hue
// screen from the original gameboy"): 18x18 as the other icons, its own colours (V2_GB_KEY)
static const char *const SPR_V2_HOME_GB[18] = {
  "...kkkkkkkkkkkk...",
  "..kwwwwwwwwwwwwk..",
  "..kwDDDDDDDDDDbk..",
  "..kwDdggggggdDbk..",
  "..kwDdggggggdDbk..",
  "..kwDrggggggdDbk..",
  "..kwDdggggGGdDbk..",
  "..kwDdGGGGGGdDbk..",
  "..kwDdddddddddbk..",
  "..kwbbbbbbbbbbbk..",
  "..kwbpbbbbbbmmbk..",
  "..kwpppbbbmmMMbk..",
  "..kwbpbbbbMMbbbk..",
  "..kwbbbbbbbbbbbk..",
  "..kwbbssbssbbBBk..",
  "..kwbbbbbbbbBBk...",
  "..kBBBBBBBBBBk....",
  "...kkkkkkkkkk.....",
};
struct V2GbKey { char ch; uint32_t rgb; };
static const V2GbKey V2_GB_KEY[13] = {  // grey body, the dark bezel and its red light, the green screen, magenta buttons
  { 'k', 0x1B1B25 }, { 'w', 0xF2F0EC }, { 'b', 0xCDC9C2 }, { 'B', 0xA39E97 }, { 'D', 0x3E3C52 }, { 'd', 0x5E5C74 },
  { 'g', 0xA6BA3C }, { 'G', 0x869C2C }, { 'r', 0xE0463A }, { 'p', 0x2C2C3A }, { 'm', 0xB83A78 }, { 'M', 0x7E2452 },
  { 's', 0x8E8A84 } };
