// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Christoph Schwering (schwering@gmail.com)
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <algorithm>
#include <vector>

template<typename Key, typename Val>
class Map {
 public:
  void Resize(int size) { vec_.resize(size); }
  void Clear() { vec_.clear(); }

        Val& operator[](const Key& x)       { return vec_[int(x)]; }
  const Val& operator[](const Key& x) const { return vec_[int(x)]; }

  int size() { return vec_.size(); }
  std::vector<Val>& vec() { return vec_; }

  typename std::vector<Val>::iterator begin() { return vec_.begin(); }
  typename std::vector<Val>::iterator end()   { return vec_.end(); }

 private:
  std::vector<Val> vec_;
};

template<typename T, typename Less>
class PriorityQueue {
 public:
  explicit PriorityQueue(Less less = Less()) : less_(less) { heap_.emplace_back(); }

  void Resize(int size) { index_.Resize(size); }

  int size()  const { return heap_.size() - 1; }
  bool contains(const T& x) const { return index_[x] != 0; }
  T top() const { return heap_[bool(size())]; }

  void Insert(const T& x) {
    assert(!contains(x));
    const int i = heap_.size();
    heap_.push_back(x);
    index_[x] = i;
    SiftUp(i);
  }

  void Remove(const T& x) {
    assert(contains(x));
    const int i = index_[x];
    heap_[i] = heap_.back();
    index_[heap_[i]] = i;
    heap_.pop_back();
    index_[x] = 0;
    if (heap_.size() > i) {
      SiftDown(i);
    }
    assert(!contains(x));
  }

 private:
  static int left(const int i)   { return 2 * i; }
  static int right(const int i)  { return 2 * i + 1; }
  static int parent(const int i) { return i / 2; }

  void SiftUp(int i) {
    assert(i > 0 && i < heap_.size());
    const T x = heap_[i];
    int p;
    while ((p = parent(i)) != 0 && less_(x, heap_[p])) {
      heap_[i] = heap_[p];
      index_[heap_[i]] = i;
      i = p;
    }
    heap_[i] = x;
    index_[x] = i;
    assert(std::all_of(heap_.begin() + 1, heap_.end(), [this](T x) { return heap_[index_[x]] == x; }));
  }

  void SiftDown(int i) {
    assert(i > 0 && i < heap_.size());
    const T x = heap_[i];
    while (left(i) < heap_.size()) {
      const int min_child = right(i) < heap_.size() && less_(heap_[right(i)], heap_[left(i)]) ? right(i) : left(i);
      if (!less_(heap_[min_child], x)) {
        break;
      }
      heap_[i] = heap_[min_child];
      index_[heap_[i]] = i;
      i = min_child;
    }
    heap_[i] = x;
    index_[x] = i;
    assert(std::all_of(heap_.begin() + 1, heap_.end(), [this](T x) { return heap_[index_[x]] == x; }));
  }

  Less less_;
  std::vector<T> heap_;
  Map<T, int> index_;
};

class Var {
 public:
  Var() : id_(0) {}
  explicit Var(int id) : id_(id) {}

  explicit operator bool() const { return id_; }
  explicit operator int() const { return id_; }

  bool operator==(Var x) const { return id_ == x.id_; }
  bool operator!=(Var x) const { return id_ != x.id_; }
  bool operator<(Var x) const { return id_ < x.id_; }

 private:
  int id_ = 0;
};

enum Value { UNASSIGNED = 0, TRUE = -1, FALSE = 1 };

class Lit {
 public:
  Lit() : id_(0) {}
  explicit Lit(int id) : id_(id) {}
  Lit(Var x, bool sign) : id_(sign ? int(x) : -int(x)) {}

  explicit operator bool() const { return id_; }
  explicit operator int() const { return id_; }

  bool operator==(Lit x) const { return id_ == x.id_; }
  bool operator!=(Lit x) const { return id_ != x.id_; }
  bool operator<(Lit x) const { return id_ < x.id_; }

  Lit flip() const { return Lit(-id_); }
  bool complements(Lit x) const { return -id_ == x.id_; }

