#pragma once
#include <unordered_map>
#include <thread>

#include "position.hpp"
#include <algorithm>
#include<atomic>
#include<cstring>
constexpr short EXACT = 0;
constexpr short UPPER = 1;
constexpr short LOWER = 2;
constexpr short Red = (short)1;
constexpr short lateMoveReduction = 2;
static constexpr int full_depth_moves = 8;
static constexpr int reduction_limit = 3;
struct stop_exception : std::exception {
	std::string source;
	stop_exception(std::string t_source) {
		source = t_source;
	}
	const std::string what() throw(){
		return "Stop exception thrown by " + source;
	}
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
	inline TableEntry lookUp() {
		auto yield = map.find(pos.current_hash);
		if (yield != map.end()) {
			return yield->second;
		}
		return TableEntry{ 0,0,0,-infinity-1 };
	}
	void print_info(const short depth, const int eval, const U64 time) {
		std::stringstream stream{};
		stream << "info score cp " << eval << " depth " << depth << " nodes " << nodes << " nps " << 1000000000 * nodes / time << " pv ";
		int j = 0;
		for (int i = 0; i < depth; i++) {
			TableEntry entry = lookUp();
			if (!entry.get_move()) break;
			j++;
			stream << uci(entry.get_move()) << " ";
			pos.make_move(entry.get_move());
		}
		for (int i = 0; i < j; i++) {
			pos.unmake_move();
		}
		stream<<"\n";
		std::string str = std::move(stream).str();
		std::cout << str;
	}

	void track_time(const U64 max_time) {
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		while (run) {
			end = std::chrono::steady_clock::now();
			if ((U64)std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count() >= max_time) {
				run = false;
				if(debug)std::cout << "stopping execution" << std::endl;
			}
		}
	}

public:
	Engine() :pos{}, max_depth{ 8 }, run{ false }, debug{ false }, killer_table{}, map{}, nodes{ 0ULL }{};
	Engine(const bool t_debug) :pos{}, max_depth{ 8 }, run{ false }, debug{ t_debug }, killer_table{}, map{}, nodes{ 0ULL }{};
	int bestMove();
	inline void printBestMove(unsigned int move) {
		//std::this_thread::sleep_for(std::chrono::milliseconds(100));
		std::cout << "bestmove " + uci(move) << std::endl;
	}
	inline short evaluate();
	
	void set_max_depth(const short depth) {
		max_depth = depth;
	}
	void set_debug(const bool t_debug) {
		debug = t_debug;
	}
	void parse_position(std::string fen) {
		std::string moves="";
		std::string str = " moves ";
		auto substr_pos = fen.find(str);
		if (substr_pos != std::string::npos) {
			moves = fen.substr(substr_pos + str.size(), fen.size());
			fen = fen.substr(0, substr_pos);
		}
		str = "startpos";
		substr_pos = fen.find(str);
		if (substr_pos != std::string::npos) {
			fen = start_position;
		}
		str = "fen ";
		substr_pos = fen.find(str);
		if (substr_pos != std::string::npos) {
			fen = fen.substr(substr_pos + str.size(), fen.size());
		}
		pos = Position(fen);
		try {
			while (moves != "") {
				std::array<unsigned,128> move_list{};
				pos.get_legal_moves(move_list);
				std::string move_string = moves.substr(0, moves.find_first_of(' '));
				if (move_string.size() > 4) {
					const int last = move_string.size() - 1;
					if ((move_string[last] != 'n') && (move_string[last] != 'b') && (move_string[last] != 'r') && (move_string[last] != 'q')) {
						move_string = move_string.substr(0, last);
					}
				}
				bool matching_move_found = false;
				for (int i = 0; i < move_list.size(); i++) {
					if (uci(move_list[i]) == move_string) {
						pos.make_move(move_list[i]);
						matching_move_found = true;
						break;
					}
				}
				if (!matching_move_found) {
					throw invalid_move_exception(pos, move_string);
				}
				if (move_string.size() + 1 < moves.size()) {
					moves = moves.substr(move_string.size() + 1, moves.size());
				}
				else { moves = ""; }
			}
		}
		catch (invalid_move_exception e) {
			std::cout << e.what() << std::endl;
		}
		if(debug) pos.print();
	}
	void reset_position() {
		pos = Position{ start_position };
	}
	void parse_go(std::string str){
		check_time = false;
		std::cout << str << std::endl;
		std::string command = "depth ";
		auto substr_pos = str.find(command);
		if (substr_pos != std::string::npos) {
			str = str.substr(substr_pos + command.size(), str.size());
			max_depth = stoi(str);
			bestMove();
			return;
		}
		command = "infinite";
		substr_pos = str.find(command);
		if (substr_pos != std::string::npos) {
			str = str.substr(substr_pos + command.size(), str.size());
			max_depth = infinity;
			bestMove();
			return;
		}
		command = "movetime ";
		substr_pos = str.find(command);
		if (substr_pos != std::string::npos) {
			str = str.substr(substr_pos + command.size(), str.size());
			std::thread time_tracker = std::thread(&Engine::track_time, this, stoull(str)*1000000ULL);
			max_depth = infinity;
			bestMove();
			while (true) {
				if (time_tracker.joinable()) {
					time_tracker.join();
					break;
				}
			}
			return;
		}
		command = "wtime ";
		substr_pos = str.find(command);
		if (substr_pos != std::string::npos) {
			str = str.substr(substr_pos + command.size(), str.size());
			std::string time_str = str.substr(0, str.find(" "));
			const int wtime = stoi(time_str);
			command = "btime ";
			substr_pos = str.find(command);
			str = str.substr(substr_pos + command.size(), str.size());
			time_str = str.substr(0, str.find(" "));
			const int btime = stoi(time_str);
			command = "winc ";
			substr_pos = str.find(command);
			str = str.substr(substr_pos + command.size(), str.size());
			time_str = str.substr(0, str.find(" "));
			const int winc = stoi(time_str);
			command = "binc ";
			substr_pos = str.find(command);
			str = str.substr(substr_pos + command.size(), str.size());
			time_str = str.substr(0, str.find(" "));
			const int binc = stoi(time_str);

			const int increment = ((pos.side) * binc) + (!(pos.side) * winc);
			const int time = ((pos.side) * btime) + ((!pos.side) * wtime);
			time_for_next_move = time / 25 + increment / 2;
			if (time_for_next_move >= time) {
				time_for_next_move = time - 500;
			}
			if (time_for_next_move < 0) {
				time_for_next_move = 100;
			}
			std::cout << "Thinking time: " << time_for_next_move << std::endl;
			time_for_next_move *= 1000000ULL;
			std::thread time_tracker = std::thread(&Engine::track_time, this, time_for_next_move);
			max_depth = 100;
			check_time = true;
			bestMove();
			while (true) {
				if (time_tracker.joinable()) {
					time_tracker.join();
					break;
				}
			}
		}
	}

