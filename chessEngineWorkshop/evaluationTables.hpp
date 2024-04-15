#pragma once
#include <array>

static constexpr short SafetyTable[100] = {
	0,  0,   1,   2,   3,   5,   7,   9,  12,  15,
  18,  22,  26,  30,  35,  39,  44,  50,  56,  62,
  68,  75,  82,  85,  89,  97, 105, 113, 122, 131,
 140, 150, 169, 180, 191, 202, 213, 225, 237, 248,
 260, 272, 283, 295, 307, 319, 330, 342, 354, 366,
 377, 389, 401, 412, 424, 436, 448, 459, 471, 483,
 494, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500,
 500, 500, 500, 500, 500, 500, 500, 500, 500, 500
};
static constexpr U64 blackKingZones[64] = {
	50529026,117901061,235802122,471604244,943208488,1886416976,3772833952,3233857600,
	12935430915,30182672135,60365344270,120730688540,241461377080,482922754160,965845508320,827867578560,
	3311470314240,7726764066560,15453528133120,30907056266240,61814112532480,123628225064960,247256450129920,211934100111360,
	847736400445440,1978051601039360,3956103202078720,7912206404157440,15824412808314880,31648825616629760,63297651233259520,54255129628508160,
	217020518514032640,506381209866076160,1012762419732152320,2025524839464304640,4051049678928609280,8102099357857218560,16204198715714437120,13889313184898088960,
	217020518463700992,506381209748635648,1012762419497271296,2025524838994542592,4051049677989085184,8102099355978170368,16204198711956340736,13889313181676863488,
	217020505578799104,506381179683864576,1012762359367729152,2025524718735458304,4051049437470916608,8102098874941833216,16204197749883666432,13889312357043142656,
	217017207043915776,506373483102470144,1012746966204940288,2025493932409880576,4050987864819761152,8101975729639522304,16203951459279044608,13889101250810609664
};
static constexpr U64 whiteKingZones[64] = {
	771,1799,3598,7196,14392,28784,57568,49344,
	197379,460551,921102,1842204,3684408,7368816,14737632,12632256,
	50529027,117901063,235802126,471604252,943208504,1886417008,3772834016,3233857728,
	12935430915,30182672135,60365344270,120730688540,241461377080,482922754160,965845508320,827867578560,
	3311470314240,7726764066560,15453528133120,30907056266240,61814112532480,123628225064960,247256450129920,211934100111360,
	847736400445440,1978051601039360,3956103202078720,7912206404157440,15824412808314880,31648825616629760,63297651233259520,54255129628508160,
	217020518514032640,506381209866076160,1012762419732152320,2025524839464304640,4051049678928609280,8102099357857218560,16204198715714437120,13889313184898088960,
	144962924425773056,362266021672779776,724532043345559552,1449064086691119104,2898128173382238208,5796256346764476416,11592512693528952832,4665941144822087680
};

