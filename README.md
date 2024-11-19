# Ray
Dedicated to Ray Finucane.\
A UCI chess engine written in C.
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