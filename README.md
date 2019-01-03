# SAT Solvers

## Overview

This archive contains the code of three different SAT solvers:

1. `sat-naive.cc` implements the naive algorithm from Slide 21
2. `sat-up.cc` implements the algorithm from Slide 25 with the
   Watched-Literal Scheme
3. `sat-cdcl.cc` implements the algorithm from Slide 31 with the
   Watched-Literal Scheme and the FirstUIP scheme for choosing
   conflict clauses.

The files represent the evolution of our SAT algorithm in the lecture.
Each of the files is stand-alone.

They are in written in C++ 2011. On Linux you can compile them with

~~~~
$ c++ -O3 -DNDEBUG -Wall -std=c++11 sat-naive.cc -o sat-naive
$ c++ -O3 -DNDEBUG -Wall -std=c++11 sat-up.cc -o sat-up
$ c++ -O3 -DNDEBUG -Wall -std=c++11 sat-cdcl.cc -o sat-cdcl
~~~~

The flag `-O3` tells the compiler to optimise the program, `-DNDEBUG` disables
assertions (the `assert()` statements in the code), `-Wall` tells the compiler
to warn about potential mistakes, and `-std=c++11` enables the C++ 2011
standard.
You may get some warnings regarding comparison between signed and unsigned
integers because I've used `int` throughout the code whereas the return type
of `std::vector<T>::size()` is the unsigned integer `std::size_type`. You can
ignore these warnings.

Then run `./sat-[naive|up|cdcl] [-e] <cnf-file>`. The optional flag `-e`
makes the solver enumerate all solutions. The input file must be in DIMACS CNF
format.

There are probably still a few bugs in the solvers. Let me know if you find one.


## Documentation

All three solvers are structured as follows and actually largely identical:

1. Type definitions
2. Global variables
3. Solver functions
5. Main function

Here's some further explanation of the types and functions.

### Type Definitions

* `Map<Key, Value>`:
  In SAT solving, maps (that is, key-value pairs) are very handy. Luckily
  the keys are integers or variables or literals, which can be represented
  as integers.
  `Map<Key, Value>` is an abstraction layer for such a array-based map where
  `Key` is a type that can be converted to int and `Value` is an arbitrary type.
* `PriorityQueue<T, Less>`:
  A priority queue allows to insert elements and take out the element with
  highest priority in an efficient way. This implementation uses a min-heap.
  You may safely skip this data structure.
  All that's important is that it provides a function `top()` to access the
  highest-priority element, and functions `Insert()` and `Remove()` to add or
  remove elements from the queue.
  If you're interested in how a min-heap works, Wikipedia is probably a good
  resource.
* `Var`:
  A propositional variable is just represented as a positive integer (the
  integer `0` is used to represent a 'null' value).
  This type allows for a little abstraction so we don't confuse other
  integers with variables.
* `Value`:
  The value to which a partial interpretation may map a variable: true,
  false, or unassigned.
* `Lit`:
  A variable or its negation is represented as a non-zero integer (the
  integer `0` is used to represent a 'null' value).
* `Clause`:
  A clause is a set of literals. We implement it as an array. To make sure
  no literal occurs twice, we normalise it, that is, we sort it and remove
  any duplicates.
* `ClauseRef`:
  A reference to a clause is a positive integer that identifies a clause (the
  integer `0` is used to represent a 'null' value).
  We will store clauses in a global array. A `ClauseRef` is the index of a
  clause in this array. The also provide a `clause()` function to obtain the
  clause associated with a reference.
  If you're familiar with C or C++ pointers: we could use pointers just as
  well. 32 bit integers should suffice though and safe a little space over
  64 bit pointers.
- `Watchers` [`sat-up.cc` and `sat-cdcl.cc` only]:
  In the Watched-Literal Scheme there are two distinguished literals for
  each clause (except for the empty and unit clauses, of course). When we
  set a literal x to False and want to propagate this assignment, it suffices
  to check those clauses which watch x. The Watchers data structure allows us
  to register a clause as a watcher for a literal. It is implemented as an
  array of arrays: for each Lit, there is an array of ClauseRefs.

### Global Variables

* `clauses_`:
  Is the list of non-empty and non-unit clauses in our formula,
  that is, of all clauses with at least two literals.
* `empty_clause_`:
  Is true when the formula contains the empty clause, in
  which case it is unsatisfiable.
* `var_order_`:
  Is a priority queue of variables. For our implementations,
  the ordering itself it not important. All that matters is that we have
  way of selecting variables to make decisions.
  Choosing a smarter order (see Slide 34) is one way to improve our
  solvers.
* `trail_`:
  The literals in the order of assignment.
  Implemented as array of literals.
  That is, it corresponds to the partial interpretation /I/ in our
  algorithms, but it additionally represents the ordering in which the
  assignments were made.
