#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <cmath>
#include <map>
#include <set>
#include <algorithm>
#include <cstdlib>
#include <string>
#include <ctime>

using namespace std;

#define THRES 20.0

double wallClock() {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return t.tv_sec + 1e-9 * t.tv_nsec;
}

string dataset = "geolife";

struct Point {
    int t;
    double x, y;
};
class Trajectory;

class Region {
public:
    Region(int x_len, int y_len, int grid_len):
        x_len_(x_len), y_len_(y_len), grid_len_(grid_len), x_num_(x_len / grid_len + 1) {
        cout << "Initialize Region " << x_len << "*" << y_len << " " << grid_len << endl;
    }
    // gid -> {(tid, start, end), ... }
    map<int, map<int, pair<int, int>>> inverted_index_;
    
    vector<double> getLimit(int gid) {
        auto y_cord = gid / x_num_;
        auto x_cord = gid % x_num_;
        double x_min = x_cord * grid_len_;
        double x_max = x_min + grid_len_;
        double y_min = y_cord * grid_len_;
        double y_max = y_min + grid_len_;
        vector<double> ret = {x_min, y_min, x_max, y_max};
        return ret;
    }

    int getGid(Point p) {
        int x_cord = floor(p.x / grid_len_);
        int y_cord = floor(p.y / grid_len_);
        return y_cord * x_num_ + x_cord;
    }

    bool near(int gid1, int gid2) {
        auto x1_cord = gid1 % x_num_;
        auto y1_cord = gid1 / x_num_;
        auto x2_cord = gid2 % x_num_;
        auto y2_cord = gid2 / x_num_;
        if (x1_cord == x2_cord && (y1_cord == y2_cord + 1 || y1_cord == y2_cord - 1)) {
            return true;
        }
        if (y1_cord == y2_cord && (x1_cord == x2_cord + 1 || x1_cord == x2_cord - 1)) {
            return true;
        }
        return true;
    }

    bool turning(int gid1, int gid2) {
        auto x1_cord = gid1 % x_num_;
        auto y1_cord = gid1 / x_num_;
        auto x2_cord = gid2 % x_num_;
        auto y2_cord = gid2 / x_num_;
        if (x1_cord != x2_cord + 1 && x1_cord != x2_cord - 1) {
            return false;
        }
        if (y1_cord != y2_cord + 1 && y1_cord != y2_cord - 1) {
            return false;
        }
        return true;
    }

    int getXNum() {
        return x_num_;
    }

    void filter(vector<int> grid_list);
    void directFilter(vector<int> grid_list);
    void constructIndex();
    void outputFilter(vector<int> tids, vector<map<int, pair<int, int>>> ts_infos, vector<int> grid_list);


private:
    int x_len_;
    int y_len_;
    int grid_len_;
    int x_num_;
    vector<int> intersection(vector<int> grid_list);
};

Region* rg;
vector<Trajectory*> trajs;

class Trajectory {
public:
    Trajectory(int tid): tid_(tid) {

    }

    void addPoint(int t, double x, double y) {
        Point p{t, x, y};
        points_.push_back(p);
    }

    int getTid() {
        return tid_;
    }

    int size() {
        return points_.size();
    }

    Point getPoint(int id) {
        return points_[id];
    }

    void print() {
        cout << tid_ << ":";
        for (int i = 0; i < points_.size(); i++) {
            cout << points_[i].t << "," << points_[i].x << "," << points_[i].y << ";";
        }
        cout << endl;
    }

    vector<int> filter(int gid, double thres);
    vector<int> segmentToGrid(Point p1, Point p2);
    vector<vector<int>> trajToEntries();
    set<int> trajToGids();
    vector<int> getGids();

private:
    int tid_;
    vector<Point> points_;
    
    bool cross(Point p1, Point p2, vector<double> limit, double thres);
};

void Region::constructIndex() {
    cout << "Begin construct inverted index" << endl;
    int n = 0;
    for (auto traj : trajs) {
        auto entries = traj->trajToEntries();
        n += entries.size();
        for (auto e : entries) {
            int gid = e[0];
            if (!inverted_index_.count(gid)) {
                map<int, pair<int, int>> dummy;
                inverted_index_[gid] = dummy;
            }
            inverted_index_[gid][traj->getTid()] = make_pair(e[1], e[2]);
        }
    }
    cout << "End construct inverted index, size = " << inverted_index_.size() << endl;
    cout << n << " " << sizeof(inverted_index_) << endl;
}

vector<int> Region::intersection(vector<int> grid_list) {
    set<int> ret;
    for (auto i : inverted_index_[grid_list[0]]) {
        ret.insert(i.first);
    }
    for (auto i = 1; i < grid_list.size(); i++) {
        set<int> tmp1, tmp2;
        for (auto j : inverted_index_[grid_list[i]]) {
            tmp1.insert(j.first);
        }
        set_intersection(ret.begin(), ret.end(), tmp1.begin(), tmp1.end(), inserter(tmp2, tmp2.begin()));
        ret = tmp2;
    }
    vector<int> vec;
    for (auto i : ret) {
        vec.push_back(i);
    }
    return vec;
}

