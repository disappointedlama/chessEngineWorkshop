#pragma once
#include <unordered_map>
#include <thread>

#include "position.hpp"
#include <algorithm>
constexpr short EXACT = 0;
constexpr short UPPER = 1;
constexpr short LOWER = 2;
constexpr short Red = (short)1;
constexpr short lateMoveReduction = 2;
static constexpr int full_depth_moves = 8;
static constexpr int reduction_limit = 3;
struct stop_exception : std::exception {
	std::string source;
	stop_exception(std::string t_source);
	const std::string what() throw();
};
struct MoveWEval {
	unsigned int move;
	int eval;
	MoveWEval(const unsigned int t_move, const int t_eval) : move{ t_move }, eval{ t_eval } {}
};
struct TableEntry {
	unsigned int move_and_flag;
	unsigned int eval_and_depth;
	//28 bits are used in the move int thus two bits can be used to encode the flag
	//eval and depth can also be saved in a single int, with eval getting the first and depth getting the last 16 bits
	TableEntry() : move_and_flag { 0 }, eval_and_depth{ (int)(Position::infinity) << 16 } {}
	TableEntry(const unsigned int t_move, const short t_eval, const short t_flag, const short t_depth) : move_and_flag{ t_move | ((unsigned int)(t_flag) << 28) }, eval_and_depth{ ((unsigned int)t_eval) | ((unsigned int)t_depth << 16) } {}
	inline unsigned int get_move() const {
		return move_and_flag & 0xfffffff;
	}
	inline short get_flag() const {
		return (move_and_flag & 0xf0000000) >> 28;
	}
	inline short get_depth() const {
		return (eval_and_depth & 0xffff0000) >> 16;
	}
	inline short get_eval() const {
		return eval_and_depth & 0xffff;
	}
};
struct KillerTable {
	int table[512][3];
	void push_move(const unsigned int move, const short depth) {
		if (move == table[depth][0] || move == table[depth][2] || move == table[depth][1]) {
			return;
		}
		table[depth][2] = table[depth][1];
		table[depth][1] = table[depth][0];
		table[depth][0] = move;
	}
	bool find(const unsigned int move, const short depth) {
		return (table[depth][0] == move) || (table[depth][1] == move) || (table[depth][2] == move);
	}
	void shift_by(int shift) {
		int temp[512][3]{};
		for (int i = shift; i < 512; i++) {
			if (table[i][0] == 0 && table[i][1] == 0 && table[i][2] == 0) break;
			temp[i - shift][0] = table[i][0];
			temp[i - shift][1] = table[i][1];
			temp[i - shift][2] = table[i][2];
		}
	}
};

static std::array<std::array<U64, 64>, 12> history = std::array<std::array<U64, 64>, 12>{};
class Engine {
	Position pos;
	int current_desired_depth;
	int max_depth;
	std::atomic<bool> run;
	std::atomic<bool> pondering;
	bool debug;
	const static short infinity=Position::infinity;
	KillerTable killer_table;
	std::unordered_map<U64, TableEntry> map;
	U64 nodes;
	U64 time_for_next_move;
	bool check_time;
	inline TableEntry lookUp();
	void print_info(const short depth, const int eval, const U64 time);
	void track_time(const U64 max_time);
public:
	Engine() :pos{}, max_depth{ 8 }, run{ false }, debug{ false }, killer_table{}, map{}, nodes{ 0ULL }{};
	Engine(const bool t_debug) :pos{}, max_depth{ 8 }, run{ false }, debug{ t_debug }, killer_table{}, map{}, nodes{ 0ULL }{};
	int bestMove();
	inline void printBestMove(unsigned int move) {
		//std::this_thread::sleep_for(std::chrono::milliseconds(100));
		std::cout << "bestmove " + uci(move) << std::endl;
	}
	inline short evaluate();
	void set_debug(const bool t_debug);
	void set_max_depth(const short depth);
	void parse_position(std::string fen);
	void parse_go(std::string str);
	void reset_position();
	void uci_loop();
};