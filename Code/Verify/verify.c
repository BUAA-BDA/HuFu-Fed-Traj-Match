#include <stdio.h>
#include <stdlib.h>
#include <obliv.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>

#include "verify.h"

int main(int argc, char *argv[]) {
    if (argc == 5) {
        const char *remote_host = strtok(argv[1], ":");
        const char *port = strtok(NULL, ":");
        ProtocolDesc pd;
        protocolIO io;

        log_info("Connecting to %s on port %s ...\n", remote_host, port);
        if (argv[2][0] == '1') {
            if (protocolAcceptTcp2P(&pd, port) != 0) {
                log_err("TCP accept from %s failed\n", remote_host);
                exit(1);
            }
        } else {
            if (protocolConnectTcp2P(&pd, remote_host, port) != 0) {
                log_err("TCP connect to %s failed\n", remote_host);
                exit(1);
            }
        }

        int party = (argv[2][0] == '1'? 1 : 2);
        setCurrentParty(&pd, party);
        io.dataset = argv[3];
        io.par_num = atoi(argv[4]);

        execYaoProtocol(&pd, Verify, &io);
        cleanupProtocol(&pd);
    } else {
        log_info("parameters error!\n");
    }
    return 0;
}

double wallClock() {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec + 1e-9 * t.tv_nsec;
}

void ocTestUtilTcpOrDie(ProtocolDesc* pd, const char* remote_host, const char* port) {
    if (!remote_host) { 
        if (protocolAcceptTcp2P(pd, port) != 0) { 
            fprintf(stderr, "TCP accept failed\n");
            exit(1);
        }
    }
  else if (protocolConnectTcp2P(pd, remote_host, port) != 0) { 
        fprintf(stderr, "TCP connect failed\n");
        exit(1);
    }
}

void loadInfo(protocolIO *io) {
    char info_file[50];
    sprintf(info_file, "./data/%s_info", io->dataset);
    log_info("load info: %s\n", info_file);
    FILE *input = fopen(info_file, "r");
    int idummy;
    fscanf(input, "%d%d%f%d%f", &idummy, &idummy, &io->grid_size, &idummy, &io->thres);
    log_info("load info: grid_size = %f, thres = %f\n", io->grid_size, io->thres);
    fclose(input);
}

void loadTestTraj(protocolIO *io) {
    testTraj = (Trajectory*)calloc(1, sizeof(Trajectory));
    char test_file[50];
    sprintf(test_file, "./data/%s_test", io->dataset);
    FILE *input = fopen(test_file, "r");
    int tid, tlen;
    fscanf(input, "%d:%d;", &tid, &tlen);
    testTraj->len = tlen;
    testTraj->p = (Point*)calloc(tlen, sizeof(Point));
    for (int i = 0; i < tlen; i++) {
        fscanf(input, "%d,%f,%f;", &(testTraj->p[i].t), &(testTraj->p[i].x), &(testTraj->p[i].y));
    }
    fclose(input);
    log_info("read test traj, tlen = %d\n", testTraj->len);
}

void loadRefTrajs(protocolIO *io) {
    int id = 0;
    refTrajs = (Trajectory*)calloc(io->par_num, sizeof(RefTrajectory));
    parSize = (int*)calloc(io->par_num, sizeof(int));
    for (int pid = 0; pid < io->par_num; pid++) {
        char par_file[50];
        sprintf(par_file, "./data/partition/%s_%d", io->dataset, id);
        FILE *input = fopen(par_file, "r");
        if (input == NULL) {
            log_info("%s not exist\n", par_file);
            break;
        }
        log_info("open %s\n", par_file);
        id++;
        int par_id, ref_len, par_size;
        fscanf(input, "%d;%d;%d;", &par_id, &ref_len, &par_size);
        io->grid_num = ref_len;
        parSize[pid] = par_size;
        refTrajs[pid].pid = par_id;
        refTrajs[pid].p1 = (Point*)calloc(io->grid_num, sizeof(Point));
        refTrajs[pid].p2 = (Point*)calloc(io->grid_num, sizeof(Point));
        for (int i = 0; i < ref_len; i++) {
            fscanf(input, "%d,%f,%f;", &refTrajs[pid].p1[i].t, &refTrajs[pid].p1[i].x, &refTrajs[pid].p1[i].y);
            fscanf(input, "%d,%f,%f;", &refTrajs[pid].p2[i].t, &refTrajs[pid].p2[i].x, &refTrajs[pid].p2[i].y);
        }
        fclose(input);
    }
    log_info("read %d reference trajs, grid_num = %d\n", id, io->grid_num);
}

int loadAllTrajs(protocolIO* io, int ref_id_len, int* ref_id) {
    int size = 0;
    for (int i = 0; i < ref_id_len; i++) {
        int pid = ref_id[i];
        size += parSize[pid];
    }
    
    allTrajs = (Trajectory*)calloc(size, sizeof(Trajectory));
    int loc = 0;
    for (int i = 0; i < ref_id_len; i++) {
        int rid = ref_id[i];
        char par_file[50];
        sprintf(par_file, "./data/partition/%s_%d", io->dataset, rid);
        FILE *input = fopen(par_file, "r");
        log_info("open %s\n", par_file);

        int idummy, ref_len, par_size;
        float fdummy;
        fscanf(input, "%d;%d;%d;", &idummy, &ref_len, &par_size);
        
        for (int i = 0; i < ref_len * 2; i++) {
            fscanf(input, "%d,%f,%f;", &idummy, &fdummy, &fdummy);
        }
        for (int j = 0; j < parSize[rid]; j++) {
            fscanf(input, "%d:%d;", &allTrajs[loc].tid, &allTrajs[loc].len);
            allTrajs[loc].p = (Point*)calloc(allTrajs[loc].len, sizeof(Point));
            for (int k = 0; k < allTrajs[loc].len; k++) {
                fscanf(input, "%d,%f,%f;", &allTrajs[loc].p[k].t, &allTrajs[loc].p[k].x, &allTrajs[loc].p[k].y);
            }
            loc++;
        }
    }
    log_info("read %d full trajs\n", loc);
    return loc;
}