  bool pos() const { return id_ > 0; }
  bool neg() const { return id_ < 0; }
  Var var() const { return Var(id_ < 0 ? -id_ : id_); }
  Value value() const { return pos() ? TRUE : FALSE; }

 private:
  int id_ = 0;
};

class Clause {
 public:
  Clause() = default;
  explicit Clause(const std::vector<Lit>& lits, bool normalized = false) : lits_(lits) {
    if (!normalized) {
      Normalize();
    }
  }

  int size() const { return lits_.size(); }
        Lit& operator[](int i)       { return lits_[i]; }
  const Lit& operator[](int i) const { return lits_[i]; }

  std::vector<Lit>::const_iterator begin() const { return lits_.begin(); }
  std::vector<Lit>::const_iterator end()   const { return lits_.end(); }

 private:
  void Normalize() {
    std::sort(lits_.begin(), lits_.end());
    lits_.erase(std::unique(lits_.begin(), lits_.end()), lits_.end());
  }

  std::vector<Lit> lits_;
};

class ClauseRef {
 public:
  ClauseRef() : id_(0) {}
  explicit ClauseRef(int id) : id_(id) {}

  explicit operator bool() const { return id_; }
  explicit operator int() const { return id_; }

  bool operator==(ClauseRef r) const { return id_ == r.id_; }
  bool operator!=(ClauseRef r) const { return id_ != r.id_; }

        Clause& clause();
  const Clause& clause() const;

 private:
  int id_ = 0;
};

class Watchers {
 public:
  explicit Watchers(int n_vars = 0) : n_vars_(n_vars) { watchers_.resize(2 * n_vars_ + 1); }

        std::vector<ClauseRef>& operator[](Lit x)       { return watchers_[n_vars_ + int(x)]; }
  const std::vector<ClauseRef>& operator[](Lit x) const { return watchers_[n_vars_ + int(x)]; }

 private:
  int n_vars_;
  std::vector<std::vector<ClauseRef>> watchers_;
};

std::vector<Clause> clauses_;                   // database of clauses
bool empty_clause_ = false;                     // flag indicates if empty clause is in database

PriorityQueue<Var, std::less<Var>> var_order_;  // queue of unset literals

std::vector<Lit> trail_;                        // sequence of assigned literals
std::vector<int> level_size_;                   // map level -> number of assigned literals up to this level
int trail_head_ = 0;                            // index of first literal that hasn't been propagated yet

Map<Var, Value> model_;                         // map variable -> currently assigned value
Map<Var, int> level_;                           // map variable -> level it was set at
Map<Var, ClauseRef> cause_;                     // map variable -> clause that caused it to be set

Watchers watchers_;                             // map literal -> set of clauses watching literal

Map<Var, char> analyze_queue_;                  // map variable -> flag if variable enqueued during conflict analysis

const int ROOT_LEVEL = 1;

      Clause& ClauseRef::clause()       { return clauses_[id_ - 1]; }
const Clause& ClauseRef::clause() const { return clauses_[id_ - 1]; }

bool satisfied(Lit x) { return model_[x.var()] == x.value(); }
bool falsified(Lit x) { return model_[x.var()] == x.flip().value(); }

bool satisfied(const Clause& c) { for (Lit x : c) { if (satisfied(x)) { return true; } } return false; }
bool falsified(const Clause& c) { for (Lit x : c) { if (!falsified(x)) { return false; } } return true; }

int current_level() { return level_size_.size(); }

void NewLevel() { level_size_.push_back(trail_.size()); }

void AddLiteral(Lit x, ClauseRef cause = ClauseRef{}) {
  assert(!falsified(x));
  trail_.push_back(x);
  model_[x.var()] = x.value();
  level_[x.var()] = current_level();
  cause_[x.var()] = cause;
  assert(satisfied(x));
}

ClauseRef AddClause(const Clause& c) {
  if (c.size() == 0) {
    empty_clause_ = true;
    return ClauseRef{};
  } else if (c.size() == 1) {
    AddLiteral(c[0]);
    return ClauseRef{};
  } else {
    clauses_.push_back(c);
    ClauseRef cr = ClauseRef(clauses_.size());
    watchers_[c[0]].push_back(cr);
    watchers_[c[1]].push_back(cr);
    return cr;
  }
}

