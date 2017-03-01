#ifndef ADJUSTDIJ_NOGLOBALSYNC_H_
#define ADJUSTDIJ_NOGLOBALSYNC_H_

#include <omp.h>
#include "../Utils/ColorUtils.h"

class AdjustDij_NoGlobalSync;

namespace AdjustDijUtils {
    class Syncer {
        omp_lock_t lock;
        bool *marker;
        int counter;
        const int N;

        inline void set(int p);

        inline void unset(int p);

    public:
        Syncer(int n);

        inline void setSafe(int p, AdjustDij_NoGlobalSync * const solver);

        inline void unsetSafe(int p);

        bool ready(int p);

        bool ready();

        ~Syncer();
    };
}

using AdjustDijUtils::Syncer;

class AdjustDij_NoGlobalSync: public Solver<AdjacentMatrix> {

    char name[1024];
    const int THREAD_NUM, NUM_RELAX_EACH_ITERATION;
    Utils::Heap **heaps;
    Utils::OMPLockStack **stacks;

    const int *colors;

    Syncer *syncer;

    friend class Syncer;

    void relax(AdjacentMatrix *matrix, int &totalScan) {
        int pid = omp_get_thread_num();
        AdjacentMatrix::Arch *&archArray = matrix->archArray;
        int *&g = matrix->g;
        int *&result = matrix->result;
        Utils::Item data;
        int &src_node = data.key, &src_result = data.value;
        int left = NUM_RELAX_EACH_ITERATION;
        while (left > 0 && !heaps[pid]->empty()) {
            heaps[pid]->removeTop(&data);
            /*
             * try to update result
             */
            if (result[src_node] > src_result) result[src_node] = src_result;
            /*
             * relax according to src_node
             */
#ifdef PERF_SCAN
            ++totalScan;
#endif
            for (int i = g[src_node]; i < g[src_node + 1]; ++i) {
                int &dst_node = archArray[i].j, &weight = archArray[i].w, new_dst_result = src_result + weight;
                if (result[dst_node] > new_dst_result) {
                    if (colors[dst_node] == pid) {
                        result[dst_node] = new_dst_result;
                        heaps[pid]->addOrUpdate(dst_node, new_dst_result);
                    } else {
                        stacks[colors[dst_node]]->addOrUpdateSafe(dst_node, new_dst_result);
                        syncer->unsetSafe(colors[dst_node]);
                    }
                }
            }
            --left;
        }
    }

    void adjust(AdjacentMatrix *matrix) {
        int pid = omp_get_thread_num();
        int *&result = matrix->result;
        Utils::Item data;
        int &src_node = data.key, &src_result = data.value;
        while (!stacks[pid]->empty()) {
            stacks[pid]->removeTopSafe(&data);
            if (result[src_node] > src_result) {
                heaps[pid]->addOrUpdate(src_node, src_result);
            }
        }
    }

    void worker(AdjacentMatrix *matrix, int &totalScan) {
        int pid = omp_get_thread_num();
        while (!syncer->ready()) {
            relax(matrix, totalScan);
            adjust(matrix);
            // check
            if (heaps[pid]->empty() && stacks[pid]->empty())
                syncer->setSafe(pid, this);
            else
                syncer->unsetSafe(pid);
        }
    }

public:
    AdjustDij_NoGlobalSync(int thread_num, int num_relax_each_iteration) :
            THREAD_NUM(thread_num), NUM_RELAX_EACH_ITERATION(num_relax_each_iteration), heaps(NULL), stacks(NULL), colors(NULL), syncer(NULL) {
        sprintf(name, "Dijstra + Adjustment + NoGlobalSync: THREAD_NUM=%d, NUM_RELAX_EACH_ITERATION=%d", THREAD_NUM, NUM_RELAX_EACH_ITERATION);
    }

    const char * getName() {
        return name;
    }

    void compute(AdjacentMatrix &adjMatrix, Colors *colors, int source, double &prepareTimeS, double &runningTimeS, double &totalScan, double &avgScan) {
        omp_set_num_threads(THREAD_NUM);

        CycleTimer prepareTimer, runningTimer;
#ifdef PERF_SCAN
        totalScan = 0;
#endif

        // Step 1: prepare Environment
        prepareTimer.Start();

        int &n = adjMatrix.n;
        this->colors = colors->getColors();

        int *&result = adjMatrix.result;
        adjMatrix.resetResult();

        /*
         * create heaps and stacks for each thread
         */
        heaps = (Utils::Heap **) malloc(sizeof(Utils::Heap *) * THREAD_NUM);
        stacks = (Utils::OMPLockStack **) malloc(sizeof(Utils::OMPLockStack *) * THREAD_NUM);
        for (int i = 0; i < THREAD_NUM; ++i) {
            heaps[i] = new Utils::Heap(n);
            stacks[i] = new Utils::OMPLockStack(n);
        }

        /*
         * create syncer
         */
        syncer = new Syncer(THREAD_NUM);

        prepareTimer.Stop();

        // Step 2: start to compute
        runningTimer.Start();

        result[source] = 0;
        heaps[0]->addOrUpdate(source, result[source]);

#pragma omp parallel
        {
            int ts = 0;
            worker(&adjMatrix, ts);
#ifdef PERF_SCAN
#pragma omp atomic
            totalScan += ts;
#endif
        }

        runningTimer.Stop();

        // Step 3: destroy environment
        prepareTimer.Start();

        delete syncer;

        /*
         * destroy heaps and stacks for each thread
         */
        for (int i = 0; i < THREAD_NUM; ++i) {
            delete heaps[i];
            delete stacks[i];
        }
        free(stacks);
        free(heaps);

        prepareTimer.Stop();

        runningTimeS = runningTimer.GetSecond();
        prepareTimeS = prepareTimer.GetSecond();
#ifdef PERF_SCAN
        avgScan = totalScan / THREAD_NUM;
#endif
    }
};

namespace AdjustDijUtils {

    inline void Syncer::set(int p) {
        if (!marker[p]) ++counter;
        marker[p] = true;
    }

    inline void Syncer::unset(int p) {
        if (marker[p]) --counter;
        marker[p] = false;
    }

    Syncer::Syncer(int n) :
            counter(0), N(n) {
        omp_init_lock(&lock);
        marker = (bool*) malloc(n * sizeof(bool));
        memset(marker, 0, sizeof(bool) * n);
    }

    inline void Syncer::setSafe(int p, AdjustDij_NoGlobalSync * const solver) {
        if (marker[p]) return;
        omp_set_lock(&lock);
        if (solver->heaps[p]->empty() && solver->stacks[p]->empty()) set(p);
        omp_unset_lock(&lock);
    }

    inline void Syncer::unsetSafe(int p) {
        if (!marker[p]) return;
        omp_set_lock(&lock);
        unset(p);
        omp_unset_lock(&lock);
    }

    bool Syncer::ready(int p) {
        return marker[p];
    }

    bool Syncer::ready() {
        return counter == N;
    }

    Syncer::~Syncer() {
        free(marker);
        omp_destroy_lock(&lock);
    }

}

#endif
