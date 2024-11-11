# Ray
Dedicated to Ray Finucane.\
A UCI chess engine written in C by Joe Chrisman.
### Features (v1.7.5)
#### Move generation
* Bitboard approach
* "Magic" bitboards for sliding pieces
* Compiler intrinsics for fast popcount
* Rigorously tested with custom perft
#### Search
* Hash table move ordering
* Captures ordered by mvv/lva
* Killer heuristic quiet move ordering
* History heuristic quiet move ordering
* Recursive null move pruning
* Futile move pruning
* Check extensions
* 512MB hash table
* Cut node, all node, and PV node hash table cutoffs
* Basic quiescence search
* Draw by repetition/50-move detection
* Iterative deepening at root
#### Evaluation
* Tapered evaluation
* Iteratively updated material balance
* Iteratively updated piece square tables
* Passed pawn bonus
* Isolated pawn penalty
* Doubled pawn penalty
* King safety evaluation
* King activity evaluation
* King pawn shield evaluation
* Bishop pair bonus
#### TODO
* Late move reductions
* Mess around with countermove heuristic
* Keep track of attack bitboard and use in evaluation
* Principal variation search experiments
* MTD(f) search experiments
* Aspiration window experiments
* More pruning/reductions in quiescence search
* Maybe use some kind of SEE in quiescence search
* Experiment with a pawn hash table
* Maybe migrate to C++ for better optimization across translation units