* `level_size_`:
  The size of the different decision levels.
  For each decision, a new level is created to ease backtracking. The
  size of the level is the number of literals that were assigned at the
  level.
  In the naive algorithm, there is exactly one literal per decision level.
  The other two algorithms do unit propagation after each assignment, which
  may lead to additional assignments after a decision. We want to memorize
  to which level which assignment belongs for backtracking.
* `trail_head_` [`sat-up.cc` and `sat-cdcl.cc` only]:
  The index of the first element of trail_ that has not been propagated yet.
  Newly assigned literals enter a queue for unit propagation, the head of
  which is where `trail_head_` points to.
* `model_`:
  The values of variables.
  Represents the same data as `trail_`, but instead of being ordered
  chronologically, `model_` allows for fast access of the value of a given
  variable.
* `level_` [`sat-cdcl.cc` only]:
  The level at which a variable was set.
  Represents the similar data as `level_size_` together with `trail_`, but
  instead of being ordered chronologically, `level_` allows for fast access
  of the level of a given variable.
  Useful for conflict analysis.
* `cause_` [`sat-cdcl.cc` only]:
  The clause that caused a given variable to be set by unit propagation.
  Useful for conflict analysis.
* `watchers_` [`sat-up.cc` and `sat-cdcl.cc` only]:
  Provides fast access to clauses that are watching a given literal.
* `analyze_queue_` [`sat-cdcl.cc` only]:
  Auxiliary array for conflict analysis, which is only global for performance
  reasons.
* `ROOT_LEVEL`:
  The root level of the decision tree. 

### Solver Functions

* `current_level()` and `NewLevel()`:
  We associate with every decision a new level, which simplifies backtracking.
* `AddLiteral()`:
  When an assignment is made, the corresponding global variables are updated:
  `trail_` and `model_` and, in `sat-cdcl.cc`, also `level_` and `cause_`.
* `AddClause()`:
  Adds a clause from the input formula, or, in `sat-up.cc` and `sat-cdcl.cc`,
  newly learnt conflict clauses.
  If the clause is empty, `empty_clause_` is set, and if it is a unit clause,
  it is handed to `AddLiteral()`. Otherwise, if it contains at least two
  literals, it is added to `clauses_`. In `sat-up.cc` and `sat-cdcl.cc`,
  the clause is furthermore registered as watcher for its first two literals
* `SelectVar()`:
  Takes the next variable out of `var_order_`. If there is none, the null `Var`
  is returned.
* `Backtrack()`:
  Undoes the assignments made since the given level. That is, the `trail_`
  and `level_size_` and, in `sat-up.cc` and `sat-cdcl.cc`, `trail_head_` and
  need to be shrinked. And for each un-assigned literal, the entry for that
  variable in `model_` is reset.
* `Propagate()` [`sat-up.cc` and `sat-cdcl.cc` only]:
  Propagates all literals that haven't been propagated yet, that is, those
  that have been added to `trail_` after `trail_head_`. This itself may
  enqueue new literals on the trail, which are also propagated iteratively.
  Propagating a literal works by inspecting all clauses that watch the negation
  of that literal, using `watches_`.
* `Analyze()` [`sat-cdcl.cc` only]:
  Analyses a conflict and returns a new conflict clause. The conflict clause
  is organised in a way that after backtracking to the destination level, the
  first literal of the clause is not falsified, but all the other ones are.
  (This first literal is the unique implication point.)
* `Solve()` [different for `sat-naive.cc`, `sat-up.cc`, `sat-cdcl.cc`]:
  Implements the algorithm from Slide 21, 25, and 31, respectively.

### Main Function

* `ReadCnf()` and `SkipComments()`:
  Parse the DIMACS CNF file and add the read clauses with `AddClause()`.
* `main()`:
  Open and process the DIMACS CNF file, run the solver, print the solution.

## Possible Improvements

The watched-literal scheme and clause learning are the most important concepts
of SAT solvers, but they are not the only ingredients that make modern SAT
solvers fast.

The next conceptual improvement would be to use a smarter variable selection
heuristic. All you need to do is to replace the `std::less<Var>` parameter in
`PriorityQueue<Var, std::less<Var>>` with a different ranking.
Using `std::less<Var>` means that at the moment variables with a small IDs are
preferred over ones with higher IDs -- which is, of course, completely
unjustified.
A better heuristic would be to score variables by their activity: assign them
some numeric scores which are increased when the variable plays a role in
a conflict or unit propagation, for example.

Other potential improvements are phase-saving or randomized restarts. As with
the variable selection heuristic, these additions are relatively easy to
implement. However, tweaking the parameters (e.g., when to restart) might be
tricky.

Finally, another way to improve performance is to improve the code. The most
promising function to start with is probably `Propagate()`. I didn't try to
optimise it in any way, but since the solver spends most of its time on unit
propagation, every improvement here should make a big difference.
I haven't profiled the solvers though and there may (also) be other high-impact
optimisations.

## Visualisation

You can play around with the visualisation of the search tree at <web/sat.html>.
You can install it locally as well with Emscripten, a compiler back end for
Clang that produces Javascript.