	void uci_loop(){

		fflush(stdin);
		fflush(stdout);
		char input[2000];

		std::vector<std::thread> workers{};
		while (true) {
			memset(input, 0, sizeof(input));
			fflush(stdout);
			if (!fgets(input, 2000, stdin)) {
				continue;
			}
			if (!run) {
				for (int i = 0; i < workers.size(); i++) {
					if (workers[i].joinable()) {
						workers[i].join();
						workers.erase(workers.begin() + i);
					}
				}
				pondering = false;
			}//if no thread is running join all threads and delete joined threads from workers

			if (input[0] == '\n') {
				continue;
			}
			if (strncmp(input, "isready", 7) == 0) {
				std::cout << "readyok\n";
			}
			else if (strncmp(input, "position", 8) == 0) {
				parse_position(input);
			}
			else if (strncmp(input, "ucinewgame", 10) == 0) {
				parse_position("position startpos");
				map = std::unordered_map<U64, TableEntry>{};
				history = std::array<std::array<U64, 64>, 12>{};
				if(debug) std::cout << "Done with cleanup\n";
			}
			else if (strncmp(input, "go", 2) == 0) {
				if (!run) {
					run = true;
					std::cout << input << std::endl;
					workers.push_back(std::thread(&Engine::parse_go, this, input));
				}

				using namespace std::literals::chrono_literals;
				std::this_thread::sleep_for(500us);
			}
			else if (strncmp(input, "quit", 4) == 0) {
				pondering = false;
				run = false;
				for (int i = 0; i < workers.size(); i++) {
					if (workers[i].joinable()) {
						workers[i].join();
						workers.erase(workers.begin() + i);
					}
				}
				break;
			}
			else if (strncmp(input, "stop", 4) == 0) {
				if (pondering) {
					pondering = false;
					run = false;
					for (int i = 0; i < workers.size(); i++) {
						if (workers[i].joinable()) {
							workers[i].join();
							workers.erase(workers.begin() + i);
						}
					}
					if (!run) {
						run = true;
						workers.push_back(std::thread(&Engine::parse_go, this, (char*)("go depth 7")));
					}

					using namespace std::literals::chrono_literals;
					std::this_thread::sleep_for(500us);
				}
				else {
					run = false;
					for (int i = 0; i < workers.size(); i++) {
						if (workers[i].joinable()) {
							workers[i].join();
							workers.erase(workers.begin() + i);
						}
					}
					pos = Position{ "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1" };
				}
			}
			else if (strncmp(input, "uci", 3) == 0) {
				std::cout << "id name ENGINE author Teilnehmer\n";
				std::cout << "option name Move Overhead type spin default 100 min 0 max 20000\noption name Threads type spin default 2 min 2 max 2\noption name Hash type spin default 512 min 256" << std::endl;
				std::cout << "uciok\n";
			}
			else if (strncmp(input, "debug", 5) == 0) {
				if (strncmp(input + 6, "true", 4) == 0) {
					debug = true;
				}
				else if (strncmp(input +6 , "false", 5) == 0) {
					debug = false;
				}
				std::cout << "Debug is set to " << ((debug) ? ("true") : ("false")) << std::endl;
			}
		}
	}
};