static constexpr short openingKingTableBlack[64] = {
	7,30,15,0,0,0,30,7,
	0,2,0,0,0,0,2,0,
	-10,-10,-10,-10,-10,-10,-10,-10,
	-10,-10,-10,-10,-10,-10,-10,-10,
	-10,-10,-10,-10,-10,-10,-10,-10,
	-10,-10,-10,-10,-10,-10,-10,-10,
	-10,-10,-10,-10,-10,-10,-10,-10,
	-10,-10,-10,-10,-10,-10,-10,-10,
};
static constexpr short openingKingTableWhite[64] = {
	-10,-10,-10,-10,-10,-10,-10,-10,
	-10,-10,-10,-10,-10,-10,-10,-10,
	-10,-10,-10,-10,-10,-10,-10,-10,
	-10,-10,-10,-10,-10,-10,-10,-10,
	-10,-10,-10,-10,-10,-10,-10,-10,
	-10,-10,-10,-10,-10,-10,-10,-10,
	0,2,0,0,0,0,2,0,
	7,30,15,0,0,0,30,7,
};
static constexpr short openingRookTableWhite[64] = {
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,5,5,5,5,0,0,
	10,0,20,30,30,20,0,10,
};
static constexpr short openingRookTableBlack[64] = {
	10,0,20,30,30,20,0,10,
	0,0,5,5,5,5,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
};
static constexpr short openingKnightsTable[64] = {
	-50,-40,-30,-30,-30,-30,-40,-50,
	-40,-20,0,0,0,0,-20,-40,
	-30,0,10,15,15,10,0,-30,
	-30,0,15,20,20,15,0,-30,
	-30,0,15,20,20,15,0,-30,
	-30,0,10,15,15,10,0,-30,
	-40,-20,0,0,0,0,-20,-40,
	-50,-30,-30,-30,-30,-30,-40,-50,
};
static constexpr short openingBishopTableWhite[64] = {
	-20,-10,-10,-10,-10,-10,-10,-20,
	-10,  0,  0,  0,  0,  0,  0,-10,
	-10,  0,  5, 10, 10,  5,  0,-10,
	-10,  5,  5, 10, 10,  5,  5,-10,
	-10,  0, 10, 10, 10, 10,  0,-10,
	-10, 10, 10, 10, 10, 10, 10,-10,
	-10,  5,  0,  0,  0,  0,  5,-10,
	-20,-10,-10,-10,-10,-10,-10,-20
};
static constexpr short openingBishopTableBlack[64] = {
	-20,-10,-10,-10,-10,-10,-10,-20
	- 10,  5,  0,  0,  0,  0,  5,-10,
	-10, 10, 10, 10, 10, 10, 10,-10,
	-10,  0, 10, 10, 10, 10,  0,-10,
	-10,  5,  5, 10, 10,  5,  5,-10,
	-10,  0,  5, 10, 10,  5,  0,-10,
	-10,  0,  0,  0,  0,  0,  0,-10,
	-20,-10,-10,-10,-10,-10,-10,-20,
};
static constexpr short openingPawnTableWhite[64] = {
	0,  0,  0,  0,  0,  0,  0,  0,
	50, 50, 50, 50, 50, 50, 50, 50,
	10, 10, 20, 30, 30, 20, 10, 10,
	5,  5, 10, 25, 25, 10,  5,  5,
	0,  0,  0, 20, 20,  0,  0,  0,
	5, -5,-10,  0,  0,-10, -5,  5,
	5, 10, 10,-20,-20, 10, 10,  5,
	0,  0,  0,  0,  0,  0,  0,  0
};
static constexpr short openingPawnTableBlack[64] = {
	0,  0,  0,  0,  0,  0,  0,  0,
	5, 10, 10,-20,-20, 10, 10,  5,
	5, -5,-10,  0,  0,-10, -5,  5,
	0,  0,  0, 20, 20,  0,  0,  0,
	5,  5, 10, 25, 25, 10,  5,  5,
	10, 10, 20, 30, 30, 20, 10, 10,
	50, 50, 50, 50, 50, 50, 50, 50,
	0,  0,  0,  0,  0,  0,  0,  0
};
static constexpr short openingQueenTableWhite[64] = {
	-20,-10,-10, -5, -5,-10,-10,-20,
	-10,  0,  0,  0,  0,  0,  0,-10,
	-10,  0,  5,  5,  5,  5,  0,-10,
	 -5,  0,  5,  5,  5,  5,  0, -5,
	  0,  0,  5,  5,  5,  5,  0, -5,
	-10,  5,  5,  5,  5,  5,  0,-10,
	-10,  0,  5,  0,  0,  0,  0,-10,
	-20,-10,-10, -5, -5,-10,-10,-20
};
static constexpr short openingQueenTableBlack[64] = {
	-20,-10,-10, -5, -5,-10,-10,-20,
	-10,  0,  5,  0,  0,  0,  0,-10,
	-10,  0,  5,  5,  5,  5,  5,-10,
	-10,  0,  5,  5,  5,  5,  0,-10,
	 -5,  0,  5,  5,  5,  5,  0, -5,
	  0,  0,  5,  5,  5,  5,  0, -5,
	-10,  0,  0,  0,  0,  0,  0,-10,
	-20,-10,-10, -5, -5,-10,-10,-20
};
static constexpr short endgameKingTable[64] = {
	-20,-15,-15,-15,-15,-15,-15,-20,
	-15,-5,-5,-5,-5,-5,-5,-15,
	-15,-5,5,5,5,5,-5,-15,
	-15,-5,5,5,5,5,-5,-15,
	-15,-5,5,5,5,5,-5,-15,
	-15,-5,5,5,5,5,-5,-15,
	-15,-5,-5,-5,-5,-5,-5,-15,
	-20,-15,-15,-15,-15,-15,-15,-20
};
static constexpr short endgamePawnTableWhite[64] = {
	0,0,0,0,0,0,0,0,
	100,100,100,100,100,100,100,100,
	80,80,80,80,80,80,80,80,
	60,60,60,60,60,60,60,60,
	40,40,40,40,40,40,40,40,
	20,20,20,20,20,20,20,20,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
};
static constexpr short endgamePawnTableBlack[64] = {
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	20,20,20,20,20,20,20,20,
	40,40,40,40,40,40,40,40,
	60,60,60,60,60,60,60,60,
	80,80,80,80,80,80,80,80,
	100,100,100,100,100,100,100,100,
	0,0,0,0,0,0,0,0,
};
static short basePieceValue[6] = { 100,305,333,563,950,0 };
static constexpr short basePiece[16] = { 0,1,2,3,4,5,0,1,2,3,4,5,0,0,0,0 };
static const std::unordered_map<short, short> pawn_shield = {
	{0,0},
	{1,2},{2,2},{4,2},{8,2},{16,2},{32,2},
	{3,7},{6,7},
	{5,3},{40,3},
	{34,5},{10,5},{20,5},{17,5},
	{36,4},{18,4},{9,4},
	{7,25},
	{39,20},{53,20},
	{21,15},{14,15},{46,15},
	{35,10}
};
static constexpr std::array<short, 10> rooks_semi_open = { 5,15,15, 15, 15, 15, 15, 15, 15, 15 };