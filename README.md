# Ray
Dedicated to Ray Finucane.\
A UCI chess engine written in C.
### Build instructions
This project uses a basic makefile. To build a binary, use one of the following commands:
* make (this will build a release version)
* make release (this will also build a release version)
* make debug (this will build a debug version)
This will work with all mainstream compilers except for MSVC

### Features
#### Move generation
* Bitboard approach
* "Magic" bitboards for sliding pieces
* Rigorously tested with custom perft
#### Search
* Principal variation search
* Hash table move ordering
* Captures ordered by mvv/lva
* Killer heuristic quiet move ordering
* History heuristic quiet move ordering
* Null move pruning (recursive)
* Futile move pruning (extended)
* Late move reductions
* Check extensions
* 512MB hash table
* Cut node, all node, and PV node hash table cutoffs
* Basic quiescence search
* Draw by repetition/50-move detection
* Iterative deepening at root
#### Evaluation
* Texel tuned piece square tables
* Tapered piece value
* Tapered placement value
* Incrementally updated end game advantage
* Incrementally updated middle game advantage
* Passed pawn bonus
* Isolated pawn penalty
* Doubled pawn penalty
* King safety with piece square tables
* King activity with piece square tables
* King pawn shield evaluation
* Bishop pair bonus
