# Ray
Dedicated to Ray Finucane.\
A UCI chess engine written in C.
### Features (v1.8.1)
#### Move generation
* Bitboard approach
* "Magic" bitboards for sliding pieces
* Rigorously tested with custom perft
#### Search
* Hash table move ordering
* Captures ordered by mvv/lva
* Killer heuristic quiet move ordering
* History heuristic quiet move ordering
* Principal variation search
* Recursive null move pruning
* Futile move pruning
* Late move reductions
* Check extensions
* 512MB hash table
* Cut node, all node, and PV node hash table cutoffs
* Basic quiescence search
* Draw by repetition/50-move detection
* Iterative deepening at root
#### Evaluation
* Tapered evaluation
* Piece square tables
* Incrementally updated material balance
* Incrementally updated piece placement evaluation
* Passed pawn bonus
* Isolated pawn penalty
* Doubled pawn penalty
* King safety evaluation
* King activity evaluation
* King pawn shield evaluation
* Bishop pair bonus