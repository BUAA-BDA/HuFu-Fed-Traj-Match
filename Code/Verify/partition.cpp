#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <deque>
#include <cmath>

using namespace std;

string dataset = "geolife";
pair<int, int> region_info;

double wallClock() {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec + 1e-9 * t.tv_nsec;
}

struct Point {
    int t;
    double x, y;
    int getGid() {
        int x_cord = floor(x / region_info.first);
        int y_cord = floor(y / region_info.first);
        return y_cord * region_info.second + x_cord;
    }
};

class Trajectory {
public:
    Trajectory(int tid, map<int, pair<int, int>> grid_time, vector<Point> points): 
        tid_(tid), grid_time_(grid_time), points_(points) {

    }

    int getTid() {
        return tid_;
    }

    int size() {
        return points_.size();
    }

    pair<int, int> getTimeRange(int gid) {
        return grid_time_[gid];
    }

    Point getPoint(int id) {
        return points_[id];
    }

    vector<Point> getPoints() {
        return points_;
    }

    vector<int> getGidList() {
        vector<int> grid_list;
        for (auto e : grid_time_) {
            grid_list.push_back(e.first);
        }
        return grid_list;
    }
    
private:
    int tid_;
    map<int, pair<int, int>> grid_time_;
    vector<Point> points_;
};

vector<Trajectory*> trajs;

class Partition {
public:
    Partition(vector<Trajectory*> trajs): trajs_(trajs), grid_list_(trajs[0]->getGidList()) {

    }

    int size() {
        return trajs_.size();
    }

    void setPid(int pid) {
        pid_ = pid;
    }

    pair<Partition*, Partition*> split();

    void chooseRepresentation();

    void output();

private:
    int pid_;
    vector<Trajectory*> trajs_;
    vector<int> grid_list_;
    vector<pair<Point, Point>> representation_;
    pair<int, int> chooseSplitCriterion();
};

// gid, end_time
pair<int, int> Partition::chooseSplitCriterion() {
    map<int, int> start_map, end_map;
    for (auto t : trajs_) {
        for (auto gid : grid_list_) {
            if (!start_map.count(gid)) {
                start_map[gid] = t->getTimeRange(gid).first;
                end_map[gid] = t->getTimeRange(gid).second;
            }
            else {
                start_map[gid] = min(start_map[gid], t->getTimeRange(gid).first);
                end_map[gid] = max(end_map[gid], t->getTimeRange(gid).second);
            }
        }
    }

    int chosen_gid = -1, max_range = 0;
    for (auto entry : start_map) {
        auto gid = entry.first;
        if (end_map[gid] - start_map[gid] > max_range) {
            max_range = end_map[gid] - start_map[gid];
            chosen_gid = gid; 
        }
    }

    vector<int> end_list;
    for (auto t : trajs_) {
        end_list.push_back(t->getTimeRange(chosen_gid).second);
    }
    sort(end_list.begin(), end_list.end());

    return make_pair(chosen_gid, end_list[end_list.size() / 2]);
}

pair<Partition*, Partition*> Partition::split() {
    cout << "Partition split, size = " << size() << endl;
    auto split_criterion = chooseSplitCriterion();
    auto gid = split_criterion.first, end_time = split_criterion.second;
    vector<Trajectory*> t1, t2;
    for (auto t : trajs_) {
        if (t->getTimeRange(gid).second < end_time) {
            t1.push_back(t);
        }
        else {
            t2.push_back(t);
        }
    }
    auto p1 = new Partition(t1);
    auto p2 = new Partition(t2);
    return make_pair(p1, p2);
}

void Partition::chooseRepresentation() {
    for (auto gid : grid_list_) {
        Point p1{-1, 0, 0}, p2{-1, 0, 0};
        for (auto t : trajs_) {
            for (auto p : t->getPoints()) {
                if (p.getGid() == gid) {
                    if (p1.t == -1 || p1.t > p.t) {
                        p1 = p;
                    }
                    if (p2.t == -1 || p2.t < p.t) {
                        p2 = p;
                    }
                }

            }
        }
        representation_.push_back(make_pair(p1, p2));
    }
}

