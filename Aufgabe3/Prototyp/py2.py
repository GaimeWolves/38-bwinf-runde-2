from pprint import pprint
import numpy as np
import math as math
from copy import deepcopy
from collections import defaultdict
import heapq
import os
import itertools
import operator

def angle_between(c, p1, p2):
    v0 = np.array(p1) - np.array(c)
    v1 = np.array(p2) - np.array(c)

    return np.math.atan2(np.linalg.det([v0,v1]),np.dot(v0,v1))

maxpercentage = float(os.sys.argv[2])

edgesPoints = []

#path = os.path.dirname(os.path.realpath(__file__))
file = open(os.sys.argv[1])
lines = file.readlines()

cnt = int(lines[0][:-1])

start = (int(lines[1].split(',')[0][1:]), int(lines[1].split(',')[1][:-2]))
end = (int(lines[2].split(',')[0][1:]), int(lines[2].split(',')[1][:-2]))

for i in range(3, 3 + cnt):
    edgesPoints.append([(int(lines[i].split()[0].split(',')[0][1:]), int(lines[i].split()[0].split(',')[1][:-1])), (int(lines[i].split()[1].split(',')[0][1:]), int(lines[i].split()[1].split(',')[1][:-1]))])

edges = []
hyperedges = []
nodes = []

for e in edgesPoints:
    if e[0] not in nodes:
        nodes.append(e[0])

    if e[1] not in nodes:
        nodes.append(e[1])

    edges.append((e[0], e[1], math.sqrt((e[0][0] - e[1][0])**2 + (e[0][1] - e[1][1])**2)))

def getDist(u, v, edges):
    for e in edges:
        if e == (u, v, e[2]) or e == (v, u, e[2]):
            return e[2]

    return float('inf')

def sortQueue(queue, nodes, predecessor):
    for node in nodes:
        for qElement in queue:
            if node == qElement[1]:
                qElement[0] = predecessor[node][0]
    queue.sort(key=lambda a: a[0])

def initializeDijkstra(nodes, start, predecessor, queue):
    for n in nodes:
        predecessor[n] = [0 if n == start else float('inf'), None]
        queue.append([predecessor[n][0], n])
    sortQueue(queue, nodes, predecessor)

def distanceUpdateDijkstra(u, v, edges, predecessor):
    alt = predecessor[u][0] + getDist(u, v, edges)
    if alt < predecessor[v][0]:
        predecessor[v] = (alt, u)

def dijkstraShortestPath(nodes, edges, start):
    predecessor = dict()
    queue = []
    initializeDijkstra(nodes, start, predecessor, queue)

    while len(queue) > 0:
        u = queue[0]
        queue.remove(u)
        for v in [e[0 if e[1] == u[1] else 1] for e in edges if e[1] == u[1] or e[0] == u[1]]:
            if any([q[1] == v for q in queue]):
                distanceUpdateDijkstra(u[1], v, edges, predecessor)
        sortQueue(queue, nodes, predecessor)

    return predecessor

def turnUpdateDijkstra(u, v, edges, predecessor):
    alt = predecessor[u][0] + (0 if predecessor[u][1] == None else (1 if angle_between(u, predecessor[u][1], v) != np.pi else 0))
    if alt < predecessor[v][0]:
        predecessor[v] = (alt, u)

def dijkstraFewestTurns(nodes, edges, start, goal, distancePredecessor):
    predecessor = dict()
    reverseDistancePredecessor = dict()
    queue = []
    initializeDijkstra(nodes, goal, predecessor, queue)
    initializeDijkstra(nodes, goal, reverseDistancePredecessor, list())

    while len(queue) > 0:
        u = queue[0]
        queue.remove(u)
        for v in [e[0 if e[1] == u[1] else 1] for e in edges if e[1] == u[1] or e[0] == u[1]]:
            if any([q[1] == v for q in queue]):
                distanceUpdateDijkstra(u[1], v, edges, reverseDistancePredecessor)
                if distancePredecessor[v][0] + reverseDistancePredecessor[v][0] > distancePredecessor[goal][0] * maxpercentage:
                    continue
                turnUpdateDijkstra(u[1], v, edges, predecessor)
        sortQueue(queue, nodes, predecessor)

    return predecessor

def fewestTurnPathBFS(nodes, edges, start, goal, distancePredecessor, pathCap = 10):
    queue = []
    queue.append([goal, [], 0, 0])

    paths = []

    visited = {}

    while len(queue) > 0:
        queue.sort(key = operator.itemgetter(2, 3))
        worker = queue.pop(0)

        if worker[3] + distancePredecessor[worker[0]][0] > distancePredecessor[goal][0] * maxpercentage:
        #if worker[3] > distancePredecessor[goal][0] * maxpercentage:
            continue

        if pathCap == 1 and len(worker[1]) > 0:
            e = (worker[0], worker[1][-1])
            if e in visited:
                if visited[e] < worker[2]:
                    continue
            elif e not in visited:
                visited[e] = worker[2]
            elif visited[e] > worker[2]:
                visited[e] = worker[2]

        worker[1].append(worker[0])

        if worker[0] == start:
            paths.append(worker)
            if len(paths) == pathCap:
                break
            continue

        for v in [e[0 if e[1] == worker[0] else 1] for e in edges if e[1] == worker[0] or e[0] == worker[0]]:
            if v not in worker[1]:
                isTurn = 1 if (len(worker[1]) > 1 and angle_between(worker[0], worker[1][-2], v) != np.pi) else 0

                newWorker = deepcopy(worker)

                newWorker[0] = v
                newWorker[2] += isTurn

                newWorker[3] += getDist(newWorker[0], newWorker[1][-1], edges)

                queue.append(newWorker)
    
    paths.sort(key= lambda path: (path[2], path[3]))
    return paths


distMap = dijkstraShortestPath(nodes, edges, start)
#turnMap = dijkstraFewestTurns(nodes, edges, start, end, distMap)
#pprint(turnMap)

current = end
distPath = []
while current != start:
    distPath.append(current)
    current = distMap[current][1]
distPath.append(start)
distPath.reverse()
#print('{:3.3f}'.format(distMap[end][0]), distPath)

#current = start
#turnPath = []
#while current != end:
#    turnPath.append(current)
#    current = turnMap[current][1]
#turnPath.append(end)
#print(turnMap[start][0], turnPath)

paths = fewestTurnPathBFS(nodes, edges, start, end, distMap, 1)

if len(paths) == 0:
    print(os.sys.argv[1], os.sys.argv[2], "No path ;(")

for path in paths:
    valid = True

    pathList = path[1][::-1]

    for i, v in enumerate(pathList):
        if i > 0:
            if getDist(v, pathList[i - 1], edges) == float('inf'):
                valid = False

    print('"{}","{}","{}","{}","{}","{}","{}"'.format(os.sys.argv[1], os.sys.argv[2], '{:d}'.format(path[2]), '{:.3f}'.format(path[3]), '{:.3f}'.format(distMap[end][0]), '{:.3f}'.format(path[3] / distMap[end][0]), path[1][::-1]))
    #print('"{}","{}","{}","{}","{}","{}"'.format(os.sys.argv[1], os.sys.argv[2], '{:d}'.format(path[2]), '{:.3f}'.format(path[3]), '{:.3f}'.format(distMap[end][0]), '{:.3f}'.format(path[3] / distMap[end][0])))