Var SelectVar() {
  Var x{};
  do {
    x = var_order_.top();
    if (!x) {
      return x;
    }
    var_order_.Remove(x);
  } while (model_[x] != UNASSIGNED);
  return x;
}

const ClauseRef Propagate(Lit x) {
  ClauseRef conflict{};
  std::vector<ClauseRef>& ws = watchers_[x.flip()];
  auto end = ws.end();
  for (auto it = ws.begin(); it != end; ++it) {
    ClauseRef cr = *it;
    Clause& c = cr.clause();
    // make c[1] a false literal
    if (x.complements(c[0])) {
      std::swap(c[0], c[1]);
    }
    assert(x.complements(c[1]));
    if (satisfied(c[0])) {
      goto next;
    }
    // find a new watched literal
    for (int i = 2; i < c.size(); ++i) {
      if (!falsified(c[i])) {
        std::swap(c[1], c[i]);
        watchers_[c[1]].push_back(cr);
        std::swap(*it--, *--end);
        goto next;
      }
    }
    // conflict or propagate if necessary
    if (falsified(c[0])) {
      assert(std::all_of(c.begin(), c.end(), [](Lit x) { return falsified(x); }));
      trail_head_ = trail_.size();
      conflict = cr;
      break;
    } else {
      assert(std::all_of(c.begin() + 1, c.end(), [](Lit x) { return falsified(x); }));
      if (!satisfied(c[0])) {
        AddLiteral(c[0], cr);
      }
    }
next: {}
  }
  assert(std::all_of(ws.begin(), end, [x](ClauseRef cr) { return x.complements(cr.clause()[0]) || x.complements(cr.clause()[1]); }));
  ws.resize(end - ws.begin());
  return conflict;
}

ClauseRef Propagate() {
  ClauseRef conflict{};
  while (trail_head_ < trail_.size() && !conflict) {
    Lit x = trail_[trail_head_++];
    conflict = Propagate(x);
  }
  return conflict;
}

void Analyze(ClauseRef conflict, std::vector<Lit>* learnt, int* backtrack_level) {
  Map<Var, char>& queue = analyze_queue_;  // global only for performance reasons
  assert(std::all_of(queue.begin(), queue.end(), [](char b) { return b == 0; }));
  int size = 0;
  Lit uip{};
  int i = trail_.size() - 1;
  learnt->emplace_back();
  // find UIP and literals from earlier levels
  do {
    for (Lit y : conflict.clause()) {
      if (uip == y) {
        continue;
      }
      if (!queue[y.var()] && level_[y.var()] > ROOT_LEVEL) {
        queue[y.var()] = true;
        if (level_[y.var()] >= current_level()) {
          ++size;
        } else {
          learnt->push_back(y);
        }
      }
    }
    while (!queue[trail_[i].var()]) {
      --i;
    }
    uip = trail_[i];
    --i;
    conflict = cause_[uip.var()];
    queue[uip.var()] = false;
    --size;
  } while (size > 0);
  (*learnt)[0] = uip.flip();
  // reset queue
  for (const Lit y : *learnt) {
    queue[y.var()] = false;
  }
  // find backtrack level and second watched literal
  if (learnt->size() == 1) {
    *backtrack_level = ROOT_LEVEL;
  } else {
    int max = 1;
    *backtrack_level = level_[(*learnt)[max].var()];
    for (int i = 2; i != learnt->size(); ++i) {
      if (*backtrack_level < level_[(*learnt)[i].var()]) {
        *backtrack_level = level_[(*learnt)[i].var()];
        max = i;
      }
    }
    std::swap((*learnt)[1], (*learnt)[max]);
  }
}

void Backtrack(int level) {
  for (auto it = trail_.cbegin() + level_size_[level]; it != trail_.cend(); ++it) {
    Var x = it->var();
    if (!var_order_.contains(x)) {
      var_order_.Insert(x);
    }
    model_[x] = UNASSIGNED;
  }
  trail_.resize(level_size_[level]);
  trail_head_ = trail_.size();
  level_size_.resize(level);
}