void Partition::output() {
    // output to file
    // first line: par_id;rep_len;par_size;
    // rep line: rep_st,x,y;rep_end,x,y; 
    // par line: tid:traj_len;t,x,y;...;

    string file_name = "./data/partition/" + dataset + "_" + to_string(pid_);
    ofstream of(file_name);

    of << pid_ << ";" << grid_list_.size() << ";" << size() << ";" << endl;
    for (int i = 0; i < grid_list_.size(); i++) {
       of << representation_[i].first.t << "," <<  representation_[i].first.x << "," << representation_[i].first.y << ";";
       of << representation_[i].second.t << "," <<  representation_[i].second.x << "," << representation_[i].second.y << ";" << endl;
    }
    for (int i = 0; i < trajs_.size(); i++) {
        of << trajs_[i]->getTid() << ":" << trajs_[i]->size() << ";";
        // for (int gid = 0; gid < grid_list_.size(); gid++) {
        //     of << trajs_[i].getTimeRange(gid).first << "," << trajs_[i].getTimeRange(gid).second << ";";
        // }
        for (int id = 0; id < trajs_[i]->size(); id++) {
            auto p = trajs_[i]->getPoint(id);
            of << p.t << "," << p.x << "," << p.y << ";";
        }
        of << endl;
    }
}

vector<Partition*> trajPartition(int partition_max_size) {
    cout << "Begin trajectory partition, thres = " << partition_max_size << endl;
    auto init = new Partition(trajs);
    deque<Partition*> cur_partitions;
    vector<Partition*> final_partitions;
    cur_partitions.push_back(init);
    if (trajs.size() < partition_max_size) {
        final_partitions.push_back(init);
        return final_partitions;
    }
    while (!cur_partitions.empty()) {
        auto cur = cur_partitions.front();
        cur_partitions.pop_front();
        auto tmp = cur->split();
        Partition *p1 = tmp.first, *p2 = tmp.second;
        if (p1->size() >= partition_max_size) {
            cur_partitions.push_back(p1);
        }
        else {
            final_partitions.push_back(p1);
        }
        if (p2->size() >= partition_max_size) {
            cur_partitions.push_back(p2);
        }
        else {
            final_partitions.push_back(p2);
        }
    }
    cout << "End trajectory partition, size = " << final_partitions.size() << endl;
    return final_partitions;
}

vector<Trajectory*> readData() {
    string file_name = "./data/" + dataset + "_filter";
    freopen(file_name.data(), "r", stdin);
    
    vector<int> gids;
    vector<Trajectory*> trajs;
    int glen, gid, tnum;
    scanf("%d;%d;", &tnum, &glen);
    for (int i = 0; i < glen; i++) {
        scanf("%d;", &gid);
        gids.push_back(gid);
    }
    for (int i = 0; i < tnum; i++) {
        int tid, tlen;
        scanf("%d,%d;", &tid, &tlen);
        map<int, pair<int, int>> grid_time;
        int grid, st, end;
        for (int i = 0; i < glen; i++) {
            scanf("%d:%d,%d;", &grid, &st, &end);
            grid_time[grid] = make_pair(st, end);
        }

        vector<Point> points;
        int t;
        double x, y;
        for (int i = 0; i < tlen; i++) {
            scanf("%d,%lf,%lf;", &t, &x, &y);
            Point p{t, x, y};
            points.push_back(p);
        }
        auto traj = new Trajectory(tid, grid_time, points);
        trajs.push_back(traj);
    }
    cout << "read " << trajs.size() << " filtered trajectories" << endl;
    return trajs;
}

int readInfos() {
    string file_name = "./data/" + dataset + "_info";
    freopen(file_name.data(), "r", stdin);
    int x_len, y_len, grid_len;
    scanf("%d %d %d", &x_len, &y_len, &grid_len);
    int traj_num, par_size;
    double thres;
    scanf("%d %lf %d", &traj_num, &thres, &par_size);
    region_info = make_pair(grid_len, x_len / grid_len + 1);
    return par_size;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "input the dataset!" << endl;
        exit(0);
    }
    dataset = argv[1];
    auto par_size = readInfos();
    trajs = readData();

    double partition_st = wallClock();
    auto partitions = trajPartition(par_size);
    for (int pid = 0; pid < partitions.size(); pid++) {
        partitions[pid]->setPid(pid);
        partitions[pid]->chooseRepresentation();
        partitions[pid]->output();
    }
    cout << "partition construct: " << wallClock() - partition_st << endl;
}
