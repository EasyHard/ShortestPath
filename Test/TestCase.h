#ifndef TEST_CASE_H_
#define TEST_CASE_H_

#include <assert.h>
#include <time.h>
#include <vector>
#include <stdlib.h>
#include "../Solver/Solver.h"

class TestCase {
public:
    static AdjacentMatrix *generateSmallRandomRawGraph(int n = 10) {
        int m = n * (n - 1);
        AdjacentMatrix::Arch* archArray = (AdjacentMatrix::Arch*) malloc(sizeof(AdjacentMatrix::Arch) * m);
        assert(archArray!=NULL);
        int top = 0;
        for (int i = 0; i < n; ++i)
            for (int j = i + 1; j < n; ++j) {
                int w = rand() % 10 + 1;
                archArray[top].i = i;
                archArray[top].j = j;
                archArray[top].w = w;
                ++top;
                archArray[top].i = j;
                archArray[top].j = i;
                archArray[top].w = w;
                ++top;
            }
        AdjacentMatrix *ret = new AdjacentMatrix(n, m, archArray);
        free(archArray);
        return ret;
    }

    static void runTest(CacheFriendlyAdjacentMatrix &g2, Colors *colors, int source, Solver<AdjacentMatrix> *solver, bool isFirstSolver = true, bool isDebug = false, int ntimes = 3, double* time = NULL) {
        static int first = -1, ret = -1;
        std::vector<int> a, b;
        for (int i = 0; i < ntimes; ++i) {
            double prepareTimeS, runningTimeS, totalScan, avgScan;
            solver->compute(g2, colors, source, prepareTimeS, runningTimeS, totalScan, avgScan);
            printf("-------------CacheFriendlyAdjacentMatrix---------------\n");
            printf("%s\n\tn=%d m=%d\n\tprepareTimeS=%lf runningTimeS=%lf checksum=%08x\n\t\ttotalScan=%.0lf avgScan=%.1lf\n", solver->getName(), g2.n, g2.m, prepareTimeS, runningTimeS, ret = g2.checkSum(), totalScan, avgScan);
            if (time != NULL) {
              time[i] = runningTimeS;
            }
            if (isDebug) g2.showResult();
            printf("\n");

            fflush(stdout);
            if (isFirstSolver)
                first = ret;
            else
                assert(first == ret);
        }
    }

    static void runTest(AdjacentMatrix &g2, Colors *colors, int source, Solver<AdjacentMatrix> *solver, bool isFirstSolver = true, bool isDebug = false, int ntimes = 3, double* time = NULL) {
      printf("ntimes = %d\n", ntimes);
        static int first = -1, ret = -1;

        for (int i = 0; i < ntimes; ++i) {
            double prepareTimeS, runningTimeS, totalScan, avgScan;
            solver->compute(g2, colors, source, prepareTimeS, runningTimeS, totalScan, avgScan);
            printf("-------------AdjacentMatrix---------------\n");
            printf("%s\n\tn=%d m=%d\n\tprepareTimeS=%lf runningTimeS=%lf checksum=%08x\n\t\ttotalScan=%.0lf avgScan=%.1lf\n", solver->getName(), g2.n, g2.m, prepareTimeS, runningTimeS, ret = g2.checkSum(), totalScan, avgScan);
            if (time != NULL) {
              time[i] = runningTimeS;
            }
            if (isDebug) g2.showResult();
            printf("\n");

            fflush(stdout);
            if (isFirstSolver)
                first = ret;
            else
                assert(first == ret);
        }
    }

public:

