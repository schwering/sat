// vim:filetype=cpp:textwidth=120:shiftwidth=2:softtabstop=2:expandtab
// Christoph Schwering (c.schwering@unsw.edu.au)
#include <cstring>
#include <cstdio>

#include <emscripten.h>

#define LOG_ENQUEUE(x, l) enqueued(x, l)

class Lit;
void enqueued(const class Lit& x, int level);

#include "sat.cc"

void enqueued(const class Lit& x, int level) {
  EM_ASM_({
    enqueued($0, $1);
  }, int(x), level);
}

extern "C" void sat_solve(const char* s, bool clause_learning) {
  FILE* fp = fmemopen((void*) s, strlen(s), "r");
  int n_vars = ReadCnf(fp);
  fclose(fp);
  std::clock_t time = std::clock();
  int sat = Solve();
  time = std::clock() - time;
  printf("%sSATISFIABLE (in %lf s, %s)\n", sat ? "" : "UN", time / static_cast<double>(CLOCKS_PER_SEC), SAT_VERSION);
  if (sat) {
    for (int x = 1; x <= n_vars; ++x) {
      switch (model_[Var(x)]) {
        case TRUE:         printf("%d ", x);     break;
        case FALSE:        printf("%d ", -x);    break;
        case UNASSIGNED:   printf("!!%d!! ", x); break;
      }
    }
    printf("0\n");
  }
}

