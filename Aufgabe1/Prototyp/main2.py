from collections import deque
from queue import Queue
from copy import deepcopy
from pprint import pprint
import os

def hamiltonEven(x, y, n):
    if x == 0 and y < n - 1:
        return (x, y + 1)

    if y == n - 1 and x < n - 1:
        return (x + 1, y)

    if y == n - 1 and x == n - 1:
        return (x, y - 1)

    if x % 2 == 1:
        if y == 0:
            return (x - 1, y)
        else:
            return (x, y - 1)
    elif x % 2 == 0:
        if y == n - 2:
            return (x - 1, y)
        else:
            return (x, y + 1)

def hamiltonOdd(x, y, n, q):
    start = (x, y)
    #Step 1: Find corner and go to it
    corner_dict = {(0, 0): x + y, (n-1, 0): (n - 1 - x) + y, (0, n-1): x + (n - 1 - y), (n-1, n-1): (n - 1 - x) + (n - 1 - y)}
    corner = [key for key in corner_dict if all(corner_dict[tmp] >= corner_dict[key] for tmp in corner_dict)][0]

    dirs = [(1, 0), (0, 1), (-1, 0), (0, -1)]
    d = -1

    if corner == (0, 0):
        d = 0
    elif corner == (n - 1, 0):
        d = 1
    elif corner == (n - 1, n - 1):
        d = 2
    elif corner == (0, n - 1):
        d = 3

    while x != corner[0] or y != corner[1]:
        if (d == 0 or d == 2 or y == corner[1]) and x != corner[0]:
            x += (corner[0] - x) // abs(corner[0] - x)
            q.append((x, y))
        if (d == 1 or d == 3 or x == corner[0]) and y != corner[1]:
            y += (corner[0] - y) // abs(corner[0] - y)
            q.append((x, y))

    x += dirs[d][0]
    y += dirs[d][1]
    q.append((x, y))

    #Step 2: Move zig-zag and turn
    while True:
        if x < 0 or x >= n or y < 0 or y >= n:
            q.pop()
            return
        if dirs[d][0] != 0:
            #Step 2b: Turn around
            if (x == start[0] or x == start[0] + dirs[d][0]) and (y == 0 or y == n - 1):
                d = (d + 1) % 4
                continue
            
            #Step 2a: Move zig-zag
            if y - 1 >= 0 and (x, y - 1) not in q:
                y -= 1
                q.append((x, y))
            elif y + 1 < n and (x, y + 1) not in q:
                y += 1
                q.append((x, y))
            elif (x + dirs[d][0], y) not in q:
                x += dirs[d][0]
                q.append((x, y))
            else:
                return
        else:
            #Step 2b: Turn around
            if (y == start[1] or y == start[1] + dirs[d][1]) and (x == 0 or x == n - 1):
                d = (d + 1) % 4
                continue

            #Step 2a: Move zig-zag
            if x - 1 >= 0 and (x - 1, y) not in q:
                x -= 1
                q.append((x, y))
            elif x + 1 < n and (x + 1, y) not in q:
                x += 1
                q.append((x, y))
            elif (x, y + dirs[d][1]) not in q:
                y += dirs[d][1]
                q.append((x, y))
            else:
                return

def hamilton(x, y, n):
    q = deque()
    q.append((x, y))

    if n % 2 == 0:
        p = hamiltonEven(x, y, n)
        while p != (x, y):
            q.append(p)
            p = hamiltonEven(p[0], p[1], n)
    else:
        hamiltonOdd(x, y, n, q)

    return q

def BFS(n, b, bS, bG):
    q = Queue()
    disc = [bS]
    parent = {}
    q.put(bS)

    while not q.empty():
        v = q.get()
        if v == bG:
            break
        for e in [(v[0] + 1, v[1]), (v[0] - 1, v[1]), (v[0], v[1] + 1), (v[0], v[1] - 1)]:
            if e in disc:
                continue
            if e[0] < 0 or e[0] == n or e[1] < 0 or e[1] == n:
                continue
            if e in b and e != bG:
                continue
            
            disc.append(e)
            parent[e] = v
            q.put(e)

    path = []

    if bG not in parent:
        return path

    v = bG
    while v != bS:
        path.append(v)
        v = parent[v]
    path.append(bS)
    return path

def parseGraph(n, r, b):
    edges = []
    nodes = [[i, b[i]] for i in b]

    for i in range(len(nodes)):
        for j in range(i + 1, len(nodes)):
            path = BFS(n, b, nodes[i][0], nodes[j][0])
            if len(path) < 2:
                continue
            edges.append([nodes[i][0], nodes[j][0], path, len(path) - 1])
        path = BFS(n, b, r[0], nodes[i][0])
        if len(path) < 2:
            continue
        edges.append([r[0], nodes[i][0], path, len(path) - 1])

    return nodes, edges

def getDist(a, b, edges):
    for edge in edges:
        if (edge[0] == a and edge[1] == b) or (edge[0] == b and edge[1] == a):
            return edge[3]
    return 0

def getNode(pos, nodes):
    for node in nodes:
        if node[0] == pos:
            return node
    return None

def solve(queue, nodes, edges, robot):
    states = [] #Data robot, nodes, queue, rotations]
    rotations = 0

    cnt = 0

    while any(n[1] > 0 for n in nodes):
        #for s in states:
        #    print(s)
        #print(robot, nodes, queue, rotations)
        #print()
        cnt += 1

        if rotations >= len(queue):
            ### Revert to previous state
            if len(states) == 0:    #No solution found
                break

        dist = getDist(queue[0], robot[0], edges)
        if dist > 0:
            if dist <= robot[1]:
                ### Make move and update state stack
                node = getNode(queue[0], nodes)
                states.append([deepcopy(robot), deepcopy(nodes), deepcopy(queue), deepcopy(rotations)])
                charge = robot[1]
                robot[1] = node[1]
                node[1] = charge - dist
                robot[0] = node[0]
                rotations = 0
                queue.rotate(-1)
                continue
                ### <END>

        rotations += 1
        queue.rotate(-1)
        if rotations >= len(queue):
            ### Revert to previous state
            if len(states) == 0:    #No solution found
                break

            state = states.pop()
            robot = state[0]
            nodes = state[1]
            queue = state[2]
            rotations = state[3] + 1

            queue.rotate(-1)
            ### <END>
    return states


file = open(os.sys.argv[1], 'r')
lines = file.readlines()

size = int(lines[0])
robotValues = [int(s) for s in lines[1].split(',')]
robot = [(robotValues[0] - 1, robotValues[1] - 1), robotValues[2]]
battCount = int(lines[2])
batteries = dict()
for i in range(battCount):
    values = [int(s) for s in lines[3 + i].split(',')]
    batteries[(values[0] - 1, values[1] - 1)] = values[2]

n, e = parseGraph(size, robot, batteries)
print(n)
for E in e:
    print(E)

q = hamilton(robot[0][0], robot[0][1], size)
q = deque([p for p in q if any(p == node[0] for node in n)])
print(q)

states = solve(q, n, e, robot)
print(states)