    static void testSmallRandomRawGraph() {
        srand(0);
        puts("test Small random graph");
        for (int times = 0; times < 1000; ++times) {
            printf("test No=%d\n", times);
            const int THREAD_NUM = 4;
            AdjacentMatrix *graph = generateSmallRandomRawGraph();
            graph->print();
            Colors colors, colors2;
            colors.color_BFS_DeltaDepth(*graph, 0, 1);
            colors.refine(THREAD_NUM);
            colors.print();

            colors2.color_BFS_DeltaDepth(*graph, 0, 1);
            colors2.refine(THREAD_NUM);
            CacheFriendlyAdjacentMatrix graph2(graph->n, graph->m, graph->archArray);
            reorg(graph2, colors2);
            colors2.print();

            {
                DijstraHeap solver;
                runTest(*graph, &colors, 0, &solver, true, true);
                runTest(graph2, &colors2, 0, &solver, true, true);
            }
            {
                AdjustDij_NoGlobalSync solver(THREAD_NUM, 2);
                runTest(*graph, &colors, 0, &solver, false, true);
                runTest(graph2, &colors2, 0, &solver, false, true);
            }
            {
                AdjustDij_GlobalHeapStack solver(THREAD_NUM, 2);
                runTest(*graph, &colors, 0, &solver, false, true);
                runTest(graph2, &colors2, 0, &solver, false, true);
            }
            {
                AdjustDij_NoLock solver(THREAD_NUM, 2);
                runTest(*graph, &colors, 0, &solver, false, true);
                runTest(graph2, &colors2, 0, &solver, false, true);
            }
            {
                AdjustDij_Timer solver(THREAD_NUM, 2);
                runTest(*graph, &colors, 0, &solver, false, true);
                runTest(graph2, &colors2, 0, &solver, false, true);
            }
            {
                //AdjustDij_NoAtomic fails when dataset is small
                //AdjustDij_NoAtomic solver(THREAD_NUM, 2);
                //runTest(*graph, &colors, 0, &solver, false, true);
            }
            {
                AdjustDij_NoAtomic2 solver(THREAD_NUM, 2);
                runTest(*graph, &colors, 0, &solver, false, true);
                runTest(graph2, &colors2, 0, &solver, false, true);
            }
            {
                AdjustDij_NoAtomic3 solver(THREAD_NUM, 2);
                runTest(*graph, &colors, 0, &solver, false, true);
                runTest(graph2, &colors2, 0, &solver, false, true);
            }
            {
                AdjustDij_NoAtomic4 solver(THREAD_NUM, 2);
                runTest(*graph, &colors, 0, &solver, false, true);
                runTest(graph2, &colors2, 0, &solver, false, true);
            }

            delete graph;
        }
    }

    static void testAdjustDij(const char * DATA_SRC) {
        const int THREAD_NUM = 8;

        AdjacentMatrix g(DATA_SRC);

        {
            DijstraHeap solver;
            runTest(g, NULL, 0, &solver, true, false);
        }

        CacheFriendlyAdjacentMatrix g2(g.n, g.m, g.archArray);

        for (int DELTA_DEPTH = 2 << 4; DELTA_DEPTH <= 2 << 4; DELTA_DEPTH <<= 1) {
            printf("\nDELTA_DEPTH=%d\n", DELTA_DEPTH);

            Colors colors;
            colors.color_BFS_DeltaDepth(g, 0, DELTA_DEPTH);
            colors.refine(THREAD_NUM);

            Colors colors2;
            colors2.color_BFS_DeltaDepth(g, 0, DELTA_DEPTH);
            colors2.refine(THREAD_NUM);
            reorg(g2, colors2);

            for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; num_relax_each_iter *= 2) {
                AdjustDij_NoGlobalSync solver(THREAD_NUM, num_relax_each_iter);
                runTest(g, &colors, 0, &solver, false, false);
                runTest(g2, &colors2, 0, &solver, true, false);
            }

            for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; num_relax_each_iter *= 2) {
                AdjustDij_GlobalHeapStack solver(THREAD_NUM, num_relax_each_iter);
                runTest(g, &colors, 0, &solver, false, false);
                runTest(g2, &colors2, 0, &solver, false, false);
            }

