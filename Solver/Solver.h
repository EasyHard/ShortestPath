#ifndef SOLVER_H_
#define SOLVER_H_

#include "../Utils/CycleTimer.h"
#include "../Utils/ColorUtils.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

template<typename GraphType>
class Solver {
public:
    virtual void compute(GraphType &graph, Colors *colors, int source, double &prepareTimeS, double &runningTimeS, double &totalScan, double &avgScan)=0;
    void compute(CacheFriendlyAdjacentMatrix &adjMatrix, Colors *colors, int source, double &prepareTimeS, double &runningTimeS, double &totalScan, double &avgScan) {
        // compute(*(AdjacentMatrix*) &adjMatrix, colors, adjMatrix.index[source], prepareTimeS, runningTimeS, totalScan, avgScan);
      compute(*(AdjacentMatrix*) &adjMatrix, colors, source, prepareTimeS, runningTimeS, totalScan, avgScan);
      //reorgResult(adjMatrix);
    }
    virtual const char * getName()=0;
    virtual ~Solver() {
    }
};

#include "DijstraHeap.h"
#include "AdjustDij_NoGlobalSync.h"
#include "AdjustDij_GlobalHeapStack.h"
#include "AdjustDij_NoLock.h"
#include "AdjustDij_Timer.h"
#include "SourceNodesGenerator.h"
#include "SourceNodesGeneratorPad.h"
#include "SeperateDijk.h"
/*
 * AdjustDij_NoAtomic.h uses a sync-proxy that is not guaranteed to be correct.
 * It has a state machine for each thread, and the machine changes one by one,
 * according to their positions of the ring.
 */
#include "AdjustDij_NoAtomic.h"

/*
 * AdjustDij_NoAtomic2.h is guaranteed to be correct. For each thread, it has
 * two states, STATE_ACTIVE and STATE_READY_TO_STOP.
 *
 * STATE_ACTIVE -> STATE_READY_TO_STOP:
 *     No more new nodes in its own heaps, and all nodes sent by itself to
 *     others are already taken.
 *
 * STATE_READY_TO_STOP -> thread exit:
 *    1. we check if each threads is in STATE_READY_TO_STOP and calculate
 *       maximum local version as maxVersion.
 *    2. Double check. For every local version, they are equal or smaller than
 *       maxVersion making sure no threads update in Step 1.
 *    3. The strategy to update local version of each thread is 1) calculate
 *       maximum local version as maxVersion, and update it using maxVersion+1.
 *       Note that, this strategy doesn't need to be atomic.
 *
 * STATE_READY_TO_STOP -> STATE_ACTIVE: other cases
 */
#include "AdjustDij_NoAtomic2.h"

/*
 * AdjustDij_NoAtomic3.h modifies a little from AdjustDij_NoAtomic2.h.
 *
 * The strategy to update local version of each thread is  when
 * STATE_ACTIVE -> STATE_READY_TO_STOP.
 */
#include "AdjustDij_NoAtomic3.h"

/*
 * AdjustDij_NoAtomic4.h modifies a little from AdjustDij_NoAtomic3.h.
 *
 * The highest bit of local version is used to stored state.
 */
#include "AdjustDij_NoAtomic4.h"


/*
 * AdjustDij_NoAtomic4Pad.h modifies from NoAtomic4.h, with padding
 */
#include "AdjustDij_NoAtomic4CompPad.h"

#include "AdjustDij_NoAtomic4CompPadLheap.h"

#endif
