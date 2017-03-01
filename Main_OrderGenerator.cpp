#include <stdio.h>

#include "Utils/Graph.h"
#include "Solver/Solver.h"

#include "Utils/Dataset.h"

#include "Test/TestCase.h"
#include "Test/TestPerf.h"

int main(int argc, char **argv) {
    const char * DATA_SRC = Dataset::DATA_CTR_OBJ, *DATA_SRC2 = Dataset::DATA_CTR_COLOR64;
  //    const char * DATA_SRC = Dataset::DATA_W_OBJ, *DATA_SRC2 = Dataset::DATA_W_COLOR64;
    //TestCase::testSmallRandomRawGraph();
    //TestCase::testAdjustDij(DATA_SRC);
    //    TestCase::testAdjustDijSpeed(DATA_SRC, DATA_SRC2);
    //TestCase::testAdjustDijSpeedCachedFriendly(DATA_SRC, DATA_SRC2);

    TestCase::testOrderGenerator(DATA_SRC, DATA_SRC2);
    // Perf Testp
    //TestPerf::testPerfAdder();
    //TestPerf::testPerfCoreSpeed();
    // for (int i = 0; i < 8; i++) {
    //   TestLinkListWriteOnly<120, 1024*32> a; a.test();
    // }
    // for (int i = 0; i < 8; i++) {
    //   TestLinkListRandomWriteOnly<120, 1024*32> a; a.test();
    // }
    // for (int i = 0; i < 8; i++) {
    //   TestLinkListRandomReadOnly<120, 1024*32> a; a.test();
    // }
    // for (int i = 0; i < 8; i++) {
    //   TestLinkListWriteWithIdx<120, 1024*32> a; a.test();
    // }

    // for (int i = 0; i < 8; i++) {
    //   TestLinkListWriteWithIdxAndRemoteRead<120, 1024*32> a; a.test();
    // }

    // for (int i = 0; i < 8; i++) {
    //   TestLinkListWriteWithIdxStrip<120, 1024*64> a; a.test();
    // }



    //AdjacentMatrix g(Dataset::DATA_USA_TEXT);
    //g.dumpToObjectFile(Dataset::DATA_USA_OBJ);
    return 0;
}