            for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; /*65536;*/num_relax_each_iter *= 2) {
                AdjustDij_NoLock solver(THREAD_NUM, num_relax_each_iter);
                runTest(g, &colors, 0, &solver, false, false);
                runTest(g2, &colors2, 0, &solver, false, false);
            }

            for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; /*65536;*/num_relax_each_iter *= 2) {
                AdjustDij_NoAtomic solver(THREAD_NUM, num_relax_each_iter);
                runTest(g, &colors, 0, &solver, false, false);
                runTest(g2, &colors2, 0, &solver, false, false);
            }

            for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; /*65536;*/num_relax_each_iter *= 2) {
                AdjustDij_NoAtomic2 solver(THREAD_NUM, num_relax_each_iter);
                runTest(g, &colors, 0, &solver, false, false);
                runTest(g2, &colors2, 0, &solver, false, false);
            }

            for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; /*65536;*/num_relax_each_iter *= 2) {
                AdjustDij_NoAtomic3 solver(THREAD_NUM, num_relax_each_iter);
                runTest(g, &colors, 0, &solver, false, false);
                runTest(g2, &colors2, 0, &solver, false, false);
            }

            for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; /*65536;*/num_relax_each_iter *= 2) {
                AdjustDij_NoAtomic4 solver(THREAD_NUM, num_relax_each_iter);
                runTest(g, &colors, 0, &solver, false, false);
                runTest(g2, &colors2, 0, &solver, false, false);
            }
        }
    }


    static void testSeperateDij(const char * DATA_SRC1, const char * DATA_SRC2, const char * SOURCE_SRC, int THREAD_NUM) {
        AdjacentMatrix g(DATA_SRC1);
        Colors colors;
        colors.readFromObjectFile(g, DATA_SRC2);
        colors.refine(THREAD_NUM);
        for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; num_relax_each_iter *= 2) {
          SeperateDijk solver(THREAD_NUM, SOURCE_SRC, 1024/* num_relax_each_iter */);
            runTest(g, &colors, 0, &solver, true, false);
        }
    }

