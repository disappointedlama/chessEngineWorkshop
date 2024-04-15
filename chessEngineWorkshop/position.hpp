#pragma once
#include <vector>
#include <chrono>
#include <cassert>
#include <unordered_map>

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
	inline bool is_attacked_by_side(const int sq, const bool color);
	inline U64 get_attacks_by(const U64 color);
	inline int get_piece_type_on(const int sq) const;
	inline int get_piece_type_or_enpassant_on(const int sq) {
		if (sq == enpassant_square && sq != a8) return ~sideMask & p;
		return square_board[sq];
	};

	int legal_move_generator(std::array<unsigned int,128>& ret, const int kingpos, const U64 enemy_attacks, int ind);
	int legal_in_check_move_generator(std::array<unsigned int,128>& ret, const int kingpos, const U64 enemy_attacks, int ind);

	int legal_capture_gen(std::array<unsigned int,128>& ret, const U64 enemy_attacks, int ind);
	int legal_in_check_capture_gen(std::array<unsigned int,128>& ret, const U64 enemy_attacks, int ind);
	int in_check_get_pawn_captures(std::array<unsigned int,128>& ret, const U64 pinned, const U64 targets, int ind);
	int get_pawn_captures(std::array<unsigned int,128>& ret, const U64 pinned, int ind);

	inline U64 get_pinned_pieces(const int kingpos, const U64 enemy_attacks);
	inline U64 get_moves_for_pinned_pieces(std::array<unsigned int,128>& ret, const int kingpos, const U64 enemy_attacks,int &ind);
	inline U64 get_captures_for_pinned_pieces(std::array<unsigned int,128>& ret, const int kingpos, const U64 enemy_attacks, int& ind);
	inline U64 get_checkers(const int kingpos);
	inline U64 get_checking_rays(const int kingpos);
	void try_out_move(std::array<unsigned int,128>& ret, unsigned int move, int& ind);
	inline int get_legal_pawn_moves(std::array<unsigned int,128>& ret, const U64 pinned, int ind);
	inline int legal_bpawn_pushes(std::array<unsigned int,128>& ret, const U64 pinned, int ind);
	inline int legal_wpawn_pushes(std::array<unsigned int,128>& ret, const U64 pinned, int ind);
	inline int legal_bpawn_captures(std::array<unsigned int,128>& ret, const U64 pinned, int ind);
	inline int legal_wpawn_captures(std::array<unsigned int, 128>& ret, const U64 pinned, int ind);
	inline int legal_b_enpassant(std::array<unsigned int, 128>& ret, int ind);
	inline int legal_w_enpassant(std::array<unsigned int, 128>& ret, int ind);

	inline int in_check_get_legal_pawn_moves(std::array<unsigned int,128>& ret, const U64 pinned, const U64 targets, const U64 in_check_valid, int ind);
	inline int in_check_legal_bpawn_pushes(std::array<unsigned int,128>& ret, const U64 pinned, const U64 targets, const U64 in_check_valid, int ind);
	inline int in_check_legal_wpawn_pushes(std::array<unsigned int,128>& ret, const U64 pinned, const U64 targets, const U64 in_check_valid, int ind);
	inline int in_check_legal_bpawn_captures(std::array<unsigned int,128>& ret, const U64 pinned, const U64 targets, int ind);
	inline int in_check_legal_wpawn_captures(std::array<unsigned int,128>& ret, const U64 pinned, const U64 targets, int ind);

	inline int get_castles(std::array<unsigned int,128>& ptr, const U64 enemy_attacks, int ind);
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
	Position();
	Position(const std::string& fen);
	void parse_fen(std::string fen);
	std::string fen() const;
	void print();
	void print_square_board() const;
	std::string to_string();
	std::string square_board_to_string() const;
	U64 get_hash() const;
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
	inline void update_hash(const unsigned int move);
	inline void make_move(const unsigned int move);
	inline void unmake_move();
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
		current_hash ^= ((old_enpassant_square != a8) && (old_enpassant_square != 64)) * keys[static_cast<std::array<size_t, 781Ui64>::size_type>(773) + old_enpassant_square % 8];
		enpassant_square = a8;
		sideMask = ~sideMask;
		side = !side;
		//const U64 generatedHash = get_hash();
		//if (current_hash != generatedHash) {
		//	print();
		//	std::cout << "updated hash: " << current_hash << "\ngenerated hash: " << generatedHash << std::endl;
		//	for (int i = 0; i < 100; i++) {
		//		std::cout << "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" << std::endl;
		//	}
		//	throw Position_Error("aaaaaaaaaaaa");
		//}
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
	int get_legal_moves(std::array<unsigned int,128>& ret);
	int get_legal_captures(std::array<unsigned int,128>& ret);
	constexpr bool is_draw_by_repetition() {
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
	invalid_move_exception(const Position t_pos, const unsigned int t_move);
	invalid_move_exception(const Position t_pos, const std::string t_move);
	const std::string what() throw();
};