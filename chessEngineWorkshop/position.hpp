#pragma once
#include <vector>
#include <chrono>
#include <cassert>
#include <unordered_map>
#include <algorithm>
#include "hashKeys.hpp"
#include "move.hpp"
#include "pawnAttacks.hpp"
#include "knightAttacks.hpp"
#include "bishopAttacks.hpp"
#include "rookAttacks.hpp"
#include "kingAttacks.hpp"
#include "masks.hpp"
#include "evaluationTables.hpp"
enum {
	a8, b8, c8, d8, e8, f8, g8, h8,
	a7, b7, c7, d7, e7, f7, g7, h7,
	a6, b6, c6, d6, e6, f6, g6, h6,
	a5, b5, c5, d5, e5, f5, g5, h5,
	a4, b4, c4, d4, e4, f4, g4, h4,
	a3, b3, c3, d3, e3, f3, g3, h3,
	a2, b2, c2, d2, e2, f2, g2, h2,
	a1, b1, c1, d1, e1, f1, g1, h1
};
//sides to move
enum { white, black, both };
//castling bits
enum { wk = 1, wq = 2, bk = 4, bq = 8 };
//pieces
enum { P, N, B, R, Q, K, p, n, b, r, q, k };
//ASCII pieces
constexpr char ascii_pieces[] = "PNBRQKpnbrqk";
//convert ascii char pieces to encoded constants
static constexpr int char_pieces(const char piece) {
	switch (piece) {
	case 'P':return P;
	case 'N':return N;
	case 'B':return B;
	case 'R':return R;
	case 'Q':return Q;
	case 'K':return K;
	case 'p':return p;
	case 'n':return n;
	case 'b':return b;
	case 'r':return r;
	case 'q':return q;
	case 'k':return k;
	default:return -1;
	}
};
static constexpr U64 get_queen_attacks(U64 occ, const int sq) {
	return get_bishop_attacks(occ, sq) | get_rook_attacks(occ, sq);
};
static const std::string start_position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
struct Position_Error : std::exception {
	std::string msg;
	Position_Error(std::string msg) {
		this->msg = msg;
	}
	const std::string what() throw() {
		return "Position Error: " + msg;
	}

};
#define timingPosition false
class Position {
	inline bool is_attacked_by_side(const int sq, const bool color) {
		const int offset = 6 * color;
		U64 attacks = get_rook_attacks(occupancies[both], sq) & (bitboards[R + offset] | bitboards[Q + offset]);
		attacks|= get_bishop_attacks(occupancies[both], sq) & (bitboards[B + offset] | bitboards[Q + offset]);
		attacks |= (king_attacks[sq] & bitboards[K + offset]) | (knight_attacks[sq] & bitboards[N + offset]);
		attacks |= (pawn_attacks[!color][sq] & bitboards[offset]);
		return attacks;
	}
	inline U64 get_attacks_by(const U64 color) {

		int offset = 6 & color;
		U64 attacks = king_attacks[bitscan(bitboards[K + offset])];
		int type = N + offset;
		U64 tempKnights = bitboards[type];

		while (tempKnights) {
			const U64 isolated = tempKnights & twos_complement(tempKnights);
			attacks |= knight_attacks[bitscan(isolated)];
			tempKnights = tempKnights & ones_decrement(tempKnights);
		}
		//knight attacks individually
		attacks |= ((((bitboards[p] << 7) & notHFile) | ((bitboards[p] << 9) & notAFile)) & color) | ((((bitboards[P] >> 7) & notAFile) | ((bitboards[P] >> 9) & notHFile)) & ~color);
		//setwise pawn attacks

		U64 rooks_w_queens = bitboards[R + offset] | bitboards[Q + offset];
		while (rooks_w_queens) {
			const U64 isolated = rooks_w_queens & twos_complement(rooks_w_queens);
			attacks |= get_rook_attacks(occupancies[both], bitscan(isolated));
			rooks_w_queens = rooks_w_queens & ones_decrement(rooks_w_queens);
		}
		U64 bishops_w_queens = bitboards[B + offset] | bitboards[Q + offset];
		while (bishops_w_queens) {
			const U64 isolated = bishops_w_queens & twos_complement(bishops_w_queens);
			attacks |= get_bishop_attacks(occupancies[both], bitscan(isolated));
			bishops_w_queens = bishops_w_queens & ones_decrement(bishops_w_queens);
		}

		return attacks;
	}
	inline int get_piece_type_on(const int sq)const {
		return square_board[sq];
	}
	inline int get_piece_type_or_enpassant_on(const int sq) {
		if (sq == enpassant_square && sq != a8) return ~sideMask & p;
		return square_board[sq];
	};

	void try_out_move(std::array<unsigned int,128>& ret, unsigned int move, int& ind) {
		make_move(move);
		if (!is_attacked_by_side(bitscan(bitboards[11 - (int)(sideMask & 6)]), sideMask)) ret[ind++] = move;
		//append move if the king is not attacked after playing the move
		unmake_move();
	}

	int legal_move_generator(std::array<unsigned int,128>& ret, const int kingpos, const U64 enemy_attacks, int ind) {
		ind = get_castles(ret, enemy_attacks, ind);
		const U64 pinned = get_moves_for_pinned_pieces(ret, kingpos, enemy_attacks, ind);
		const U64 not_pinned = ~pinned;
		const U64 enemy_pieces = occupancies[(!side)];
		const U64 valid_targets = (~occupancies[both]) | enemy_pieces;
		unsigned int move;
		int type = N + (int)(6 & sideMask);
		U64 tempKnights = bitboards[type] & not_pinned;
		U64 tempBishops = bitboards[type + 1] & not_pinned;
		U64 tempRooks = bitboards[type + 2] & not_pinned;
		U64 tempQueens = bitboards[type + 3] & not_pinned;
		while (tempKnights) {
			const U64 isolated = tempKnights & twos_complement(tempKnights);
			
			unsigned long sq = bitscan(isolated);
			U64 attacks = knight_attacks[sq] & valid_targets;

			while (attacks) {
				const U64 isolated2 = attacks & twos_complement(attacks);

				unsigned long to = bitscan(isolated2);
				
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, bittest(enemy_pieces,to), false, false, false);
				ret[ind++] = move;

				attacks = attacks & ones_decrement(attacks);
			}
			tempKnights = tempKnights & ones_decrement(tempKnights);
		}

		type++;