private:


    static void testAdjustDijSpeed(const char * DATA_SRC2, int THREAD_NUM, AdjacentMatrix &g) {
        Colors colors;
        colors.readFromObjectFile(g, DATA_SRC2);
        colors.refine(THREAD_NUM);

        for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; num_relax_each_iter *= 2) {
            AdjustDij_GlobalHeapStack solver(THREAD_NUM, num_relax_each_iter);
            runTest(g, &colors, 0, &solver, true, false);
        }

        for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; /*65536;*/num_relax_each_iter *= 2) {
            AdjustDij_NoLock solver(THREAD_NUM, num_relax_each_iter);
            runTest(g, &colors, 0, &solver, false, false);
        }

        // for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; /*65536;*/num_relax_each_iter *= 2) {
        //     AdjustDij_NoAtomic solver(THREAD_NUM, num_relax_each_iter);
        //     runTest(g, &colors, 0, &solver, false, false);
        // }

        // for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; /*65536;*/num_relax_each_iter *= 2) {
        //     AdjustDij_NoAtomic2 solver(THREAD_NUM, num_relax_each_iter);
        //     runTest(g, &colors, 0, &solver, false, false);
        // }

        // for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; /*65536;*/num_relax_each_iter *= 2) {
        //     AdjustDij_NoAtomic3 solver(THREAD_NUM, num_relax_each_iter);
        //     runTest(g, &colors, 0, &solver, false, false);
        // }

        for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; /*65536;*/num_relax_each_iter *= 2) {
            AdjustDij_NoAtomic4 solver(THREAD_NUM, num_relax_each_iter);
            runTest(g, &colors, 0, &solver, false, false);
        }
    }

    static void testSourceNodes(const char * DATA_SRC, const char * DATA_SRC2,
                                int source, int nthread, int bfsdelta,
                                const char * SOURCE_SRC = "./Dataset/sourceNodes") {
        AdjacentMatrix g(DATA_SRC);
        Colors colors;
        if (bfsdelta != -1 && bfsdelta != 0) {
          colors.color_BFS_DeltaDepth(g, 0, 16);
        } else {
          colors.readFromObjectFile(g, DATA_SRC2);
        }
        colors.refine(nthread);
        SourceNodesGenerator solver(SOURCE_SRC);
        double prepareTimeS, runningTimeS, totalScan, avgScan;
        solver.compute(g, &colors, source, prepareTimeS, runningTimeS, totalScan, avgScan);
    }

    static void testSourceNodesPad(const char * DATA_SRC, const char * DATA_SRC2,
                                int source, int nthread, int bfsdelta,
                                const char * SOURCE_SRC = "./Dataset/sourceNodes") {
      AdjacentMatrix g(DATA_SRC);
      Colors colors;
      if (bfsdelta != -1 && bfsdelta != 0) {
        colors.color_BFS_DeltaDepth(g, 0, 16);
      } else {
        colors.readFromObjectFile(g, DATA_SRC2);
      }
      colors.refine(nthread);
      SourceNodesGenerator solver(SOURCE_SRC);
      double prepareTimeS, runningTimeS, totalScan, avgScan;
      solver.compute(g, &colors, source, prepareTimeS, runningTimeS, totalScan, avgScan);
    }
    static void testSourceNodesReorg(const char * DATA_SRC, const char * DATA_SRC2,
                                     int source, int nthread, int bfsdelta,
                                     const char * SOURCE_PATH = "./Dataset/sourceNodes") {
      CacheFriendlyAdjacentMatrix g(DATA_SRC); Colors colors;
      colors.readFromObjectFile(g, DATA_SRC2);
      int max = -1, min = 0x7fffffff;
      for (int i = 0; i < g.n; i++) {
        if (max < colors.getColors()[i]) max = colors.getColors()[i];
        if (min > colors.getColors()[i]) min = colors.getColors()[i];
      }
      printf("max = %d, min = %d\n", max, min);
      colors.refine(max+1);
      colors.printSummary();
      reorg(g, colors);
      colors.refine(nthread);
      colors.printSummary();
      reorg(g, colors);
      SourceNodesGenerator solver(SOURCE_PATH);
      double prepareTimeS, runningTimeS, totalScan, avgScan;
      solver.compute(g, &colors, source, prepareTimeS, runningTimeS, totalScan, avgScan);
    }

    static void testSourceNodesReorgPad(const char * DATA_SRC, const char * DATA_SRC2,
                                        int source, int nthread,
                                        const char * SOURCE_PATH = "./Dataset/sourceNodes") {
        CacheFriendlyAdjacentMatrix g(DATA_SRC); Colors colors;
        colors.readFromObjectFile(g, DATA_SRC2);
        int max = -1, min = 0x7fffffff;
        for (int i = 0; i < g.n; i++) {
          if (max < colors.getColors()[i]) max = colors.getColors()[i];
          if (min > colors.getColors()[i]) min = colors.getColors()[i];
        }
        printf("max = %d, min = %d\n", max, min);
        colors.refine(max+1);
        colors.printSummary();
        reorg(g, colors);
        colors.refine(nthread);
        colors.printSummary();
        reorg(g, colors);
        g.pad(4, colors.colors);
        for (int j = 0; j < 4; j++) {
          printf("bfa tos[%d] = %d\n", j, g.tos[j]);
        }
        Colors padcolors;
        padcolors.fromArray(g.nafterpad, g.padcolor);
        for (int j = 0; j < 4; j++) {
          printf("tos[%d] = %d\n", j, g.tos[j]);
        }
        SourceNodesGeneratorPad solver(SOURCE_PATH);
        double prepareTimeS, runningTimeS, totalScan, avgScan;
        solver.compute(g, &padcolors, source, prepareTimeS, runningTimeS, totalScan, avgScan);
    }

