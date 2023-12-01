import argparse
import geoi
import random
import math


class Point:
    def __init__(self, t, x, y):
        self.t = t
        self.x = x
        self.y = y
        self.g = 0

    def set_grid(self, g):
        self.g = g


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--grid', default=True)
    parser.add_argument('--eps', default=0.01)
    parser.add_argument('--delta', default=0.00001)
    parser.add_argument('--dataset', default='geolife')
    parser.add_argument('--publish_ratio', default=0.6)
    args = parser.parse_args()
    test_points = []
    with open(args.dataset + '_test') as test_file:
        raw = test_file.readline().split(';')
        length = int(raw[0].split(':')[1])
        for i in range(length):
            p = raw[i + 1].split(',')
            test_points.append(Point(int(p[0]), float(p[1]), float(p[2])))
    if args.grid:
        with open(args.dataset + '_info') as info_file, open(args.dataset + '_grid', 'w') as grid_file:
            l1 = info_file.readline().split()
            x_max = int(l1[0])
            y_max = int(l1[1])
            grid_size = int(l1[2])
            x_num = x_max // grid_size + 1
            y_num = y_max // grid_size

            def gridify(xx, yy):
                cor_x = math.floor(xx / grid_size)
                cor_y = int(yy / grid_size)
                return cor_y * x_num + cor_x

            success_point = []
            for i in range(length):
                bias = geoi.get_bias(args.eps, 1 - args.delta)
                new_x = test_points[i].x + bias[0]
                new_y = test_points[i].y + bias[1]
                if gridify(new_x, new_y) == gridify(test_points[i].x, test_points[i].y):
                    success_point.append(test_points[i])
            # print(len(success_point))
            publish_num = length * args.publish_ratio
            if len(success_point) < publish_num:
                print('Error! len(success_point) < publish_num')
                exit(0)
            grid2point = {}
            chosen = set()
            for i in range(len(success_point)):
                p = success_point[i]
                g = gridify(p.x, p.y)
                if g not in grid2point:
                    if len(chosen) < publish_num:
                        chosen.add(i)
                    grid2point[g] = []
                grid2point[g].append(i)

            while len(chosen) < publish_num:
                chosen.add(random.randint(1, len(success_point) - 1))

            chosen = sorted(list(chosen))
            grid_file.write(str(publish_num) + ';')
            for index in chosen:
                p = success_point[index]
                grid_file.write(str(p.t) + ',' + str(gridify(p.x, p.y)) + ';')
    else:
        with open(args.dataset + '_perturb', 'w') as perturb_file:
            perturb_file.write(raw[0] + ";")
            for i in range(length):
                bias = geoi.get_bias(args.eps, 1 - args.delta)
                x = test_points[i].x + bias[0]
                y = test_points[i].y + bias[1]
                perturb_file.write(str(test_points[i].t) + "," + '{:.2f}'.format(x) + "," + '{:.2f}'.format(y) + ";")
            perturb_file.write('\n')