void Region::outputFilter(vector<int> tids, vector<map<int, pair<int, int>>> ts_infos, vector<int> grid_list) {
    // output to file
    // first line: traj_num;grid_num;gid1;gid2;...
    // other line: tid:traj_len;gid:start,end;...;t,x,y;...;

    ofstream of("./data/" + dataset + "_filter");
    of << tids.size() << ";";
    of << grid_list.size() << ";";
    for (auto gid : grid_list) {
        of << gid << ";";
    }
    of << endl;
    for (int i = 0; i < tids.size(); i++) {
        of << tids[i] << "," << trajs[tids[i]]->size() << ";";
        for (auto gid : grid_list) {
            of << gid << ":" << ts_infos[i][gid].first << "," << ts_infos[i][gid].second << ";";
        }
        for (int id = 0; id < trajs[tids[i]]->size(); id++) {
            auto p = trajs[tids[i]]->getPoint(id);
            of << p.t << "," << p.x << "," << p.y << ";";
        }
        of << endl;
    }
}

void Region::filter(vector<int> grid_list) {
    cout << "Begin filter" << endl;
    // step1: calculate set
    auto tids = intersection(grid_list);

    // step2: add time range info
    vector<map<int, pair<int, int>>> ts_infos;
    for (auto tid : tids) {
        map<int, pair<int, int>> tmp;
        for (auto gid : grid_list) {
            tmp[gid] = inverted_index_[gid][tid];
        }
        ts_infos.push_back(tmp);
    }
    cout << "End filter " << tids.size() << endl;
    outputFilter(tids, ts_infos, grid_list);
}

void Region::directFilter(vector<int> grid_list) {
    cout << "Begin direct filter" << endl;
    // step1: calculate set
    vector<int> tids;
    for (auto traj : trajs) {
        auto cover = true;
        auto gids = traj->trajToGids();
        for (auto g : grid_list) {
            if (!gids.count(g)) {
                cover = false;
                break;
            }
        }
        if (cover) {
            tids.push_back(traj->getTid());
        }
    }

    // step2: add time range info
    vector<map<int, pair<int, int>>> ts_infos;
    for (auto tid : tids) {
        map<int, pair<int, int>> tmp;
        for (auto gid : grid_list) {
            tmp[gid] = inverted_index_[gid][tid];
        }
        ts_infos.push_back(tmp);
    }
    cout << "End direct filter " << tids.size() << endl;
}

set<int> Trajectory::trajToGids() {
    set<int> gids;
    for (auto p : points_) {
        gids.insert(rg->getGid(p));
    }
    return gids;
}

vector<int> Trajectory::getGids() {
    set<int> gids;
    for (auto p : points_) {
        gids.insert(rg->getGid(p));
    }
    vector<int> vec;
    cout << "Gids: " << endl;
    for (auto g : gids) {
        vec.push_back(g);
        cout << g << ", ";
    }
    cout << endl;
    return vec;
}

bool Trajectory::cross(Point p1, Point p2, vector<double> limit, double thres) {
    double x_min = limit[0], y_min = limit[1], x_max = limit[2], y_max = limit[3];
    
    // in box
    if (p1.x >= x_min && p1.x <= x_max && p1.y >= y_min && p1.y <= y_max) {
        return true;
    }
    if (p2.x >= x_min && p2.x <= x_max && p2.y >= y_min && p2.y <= y_max) {
        return true;
    }

    // cross box
    double k = (p2.y - p1.y) / (p2.x - p1.x);
    double b = p2.y - k * p2.x;
    double y_left = k * x_min + b;
    double y_right = k * x_max + b;
    if (y_left >= y_min && y_left <= y_max) {
        return true;
    }
    if (y_right >= y_min && y_right <= y_max) {
        return true;
    }

    // near box
    double base = (sqrt(k * k + 1)) * thres;
    double dis0 = k * x_min - y_min + b;
    double dis1 = k * x_max - y_min + b;
    double dis2 = k * x_min - y_max + b;
    double dis3 = k * x_max - y_max + b;
    if (dis0 <= base || dis1 <= base || dis2 <= base || dis3 <= base) {
        return true;
    }
}

vector<int> Trajectory::filter(int gid, double thres) {
    auto limit = rg->getLimit(gid);
    auto start_time = -1, end_time = -1;
    for (int i = 0; i < points_.size() - 1; i++) {
        if (cross(points_[i], points_[i + 1], limit, thres)) {
            if (start_time == -1) {
                start_time = points_[i].t;
            }
            end_time = points_[i + 1].t;
        }
    }
    vector<int> ret;
    if (start_time == -1) {
        ret.push_back(-1);
    }
    else {
        ret.push_back(tid_);
        ret.push_back(start_time);
        ret.push_back(end_time);
    }
    return ret;
}