public:

    static void testAdjustDijSpeed(const char * DATA_SRC, const char * DATA_SRC2) {
        AdjacentMatrix g(DATA_SRC);

#ifndef IS_MIC
        testAdjustDijSpeed(DATA_SRC2, 4, g);
        testAdjustDijSpeed(DATA_SRC2, 8, g);
#else
        testAdjustDijSpeed(DATA_SRC2, 60, g);
        testAdjustDijSpeed(DATA_SRC2, 120, g);
        //        testAdjustDijSpeed(DATA_SRC2, 240, g);
#endif
    }






    static void testSourceNodesReorgNomerge(const char * DATA_SRC, const char * DATA_SRC2) {
        CacheFriendlyAdjacentMatrix g(DATA_SRC); Colors colors;
        colors.readFromObjectFile(g, DATA_SRC2);
        int max = -1, min = 0x7fffffff;
        for (int i = 0; i < g.n; i++) {
          if (max < colors.getColors()[i]) max = colors.getColors()[i];
          if (min > colors.getColors()[i]) min = colors.getColors()[i];
        }
        printf("max = %d, min = %d\n", max, min);
        colors.refine(max+1);
        colors.printSummary();
        reorg(g, colors);
        g.buildSimple();
        SourceNodesGenerator solver("./Dataset/USA-road-CTR.obj.source3500");
        double prepareTimeS, runningTimeS, totalScan, avgScan;
        solver.compute(g, &colors, 0, prepareTimeS, runningTimeS, totalScan, avgScan);
    }

    static void testSeperateDijReorg(const char * DATA_SRC, const char * DATA_SRC2, const char * SOURCE_SRC, int THREAD_NUM) {
        CacheFriendlyAdjacentMatrix g(DATA_SRC); Colors colors;
        colors.readFromObjectFile(g, DATA_SRC2);
        printf("read color, finished\n");
        int max = -1, min = 0x7fffffff;
        for (int i = 0; i < g.n; i++) {
          if (max < colors.getColors()[i]) max = colors.getColors()[i];
          if (min > colors.getColors()[i]) min = colors.getColors()[i];
        }
        printf("max = %d, min = %d\n", max, min);
        colors.refine(max+1);
        reorg(g, colors);
        summaryReorg(g, colors);
        colors.refine(THREAD_NUM);
        reorg(g, colors);
        //g.buildSimple();
        SeperateDijk solver(THREAD_NUM, SOURCE_SRC, 32);
        runTest(g, &colors, 0, &solver, true, false);

        // for (int i = 0; i < 3; i++) {
        //   SeperateDijk solver(THREAD_NUM, SOURCE_SRC, 32);
        //   double pp, rt, ts, as;
        //   solver.compute(g, &colors, 0, pp, rt, ts, as);
        // }

    }

    static void testSeperateDijReorgPad(const char * DATA_SRC, const char * DATA_SRC2, const char * SOURCE_SRC, int THREAD_NUM) {
        CacheFriendlyAdjacentMatrix g(DATA_SRC); Colors colors;
        colors.readFromObjectFile(g, DATA_SRC2);
        printf("read color, finished\n");
        int max = -1, min = 0x7fffffff;
        for (int i = 0; i < g.n; i++) {
          if (max < colors.getColors()[i]) max = colors.getColors()[i];
          if (min > colors.getColors()[i]) min = colors.getColors()[i];
        }
        printf("max = %d, min = %d\n", max, min);
        colors.refine(max+1);
        reorg(g, colors);
        summaryReorg(g, colors);
        colors.refine(THREAD_NUM);
        reorg(g, colors);
        g.pad(4, colors.colors);
        Colors padcolors;
        padcolors.fromArray(g.nafterpad, g.padcolor);
        padcolors.refine(max+1);
        g.reallocResultPad();
        SeperateDijk solver(THREAD_NUM, SOURCE_SRC, 32);
        runTest(g, &padcolors, 0, &solver, true, false);

        // for (int i = 0; i < 3; i++) {
        //   SeperateDijk solver(THREAD_NUM, SOURCE_SRC, 32);
        //   double pp, rt, ts, as;
        //   solver.compute(g, &colors, 0, pp, rt, ts, as);
        // }
    }

    static void testSeperateDijReorgNomerge(const char * DATA_SRC, const char * DATA_SRC2, const char * SOURCE_SRC, int THREAD_NUM) {
        CacheFriendlyAdjacentMatrix g(DATA_SRC); Colors colors;
        colors.readFromObjectFile(g, DATA_SRC2);
        int max = -1, min = 0x7fffffff;
        for (int i = 0; i < g.n; i++) {
          if (max < colors.getColors()[i]) max = colors.getColors()[i];
          if (min > colors.getColors()[i]) min = colors.getColors()[i];
        }
        printf("max = %d, min = %d\n", max, min);
        colors.refine(max+1);
        reorg(g, colors);
        // summaryReorg(g, colors);
        g.buildSimple();
        for (int i = 0; i < 3; i++) {
          SeperateDijk solver(THREAD_NUM, SOURCE_SRC, 32);
          double pp, rt, ts, as;
          solver.compute(g, &colors, 0, pp, rt, ts, as);
        }
        //runTest(g, &colors, 0, &solver, true, false);
    }

    static void summaryReorg(CacheFriendlyAdjacentMatrix& g, Colors& colors) {
      for (int i = 0; i < g.n-1; i++) {
        if (colors.getColors()[i] != colors.getColors()[i+1]) {
          assert(colors.getColors()[i] + 1 == colors.getColors()[i+1]);
          printf("seperator: %d, color: %d\n", i, colors.getColors()[i]);
        }
      }
    }
    static void testAdjustDijSpeedCachedFriendly(const char * DATA_SRC, const char * DATA_SRC2, int THREAD_NUM) {
        CacheFriendlyAdjacentMatrix g(DATA_SRC); Colors colors;
        srand(time(NULL));
        //int source = rand() % g.n;
        int source = 0;
        printf("source = %d\n", source);
        //colors.readFromObjectFile(g, DATA_SRC2);
        colors.color_BFS_DeltaDepth(g, 0, 16);
        int max = -1, min = 0x7fffffff;
        for (int i = 0; i < g.n; i++) {
          if (max < colors.getColors()[i]) max = colors.getColors()[i];
          if (min > colors.getColors()[i]) min = colors.getColors()[i];
        }
        assert(min == 0);
        colors.refine(max + 1);
        reorg(g, colors);
        colors.refine(THREAD_NUM);
        colors.printSummary();
        reorg(g, colors);
        g.pad(4, colors.colors);
        g.reallocResultPad();
        Colors padcolors;
        padcolors.fromArray(g.nafterpad, g.padcolor);
        padcolors.refine(THREAD_NUM);

        for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; /*65536;*/num_relax_each_iter *= 2) {
            AdjustDij_NoAtomic4 solver(THREAD_NUM, num_relax_each_iter);
            runTest(g, &colors, source, &solver, true, false);
        }
        for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; /*65536;*/num_relax_each_iter *= 2) {
            AdjustDij_NoAtomic4Pad solver(THREAD_NUM, num_relax_each_iter);
            runTest(g, &padcolors, g.newNodeMap[source], &solver, true, false);
        }
        for (int num_relax_each_iter = 32; num_relax_each_iter <= 32; /*65536;*/num_relax_each_iter *= 2) {
            AdjustDij_NoAtomic4PadL solver(THREAD_NUM, num_relax_each_iter);
            runTest(g, &padcolors, g.newNodeMap[source], &solver, true, false);
        }
    }

  static void runSolverNoReorg(const char * DATA_SRC, const char * DATA_SRC2, int THREAD_NUM, int source, int bfsdelta, int num_relax_each_iter, int ntimes, FILE* file = NULL) {
    AdjacentMatrix g(DATA_SRC); Colors colors;
    if (source == -1) {
      source = rand() % g.n;
    } else {
      source = source % g.n;
    }
    printf("source = %d, ntimes = %d\n", source, ntimes);
    if (bfsdelta != 0) {
      colors.color_BFS_DeltaDepth(g, 0, bfsdelta);
    } else {
      colors.readFromObjectFile(g, DATA_SRC2);
    }
    colors.refine(THREAD_NUM);
    colors.printSummary();
    g.pad(4, colors.colors);
    g.reallocResultPad();
    Colors padcolors;
    padcolors.fromArray(g.nafterpad, g.padcolor);
    padcolors.refine(THREAD_NUM);
    {
      AdjustDij_NoAtomic4 solver(THREAD_NUM, num_relax_each_iter);
      runTest(g, &colors, source, &solver, true, false, 2, NULL);
    }
    double times[3][ntimes];
    {
      AdjustDij_NoAtomic4 solver(THREAD_NUM, num_relax_each_iter);
      runTest(g, &colors, source, &solver, true, false, ntimes, &times[0][0]);
    }
    {
      AdjustDij_NoAtomic4Pad solver(THREAD_NUM, num_relax_each_iter);
      runTest(g, &padcolors, g.newNodeMap[source], &solver, true, false, ntimes, &times[1][0]);
    }
    {
      AdjustDij_NoAtomic4PadL solver(THREAD_NUM, num_relax_each_iter);
      runTest(g, &padcolors, g.newNodeMap[source], &solver, true, false, ntimes, &times[2][0]);
    }
    if (file != NULL) {
      fprintf(file, "%s, thread = %d noreorg\n", DATA_SRC, THREAD_NUM);
    }
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < ntimes; j++) {
        printf("%lf\t", times[i][j]);
        if (file != NULL) {
          fprintf(file, "%lf\t", times[i][j]);
        }
      }
      printf("\n");
      if (file != NULL) {
        fprintf(file, "\n");
      }
    }
  }

  static void runSolverReorg(const char * DATA_SRC, const char * DATA_SRC2, int THREAD_NUM, int source, int bfsdelta, int num_relax_each_iter, int ntimes, FILE* file = NULL) {
    CacheFriendlyAdjacentMatrix g(DATA_SRC); Colors colors;
    if (source == -1) {
      source = rand() % g.n;
    } else {
      source = source % g.n;
    }
    printf("source = %d\n", source);
    if (bfsdelta != 0) {
      colors.color_BFS_DeltaDepth(g, 0, bfsdelta);
    } else {
      colors.readFromObjectFile(g, DATA_SRC2);
    }
    int max = -1, min = 0x7fffffff;
    for (int i = 0; i < g.n; i++) {
      if (max < colors.getColors()[i]) max = colors.getColors()[i];
      if (min > colors.getColors()[i]) min = colors.getColors()[i];
    }
    assert(min == 0);
    colors.refine(max + 1);
    reorg(g, colors);
    source = g.index[source];
    colors.refine(THREAD_NUM);
    colors.printSummary();
    reorg(g, colors);
    source = g.index[source];
    g.pad(4, colors.colors);
    g.reallocResultPad();
    Colors padcolors;
    padcolors.fromArray(g.nafterpad, g.padcolor);
    padcolors.refine(THREAD_NUM);
    {
      AdjustDij_NoAtomic4 solver(THREAD_NUM, num_relax_each_iter);
      runTest(g, &colors, source, &solver, true, false, 2, NULL);
    }
    double times[3][ntimes];
    {
      AdjustDij_NoAtomic4 solver(THREAD_NUM, num_relax_each_iter);
      runTest(g, &colors, source, &solver, true, false, ntimes, &times[0][0]);
    }
    {
      AdjustDij_NoAtomic4Pad solver(THREAD_NUM, num_relax_each_iter);
      runTest(g, &padcolors, g.newNodeMap[source], &solver, true, false, ntimes, &times[1][0]);
    }
    {
      AdjustDij_NoAtomic4PadL solver(THREAD_NUM, num_relax_each_iter);
      runTest(g, &padcolors, g.newNodeMap[source], &solver, true, false, ntimes, &times[2][0]);
    }
    if (file != NULL) {
      fprintf(file, "%s, thread = %d reorg\n", DATA_SRC, THREAD_NUM);
    }
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < ntimes; j++) {
        if (file != NULL) {
          fprintf(file, "%lf\t", times[i][j]);
        }
        printf("%lf\t", times[i][j]);
      }
      if (file != NULL) {
        fprintf(file, "\n");
      }
      printf("\n");
    }
  }

    static void testAdjustDijSpeedCachedFriendly(const char * DATA_SRC, const char * DATA_SRC2) {
#ifndef IS_MIC
        testAdjustDijSpeedCachedFriendly(DATA_SRC, DATA_SRC2, 4);
        testAdjustDijSpeedCachedFriendly(DATA_SRC, DATA_SRC2, 8);
#else
        //        testAdjustDijSpeedCachedFriendly(DATA_SRC, DATA_SRC2, 60);
        testAdjustDijSpeedCachedFriendly(DATA_SRC, DATA_SRC2, 120);
        // testAdjustDijSpeedCachedFriendly(DATA_SRC, DATA_SRC2, 180);
        // testAdjustDijSpeedCachedFriendly(DATA_SRC, DATA_SRC2, 240);
#endif
    }

    static void testClock() {
        omp_lock_t lock;
        omp_init_lock(&lock);

        const int THREAD_NUM = 8;
        omp_set_num_threads(THREAD_NUM);

        CycleTimer mainTimer;
        mainTimer.Start();

#pragma omp parallel
        {
            CycleTimer localTimer;
            localTimer.Start(&mainTimer.getStart());
            const int pid = omp_get_thread_num();
            for (int i = 0; i < 10; ++i) {
                omp_set_lock(&lock);
                printf("pid=%d, clock()=%ld, timer=%lld\n", pid, clock(), localTimer.getTimeFromStart());
                fflush(stdout);
                omp_unset_lock(&lock);
            }
        }

        omp_destroy_lock(&lock);
    }

    enum SOLVER_TYPE {
        ST_DIJ_HEAP, ST_OMP_LOCK_EXTRA_MEM, ST_OMP_LOCK, ST_GLOBAL_VERSION, ST_TIMER_VERSION, ST_PROTOCOL, ST_PROTOCOL2
    };

    static void singleTest(const SOLVER_TYPE stype, const int THREAD_NUM = 8, const int num_relax_each_iter = 64) {
        AdjacentMatrix g(Dataset::DATA_CTR_OBJ);
        Colors colors;
        colors.readFromObjectFile(g, Dataset::DATA_CTR_COLOR64);
        colors.refine(THREAD_NUM);

        Solver<AdjacentMatrix> *pSolver = NULL;
        AdjustDij_GlobalHeapStack solver(THREAD_NUM, num_relax_each_iter);
        switch (stype) {
            case ST_DIJ_HEAP:
                pSolver = new DijstraHeap();
                break;
            case ST_OMP_LOCK_EXTRA_MEM:
                pSolver = new AdjustDij_NoGlobalSync(THREAD_NUM, num_relax_each_iter);
                break;
            case ST_OMP_LOCK:
                pSolver = new AdjustDij_GlobalHeapStack(THREAD_NUM, num_relax_each_iter);
                break;
            case ST_GLOBAL_VERSION:
                pSolver = new AdjustDij_NoLock(THREAD_NUM, num_relax_each_iter);
                break;
            case ST_TIMER_VERSION:
                pSolver = new AdjustDij_Timer(THREAD_NUM, num_relax_each_iter);
                break;
            case ST_PROTOCOL:
                pSolver = new AdjustDij_NoAtomic(THREAD_NUM, num_relax_each_iter);
                break;
            case ST_PROTOCOL2:
                pSolver = new AdjustDij_NoAtomic2(THREAD_NUM, num_relax_each_iter);
                break;
        }
        assert(pSolver!=NULL);
        runTest(g, &colors, 0, pSolver, true, false);

        delete pSolver;
    }
};
#endif
