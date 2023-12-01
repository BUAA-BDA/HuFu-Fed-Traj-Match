from scipy.special import lambertw
import math
import random
import argparse


# Pr(noise_len < len) = prob
def get_rad(ep, prob):
    w0 = (prob - 1) / math.e
    lw = lambertw(w0, -1)
    w1 = lw.real
    return -1 * (w1 + 1) / ep


def get_bias_unlimited(ep):
    theta = random.uniform(0, 2 * math.pi)
    r = get_rad(ep, random.uniform(0, 1))
    x = r * math.cos(theta)
    y = r * math.sin(theta)
    return x, y


def get_noise_radius(ep, delta):
    big_delta = 0
    while True:
        big_delta += 0.0001
        right = delta * math.pi * get_rad(ep, 1 - big_delta) ** 2
        if big_delta > right:
            break
    return get_rad(ep, 1 - big_delta)


def get_bias(ep, delta):
    theta = random.uniform(0, 2 * math.pi)
    rd = random.uniform(0, 1)
    big_delta = 0
    while True:
        big_delta += 0.0001
        right = delta * math.pi * get_rad(ep, 1 - big_delta) ** 2
        if big_delta > right:
            break

    if rd > 1 - big_delta:
        r = random.uniform(0, get_rad(ep, 1 - big_delta) ** 2) ** 0.5
    else:
        r = get_rad(ep, rd)

    # print("delta = " + str(delta) + ", big_delta = " + str(big_delta) + ", ep = " + str(ep) + ", r = " + str(r))
    # print("R = " + str(get_rad(ep, 1 - big_delta)) + ", r = " + str(r))
    # print(big_delta / math.pi / (get_rad(ep, 1 - big_delta) ** 2))
    x = r * math.cos(theta)
    y = r * math.sin(theta)
    return x, y


def upper_bound(eps, delta, success):
    r = get_noise_radius(eps, delta)
    return 0.5 * r / (1 - success ** 0.5)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--eps', default=0.01)
    parser.add_argument('--delta', default=0.00001)
    parser.add_argument('--publish_ratio', default=0.6)
    args = parser.parse_args()
    print(upper_bound(args.eps, args.delta, args.publish_ratio))

