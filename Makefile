PERF_FLAGS=-DPERF_SCAN # -DSPDEBUG

CC=icpc
FLAGS=-fopenmp -g -Wall $(PERF_FLAGS) -O3

#MIC_CC=/opt/intel/composer_xe_2015.3.187/bin/intel64/icpc #icpc
MIC_CC=icpc
MIC_FLAGS=-fopenmp -g -Wall -O3 -march=native -lmemkind -DIS_MIC $(PERF_FLAGS)

HEADER= \
	Solver/Solver.h \
	Solver/DijstraHeap.h Solver/AdjustDij_NoGlobalSync.h \
	Solver/AdjustDij_NoLock.h \
	Solver/SourceNodesGenerator.h \
	Test/TestCase.h Test/TestPerf.h\
	Utils/CycleTimer.h Utils/Dataset.h Utils/Graph.h Utils/Utils.h Utils/Queue.h \
	Utils/Coordinate.h Utils/ColorUtils.h
HEADER_SRC= Utils/Graph.cpp Utils/ColorUtils.cpp Utils/Coordinate.cpp
SRC= Main.cpp $(HEADER)
SRC_COLOR= Main_Color.cpp $(HEADER)

all: Solver

RA: $(SRC)
	$(MIC_CC) Main_RA.cpp $(HEADER_SRC) -o bin/RA $(MIC_FLAGS)
	./bin/RA

SourceNode: $(SRC)
	$(CC) Main_SourceGenerator.cpp $(HEADER_SRC) -o bin/sourceNodes $(FLAGS)

CoSplit: $(SRC)
	$(CC) Main_CoSplit.cpp $(HEADER_SRC) -o bin/coSplit $(FLAGS)


SeperateDijk: $(SRC)
	$(MIC_CC) SeperateDijk.cpp $(HEADER_SRC) -o bin/SeperateDijk $(MIC_FLAGS)
	scp bin/SeperateDijk mic0:ShortestPath/
	scp Conf/SeperateDijk.conf mic0:ShortestPath/Conf/
	ssh -t mic0 "cd ShortestPath && LD_LIBRARY_PATH=~ KMP_AFFINITY=scatter ./SeperateDijk"

RAVEC: $(SRC)
	$(MIC_CC) Main_RAVEC.cpp $(HEADER_SRC) -o bin/RAVEC $(MIC_FLAGS)
	scp bin/RAVEC mic0:
	ssh -t mic0 LD_LIBRARY_PATH=~ KMP_AFFINITY=scatter ./RAVEC

ObjGraph: $(SRC)
	$(CC) GenerateObjGraph.cpp $(HEADER_SRC) -o bin/objGraphRunner $(FLAGS)

GraphStats: $(SRC)
	$(CC) GraphStats.cpp $(HEADER_SRC) -o bin/graphStats $(FLAGS)


RunSolver: $(SRC)
	$(MIC_CC) Run_Solver.cpp $(HEADER_SRC) -o runSolver $(MIC_FLAGS)
	scp runSolver mic0:ShortestPath/

Solver: $(SRC)
	$(CC) Main.cpp $(HEADER_SRC) -o main $(FLAGS)

Color: $(SRC_COLOR)
	$(CC) Main_Color.cpp $(HEADER_SRC) -o main $(FLAGS)

Mic: Mic-Solver

Mic-Solver: $(SRC)
	$(MIC_CC) Main.cpp $(HEADER_SRC) -o main $(MIC_FLAGS)

clean:
	rm -f main