		while (tempBishops) {
			const U64 isolated = tempBishops & twos_complement(tempBishops);
			
			unsigned long sq = bitscan(isolated);
			U64 attacks = get_bishop_attacks(occupancies[both], sq) & valid_targets;

			while (attacks) {
				const U64 isolated2 = attacks & twos_complement(attacks);

				unsigned long to = bitscan(isolated2);
				
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, bittest(enemy_pieces,to), false, false, false);
				ret[ind++] = move;

				attacks = attacks & ones_decrement(attacks);
			}
			tempBishops = tempBishops & ones_decrement(tempBishops);
		}

		type++;

		while (tempRooks) {
			const U64 isolated = tempRooks & twos_complement(tempRooks);
			
			unsigned long sq = bitscan(isolated);
			U64 attacks = get_rook_attacks(occupancies[both], sq) & valid_targets;

			while (attacks) {
				const U64 isolated2 = attacks & twos_complement(attacks);

				unsigned long to = bitscan(isolated2);
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, bittest(enemy_pieces,to), false, false, false);
				ret[ind++] = move;

				attacks = attacks & ones_decrement(attacks);
			}
			tempRooks = tempRooks & ones_decrement(tempRooks);
		}

		type++;

		while (tempQueens) {
			const U64 isolated = tempQueens & twos_complement(tempQueens);
			
			unsigned long sq = bitscan(isolated);
			U64 attacks = get_queen_attacks(occupancies[both], sq) & valid_targets;

			while (attacks) {
				const U64 isolated2 = attacks & twos_complement(attacks);

				unsigned long to = bitscan(isolated2);
				
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, bittest(enemy_pieces,to), false, false, false);
				ret[ind++] = move;

				attacks = attacks & ones_decrement(attacks);
			}
			tempQueens = tempQueens & ones_decrement(tempQueens);
		}


		type++;
		U64 attacks = king_attacks[kingpos] & (~(occupancies[(side)] | enemy_attacks));
		while (attacks) {
			const U64 isolated = attacks & twos_complement(attacks);

			unsigned long to = bitscan(isolated);

			move = encode_move(kingpos, to, type, get_piece_type_on(to), no_piece, bittest(enemy_pieces,to), false, false, false);
			ret[ind++] = move;

			attacks = attacks & ones_decrement(attacks);
		}
		return get_legal_pawn_moves(ret, pinned, ind);
	}
	int legal_in_check_move_generator(std::array<unsigned int, 128>& ret, const int kingpos, const U64 enemy_attacks, int ind){
		const U64 pinned = get_pinned_pieces(kingpos, enemy_attacks);
		const U64 enemy_pieces = occupancies[(!side)];
		const U64 checkers = get_checkers(kingpos);
		const bool not_double_check = count_bits(checkers) < 2;
		const U64 valid_targets = (not_double_check) * (get_checking_rays(kingpos) | checkers);
		const U64 valid_pieces = (~pinned) * (not_double_check);
		unsigned int move;
		int type = N + (int)(6 & sideMask);
		U64 tempKnights = bitboards[type] & valid_pieces;
		U64 tempBishops = bitboards[type+1] & valid_pieces;
		U64 tempRooks = bitboards[type+2] & valid_pieces;
		U64 tempQueens = bitboards[type+3] & valid_pieces;
		while (tempKnights) {
			const U64 isolated = tempKnights & twos_complement(tempKnights);
			const int sq = bitscan(isolated);
			U64 attacks = knight_attacks[sq] & valid_targets;

			while (attacks) {

				const U64 isolated2 = attacks & twos_complement(attacks);

				unsigned long to = bitscan(isolated2);
				
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, bittest(enemy_pieces,to), false, false, false);
				ret[ind++]=move;

				attacks = attacks & ones_decrement(attacks);
			}
			tempKnights = tempKnights & ones_decrement(tempKnights);
		}

		type++;

		while (tempBishops) {
			const U64 isolated = tempBishops & twos_complement(tempBishops);
			const int sq = bitscan(isolated);
			U64 attacks = get_bishop_attacks(occupancies[both], sq) & valid_targets;

			while (attacks) {

				const U64 isolated2 = attacks & twos_complement(attacks);

				unsigned long to = bitscan(isolated2);
				
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, bittest(enemy_pieces,to), false, false, false);
				ret[ind++]=move;

				attacks = attacks & ones_decrement(attacks);
			}
			tempBishops = tempBishops & ones_decrement(tempBishops);
		}

		type++;

		while (tempRooks) {
			const U64 isolated = tempRooks & twos_complement(tempRooks);
			const int sq = bitscan(isolated);
			U64 attacks = get_rook_attacks(occupancies[both], sq) & valid_targets;

			while (attacks) {

				const U64 isolated2 = attacks & twos_complement(attacks);

				unsigned long to = bitscan(isolated2);
				
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, bittest(enemy_pieces,to), false, false, false);
				ret[ind++]=move;

				attacks = attacks & ones_decrement(attacks);
			}
			tempRooks = tempRooks & ones_decrement(tempRooks);
		}

		type++;

		while (tempQueens) {
			const U64 isolated = tempQueens & twos_complement(tempQueens);
			const int sq = bitscan(isolated);
			U64 attacks = get_queen_attacks(occupancies[both], sq) & valid_targets;

			while (attacks) {

				const U64 isolated2 = attacks & twos_complement(attacks);

				unsigned long to = bitscan(isolated2);
				
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, bittest(enemy_pieces,to), false, false, false);
				ret[ind++]=move;

				attacks = attacks & ones_decrement(attacks);
			}
			tempQueens = tempQueens & ones_decrement(tempQueens);
		}

		type++;
		pop_bit(occupancies[both], kingpos);
		const U64 enemy_attacks_without_king = get_attacks_by(~sideMask);
		set_bit(occupancies[both], kingpos);
		U64 attacks = king_attacks[kingpos] & (~(enemy_attacks_without_king | occupancies[(side)]));
		while (attacks) {

			const U64 isolated = attacks & twos_complement(attacks);

			unsigned long to = bitscan(isolated);
			
			move = encode_move(kingpos, to, type, get_piece_type_on(to), no_piece, bittest(enemy_pieces,to), false, false, false);
			ret[ind++]=move;

			attacks = attacks & ones_decrement(attacks);
		}
		return in_check_get_legal_pawn_moves(ret, pinned, (not_double_check)*checkers, (not_double_check)*valid_targets, ind);
	}

	int get_legal_captures(std::array<unsigned int,128>& ret) {
		const int kingpos = bitscan(bitboards[K + (int)(sideMask & 6)]);
		const bool in_check = is_attacked_by_side(kingpos, ~sideMask);
		const U64 enemy_attacks = get_attacks_by(~sideMask);
		int ind = (in_check) ? (legal_in_check_capture_gen(ret, enemy_attacks, 0)) : (legal_capture_gen(ret, enemy_attacks, 0));
		return ind;
	}
	int legal_capture_gen(std::array<unsigned int,128>& ret, const U64 enemy_attacks, int ind) {
		const int kingpos = bitscan(bitboards[K + (int)(sideMask & 6)]);
		const U64 pinned = get_captures_for_pinned_pieces(ret, kingpos, enemy_attacks, ind);
		const U64 enemy_pieces = occupancies[(!side)];
		const U64 not_pinned = ~pinned;
		unsigned int move;
		int type = N + (int)(6 & sideMask);
		U64 tempKnights = bitboards[type] & not_pinned;

		while (tempKnights) {
			const U64 isolated = tempKnights & twos_complement(tempKnights);
			
			unsigned long sq = bitscan(isolated);
			U64 attacks = knight_attacks[sq] & enemy_pieces;

			while (attacks) {

				const U64 isolated2 = attacks & twos_complement(attacks);

				const int to = bitscan(isolated2);
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				ret[ind++] = move;

				attacks = attacks & ones_decrement(attacks);
			}
			tempKnights = tempKnights & ones_decrement(tempKnights);
		}
		type++;
		U64 tempBishops = bitboards[type] & not_pinned;

		while (tempBishops) {
			const U64 isolated = tempBishops & twos_complement(tempBishops);
			
			unsigned long sq = bitscan(isolated);
			U64 attacks = get_bishop_attacks(occupancies[both], sq) & enemy_pieces;
			
			while (attacks) {

				const U64 isolated2 = attacks & twos_complement(attacks);

				const int to = bitscan(isolated2);
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				ret[ind++] = move;

				attacks = attacks & ones_decrement(attacks);
			}
			tempBishops = tempBishops & ones_decrement(tempBishops);
		}

		type++;
		U64 tempRooks = bitboards[type] & not_pinned;
		while (tempRooks) {
			const U64 isolated = tempRooks & twos_complement(tempRooks);
			
			unsigned long sq = bitscan(isolated);
			U64 attacks = get_rook_attacks(occupancies[both], sq) & enemy_pieces;

			while (attacks) {

				const U64 isolated2 = attacks & twos_complement(attacks);

				const int to = bitscan(isolated2);
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				ret[ind++] = move;

				attacks = attacks & ones_decrement(attacks);
			}
			tempRooks = tempRooks & ones_decrement(tempRooks);
		}

		type++;
		U64 tempQueens = bitboards[type] & not_pinned;

		while (tempQueens) {
			const U64 isolated = tempQueens & twos_complement(tempQueens);
			
			unsigned long sq = bitscan(isolated);
			U64 attacks = get_queen_attacks(occupancies[both], sq) & enemy_pieces;

			while (attacks) {

				const U64 isolated2 = attacks & twos_complement(attacks);

				const int to = bitscan(isolated2);
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				ret[ind++] = move;

				attacks = attacks & ones_decrement(attacks);
			}
			tempQueens = tempQueens & ones_decrement(tempQueens);
		}

		type++;
		U64 tempKing = bitboards[type];

		U64 attacks = king_attacks[kingpos] & (enemy_pieces & (~enemy_attacks));

		while (attacks) {

			const U64 isolated2 = attacks & twos_complement(attacks);

			const int to = bitscan(isolated2);
			move = encode_move(kingpos, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
			ret[ind++] = move;

			attacks = attacks & ones_decrement(attacks);
		}
		return get_pawn_captures(ret, pinned, ind);
	}
	int legal_in_check_capture_gen(std::array<unsigned int, 128>& ret, const U64 enemy_attacks, int ind) {
		const int kingpos = bitscan(bitboards[K + (int)(sideMask & 6)]);
		const U64 pinned = get_pinned_pieces(kingpos, enemy_attacks);
		const U64 checkers = get_checkers(kingpos);
		const bool not_double_check = count_bits(checkers) < 2;
		const U64 valid_pieces = (~pinned) * (not_double_check);
		unsigned int move;

		int type = N + (int)(6 & sideMask);
		U64 tempKnights = bitboards[type] & valid_pieces;

		while (tempKnights) {
			const U64 isolated = tempKnights & twos_complement(tempKnights);
			
			unsigned long sq = bitscan(isolated);
			U64 attacks = knight_attacks[sq] & checkers;

			while (attacks) {

				const U64 isolated2 = attacks & twos_complement(attacks);

				unsigned long to = bitscan(isolated2);
				
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				try_out_move(ret, move, ind);

				attacks = attacks & ones_decrement(attacks);
			}
			tempKnights = tempKnights & ones_decrement(tempKnights);
		}

		type++;
		U64 tempBishops = bitboards[type] & valid_pieces;

		while (tempBishops) {
			const U64 isolated = tempBishops & twos_complement(tempBishops);
			
			unsigned long sq = bitscan(isolated);
			U64 attacks = get_bishop_attacks(occupancies[both], sq) & checkers;
			
			while (attacks) {

				const U64 isolated2 = attacks & twos_complement(attacks);

				unsigned long to = bitscan(isolated2);
				
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				try_out_move(ret, move, ind);

				attacks = attacks & ones_decrement(attacks);
			}
			tempBishops = tempBishops & ones_decrement(tempBishops);
		}

		type++;
		U64 tempRooks = bitboards[type] & valid_pieces;
		while (tempRooks) {
			const U64 isolated = tempRooks & twos_complement(tempRooks);
			
			unsigned long sq = bitscan(isolated);
			U64 attacks = get_rook_attacks(occupancies[both], sq) & checkers;

			while (attacks) {

				const U64 isolated2 = attacks & twos_complement(attacks);

				unsigned long to = bitscan(isolated2);
				
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, true, false, false, false);

				try_out_move(ret, move, ind);

				attacks = attacks & ones_decrement(attacks);
			}
			tempRooks = tempRooks & ones_decrement(tempRooks);
		}

		type++;
		U64 tempQueens = bitboards[type] & valid_pieces;

		while (tempQueens) {
			const U64 isolated = tempQueens & twos_complement(tempQueens);
			
			unsigned long sq = bitscan(isolated);
			U64 attacks = get_queen_attacks(occupancies[both], sq) & checkers;

			while (attacks) {

				const U64 isolated2 = attacks & twos_complement(attacks);

				unsigned long to = bitscan(isolated2);
				
				move = encode_move(sq, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				try_out_move(ret, move, ind);

				attacks = attacks & ones_decrement(attacks);
			}
			tempQueens = tempQueens & ones_decrement(tempQueens);
		}

		type++;
		U64 tempKing = bitboards[type];

		pop_bit(occupancies[both], kingpos);
		const U64 enemy_attacks_without_king = get_attacks_by(~sideMask);
		set_bit(occupancies[both], kingpos);
		U64 attacks = (king_attacks[kingpos] & occupancies[(!side)]) & (~(enemy_attacks_without_king | occupancies[(side)]));
		while (attacks) {
			const U64 isolated = attacks & twos_complement(attacks);

			unsigned long to = bitscan(isolated);

			move = encode_move(kingpos, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
			ret[ind++]=move;

			attacks = attacks & ones_decrement(attacks);
		}
		return in_check_get_pawn_captures(ret, pinned, (not_double_check)*checkers, ind);
	}

	int get_pawn_captures(std::array<unsigned int,128>& ret, const U64 pinned, int ind) {
		return (sideMask) ? (legal_bpawn_captures(ret, pinned, ind)) : (legal_wpawn_captures(ret, pinned, ind));
	}
	int in_check_get_pawn_captures(std::array<unsigned int,128>& ret, const U64 pinned, const U64 targets, int ind) {
		return (sideMask) ? (in_check_legal_bpawn_captures(ret, pinned, targets, ind)) : (in_check_legal_wpawn_captures(ret, pinned, targets, ind));
	}

	inline int get_legal_pawn_moves(std::array<unsigned int,128>& ret, const U64 pinned, int ind) {
		ind = (sideMask) ? (legal_bpawn_pushes(ret, pinned, ind)) : (legal_wpawn_pushes(ret, pinned, ind));
		ind = (sideMask) ? (legal_bpawn_captures(ret, pinned, ind)) : (legal_wpawn_captures(ret, pinned, ind));
		return ind;
	}
	inline int legal_bpawn_pushes(std::array<unsigned int,128>& ret, const U64 pinned, int ind) {
		U64 valid_targets = ~occupancies[both];
		U64 promoters = bitboards[6] & (~pinned) & rank2;
		U64 push_promotions = (promoters << 8) & valid_targets;
		U64 pushes = ((bitboards[6] & ~(pinned | promoters)) << 8) & valid_targets;
		U64 doublePushes = ((pushes << 8) & rank5) & valid_targets;
		unsigned int move;
		while (push_promotions) {
			const U64 isolated = push_promotions & twos_complement(push_promotions);
			
			unsigned long sq = bitscan(isolated);
			move = encode_move(sq - 8, sq, p, no_piece, n, false, false, false, false);
			ret[ind++]=move;
			set_promotion_type(move, b);
			ret[ind++]=move;
			set_promotion_type(move, r);
			ret[ind++]=move;
			set_promotion_type(move, q);
			ret[ind++]=move;
			push_promotions = push_promotions & ones_decrement(push_promotions);
		}
		while (pushes) {
			const U64 isolated = pushes & twos_complement(pushes);
			
			unsigned long sq = bitscan(isolated);
			move = 0xff6000;
			move |= sq - 8;
			move |= sq << 6;
			ret[ind++]=move;
			pushes = pushes & ones_decrement(pushes);
		}
		while (doublePushes) {
			const U64 isolated = doublePushes & twos_complement(doublePushes);
			
			unsigned long sq = bitscan(isolated);
			move = 0x2ff6000;
			move |= sq - 16;
			move |= sq << 6;
			ret[ind++]=move;
			doublePushes = doublePushes & ones_decrement(doublePushes);
		}
		return ind;
	}
	inline int legal_wpawn_pushes(std::array<unsigned int,128>& ret, const U64 pinned, int ind) {
		U64 valid_targets = ~occupancies[both];
		U64 promoters = bitboards[0] & (~pinned) & rank7;
		U64 push_promotions = (promoters >> 8) & valid_targets;
		U64 pushes = ((bitboards[0] & ~(pinned | promoters)) >> 8) & valid_targets;
		U64 doublePushes = ((pushes >> 8) & rank4) & valid_targets;
		unsigned int move;
		while (push_promotions) {
			const U64 isolated = push_promotions & twos_complement(push_promotions);
			
			unsigned long sq = bitscan(isolated);
			move = encode_move(sq + 8, sq, P, no_piece, N, false, false, false, false);
			ret[ind++]=move;
			set_promotion_type(move, B);
			ret[ind++]=move;
			set_promotion_type(move, R);
			ret[ind++]=move;
			set_promotion_type(move, Q);
			ret[ind++]=move;
			push_promotions = push_promotions & ones_decrement(push_promotions);
		}
		while (pushes) {
			const U64 isolated = pushes & twos_complement(pushes);
			
			unsigned long sq = bitscan(isolated);

			move = 0xff0000;
			move |= sq + 8;
			move |= sq << 6;
			ret[ind++]=move;
			pushes = pushes & ones_decrement(pushes);
		}
		while (doublePushes) {
			const U64 isolated = doublePushes & twos_complement(doublePushes);
			
			unsigned long sq = bitscan(isolated);
			move = 0x2ff0000;
			move |= sq + 16;
			move |= sq << 6;
			ret[ind++]=move;
			doublePushes = doublePushes & ones_decrement(doublePushes);
		}
		return ind;
	}
	inline int legal_bpawn_captures(std::array<unsigned int,128>& ret, const U64 pinned, int ind) {
		const U64 targets = occupancies[white];
		U64 promoters = bitboards[6] & (~pinned) & rank2;
		U64 captures = ((bitboards[6] & ~(pinned | promoters)) << 7) & notHFile & targets;

		ind = legal_b_enpassant(ret, ind);

		while (captures) {
			const U64 isolated = captures & twos_complement(captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq - 7, sq, p, get_piece_type_on(sq), no_piece, true, false, false, false);
			ret[ind++]=move;
			captures = captures & ones_decrement(captures);
		}
		captures = ((bitboards[6] & ~(pinned | promoters)) << 9) & notAFile & targets;
		while (captures) {
			const U64 isolated = captures & twos_complement(captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq - 9, sq, p, get_piece_type_on(sq), no_piece, true, false, false, false);
			ret[ind++]=move;
			captures = captures & ones_decrement(captures);
		}

		U64 promotion_captures = ((promoters) << 7) & notHFile & targets;
		while (promotion_captures) {
			const U64 isolated = promotion_captures & twos_complement(promotion_captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq - 7, sq, p, get_piece_type_on(sq), n, true, false, false, false);
			ret[ind++]=move;
			set_promotion_type(move, b);
			ret[ind++]=move;
			set_promotion_type(move, r);
			ret[ind++]=move;
			set_promotion_type(move, q);
			ret[ind++]=move;
			promotion_captures = promotion_captures & ones_decrement(promotion_captures);
		}
		promotion_captures = ((promoters) << 9) & notAFile & targets;
		while (promotion_captures) {
			const U64 isolated = promotion_captures & twos_complement(promotion_captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq - 9, sq, p, get_piece_type_on(sq), n, true, false, false, false);
			ret[ind++]=move;
			set_promotion_type(move, b);
			ret[ind++]=move;
			set_promotion_type(move, r);
			ret[ind++]=move;
			set_promotion_type(move, q);
			ret[ind++]=move;
			promotion_captures = promotion_captures & ones_decrement(promotion_captures);
		}
		return ind;
	}
	inline int legal_wpawn_captures(std::array<unsigned int,128>& ret, const U64 pinned, int ind) {
		const U64 targets = occupancies[black];
		U64 promoters = bitboards[0] & (~pinned) & rank7;
		U64 captures = ((bitboards[0] & ~(pinned | promoters)) >> 7) & notAFile & targets;
		
		ind = legal_w_enpassant(ret, ind);

		while (captures) {
			const U64 isolated = captures & twos_complement(captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq + 7, sq, P, get_piece_type_on(sq), no_piece, true, false, false, false);
			ret[ind++]=move;
			captures = captures & ones_decrement(captures);
		}
		captures = ((bitboards[0] & ~(pinned | promoters)) >> 9) & notHFile & targets;
		while (captures) {
			const U64 isolated = captures & twos_complement(captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq + 9, sq, P, get_piece_type_on(sq), no_piece, true, false, false, false);
			ret[ind++]=move;
			captures = captures & ones_decrement(captures);
		}
		U64 promotion_captures = ((promoters) >> 7) & notAFile & targets;
		while (promotion_captures) {
			const U64 isolated = promotion_captures & twos_complement(promotion_captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq + 7, sq, P, get_piece_type_on(sq), N, true, false, false, false);
			ret[ind++]=move;
			set_promotion_type(move, B);
			ret[ind++]=move;
			set_promotion_type(move, R);
			ret[ind++]=move;
			set_promotion_type(move, Q);
			ret[ind++]=move;
			promotion_captures = promotion_captures & ones_decrement(promotion_captures);
		}
		promotion_captures = ((promoters) >> 9) & notHFile & targets;
		while (promotion_captures) {
			const U64 isolated = promotion_captures & twos_complement(promotion_captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq + 9, sq, P, get_piece_type_on(sq), N, true, false, false, false);
			ret[ind++]=move;
			set_promotion_type(move, B);
			ret[ind++]=move;
			set_promotion_type(move, R);
			ret[ind++]=move;
			set_promotion_type(move, Q);
			ret[ind++]=move;
			promotion_captures = promotion_captures & ones_decrement(promotion_captures);
		}
		return ind;
	}
	inline int legal_b_enpassant(std::array<unsigned int, 128>& ret, int ind) {
		if (enpassant_square == a8) return ind;
		U64 enpassant_bitboard = 1ULL << enpassant_square;
		U64 pawn_to_be_captured = 1ULL << (enpassant_square - 8);
		U64 enpassant_candidates = pawn_attacks[white][enpassant_square] & bitboards[p];
		int kingpos = bitscan(bitboards[k]);
		unsigned int move = 0;
		const U64 rook_like = bitboards[R] | bitboards[Q];
		const U64 bishop_like = bitboards[B] | bitboards[Q];
		while (enpassant_candidates) {
			const U64 isolated = enpassant_candidates & twos_complement(enpassant_candidates);
			U64 occupanciesAfterMove = occupancies[both] ^ (isolated | pawn_to_be_captured | enpassant_bitboard);
			U64 checking_rooks = get_rook_attacks(occupanciesAfterMove, kingpos) & rook_like;
			U64 checking_bishops = get_bishop_attacks(occupanciesAfterMove, kingpos) & bishop_like;
			bool in_check = checking_rooks | checking_bishops;
			if (!in_check) {
				move = encode_move(bitscan(isolated), enpassant_square, p, P, no_piece, true, false, false, true);
				ret[ind++] = move;
			}
			enpassant_candidates = enpassant_candidates & ones_decrement(enpassant_candidates);
		}
		return ind;
	}
	inline int legal_w_enpassant(std::array<unsigned int, 128>& ret, int ind) {
		if (enpassant_square == a8) return ind;
		U64 enpassant_bitboard = 1ULL << enpassant_square;
		U64 pawn_to_be_captured = 1ULL << (enpassant_square + 8);
		U64 enpassant_candidates = pawn_attacks[black][enpassant_square] & bitboards[P];
		int kingpos = bitscan(bitboards[K]);
		unsigned int move = 0;
		const U64 rook_like = bitboards[r] | bitboards[q];
		const U64 bishop_like = bitboards[b] | bitboards[q];
		while (enpassant_candidates) {
			const U64 isolated = enpassant_candidates & twos_complement(enpassant_candidates);
			U64 occupanciesAfterMove = occupancies[both] ^ (isolated | pawn_to_be_captured | enpassant_bitboard);
			U64 checking_rooks = get_rook_attacks(occupanciesAfterMove, kingpos) & rook_like;
			U64 checking_bishops = get_bishop_attacks(occupanciesAfterMove, kingpos) & bishop_like;
			bool in_check = checking_rooks | checking_bishops;
			if (!in_check) {
				move = encode_move(bitscan(isolated), enpassant_square, P, p, no_piece, true, false, false, true);
				ret[ind++] = move;
			}
			enpassant_candidates = enpassant_candidates & ones_decrement(enpassant_candidates);
		}
		return ind;
	}

	inline int in_check_get_legal_pawn_moves(std::array<unsigned int,128>& ret, const U64 pinned, const U64 targets, const U64 in_check_valid, int ind) {
		ind = (sideMask) ? (in_check_legal_bpawn_pushes(ret, pinned, targets, in_check_valid, ind)) : (in_check_legal_wpawn_pushes(ret, pinned, targets, in_check_valid, ind));
		ind = (sideMask) ? (in_check_legal_bpawn_captures(ret, pinned, targets, ind)) : (in_check_legal_wpawn_captures(ret, pinned, targets, ind));
		return ind;
	}
	inline int in_check_legal_bpawn_pushes(std::array<unsigned int,128>& ret, const U64 pinned, const U64 targets, const U64 in_check_valid, int ind) {
		const U64 valid_targets = ~occupancies[both];
		U64 promoters = bitboards[6] & (~pinned) & rank2;
		U64 push_promotions = (promoters << 8) & in_check_valid & valid_targets;
		U64 pushes = ((bitboards[6] & ~(pinned | promoters)) << 8) & valid_targets;
		U64 doublePushes = ((pushes << 8) & rank5) & in_check_valid & valid_targets;
		pushes = pushes & in_check_valid & valid_targets;
		unsigned int move;
		while (push_promotions) {
			const U64 isolated = push_promotions & twos_complement(push_promotions);
			
			unsigned long sq = bitscan(isolated);
			move = encode_move(sq - 8, sq, p, no_piece, n, false, false, false, false);
			ret[ind++]=move;
			set_promotion_type(move, b);
			ret[ind++]=move;
			set_promotion_type(move, r);
			ret[ind++]=move;
			set_promotion_type(move, q);
			ret[ind++]=move;
			push_promotions = push_promotions & ones_decrement(push_promotions);
		}
		while (pushes) {
			const U64 isolated = pushes & twos_complement(pushes);
			
			unsigned long sq = bitscan(isolated);
			move = encode_move(sq - 8, sq, p, no_piece, no_piece, false, false, false, false);
			ret[ind++]=move;
			pushes = pushes & ones_decrement(pushes);
		}
		while (doublePushes) {
			const U64 isolated = doublePushes & twos_complement(doublePushes);
			
			unsigned long sq = bitscan(isolated);
			move = encode_move(sq - 16, sq, p, no_piece, no_piece, false, true, false, false);
			ret[ind++]=move;
			doublePushes = doublePushes & ones_decrement(doublePushes);
		}
		return ind;
	}
	inline int in_check_legal_wpawn_pushes(std::array<unsigned int,128>& ret, const U64 pinned, const U64 targets, const U64 in_check_valid, int ind) {
		U64 valid_targets = ~occupancies[both];
		U64 promoters = bitboards[0] & (~pinned) & rank7;
		U64 push_promotions = (promoters >> 8) & in_check_valid & valid_targets;
		U64 pushes = ((bitboards[0] & ~(pinned | promoters)) >> 8) & valid_targets;
		U64 doublePushes = ((pushes >> 8) & rank4) & in_check_valid & valid_targets;
		pushes = pushes & in_check_valid & valid_targets;
		unsigned int move;
		while (push_promotions) {
			const U64 isolated = push_promotions & twos_complement(push_promotions);
			
			unsigned long sq = bitscan(isolated);
			move = encode_move(sq + 8, sq, P, no_piece, N, false, false, false, false);
			ret[ind++]=move;
			set_promotion_type(move, B);
			ret[ind++]=move;
			set_promotion_type(move, R);
			ret[ind++]=move;
			set_promotion_type(move, Q);
			ret[ind++]=move;
			push_promotions = push_promotions & ones_decrement(push_promotions);
		}
		while (pushes) {
			const U64 isolated = pushes & twos_complement(pushes);
			
			unsigned long sq = bitscan(isolated);
			move = encode_move(sq + 8, sq, P, no_piece, no_piece, false, false, false, false);
			ret[ind++]=move;
			pushes = pushes & ones_decrement(pushes);
		}
		while (doublePushes) {
			const U64 isolated = doublePushes & twos_complement(doublePushes);
			
			unsigned long sq = bitscan(isolated);
			move = encode_move(sq + 16, sq, P, no_piece, no_piece, false, true, false, false);
			ret[ind++]=move;
			doublePushes = doublePushes & ones_decrement(doublePushes);
		}
		return ind;
	}
	inline int in_check_legal_bpawn_captures(std::array<unsigned int,128>& ret, const U64 pinned, const U64 targets, int ind) {
		U64 promoters = bitboards[6] & (~pinned) & rank2;
		U64 captures = ((bitboards[6] & (~(pinned | promoters))) << 7) & notHFile & targets;

		U64 enpassant = 0ULL;
		set_bit(enpassant, enpassant_square);
		const bool left_enpassant = (enpassant >> 7) & notAFile & (bitboards[p]);
		const bool right_enpassant = (enpassant >> 9) & notHFile & (bitboards[p]);
		if (left_enpassant) {
			unsigned int move = encode_move(enpassant_square - 7, enpassant_square, p, P, no_piece, true, false, false, true);
			try_out_move(ret, move, ind);
		}
		if (right_enpassant) {
			unsigned int move = encode_move(enpassant_square - 9, enpassant_square, p, P, no_piece, true, false, false, true);
			try_out_move(ret, move, ind);
		}

		while (captures) {
			const U64 isolated = captures & twos_complement(captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq - 7, sq, p, get_piece_type_on(sq), no_piece, true, false, false, false);
			ret[ind++] = move;
			captures = captures & ones_decrement(captures);
		}
		captures = ((bitboards[6] & ~(pinned | promoters)) << 9) & notAFile & targets;
		while (captures) {
			const U64 isolated = captures & twos_complement(captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq - 9, sq, p, get_piece_type_on(sq), no_piece, true, false, false, false);
			ret[ind++] = move;
			captures = captures & ones_decrement(captures);
		}

		U64 promotion_captures = ((promoters) << 7) & notHFile & targets;
		while (promotion_captures) {
			U64 isolated = promotion_captures & twos_complement(promotion_captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq - 7, sq, p, get_piece_type_on(sq), n, true, false, false, false);
			ret[ind++] = move;
			set_promotion_type(move, b);
			ret[ind++] = move;
			set_promotion_type(move, r);
			ret[ind++] = move;
			set_promotion_type(move, q);
			ret[ind++] = move;
			promotion_captures = promotion_captures & ones_decrement(promotion_captures);
		}
		promotion_captures = ((promoters) << 9) & notAFile & targets;
		while (promotion_captures) {
			U64 isolated = promotion_captures & twos_complement(promotion_captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq - 9, sq, p, get_piece_type_on(sq), n, true, false, false, false);
			ret[ind++] = move;
			set_promotion_type(move, b);
			ret[ind++] = move;
			set_promotion_type(move, r);
			ret[ind++] = move;
			set_promotion_type(move, q);
			ret[ind++] = move;
			promotion_captures = promotion_captures & ones_decrement(promotion_captures);
		}
		return ind;
	}
	inline int in_check_legal_wpawn_captures(std::array<unsigned int, 128>& ret, const U64 pinned, const U64 targets, int ind) {
		U64 promoters = bitboards[0] & (~pinned) & rank7;
		U64 captures = ((bitboards[0] & (~(pinned | promoters))) >> 7) & notAFile & targets;

		U64 enpassant = (enpassant_square != a8) * (1ULL << enpassant_square);
		const bool left_enpassant = (enpassant << 7) & notHFile & bitboards[P];
		const bool right_enpassant = (enpassant << 9) & notAFile & bitboards[P];
		if (left_enpassant) {
			unsigned int move = encode_move(enpassant_square + 7, enpassant_square, P, p, no_piece, true, false, false, true);
			try_out_move(ret, move, ind);
		}
		if (right_enpassant) {
			unsigned int move = encode_move(enpassant_square + 9, enpassant_square, P, p, no_piece, true, false, false, true);
			try_out_move(ret, move, ind);
		}

		while (captures) {
			U64 isolated = captures & twos_complement(captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq + 7, sq, P, get_piece_type_on(sq), no_piece, true, false, false, false);
			ret[ind++]=move;
			captures = captures & ones_decrement(captures);
		}
		captures = ((bitboards[0] & ~(pinned | promoters)) >> 9) & notHFile & targets;
		while (captures) {
			U64 isolated = captures & twos_complement(captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq + 9, sq, P, get_piece_type_on(sq), no_piece, true, false, false, false);
			ret[ind++]=move;
			captures = captures & ones_decrement(captures);
		}
		U64 promotion_captures = ((promoters) >> 7) & notAFile & targets;
		while (promotion_captures) {
			U64 isolated = promotion_captures & twos_complement(promotion_captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq + 7, sq, P, get_piece_type_on(sq), N, true, false, false, false);
			ret[ind++]=move;
			set_promotion_type(move, B);
			ret[ind++]=move;
			set_promotion_type(move, R);
			ret[ind++]=move;
			set_promotion_type(move, Q);
			ret[ind++]=move;
			promotion_captures = promotion_captures & ones_decrement(promotion_captures);
		}
		promotion_captures = ((promoters) >> 9) & notHFile & targets;
		while (promotion_captures) {
			U64 isolated = promotion_captures & twos_complement(promotion_captures);
			
			unsigned long sq = bitscan(isolated);
			unsigned int move = encode_move(sq + 9, sq, P, get_piece_type_on(sq), N, true, false, false, false);
			ret[ind++]=move;
			set_promotion_type(move, B);
			ret[ind++]=move;
			set_promotion_type(move, R);
			ret[ind++]=move;
			set_promotion_type(move, Q);
			ret[ind++]=move;
			promotion_captures = promotion_captures & ones_decrement(promotion_captures);
		}
		return ind;
	}

	inline U64 get_pinned_pieces(const int kingpos, const U64 enemy_attacks) {
		U64 potentially_pinned_by_bishops = enemy_attacks & get_bishop_attacks(occupancies[both], kingpos) & occupancies[(side)];
		U64 potentially_pinned_by_rooks = enemy_attacks & get_rook_attacks(occupancies[both], kingpos) & occupancies[(side)];
		U64 pinned_pieces = 0ULL;
		const int offset = 6 & ~sideMask;
		while (potentially_pinned_by_bishops) {
			const U64 isolated = potentially_pinned_by_bishops & twos_complement(potentially_pinned_by_bishops);
			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			U64 pot_pinner = get_bishop_attacks(occupancies[both], kingpos) & (bitboards[B + offset] | bitboards[Q + offset]);
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			while (pot_pinner) {
				const U64 isolatedPinner = pot_pinner & twos_complement(pot_pinner);
				pinned_pieces |= (bool)(isolated & get_bishop_attacks(occupancies[both], static_cast<unsigned long long>(bitscan(pot_pinner)))) * isolated;
				pot_pinner = pot_pinner & ones_decrement(pot_pinner);
			}
			potentially_pinned_by_bishops = potentially_pinned_by_bishops & ones_decrement(potentially_pinned_by_bishops);
		}
		while (potentially_pinned_by_rooks) {
			const U64 isolated = potentially_pinned_by_rooks & twos_complement(potentially_pinned_by_rooks);
			occupancies[both] &= ~isolated;//pop bit of piece from occupancies
			U64 pot_pinner = get_rook_attacks(occupancies[both], kingpos) & (bitboards[R + offset] | bitboards[Q + offset]);
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			while (pot_pinner) {
				const U64 isolatedPinner = pot_pinner & twos_complement(pot_pinner);
				pinned_pieces |= (bool)(isolated & get_rook_attacks(occupancies[both], static_cast<unsigned long long>(bitscan(isolatedPinner)))) * isolated;
				pot_pinner = pot_pinner & ones_decrement(pot_pinner);
			}
			potentially_pinned_by_rooks = potentially_pinned_by_rooks & ones_decrement(potentially_pinned_by_rooks);
		}
		return pinned_pieces;
	}
	inline U64 get_moves_for_pinned_pieces(std::array<unsigned int,128>& ret, const int kingpos, const U64 enemy_attacks, int& ind) {
		const int piece_offset = 6 & sideMask;
		const U64 bishop_attacks = get_bishop_attacks(occupancies[both], kingpos);
		const U64 rook_attacks = get_rook_attacks(occupancies[both], kingpos);
		const U64 pot_pinned_by_bishops = enemy_attacks & bishop_attacks;
		const U64 pot_pinned_by_rooks = enemy_attacks & rook_attacks;
		int type = P + piece_offset;

		U64 pawns_pot_pinned_by_bishops = bitboards[type] & pot_pinned_by_bishops;
		U64 knights_pot_pinned_by_bishops = bitboards[type + 1] & pot_pinned_by_bishops;
		U64 bishops_pot_pinned_by_bishops = bitboards[type + 2] & pot_pinned_by_bishops;
		U64 rooks_pot_pinned_by_bishops = bitboards[type + 3] & pot_pinned_by_bishops;
		U64 queens_pot_pinned_by_bishops = pot_pinned_by_bishops & bitboards[Q + piece_offset];

		U64 pawns_pot_pinned_by_rooks = bitboards[type] & pot_pinned_by_rooks;
		U64 knights_pot_pinned_by_rooks = bitboards[type + 1] & pot_pinned_by_rooks;
		U64 bishops_pot_pinned_by_rooks = bitboards[type + 2] & pot_pinned_by_rooks;
		U64 rooks_pot_pinned_by_rooks = bitboards[type + 3] & pot_pinned_by_rooks;
		U64 queens_pot_pinned_by_rooks = pot_pinned_by_rooks & bitboards[Q + piece_offset];

		U64 pinned_pieces = 0ULL;

		const int offset = 6 & ~sideMask;

		const U64 rook_and_queen_mask=bitboards[R + offset] | bitboards[Q + offset];
		const U64 bishop_and_queen_mask= bitboards[B + offset] | bitboards[Q + offset];
		type = P + piece_offset;
		while (pawns_pot_pinned_by_bishops) {
			const U64 isolated = pawns_pot_pinned_by_bishops & twos_complement(pawns_pot_pinned_by_bishops);
			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			U64 pot_attacks = get_bishop_attacks(occupancies[both], kingpos);
			const U64 pinner = pot_attacks & bishop_and_queen_mask;
			pinned_pieces |= ((bool)(pinner)) * isolated;
			if (pinner) {
				unsigned long from = bitscan(isolated);
				unsigned long to = bitscan(pinner);
				U64 attacks = pawn_attacks[(side)][from];
				if (attacks & pinner) {
					unsigned int move = encode_move(from, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
					if ((rank7 << (a2 & sideMask)) & isolated) {
						set_promotion_type(move, N + piece_offset);
						ret[ind++]=move;
						set_promotion_type(move, B + piece_offset);
						ret[ind++]=move;
						set_promotion_type(move, R + piece_offset);
						ret[ind++]=move;
						set_promotion_type(move, Q + piece_offset);
					}
					ret[ind++]=move;
				}
			}
			occupancies[both] |= isolated;//reset bit of piece from occupancies

			pawns_pot_pinned_by_bishops = pawns_pot_pinned_by_bishops & ones_decrement(pawns_pot_pinned_by_bishops);
		}
		const int sign = -1 + (2 & sideMask);
		while (pawns_pot_pinned_by_rooks) {
			const U64 isolated = pawns_pot_pinned_by_rooks & twos_complement(pawns_pot_pinned_by_rooks);
			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			U64 pot_attacks = get_rook_attacks(occupancies[both], kingpos);
			const U64 pinner = pot_attacks & rook_and_queen_mask;
			pinned_pieces |= ((bool)(pinner)) * isolated;
			if (pinner) {
				unsigned long from = bitscan(isolated);
				U64 valid_targets = (~occupancies[both]) & pot_attacks;
				const int push_target = from + 8 * sign;
				const int double_push_target = from + 16 * sign;
				if (get_bit(valid_targets, push_target)) {
					unsigned int move = encode_move(from, push_target, type, no_piece, no_piece, false, false, false, false);
					if ((rank7 << (a2 & sideMask)) & isolated) {
						set_promotion_type(move, N + piece_offset);
						ret[ind++]=move;
						set_promotion_type(move, B + piece_offset);
						ret[ind++]=move;
						set_promotion_type(move, R + piece_offset);
						ret[ind++]=move;
						set_promotion_type(move, Q + piece_offset);
					}
					ret[ind++]=move;
				}
				if ((get_bit(valid_targets, push_target) && get_bit(valid_targets, double_push_target)) && ((rank4 >> (8 & sideMask)) & (1ULL << double_push_target))) {
					unsigned int move = encode_move(from, double_push_target, type, no_piece, no_piece, false, true, false, false);
					ret[ind++] = move;
				}
			}
			occupancies[both] |= isolated;//reset bit of piece from occupancies

			pawns_pot_pinned_by_rooks = pawns_pot_pinned_by_rooks & ones_decrement(pawns_pot_pinned_by_rooks);
		}

		type++;
		while (knights_pot_pinned_by_bishops) {
			const U64 isolated = knights_pot_pinned_by_bishops & twos_complement(knights_pot_pinned_by_bishops);

			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			pinned_pieces |= ((bool)(get_bishop_attacks(occupancies[both], kingpos) & bishop_and_queen_mask)) * isolated;
			occupancies[both] |= isolated;//reset bit of piece from occupancies

			knights_pot_pinned_by_bishops = knights_pot_pinned_by_bishops & ones_decrement(knights_pot_pinned_by_bishops);
		}
		while (knights_pot_pinned_by_rooks) {
			const U64 isolated = knights_pot_pinned_by_rooks & twos_complement(knights_pot_pinned_by_rooks);

			occupancies[both] &= ~isolated;//pop bit of piece from occupancies
			pinned_pieces |= ((bool)(get_rook_attacks(occupancies[both], kingpos) & rook_and_queen_mask)) * isolated;
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			knights_pot_pinned_by_rooks = knights_pot_pinned_by_rooks & ones_decrement(knights_pot_pinned_by_rooks);
		}

		type++;
		while (bishops_pot_pinned_by_bishops) {
			const U64 isolated = bishops_pot_pinned_by_bishops & twos_complement(bishops_pot_pinned_by_bishops);

			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			U64 pot_attacks = get_bishop_attacks(occupancies[both], kingpos);
			const U64 pinner = pot_attacks & bishop_and_queen_mask;
			pinned_pieces |= ((bool)(pinner)) * isolated;
			if (pinner) {
				const int from = bitscan(isolated);
				const int to = bitscan(pinner);
				unsigned int move = encode_move(from, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				ret[ind++]=move;
				U64 attacks = (pot_attacks & get_bishop_attacks(occupancies[both], to)) & (~isolated);
				while (attacks) {
					const U64 isolated2 = attacks & twos_complement(attacks);
					move = encode_move(from, bitscan(isolated2), type, no_piece, no_piece, false, false, false, false);
					ret[ind++]=move;
					attacks = attacks & ones_decrement(attacks);
				}
			}
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			bishops_pot_pinned_by_bishops = bishops_pot_pinned_by_bishops & ones_decrement(bishops_pot_pinned_by_bishops);
		}
		while (bishops_pot_pinned_by_rooks) {
			const U64 isolated = bishops_pot_pinned_by_rooks & twos_complement(bishops_pot_pinned_by_rooks);
			occupancies[both] &= ~isolated;//pop bit of piece from occupancies
			pinned_pieces |= ((bool)(get_rook_attacks(occupancies[both], kingpos) & rook_and_queen_mask)) * isolated;
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			bishops_pot_pinned_by_rooks = bishops_pot_pinned_by_rooks & ones_decrement(bishops_pot_pinned_by_rooks);
		}
		type++;
		while (rooks_pot_pinned_by_rooks) {
			const U64 isolated = rooks_pot_pinned_by_rooks & twos_complement(rooks_pot_pinned_by_rooks);

			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			U64 pot_attacks = get_rook_attacks(occupancies[both], kingpos);
			const U64 pinner = pot_attacks & rook_and_queen_mask;
			pinned_pieces |= ((bool)(pinner)) * isolated;
			if (pinner) {
				const int from = bitscan(isolated);
				const int to = bitscan(pinner);
				unsigned int move = encode_move(from, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				ret[ind++]=move;
				U64 attacks = (pot_attacks & get_rook_attacks(occupancies[both], to)) & (~isolated);
				while (attacks) {
					const U64 isolated2 = attacks & twos_complement(attacks);
					move = encode_move(from, bitscan(isolated2), type, no_piece, no_piece, false, false, false, false);
					ret[ind++]=move;
					attacks = attacks & ones_decrement(attacks);
				}
			}
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			rooks_pot_pinned_by_rooks = rooks_pot_pinned_by_rooks & ones_decrement(rooks_pot_pinned_by_rooks);
		}
		while (rooks_pot_pinned_by_bishops) {
			const U64 isolated = rooks_pot_pinned_by_bishops & twos_complement(rooks_pot_pinned_by_bishops);
			occupancies[both] &= ~isolated;//pop bit of piece from occupancies
			pinned_pieces |= ((bool)(get_bishop_attacks(occupancies[both], kingpos) & bishop_and_queen_mask)) * isolated;
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			rooks_pot_pinned_by_bishops = rooks_pot_pinned_by_bishops & ones_decrement(rooks_pot_pinned_by_bishops);
		}

		type++;
		while (queens_pot_pinned_by_bishops) {
			const U64 isolated = queens_pot_pinned_by_bishops & twos_complement(queens_pot_pinned_by_bishops);

			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			U64 pot_attacks = get_bishop_attacks(occupancies[both], kingpos);
			const U64 pinner = pot_attacks & bishop_and_queen_mask;
			pinned_pieces |= ((bool)(pinner)) * isolated;
			if (pinner) {
				const int from = bitscan(isolated);
				const int to = bitscan(pinner);
				unsigned int move = encode_move(from, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				ret[ind++]=move;
				U64 attacks = (pot_attacks & get_bishop_attacks(occupancies[both], to)) & (~isolated);
				while (attacks) {
					const U64 isolated2 = attacks & twos_complement(attacks);
					move = encode_move(from, bitscan(isolated2), type, no_piece, no_piece, false, false, false, false);
					ret[ind++]=move;
					attacks = attacks & ones_decrement(attacks);
				}
			}
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			queens_pot_pinned_by_bishops = queens_pot_pinned_by_bishops & ones_decrement(queens_pot_pinned_by_bishops);
		}
		while (queens_pot_pinned_by_rooks) {
			const U64 isolated = queens_pot_pinned_by_rooks & twos_complement(queens_pot_pinned_by_rooks);

			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			U64 pot_attacks = get_rook_attacks(occupancies[both], kingpos);
			const U64 pinner = pot_attacks & rook_and_queen_mask;
			pinned_pieces |= ((bool)(pinner)) * isolated;
			if (pinner) {
				const int from = bitscan(isolated);
				const int to = bitscan(pinner);
				unsigned int move = encode_move(from, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				ret[ind++]=move;
				U64 attacks = (pot_attacks & get_rook_attacks(occupancies[both], to)) & (~isolated);
				while (attacks) {
					const U64 isolated2 = attacks & twos_complement(attacks);
					move = encode_move(from, bitscan(isolated2), type, no_piece, no_piece, false, false, false, false);
					ret[ind++]=move;
					attacks = attacks & ones_decrement(attacks);
				}
			}
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			queens_pot_pinned_by_rooks = queens_pot_pinned_by_rooks & ones_decrement(queens_pot_pinned_by_rooks);
		}
		return pinned_pieces;
	}
	inline U64 get_captures_for_pinned_pieces(std::array<unsigned int,128>& ret, const int kingpos, const U64 enemy_attacks, int& ind) {
		const int piece_offset = 6 & sideMask;
		const U64 bishop_attacks = get_bishop_attacks(occupancies[both], kingpos);
		const U64 rook_attacks = get_rook_attacks(occupancies[both], kingpos);
		const U64 pot_pinned_by_bishops = enemy_attacks & bishop_attacks;
		const U64 pot_pinned_by_rooks = enemy_attacks & rook_attacks;
		int type = P + piece_offset;

		U64 pawns_pot_pinned_by_bishops = bitboards[type] & pot_pinned_by_bishops;
		U64 knights_pot_pinned_by_bishops = bitboards[type + 1] & pot_pinned_by_bishops;
		U64 bishops_pot_pinned_by_bishops = bitboards[type + 2] & pot_pinned_by_bishops;
		U64 rooks_pot_pinned_by_bishops = bitboards[type + 3] & pot_pinned_by_bishops;
		U64 queens_pot_pinned_by_bishops = pot_pinned_by_bishops & bitboards[Q + piece_offset];

		U64 pawns_pot_pinned_by_rooks = bitboards[type] & pot_pinned_by_rooks;
		U64 knights_pot_pinned_by_rooks = bitboards[type + 1] & pot_pinned_by_rooks;
		U64 bishops_pot_pinned_by_rooks = bitboards[type + 2] & pot_pinned_by_rooks;
		U64 rooks_pot_pinned_by_rooks = bitboards[type + 3] & pot_pinned_by_rooks;
		U64 queens_pot_pinned_by_rooks = pot_pinned_by_rooks & bitboards[Q + piece_offset];

		U64 pinned_pieces = 0ULL;

		const int offset = 6 & ~sideMask;
		const U64 bishop_and_queen_mask = bitboards[B + offset] | bitboards[Q + offset];
		const U64 rook_and_queen_mask = bitboards[R + offset] | bitboards[Q + offset];
		type = B + piece_offset;
		while (bishops_pot_pinned_by_bishops) {
			const U64 isolated = bishops_pot_pinned_by_bishops & twos_complement(bishops_pot_pinned_by_bishops);

			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			U64 pot_attacks = get_bishop_attacks(occupancies[both], kingpos);
			const U64 pinner = pot_attacks & bishop_and_queen_mask;
			pinned_pieces |= ((bool)(pinner)) * isolated;
			if (pinner) {
				const int from = bitscan(isolated);
				const int to = bitscan(pinner);
				unsigned int move = encode_move(from, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				ret[ind++]=move;
			}
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			bishops_pot_pinned_by_bishops = bishops_pot_pinned_by_bishops & ones_decrement(bishops_pot_pinned_by_bishops);
		}
		while (bishops_pot_pinned_by_rooks) {
			const U64 isolated = bishops_pot_pinned_by_rooks & twos_complement(bishops_pot_pinned_by_rooks);
			occupancies[both] &= ~isolated;//pop bit of piece from occupancies
			pinned_pieces |= ((bool)(get_rook_attacks(occupancies[both], kingpos) & rook_and_queen_mask)) * isolated;
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			bishops_pot_pinned_by_rooks = bishops_pot_pinned_by_rooks & ones_decrement(bishops_pot_pinned_by_rooks);
		}

		type = Q + piece_offset;
		while (queens_pot_pinned_by_bishops) {
			const U64 isolated = queens_pot_pinned_by_bishops & twos_complement(queens_pot_pinned_by_bishops);

			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			U64 pot_attacks = get_bishop_attacks(occupancies[both], kingpos);
			const U64 pinner = pot_attacks & bishop_and_queen_mask;
			pinned_pieces |= ((bool)(pinner)) * isolated;
			if (pinner) {
				const int from = bitscan(isolated);
				const int to = bitscan(pinner);
				unsigned int move = encode_move(from, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				ret[ind++]=move;
			}
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			queens_pot_pinned_by_bishops = queens_pot_pinned_by_bishops & ones_decrement(queens_pot_pinned_by_bishops);
		}
		while (queens_pot_pinned_by_rooks) {
			const U64 isolated = queens_pot_pinned_by_rooks & twos_complement(queens_pot_pinned_by_rooks);

			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			U64 pot_attacks = get_rook_attacks(occupancies[both], kingpos);
			const U64 pinner = pot_attacks & rook_and_queen_mask;
			pinned_pieces |= ((bool)(pinner)) * isolated;
			if (pinner) {
				const int from = bitscan(isolated);
				const int to = bitscan(pinner);
				unsigned int move = encode_move(from, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				ret[ind++]=move;
			}
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			queens_pot_pinned_by_rooks = queens_pot_pinned_by_rooks & ones_decrement(queens_pot_pinned_by_rooks);
		}
		type = R + piece_offset;
		while (rooks_pot_pinned_by_rooks) {
			const U64 isolated = rooks_pot_pinned_by_rooks & twos_complement(rooks_pot_pinned_by_rooks);

			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			U64 pot_attacks = get_rook_attacks(occupancies[both], kingpos);
			const U64 pinner = pot_attacks & rook_and_queen_mask;
			pinned_pieces |= ((bool)(pinner)) * isolated;
			if (pinner) {
				const int from = bitscan(isolated);
				const int to = bitscan(pinner);
				unsigned int move = encode_move(from, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
				ret[ind++]=move;
			}
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			rooks_pot_pinned_by_rooks = rooks_pot_pinned_by_rooks & ones_decrement(rooks_pot_pinned_by_rooks);
		}
		while (rooks_pot_pinned_by_bishops) {
			const U64 isolated = rooks_pot_pinned_by_bishops & twos_complement(rooks_pot_pinned_by_bishops);
			occupancies[both] &= ~isolated;//pop bit of piece from occupancies
			pinned_pieces |= ((bool)(get_bishop_attacks(occupancies[both], kingpos) & bishop_and_queen_mask)) * isolated;
			occupancies[both] |= isolated;//reset bit of piece from occupancies
			rooks_pot_pinned_by_bishops = rooks_pot_pinned_by_bishops & ones_decrement(rooks_pot_pinned_by_bishops);
		}
		type = P + piece_offset;
		while (pawns_pot_pinned_by_bishops) {
			const U64 isolated = pawns_pot_pinned_by_bishops & twos_complement(pawns_pot_pinned_by_bishops);
			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			U64 pot_attacks = get_bishop_attacks(occupancies[both], kingpos);
			const U64 pinner = pot_attacks & bishop_and_queen_mask;
			pinned_pieces |= ((bool)(pinner)) * isolated;
			if (pinner) {
				const int from = bitscan(isolated);
				const int to = bitscan(pinner);
				U64 attacks = pawn_attacks[(side)][from];
				if (attacks & pinner) {
					unsigned int move = encode_move(from, to, type, get_piece_type_on(to), no_piece, true, false, false, false);
					if ((rank7 << (a2 & sideMask)) & isolated) {
						set_promotion_type(move, N + piece_offset);
						ret[ind++]=move;
						set_promotion_type(move, B + piece_offset);
						ret[ind++]=move;
						set_promotion_type(move, R + piece_offset);
						ret[ind++]=move;
						set_promotion_type(move, Q + piece_offset);
					}
					ret[ind++]=move;
				}
			}
			occupancies[both] |= isolated;//reset bit of piece from occupancies

			pawns_pot_pinned_by_bishops = pawns_pot_pinned_by_bishops & ones_decrement(pawns_pot_pinned_by_bishops);
		}
		while (pawns_pot_pinned_by_rooks) {
			const U64 isolated = pawns_pot_pinned_by_rooks & twos_complement(pawns_pot_pinned_by_rooks);
			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			U64 pot_attacks = get_rook_attacks(occupancies[both], kingpos);
			const U64 pinner = pot_attacks & rook_and_queen_mask;
			pinned_pieces |= ((bool)(pinner)) * isolated;
			occupancies[both] |= isolated;//reset bit of piece from occupancies

			pawns_pot_pinned_by_rooks = pawns_pot_pinned_by_rooks & ones_decrement(pawns_pot_pinned_by_rooks);
		}


		while (knights_pot_pinned_by_bishops) {
			const U64 isolated = knights_pot_pinned_by_bishops & twos_complement(knights_pot_pinned_by_bishops);

			occupancies[both] &= ~isolated;//pop bit of piece from occupancie
			pinned_pieces |= ((bool)(get_bishop_attacks(occupancies[both], kingpos) & bishop_and_queen_mask)) * isolated;
			occupancies[both] |= isolated;//reset bit of piece from occupancies

			knights_pot_pinned_by_bishops = knights_pot_pinned_by_bishops & ones_decrement(knights_pot_pinned_by_bishops);
		}
		while (knights_pot_pinned_by_rooks) {
			const U64 isolated = knights_pot_pinned_by_rooks & twos_complement(knights_pot_pinned_by_rooks);

			occupancies[both] &= ~isolated;//pop bit of piece from occupancies
			pinned_pieces |= ((bool)(get_rook_attacks(occupancies[both], kingpos) & rook_and_queen_mask)) * isolated;
			occupancies[both] |= isolated;//reset bit of piece from occupancies

			knights_pot_pinned_by_rooks = knights_pot_pinned_by_rooks & ones_decrement(knights_pot_pinned_by_rooks);
		}
		return pinned_pieces;
	}
	inline U64 get_checkers(const int kingpos) {
		const int offset = 6 & ~sideMask;
		return (get_bishop_attacks(occupancies[both], kingpos) & (bitboards[B + offset] | bitboards[Q + offset])) | (get_rook_attacks(occupancies[both], kingpos) & (bitboards[R + offset] | bitboards[Q + offset])) | (knight_attacks[kingpos] & bitboards[N + offset]) | (pawn_attacks[(side)][kingpos] & bitboards[P + offset]);
	}
	inline U64 get_checking_rays(const int kingpos) {
		const int offset = 6 & ~sideMask;
		const U64 kings_rook_scope = get_rook_attacks(occupancies[both], kingpos);
		const U64 kings_bishop_scope = get_bishop_attacks(occupancies[both], kingpos);
		const U64 checking_rooks = kings_rook_scope & (bitboards[R + offset] | bitboards[Q + offset]);
		const U64 checking_bishops = kings_bishop_scope & (bitboards[B + offset] | bitboards[Q + offset]);
		U64 ret = 0ULL;
		ret |= ((bool)checking_rooks) * (checkingRays[kingpos][bitscan(checking_rooks)] | checking_rooks);
		ret |= ((bool)checking_bishops) * (checkingRays[kingpos][bitscan(checking_bishops)] | checking_bishops);
		return ret;
	}
	inline int get_castles(std::array<unsigned int,128>& ptr, const U64 enemy_attacks, int ind) {
		const U64 empty = ~(occupancies[both]);
		const U64 notAttacked = ~enemy_attacks;

		const int king_index = K + (int)(sideMask & p);
		const int kingpos = bitscan(bitboards[king_index]);
		const U64 king_in_valid_state = (~(bitboards[king_index])) ^ (notAttacked & (1ULL << (e1 - (a1 & (int)(sideMask)))));

		const int bit_offset = 2 & sideMask;

		const int square_offset = 56 & sideMask;

		U64 mustBeEmptyMask = ((1ULL << b1) | (1ULL << c1) | (1ULL << d1)) >> (a1 & sideMask);
		const U64 mustNotBeChecked = ((1ULL << c1) | (1ULL << d1)) >> (a1 & sideMask);
		
		const U64 has_queenside_rights = ((bool)((U64)castling_rights & (1ULL << (1 + bit_offset)))) * trueMask;
		const U64 queenside = (~(mustNotBeChecked & notAttacked) ^ mustNotBeChecked) & (~(mustBeEmptyMask & empty) ^ mustBeEmptyMask) & king_in_valid_state & has_queenside_rights;
		unsigned int move = encode_move(kingpos, kingpos - 2, king_index, no_piece, no_piece, false, false, true, false);

		if (queenside == trueMask) {
			ptr[ind++]=move;
		}
		const U64 has_kingside_rights = ((bool)((U64)castling_rights & (1ULL << bit_offset))) * trueMask;
		mustBeEmptyMask = ((1ULL << f1) | (1ULL << g1)) >> (a1 & sideMask);
		const U64 kingside = ~((empty & notAttacked & mustBeEmptyMask) ^ mustBeEmptyMask) & king_in_valid_state & has_kingside_rights;
		set_to_square(move, kingpos + 2);
		if (kingside == trueMask) {
			ptr[ind++] = move;
		}
		return ind;
	}

public:
	std::array<U64, 12> bitboards; // P, N, B, R, Q, K, p, n, b, r, q, k
	std::array<U64, 3> occupancies;
	std::array<short, 64> square_board;
	U64 sideMask;//white: false, black: true
	bool side;
	int ply;
	short enpassant_square;
	short castling_rights;//wk,wq,bk,bq
	short no_pawns_or_captures;
	std::vector<unsigned int> move_history;
	std::vector<short> enpassant_history;
	std::vector<short> castling_rights_history;
	std::vector<short> no_pawns_or_captures_history;
	std::vector<U64> hash_history;
	std::unordered_map<U64, short> pawn_evaluation_map;
	const static short infinity = 30000;
	const static short no_piece = 15;
	U64 current_hash;
	Position() :
		bitboards{ { 71776119061217280ULL, 4755801206503243776ULL, 2594073385365405696ULL, 9295429630892703744ULL, 576460752303423488ULL, 1152921504606846976ULL, 65280ULL, 66ULL, 36ULL, 129ULL, 8ULL, 16ULL } },
		occupancies{ { 18446462598732840960ULL, 65535ULL, 18446462598732906495ULL} },
		sideMask{ falseMask }, side{ false }, ply{ 1 }, enpassant_square{ a8 }, castling_rights{ 15 }, no_pawns_or_captures{0},
		move_history{},
		enpassant_history{},
		castling_rights_history{},
		no_pawns_or_captures_history{},
		hash_history{},
		square_board{}
		{
		current_hash = get_hash();
		move_history.reserve(256);
		enpassant_history.reserve(256);
		castling_rights_history.reserve(256);
		no_pawns_or_captures_history.reserve(256);
		hash_history.reserve(256);
		for (int i = 0; i < square_board.size(); i++) {
			square_board[i] = no_piece;
		}
	}
	Position(const std::string& fen) {
		parse_fen(fen);
	}

	void parse_fen(std::string fen) {
		unsigned int i, j;
		int sq;
		char letter;
		int aRank, aFile;
		std::vector<std::string> strList;
		const std::string delimiter = " ";
		strList.push_back(fen.substr(0, fen.find(delimiter)));
		fen = fen.substr(fen.find(delimiter));
		fen = fen.substr(1);
		// Empty the board quares
		for (int i = 0; i < 12; i++) {
			bitboards[i] = 0ULL;
		}
		square_board = std::array<short, 64>{};
		for (int i = 0; i < square_board.size(); i++) {
			square_board[i] = no_piece;
		}
		castling_rights = 0;
		no_pawns_or_captures = 0;
		// read the board - translate each loop idx into a square
		j = 1; i = 0;
		while ((j <= 64) && (i <= strList[0].length()))
		{
			letter = strList[0].at(i);
			i++;
			aFile = 1 + ((j - 1) % 8);
			aRank = 1 + ((j - 1) / 8);
			sq = (int)(((aRank - 1) * 8) + (aFile - 1));
			switch (letter)
			{
			case '/': j--; break;
			case '1': break;
			case '2': j++; break;
			case '3': j += 2; break;
			case '4': j += 3; break;
			case '5': j += 4; break;
			case '6': j += 5; break;
			case '7': j += 6; break;
			case '8': j += 7; break;
			default:
				set_bit(bitboards[char_pieces(letter)], sq);
				square_board[sq] = char_pieces(letter);
			}
			j++;
		}

		strList.push_back(fen.substr(0, fen.find(delimiter)));
		fen = fen.substr(fen.find(delimiter));
		fen = fen.substr(1);

		// set the turn; default = White
		int sideToMove = white;
		if (strList.size() >= 2)
		{
			if (strList[1] == "w") {
				sideMask = falseMask;
				side = false;
			}
			else if (strList[1] == "b") {
				sideMask = trueMask;
				side = true;
			}
		}

		strList.push_back(fen.substr(0, fen.find(delimiter)));
		fen = fen.substr(fen.find(delimiter));
		fen = fen.substr(1);

		if (strList[2].find('K') != std::string::npos) {
			castling_rights |= wk;
		}
		if (strList[2].find('Q') != std::string::npos) {
			castling_rights |= wq;
		}
		if (strList[2].find('k') != std::string::npos) {
			castling_rights |= bk;
		}
		if (strList[2].find('q') != std::string::npos) {
			castling_rights |= bq;
		}

		strList.push_back(fen.substr(0, fen.find(delimiter)));
		fen = fen.substr(fen.find(delimiter));
		fen = fen.substr(1);

		if (fen[0] == '-') {
			enpassant_square = a8;
		}
		else {
			enpassant_square = (int)(std::find(square_coordinates, square_coordinates + sizeof(square_coordinates) / sizeof(square_coordinates[0]), strList[3]) - square_coordinates);
			enpassant_square = (enpassant_square == 64) * a8 + (enpassant_square != 64) * enpassant_square;
		}

		strList.push_back(fen.substr(0, fen.find(delimiter)));
		fen = fen.substr(fen.find(delimiter));
		fen = fen.substr(1);

		move_history.clear();
		no_pawns_or_captures_history.clear();
		castling_rights_history.clear();
		enpassant_history.clear();
		hash_history.clear();

		move_history.reserve(256);
		no_pawns_or_captures_history.reserve(256);
		castling_rights_history.reserve(256);
		enpassant_history.reserve(256);
		hash_history.reserve(256);

		no_pawns_or_captures = stoi(strList[4]);
		ply = 2 * stoi(fen) - (sideMask == falseMask);
		occupancies[0] = bitboards[0] | bitboards[1] | bitboards[2] | bitboards[3] | bitboards[4] | bitboards[5];
		occupancies[1] = bitboards[6] | bitboards[7] | bitboards[8] | bitboards[9] | bitboards[10] | bitboards[11];
		occupancies[2] = occupancies[0] | occupancies[1];
		no_pawns_or_captures_history.push_back(no_pawns_or_captures);
		//castling_rights_history.push_back(0);
		castling_rights_history.push_back(castling_rights);
		//enpassant_history.push_back(a8);
		enpassant_history.push_back(enpassant_square);
		current_hash = get_hash();
		hash_history.push_back(current_hash);
	}

	std::string fen() const{
		std::string ret = "";
		const int arr[8] = { 7,6,5,4,3,2,1,0 };
		for (int i = 0; i < 8; i++) {
			int empty_spaces = 0;
			for (int j = 0; j < 8; j++) {
				const int ind = 8 * i + j;
				int piece = get_piece_type_on(ind);
				if (piece == no_piece) {
					piece = get_piece_type_on(ind);
				}
				if ((piece == no_piece) || ((ind == enpassant_square) && (enpassant_square != a8))) {
					empty_spaces++;
				}
				else {
					if (empty_spaces) {
						ret += std::to_string(empty_spaces);
					}
					empty_spaces = 0;
					ret += ascii_pieces[piece];
				}
			}
			if (empty_spaces) {
				ret += std::to_string(empty_spaces);
			}
			if (i < 7) {
				ret += "/";
			}
		}
		ret += " ";
		if (sideMask) {
			ret += "b";
		}
		else {
			ret += "w";
		}
		ret += " ";
		if (get_bit(castling_rights, 0)) {
			ret += "K";
		}
		if (get_bit(castling_rights, 1)) {
			ret += "Q";
		}
		if (get_bit(castling_rights, 2)) {
			ret += "k";
		}
		if (get_bit(castling_rights, 3)) {
			ret += "q";
		}
		if (!(get_bit(castling_rights, 0) || get_bit(castling_rights, 1) || get_bit(castling_rights, 2) || get_bit(castling_rights, 3))) {
			ret += "-";
		}
		ret += " ";
		if ((enpassant_square == a8) || (enpassant_square == 64)) {
			ret += "-";
		}
		else {
			ret += square_coordinates[enpassant_square];
		}
		ret += " ";
		ret += std::to_string(no_pawns_or_captures);
		ret += " ";
		if (ply % 2) {
			ret += std::to_string((ply + 1) / 2);
		}
		else {
			ret += std::to_string(ply / 2 + 1);
		}
		return ret;
	}
	void print() {
		printf("\n");
		for (int rank = 0; rank < 8; rank++) {
			for (int file = 0; file < 8; file++) {
				//loop over board ranks and files

				int square = rank * 8 + file;
				//convert to square index

				if (!file) {
					printf(" %d ", 8 - rank);
				}//print rank on the left side

				int piece = no_piece;

				for (int piece_on_square = P; piece_on_square <= k; piece_on_square++) {
					if (get_bit(bitboards[piece_on_square], square)) {
						piece = piece_on_square;
						break;
					}
				}
				printf(" %c", (piece == no_piece) ? '.' : ascii_pieces[piece]);
			}
			printf("\n");
		}
		printf("\n    a b c d e f g h \n\n");
		printf("    To move: %s\n", (sideMask) ? "black" : "white");
		printf("    enpassant square: %s\n", (enpassant_square != a8) ? square_coordinates[enpassant_square].c_str() : "none");

		printf("    castling rights: %c%c%c%c\n", (get_bit(castling_rights, 0)) ? 'K' : '-', (get_bit(castling_rights, 1)) ? 'Q' : '-', (get_bit(castling_rights, 2)) ? 'k' : '-', (get_bit(castling_rights, 3)) ? 'q' : '-');
		//print castling rights

		printf("    halfmoves since last pawn move or capture: %d\n", no_pawns_or_captures);
		printf("    current halfclock turn: %d\n", ply);
		printf("    current game turn: %d\n", (int)ply / 2 + (sideMask == falseMask));
		std::cout << "    current hash: " << current_hash << "\n";
		std::cout << "    fen: " << fen() << "\n";
	}
	void print_square_board() const {
		printf("\n");
		for (int rank = 0; rank < 8; rank++) {
			for (int file = 0; file < 8; file++) {
				//loop over board ranks and files

				int square = rank * 8 + file;
				//convert to square index

				if (!file) {
					printf(" %d ", 8 - rank);
				}//print rank on the left side

				int piece = square_board[square];

				printf(" %c", (piece == no_piece) ? '.' : ascii_pieces[piece]);
			}
			printf("\n");
		}
		printf("\n    a b c d e f g h \n\n");
		printf("    To move: %s\n", (sideMask) ? "black" : "white");
		printf("    enpassant square: %s\n", (enpassant_square != a8) ? square_coordinates[enpassant_square].c_str() : "none");

		printf("    castling rights: %c%c%c%c\n", (get_bit(castling_rights, 0)) ? 'K' : '-', (get_bit(castling_rights, 1)) ? 'Q' : '-', (get_bit(castling_rights, 2)) ? 'k' : '-', (get_bit(castling_rights, 3)) ? 'q' : '-');
		//print castling rights

		printf("    halfmoves since last pawn move or capture: %d\n", no_pawns_or_captures);
		printf("    current halfclock turn: %d\n", ply);
		printf("    current game turn: %d\n", (int)ply / 2 + (sideMask == falseMask));
		std::cout << "    current hash: " << current_hash << "\n";
	}
	std::string to_string() {
		std::stringstream stream = std::stringstream{};
		for (int rank = 0; rank < 8; rank++) {
			for (int file = 0; file < 8; file++) {
				//loop over board ranks and files

				int square = rank * 8 + file;
				//convert to square index

				if (!file) {
					stream<<' ' << 8 - rank;
				}//print rank on the left side

				int piece = no_piece;

				for (int piece_on_square = P; piece_on_square <= k; piece_on_square++) {
					if (get_bit(bitboards[piece_on_square], square)) {
						piece = piece_on_square;
						break;
					}
				}
				stream<< ' ' << ((piece == no_piece) ? '.' : ascii_pieces[piece]);
			}
			stream<<"\n";
		}
		stream<<"\n    a b c d e f g h \n\n";
		stream<<"    To move: "<< ((sideMask) ? "black" : "white") <<"\n";
		stream<<"    enpassant square: "<< ((enpassant_square != a8) ? square_coordinates[enpassant_square].c_str() : "none") <<"\n";
		std::string castling_rights_str = "";
		castling_rights_str += (get_bit(castling_rights, 0)) ? "K" : "-";
		castling_rights_str += (get_bit(castling_rights, 1)) ? "Q" : "-";
		castling_rights_str += (get_bit(castling_rights, 2)) ? "k" : "-";
		castling_rights_str += (get_bit(castling_rights, 3)) ? "q" : "-";
		stream << "    castling rights: "<<castling_rights_str<<"\n";

		stream << "    halfmoves since last pawn move or capture: "<< no_pawns_or_captures <<"\n";;
		stream<<"    current halfclock turn: "<< ply<<"\n";
		stream << "    current game turn: " << ((int)ply / 2 + (sideMask == falseMask)) << "\n";
		stream << "    current hash: " << current_hash << "\n";
		stream << "    fen: " << fen() << "\n";
		std::string ret = std::move(stream).str();
		return ret;
	}
	std::string square_board_to_string() const {
		std::stringstream stream = std::stringstream{};
		for (int rank = 0; rank < 8; rank++) {
			for (int file = 0; file < 8; file++) {
				//loop over board ranks and files

				int square = rank * 8 + file;
				//convert to square index

				if (!file) {
					stream << ' ' << 8 - rank;
				}//print rank on the left side

				int piece = square_board[square];

				for (int piece_on_square = P; piece_on_square <= k; piece_on_square++) {
					if (get_bit(bitboards[piece_on_square], square)) {
						piece = piece_on_square;
						break;
					}
				}
				stream << ' ' << ((piece == no_piece) ? '.' : ascii_pieces[piece]);
			}
			stream << "\n";
		}
		stream << "\n    a b c d e f g h \n\n";
		stream << "    To move: " << ((sideMask) ? "black" : "white") << "\n";
		stream << "    enpassant square: " << ((enpassant_square != a8) ? square_coordinates[enpassant_square].c_str() : "none") << "\n";
		std::string castling_rights_str = "";
		castling_rights_str += (get_bit(castling_rights, 0)) ? "K" : "-";
		castling_rights_str += (get_bit(castling_rights, 1)) ? "Q" : "-";
		castling_rights_str += (get_bit(castling_rights, 2)) ? "k" : "-";
		castling_rights_str += (get_bit(castling_rights, 3)) ? "q" : "-";
		stream << "    castling rights: " << castling_rights_str << "\n";

		stream << "    halfmoves since last pawn move or capture: " << no_pawns_or_captures << "\n";;
		stream << "    current halfclock turn: " << ply << "\n";
		stream << "    current game turn: " << ((int)ply / 2 + (sideMask == falseMask)) << "\n";
		stream << "    current hash: " << current_hash << "\n";
		std::string ret = std::move(stream).str();
		return ret;
	}
	U64 get_hash() const {
		U64 ret = 0ULL;
		for (int i = 0; i < 12; i++) {
			U64 board = bitboards[i];
			while (board) {
				const U64 isolated = board & twos_complement(board);
				ret ^= keys[bitscan(isolated) + i * 64];
				board = board & ones_decrement(board);
			}
		}

		ret ^= (get_bit(castling_rights, 0)) * keys[12 * 64];
		ret ^= (get_bit(castling_rights, 1)) * keys[12 * 64 + 1];
		ret ^= (get_bit(castling_rights, 2)) * keys[12 * 64 + 2];
		ret ^= (get_bit(castling_rights, 3)) * keys[12 * 64 + 3];
		//should be:
		//current_hash ^= ((bool)get_bit(different_rights, 0)) * keys[12 * 64];
		//current_hash ^= ((bool)get_bit(different_rights, 1)) * keys[12 * 64 + 1];
		//current_hash ^= ((bool)get_bit(different_rights, 2)) * keys[12 * 64 + 2];
		//current_hash ^= ((bool)get_bit(different_rights, 3)) * keys[12 * 64 + 3];
		//but the opening book was generated with the mistake, thus it is kept

		ret ^= sideMask & keys[772];
		assert(773 + enpassant_square % 8<781);
		ret ^= ((enpassant_square != a8) && (enpassant_square != 64)) * keys[static_cast<std::array<size_t, 781ULL>::size_type>(773 + (enpassant_square % 8))];
		return ret % 4294967296;
	}
	inline bool valid_move(const unsigned int move) {
		std::array<unsigned int, 128> legal_moves{};
		const int number_of_moves = get_legal_moves(legal_moves);
		bool found = false;
		for (int i = 0; i < number_of_moves; i++) {
			if (legal_moves[i] == move) return true;
		}
		return false;
	}
	constexpr U64 get_side() const { return sideMask; };
	constexpr int get_ply() const { return ply; };
	inline void update_hash(const unsigned int move) {
		const bool capture = get_capture_flag(move);
		const bool is_enpassant = get_enpassant_flag(move);
		const bool is_castle = get_castling_flag(move);
		const bool is_double_push = get_double_push_flag(move);

		const int piece_type = get_piece_type(move);
		const int from_square = get_from_square(move);
		const int to_square = get_to_square(move);
		const int captured_type = (capture)*get_captured_type(move);
		const int promoted_type = get_promotion_type(move);
		const bool isPromotion = promoted_type != no_piece;

		const int pieceOffset = 64 * piece_type;
		current_hash ^= keys[pieceOffset + from_square];
		const int afterPotentialPromotionOffset = 64 * ((isPromotion) * promoted_type + (!isPromotion) * piece_type);
		current_hash ^= keys[afterPotentialPromotionOffset + to_square];
		if (capture) {
			int actualCaptureSquare = to_square + (is_enpassant) * (-8 + (16 * (side)));
			current_hash ^= keys[captured_type * 64 + actualCaptureSquare];
		}
		else if (is_castle) {
			const int squareOffset = 56 * (side);
			const int rookSource = squareOffset + h8 * (to_square == (g8 + squareOffset));
			const int rookTarget = squareOffset + d8 + 2 * (to_square == (g8 + squareOffset));
			const int rookOffset = (piece_type - 2) * 64;
			current_hash ^= keys[rookOffset + rookSource];
			current_hash ^= keys[rookOffset + rookTarget];
		}
		const int different_rights = castling_rights ^ castling_rights_history.back();
		current_hash ^= (get_bit(different_rights, 0)) * keys[12 * 64];
		current_hash ^= (get_bit(different_rights, 1)) * keys[12 * 64 + 1];
		current_hash ^= (get_bit(different_rights, 2)) * keys[12 * 64 + 2];
		current_hash ^= (get_bit(different_rights, 3)) * keys[12 * 64 + 3];
		//should be:
		//current_hash ^= ((bool)get_bit(different_rights, 0)) * keys[12 * 64];
		//current_hash ^= ((bool)get_bit(different_rights, 1)) * keys[12 * 64 + 1];
		//current_hash ^= ((bool)get_bit(different_rights, 2)) * keys[12 * 64 + 2];
		//current_hash ^= ((bool)get_bit(different_rights, 3)) * keys[12 * 64 + 3];
		//but the opening book was generated with the mistake, thus it is kept
		const int old_enpassant_square = enpassant_history.back();
		current_hash ^= ((old_enpassant_square != a8) && (old_enpassant_square != 64)) * keys[static_cast<std::array<size_t, 781ULL>::size_type>(773) + old_enpassant_square % 8];
		//undo old enpassant key
		current_hash ^= (is_double_push)*keys[773 + (from_square % 8)];

		current_hash ^= keys[772];
		
		current_hash = current_hash % 4294967296;
	}
	inline void make_move(const unsigned int move) {
		no_pawns_or_captures_history.push_back(no_pawns_or_captures);
		enpassant_history.push_back(enpassant_square);
		move_history.push_back(move);
		castling_rights_history.push_back(castling_rights);
		hash_history.push_back(current_hash);
		ply++;
		
		const int piece_type = get_piece_type(move);
		const int from_square = get_from_square(move);
		const int to_square = get_to_square(move);
		const int captured_type = get_captured_type(move);
		const int promoted_type = get_promotion_type(move);

		const bool double_pawn_push = get_double_push_flag(move);
		const bool capture = get_capture_flag(move);
		const bool is_enpassant = get_enpassant_flag(move);
		const bool is_castle = get_castling_flag(move);


		const bool is_white_pawn = (piece_type == P);
		const bool is_black_pawn = (piece_type == p);
		no_pawns_or_captures = (!(is_white_pawn || is_black_pawn || capture)) * (no_pawns_or_captures + 1);
		//branchlessly increment the counter if move was not a pawn move^or a capture

		enpassant_square = (double_pawn_push) * (to_square + 8 - (16 & sideMask));
		//branchlessly set enpassant square

		const int offset = 6 & sideMask;

		if (!is_castle) {
			const bool bK = (piece_type == k);
			const bool bR = (piece_type == r);
			const bool wK = (piece_type == K);
			const bool wR = (piece_type == R);
			castling_rights &= ~(
				((int)(bK || (bR && (from_square == a8)) || (to_square == a8)) << 3)
				| ((int)(bK || (bR && (from_square == h8)) || (to_square == h8)) << 2)
				| ((int)(wK || (wR && (from_square == a1)) || (to_square == a1)) << 1)
				| ((int)(wK || (wR && (from_square == h1)) || (to_square == h1)))
				);
			//pop castle right bits
			if (capture) {
				int actualCaptureSquare = to_square + (is_enpassant) * (8 - (16 & sideMask));
				square_board[actualCaptureSquare] = no_piece;
				pop_bit(bitboards[captured_type], actualCaptureSquare);
				occupancies[(!side)] ^= (1ULL << actualCaptureSquare);
			}

			const bool not_promotion = promoted_type == no_piece;
			const int true_piece_type = (!not_promotion) * promoted_type | (not_promotion) * piece_type;
			bitboards[piece_type] ^= (1ULL << from_square) | ((U64)(not_promotion) << to_square);
			bitboards[true_piece_type] |= (U64)(!not_promotion) << to_square;
			square_board[to_square] = true_piece_type;
			occupancies[(side)] ^= (1ULL << to_square) | (1ULL << from_square);
		}
		else {
			const int square_offset = 56 & sideMask;
			const U32 is_kingside = (to_square > from_square) * trueMask32;
			const int rook_source = (int)((h1 & is_kingside) | (a1 & ~is_kingside)) - square_offset;
			const int rook_target = from_square + (to_square == g1 - square_offset) - (to_square == c1 - square_offset);
			bitboards[piece_type-2] ^= (1ULL << rook_source) | (1ULL << rook_target);
			const int bit_offset = 2 * (side);
			castling_rights &= ~(3ULL << bit_offset);
			//pop castle right bits if move is castling

			square_board[rook_source] = no_piece;
			square_board[rook_target] = piece_type - 2;
			square_board[to_square] = piece_type;
			bitboards[piece_type] ^= (1ULL << from_square) | (1ULL << to_square);
			occupancies[(side)] ^= (1ULL << to_square) | (1ULL << from_square) | (1ULL << rook_source) | (1ULL << rook_target);
		}
		square_board[from_square] = no_piece;
		//make move of piece

		sideMask = ~sideMask;
		side = !side;
		occupancies[both] = occupancies[white] | occupancies[black];

		update_hash(move);
	}
	inline void unmake_move() {
		const unsigned int move = move_history.back();
		move_history.pop_back();
		no_pawns_or_captures = no_pawns_or_captures_history.back();
		no_pawns_or_captures_history.pop_back();
		enpassant_square = enpassant_history.back();
		enpassant_history.pop_back();
		castling_rights = castling_rights_history.back();
		castling_rights_history.pop_back();
		current_hash = hash_history.back();
		hash_history.pop_back();
		ply--;

		const int piece_type = get_piece_type(move);
		const int from_square = get_from_square(move);
		const int to_square = get_to_square(move);
		const int captured_type = get_captured_type(move);
		const int promoted_type = get_promotion_type(move);

		const bool double_pawn_push = get_double_push_flag(move);
		const bool capture = get_capture_flag(move);
		const bool is_enpassant = get_enpassant_flag(move);
		const bool is_castle = get_castling_flag(move);

		set_bit(bitboards[piece_type], from_square);
		pop_bit(bitboards[piece_type], to_square);
		square_board[from_square] = piece_type;
		square_board[to_square] = no_piece;

		const bool is_promotion = promoted_type != no_piece;
		bitboards[(is_promotion)*promoted_type] &= ~(((U64)(is_promotion)) << (to_square));
		//branchlessly pop the piece that was promoted. if move was not a promotion the white pawns are and'ed with a bitboard of ones, thus it would not change
		if (!is_castle) {
			if (capture) {//set the captured piece
				const int captured_square = to_square + (is_enpassant) * (-8 + (16 & sideMask));
				square_board[captured_square] = captured_type;
				bitboards[captured_type] |= (1ULL << captured_square);
				occupancies[(side)] |= (1ULL << captured_square);
			}
		}
		else {
			const int square_offset = 56 & sideMask;
			const int is_kingside = (to_square > from_square) * trueMask32;
			const int rook_source = square_offset + a8 + 7 * (1 & is_kingside);
			const int rook_target = square_offset + d8 + 2 * (1 & is_kingside);
			const int rook_type = piece_type - 2; //since piece_type is the king of the castling color you can calc the rook_type
			bitboards[rook_type] ^= (1ULL << rook_source) | (1ULL << rook_target);
			//branchlessly set and pop rook bits if move was castling
			square_board[rook_target] = no_piece;
			square_board[rook_source] = rook_type;
			occupancies[(!side)] ^= (1ULL << rook_source) | (1ULL << rook_target);
		}
		occupancies[(!side)] ^= (1ULL << to_square) | (1ULL << from_square);
		occupancies[both] = occupancies[white] | occupancies[black];
		sideMask = ~sideMask;
		side = !side;
	}
	inline void make_nullmove() {
		move_history.push_back(0);
		enpassant_history.push_back(enpassant_square);
		castling_rights_history.push_back(castling_rights);
		no_pawns_or_captures_history.push_back(no_pawns_or_captures);
		hash_history.push_back(current_hash);
		no_pawns_or_captures++;
		ply++;
		current_hash ^= keys[772];
		const int old_enpassant_square = enpassant_history.back();
		//undo en passant hash in case it was not a8 (none)
		current_hash ^= ((old_enpassant_square != a8) && (old_enpassant_square != 64)) * keys[static_cast<std::array<size_t, 781ULL>::size_type>(773) + old_enpassant_square % 8];
		enpassant_square = a8;
		sideMask = ~sideMask;
		side = !side;
	}
	inline void unmake_nullmove() {
		no_pawns_or_captures = no_pawns_or_captures_history.back();
		no_pawns_or_captures_history.pop_back();
		enpassant_square = enpassant_history.back();
		enpassant_history.pop_back();
		castling_rights = castling_rights_history.back();
		castling_rights_history.pop_back();
		current_hash = hash_history.back();
		hash_history.pop_back();
		move_history.pop_back();
		ply--;
		sideMask = ~sideMask;
		side = !side;
	}
	int get_legal_moves(std::array<unsigned int,128>& ret) {
		const int kingpos = bitscan(bitboards[K + (int)(sideMask & 6)]);
		const bool in_check = is_attacked_by_side(kingpos, ~sideMask);
		const U64 enemy_attacks = get_attacks_by(~sideMask);
		int ind = (in_check) ? (legal_in_check_move_generator(ret, kingpos, enemy_attacks,0)) : (legal_move_generator(ret, kingpos, enemy_attacks,0));
		return ind;
	}
	inline bool is_draw_by_repetition() {
		return (std::count(hash_history.begin(), hash_history.end(), current_hash)) >= 3;
	}
	constexpr bool is_draw_by_fifty_moves() {
		return no_pawns_or_captures >= 50;
	}
	inline bool currently_in_check() {
		return is_attacked_by_side(bitscan(bitboards[K + (int)(sideMask & 6)]), ~sideMask);
	}
	inline U64 get_pawn_hash() {
		U64 ret = 0ULL;

		U64 board = bitboards[P];
		while (board) {
			const U64 isolated = board & twos_complement(board);
			ret ^= keys[bitscan(isolated) + P * 64];
			board = board & ones_decrement(board);
		}
		board = bitboards[p];
		while (board) {
			const U64 isolated = board & twos_complement(board);
			ret ^= keys[bitscan(isolated) + p * 64];
			board = board & ones_decrement(board);
		}
		return ret;
	}
	inline int get_kind_of_piece_on(const int sq) {
		bool found_piece;
		int piece_type;
		bool bit;
		for (int ind = 0; ind < 12; ind++) {
			bit = get_bit(bitboards[ind], sq);
			found_piece = bit;
			piece_type = (bit)*ind;
			if (bit) { break; }
		}
		const bool is_enpassant = (sq == enpassant_square);
		return no_piece * (!(found_piece || is_enpassant)) + (found_piece)*piece_type;
	}
	inline int get_smallest_attack(const int sq, const U64 color) {
		if (get_bit(occupancies[both], sq)) {
			unsigned int move = 0;
			set_promotion_type(move, no_piece);
			set_to_square(move, sq);
			set_captured_type(move, get_piece_type_on(sq));
			set_capture_flag(move, true);
			const int offset = 6 & color;
			U64 pot_pawns = pawn_attacks[1 & color][sq] & bitboards[offset];
			if (pot_pawns) {
				set_piece_type(move, P + offset);
				set_from_square(move, bitscan(pot_pawns));
				return move;
			}
			U64 pot_knights = knight_attacks[sq] & bitboards[N + offset];
			if (pot_knights) {
				set_piece_type(move, N + offset);
				set_from_square(move, bitscan(pot_knights));
				return move;
			}
			const U64 bishop_attacks = get_bishop_attacks(occupancies[both], sq);
			U64 pot_bishops = bishop_attacks & bitboards[B + offset];
			if (pot_bishops) {
				set_piece_type(move, B + offset);
				set_from_square(move, bitscan(pot_bishops));
				return move;
			}
			const U64 rook_attacks = get_rook_attacks(occupancies[both], sq);
			U64 pot_rooks = rook_attacks & bitboards[R + offset];
			if (pot_rooks) {
				set_piece_type(move, R + offset);
				set_from_square(move, bitscan(pot_rooks));
				return move;
			}
			U64 pot_queens = (rook_attacks | bishop_attacks) & bitboards[Q + offset];
			if (pot_queens) {
				set_piece_type(move, Q + offset);
				set_from_square(move, bitscan(pot_queens));
				return move;
			}
		}
		return 0;
	}
	inline int see(const int square) {
		int value = 0;
		unsigned int move = get_smallest_attack(square, sideMask);
		/* skip if the square isn't attacked anymore by this side */
		if (move) {
			make_move(move);
			/* Do not consider captures if they lose material, therefor max zero */
			value = std::max(0, basePieceValue[basePiece[get_captured_type(move)]] - see(square));
			unmake_move();
		}
		return value;
	}
	inline int seeByMove(const int move) {
		int value;
		make_move(move);
		/* Do not consider captures if they lose material, therefor max zero */
		value = std::max(0, basePieceValue[basePiece[get_captured_type(move)]] - see(get_to_square(move)));
		unmake_move();
		return value;
	}
	inline bool boardsMatch() {
		for (int i = 0; i < 64; i++) {
			short type = no_piece;
			for (int j = P; j <= k; j++) {
				if (get_bit(bitboards[j], i)) {
					type = j;
					break;
				}
			}
			if (type != square_board[i]) {
				std::cout << std::endl<< "mismatch at " << square_coordinates[i] << " : " << type << ", " << square_board[i] << std::endl;
				return false;
			}
		}
		return true;
	}
	inline std::string get_move_history() {
		std::string ret = "";
		for (int i = 0; i < move_history.size(); i++) {
			ret += uci(move_history[i]) +" ";
		}
		return ret;
	}
};
struct invalid_move_exception : std::exception {
	unsigned int move;
	std::string move_str;
	Position pos;
	invalid_move_exception(const Position t_pos, const unsigned int t_move) {
		pos = t_pos;
		move = t_move;
		move_str = uci(t_move);
	}
	invalid_move_exception(const Position t_pos, const std::string t_move) {
		pos = t_pos;
		move = -1;
		move_str = t_move;
	}
	const std::string what() throw() {
		return "Found invalid move " + move_str + " in position "+pos.fen();
	}
};