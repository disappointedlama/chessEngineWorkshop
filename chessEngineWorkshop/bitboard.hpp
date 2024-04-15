#pragma once
#include <iostream>
#include <string>
#define U64 unsigned long long
#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))
#define ones_decrement(bitboard) (bitboard - 1)
#define twos_complement(bitboard) ((~bitboard) + 1)
#define count_bits(bitboard) (std::_Popcount(bitboard))
#define bittest(bitboard, square) ((bool)((bitboard) & (1ULL << (square))))
#define bitscan(bitboard) (index64[((bitboard & twos_complement(bitboard)) * debruijn64) >> 58])
constexpr U64 falseMask = 0ULL;
constexpr U64 trueMask = ~falseMask;
#define U32 unsigned __int32
constexpr U32 falseMask32 = 0;
constexpr U32 trueMask32 = ~0;
constexpr U64 debruijn64 = 0x07EDD5E59A4E28C2;
constexpr int index64[64] = {
   63,  0, 58,  1, 59, 47, 53,  2,
   60, 39, 48, 27, 54, 33, 42,  3,
   61, 51, 37, 40, 49, 18, 28, 20,
   55, 30, 34, 11, 43, 14, 22,  4,
   62, 57, 46, 52, 38, 26, 32, 41,
   50, 36, 17, 19, 29, 10, 13, 21,
   56, 45, 25, 31, 35, 16,  9, 12,
   44, 24, 15,  8, 23,  7,  6,  5
};

void print_bitboard(const U64 bitboard);
const std::string square_coordinates[64] = {
"a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
"a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
"a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
"a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
"a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
"a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
"a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
"a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1"
};