bool Solve() {
  if (empty_clause_) {
    return false;
  }
  for (;;) {
    ClauseRef conflict = Propagate();
    if (conflict) {
      if (current_level() == ROOT_LEVEL) {
        return false;
      }
      int backtrack_level;
      std::vector<Lit> learnt;
      Analyze(conflict, &learnt, &backtrack_level);
      Backtrack(backtrack_level);
      if (learnt.size() == 1) {
        AddLiteral(learnt[0]);
      } else {
        Lit x = learnt[0];
        ClauseRef cr = AddClause(Clause(learnt, true));
        AddLiteral(x, cr);
      }
    } else {
      Var x = SelectVar();
      if (!x) {
        return true;
      }
      NewLevel();
      AddLiteral(Lit(x, false));
    }
  }
}

void SkipComments(FILE* fp) {
  for (;;) {
    int c;
    while ((c = getc(fp)) == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r' || c == '\n') {
    }
    if (c != EOF && c == 'c') {
      while ((c = getc(fp)) != '\r' && c != '\n') {
      }
    } else {
      ungetc(c, fp);
      return;
    }
  }
}

int ReadCnf(FILE* fp) {
  int n_clauses;
  int n_vars;
  SkipComments(fp);
  int r = fscanf(fp, "p cnf %d %d\n", &n_vars, &n_clauses);
  if (r != 2) { fprintf(stderr, "Invalid header\n"); exit(1); }
  level_size_.push_back(0);
  model_.Resize(n_vars + 1);
  watchers_ = Watchers(n_vars);
  level_.Resize(n_vars + 1);
  cause_.Resize(n_vars + 1);
  var_order_.Resize(n_vars + 1);
  analyze_queue_.Resize(n_vars + 1);
  for (int x = 1; x <= n_vars; ++x) {
    var_order_.Insert(Var(x));
  }
  while (n_clauses > 0) {
    SkipComments(fp);
    std::vector<Lit> lits;
    int x = -1;
    while (fscanf(fp, "%d", &x) == 1) {
      if (x != 0) {
        if (!(-n_vars <= x && x <= n_vars)) { fprintf(stderr, "Literal %d out of range\n", x); exit(1); }
        lits.push_back(Lit(x));
      } else {
        AddClause(Clause(lits));
        --n_clauses;
        break;
      }
    }
    if (x != 0) { fprintf(stderr, "Found %d instead of clause terminating 0\n", x); exit(1); }
  }
  return n_vars;
}

int main(int argc, char* argv[]) {
  bool enumerate = false;
  const char* path = nullptr;
  for (int i = 1; i < argc; ++i) {
    if (!std::strcmp(argv[i], "-e")) {
      enumerate = true;
    } else {
      path = argv[i];
    }
  }
  if (!path) {
    printf("Usage: %s [-e] <file>\n", argv[0]);
    std::exit(1);
  }
  FILE* fp = fopen(path, "r");
  if (!fp) {
    printf("Could not open file\n");
    std::exit(1);
  }
  int n_vars = ReadCnf(fp);
  fclose(fp);
  for (;;) {
    std::clock_t time = std::clock();
    bool sat = Solve();
    time = std::clock() - time;
    printf("%sSATISFIABLE (in %lf s, sat-cdcl)\n", sat ? "" : "UN", time / static_cast<double>(CLOCKS_PER_SEC));
    if (sat) {
      for (int x = 1; x <= n_vars; ++x) {
        switch (model_[Var(x)]) {
          case TRUE:         printf("%d ", x);     break;
          case FALSE:        printf("%d ", -x);    break;
          case UNASSIGNED:   printf("!!%d!! ", x); break;
        }
      }
      printf("0\n");
      assert(std::all_of(clauses_.begin(), clauses_.end(), [](const Clause& c) { return satisfied(c); }));
    }
    if (enumerate && sat) {
      trail_head_ = 0;
      std::vector<Lit> lits;
      for (int x = 1; x <= n_vars; ++x) {
        lits.push_back(Lit(Var(x), model_[Var(x)] == TRUE).flip());
      }
      AddClause(Clause(lits));
      continue;
    } else {
      return 0;
    }
  }
}

