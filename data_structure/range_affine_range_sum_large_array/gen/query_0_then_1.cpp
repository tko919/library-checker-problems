#include "random.h"
#include <iostream>
#include "../params.h"

using namespace std;

int main(int, char* argv[]) {
  long long seed = atoll(argv[1]);
  auto gen = Random(seed);

  int n = N_MAX;
  int q = Q_MAX;
  printf("%d %d\n", n, q);
  for (int i = 0; i < q; i++) {
    int t = (i < q / 2 ? 0 : 1);
    if (gen.uniform<int>(0, 100) == 0) t = 1 - t;
    auto p = gen.uniform_pair(0, n);
    if (t == 0) {
      int b = gen.uniform(1LL, MOD - 1);
      int c = gen.uniform(0LL, MOD - 1);
      printf("%d %d %d %d %d\n", t, p.first, p.second, b, c);
    } else {
      printf("%d %d %d\n", t, p.first, p.second);
    }
  }
  return 0;
}
