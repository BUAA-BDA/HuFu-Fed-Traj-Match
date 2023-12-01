#pragma once
#include <obliv.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "dbg.h"

typedef struct {
    int t;
    float x, y;
} Point;

typedef struct {
    obliv float ot;
    obliv float ox, oy;
} OblivPoint;

typedef struct {
    int tid, len;
    Point *p;
} Trajectory;

typedef struct {
    int pid, len;
    Point *p1, *p2;
} RefTrajectory;

Trajectory *testTraj;
RefTrajectory *refTrajs;
Trajectory *allTrajs;
int *parSize;

typedef struct {
    char *dataset;
    int par_num;
    int grid_num;
    float thres;
    float grid_size;
} protocolIO;

double wallClock();
void ocTestUtilTcpOrDie(ProtocolDesc*, const char*, const char*);
void loadTestTraj(protocolIO*);
int loadAllTrajs(protocolIO*, int, int*);
void loadRefTrajs(protocolIO*);
void loadInfo(protocolIO*);
void Verify(void* args);

