#include <stdio.h>

#include "Utils/Graph.h"

#include "Utils/Dataset.h"

#include <set>

int main(int argc, char **argv) {
    const char * DATA_SRC = Dataset::DATA_CTR_OBJ, *DATA_SRC2 = Dataset::DATA_CTR_COLOR;
    AdjacentMatrix g(Dataset::DATA_CTR_OBJ);
    // count for edges (excluding parallel edges)
    int m = 0;
    for (int i = 0; i < g.n; i++) {
      std::set<int> targets;
      for (int j = g.g[i]; j < g.g[i+1]; j++) {
        if (targets.find(g.archArray[j].j) == targets.end() && g.archArray[j].j != i) {
          m++;
          targets.insert(g.archArray[j].j);
        }
      }
    }
    printf("%d %d\n", g.n, m/2);
    // for (int i = 0; i < g.n; i++) {
    //   std::set<int> targets;
    //   for (int j = g.g[i]; j < g.g[i+1]; j++) {
    //     if (targets.find(g.archArray[j].j) == targets.end()) {
    //       if (j == g.g[i]) {
    //         printf("%d", g.archArray[j].j+1);
    //       } else {
    //         printf(" %d", g.archArray[j].j+1);
    //       }
    //       targets.insert(g.archArray[j].j);
    //     }
    //   }
    //   printf("\n");
    // }
    return 0;
}

