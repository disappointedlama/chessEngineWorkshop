#include "engine.hpp"

int Engine::bestMove() {
	std::array<unsigned int, 128> moves{};
	const int number_of_legal_moves = pos.get_legal_moves(moves);
	printBestMove(moves[0]);
	return moves[0];
}
inline short Engine::evaluate() {
	return 0;
}