vector<int> Trajectory::segmentToGrid(Point p1, Point p2) {
    int g1 = rg->getGid(p1);
    int g2 = rg->getGid(p2);
    vector<int> gids;
    if (g1 == g2) {
        gids.push_back(g1);
        return gids;
    }
    gids.push_back(g1);
    gids.push_back(g2);
    if (rg->near(g1, g2)) {
        return gids;
    }
    if (rg->turning(g1, g2)) {
        Point tp1, tp2;
        int tg1, tg2;
        if (p1.x < p2.x) {
            tp1 = p1, tp2 = p2;
            tg1 = g1, tg2 = g2;
        }
        else {
            tp1 = p2, tp2 = p1;
            tg1 = g2, tg2 = g1;
        }

        double k = (tp2.y - tp1.y) / (tp2.x - tp1.x);
        double b = tp1.y - tp1.x * k;
        auto bounds = rg->getLimit(tg1);
        if (tp1.y < tp2.y) {
            auto bx = bounds[2];
            auto by = bounds[3];
            auto delta = by - (k * bx + b);
            if (delta * delta / (k * k + 1) < THRES * THRES) {
                gids.push_back(tg1 + 1);
                gids.push_back(tg2 - 1);
            } 
            else if (delta > 0) {
                gids.push_back(tg2 - 1);
            }
            else {
                gids.push_back(tg1 + 1);
            }
        }
        else {
            auto bx = bounds[2];
            auto by = bounds[1];
            auto delta = by - (k * bx + b);
            if (delta * delta / (k * k + 1) < THRES * THRES) {
                gids.push_back(tg1 + 1);
                gids.push_back(tg2 - 1);
            } 
            else if (delta > 0) {
                gids.push_back(tg1 + 1);
            }
            else {
                gids.push_back(tg2 - 1);
            }
        }
    }
    return gids;
}

vector<vector<int>> Trajectory::trajToEntries() {
    map<int, int> start_map;
    map<int, int> end_map;
    for (int i = 0; i < points_.size() - 1; i++) {
        auto gids = segmentToGrid(points_[i], points_[i + 1]);
        for (auto gid : gids) {
            if (!start_map.count(gid)) {
                start_map[gid] = points_[i].t;
            }
            end_map[gid] = points_[i + 1].t;
        }
    }
    vector<vector<int>> entries;
    for (auto entry : start_map) {
        entries.push_back({entry.first, start_map[entry.first], end_map[entry.first]});
    }
    return entries;
}

vector<Trajectory*> readData(int tnum) {
    string file_name = "./data/" + dataset + "_input";
    freopen(file_name.data(), "r", stdin);
    vector<Trajectory*> trajs;
    int t, tlen;
    double x, y;
    int tid = 0;
    char c;
    for (int l = 0; l < tnum; l++) {
        auto traj = new Trajectory(tid++);
        scanf("%d;", &tlen);
        for (int i = 0; i < tlen; i++) {
            scanf("%d,%lf,%lf;", &t, &x, &y);
            traj->addPoint(t, x, y);
        }
        trajs.push_back(traj);
    }
    cout << "read " << trajs.size() << " trajectories" << endl;
    return trajs;
}

Trajectory* readTest() {
    string file_name = "./data/" + dataset + "_test";
    freopen(file_name.data(), "r", stdin);
    
    int tid, tlen;
    scanf("%d:%d;", &tid, &tlen);
    int t;
    double x, y;
    Trajectory* traj = new Trajectory(tid);
    for (int i = 0; i < tlen; i++) {
        scanf("%d,%lf,%lf;", &t, &x, &y);
        traj->addPoint(t, x, y);
    }
    return traj;
}

vector<int> readTestGrids() {
    string file_name = "./data/" + dataset + "_grid";
    freopen(file_name.data(), "r", stdin);
    int tlen;
    scanf("%d;", &tlen);
    set<int> gids;
    for (int i = 0; i < tlen; i++) {
        int t, gid;
        scanf("%d,%d;", &t, &gid);
        gids.insert(gid);
    }
    vector<int> vec;
    for (auto g : gids) {
        vec.push_back(g);
    }
    for (auto i : vec) {
        cout << i << " ";
    }
    cout << endl;
    return vec;
}

int readInfos() {
    string file_name = "./data/" + dataset + "_info";
    freopen(file_name.data(), "r", stdin);
    int xlen, ylen, grid_len;
    scanf("%d %d %d", &xlen, &ylen, &grid_len);
    rg = new Region(xlen, ylen, grid_len);
    int tnum;
    scanf("%d", &tnum);
    return tnum;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "input the dataset!" << endl;
        exit(0);
    }
    dataset = argv[1];
    auto tnum = readInfos();
    
    // auto testTraj = readTest();
    // auto testGids = testTraj->getGids();

    auto testGids = readTestGrids();
    
    double read_st = wallClock();
    trajs = readData(tnum);
    cout << "read data: " << wallClock() - read_st << endl;

    double index_st = wallClock();
    rg->constructIndex();
    cout << "index construct: " << wallClock() - index_st << endl;

    double filter_st = wallClock();
    rg->filter(testGids);
    cout << "filter: " << wallClock() - filter_st << endl;

    // double direct_filter_st = wallClock();
    // rg->directFilter(testGids);
    // cout << "direct filter: " << wallClock() - direct_filter_st << endl;
}