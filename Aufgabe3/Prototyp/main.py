from pprint import pprint
from queue import Queue
import numpy as np
import math as math
from copy import deepcopy

def angle_between(c, p1, p2):
	v0 = np.array(p1) - np.array(c)
	v1 = np.array(p2) - np.array(c)

	return np.math.atan2(np.linalg.det([v0,v1]),np.dot(v0,v1))

maxpercentage = 1.15

edges = []

file = open('C:/Users/n.djerfi/OneDrive - WMS-HN.ORG/38-bwinf-runde-2/Aufgabe3/Beispiele/abbiegen3.txt', 'r')
lines = file.readlines()

cnt = int(lines[0][:-1])

start = [int(lines[1].split(',')[0][1:]), int(lines[1].split(',')[1][:-2])]
end = [int(lines[2].split(',')[0][1:]), int(lines[2].split(',')[1][:-2])]

for i in range(3, 3 + cnt):
	edges.append([[int(lines[i].split()[0].split(',')[0][1:]), int(lines[i].split()[0].split(',')[1][:-1])], [int(lines[i].split()[1].split(',')[0][1:]), int(lines[i].split()[1].split(',')[1][:-1])]])

paths = []

workers = Queue()
workers.put([start, [], 0, 0])

while not workers.empty():
	worker = workers.get()

	print(worker)

	if worker[0] in worker[1]:
		continue

	worker[1].append(worker[0])

	if len(worker[1]) > 1:
		worker[2] += math.sqrt(math.pow(worker[1][-1][0] - worker[1][-2][0], 2) + math.pow(worker[1][-1][1] - worker[1][-2][1], 2))

	if len(worker[1]) > 2 and angle_between(worker[1][-2], worker[1][-1], worker[1][-3]) != np.pi:
		worker[3] += 1

	if len(paths) > 0 and worker[2] / paths[0][1] > maxpercentage:
			continue

	if worker[0] == end:
		paths.append([worker[1], worker[2], worker[3]])
		break
		continue

	possible = [e[1] for e in edges if e[0] == worker[0]]
	possible.extend([e[0] for e in edges if e[1] == worker[0]])

	for p in possible:
		nworker = deepcopy(worker)
		nworker[0] = p
		workers.put(nworker)

for p in paths:
	print(p)