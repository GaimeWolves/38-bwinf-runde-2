from collections import deque
from copy import deepcopy
from pprint import pprint

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

def solve(n, r, b):
    q = hamilton(r[0][0], r[0][1], size)
    q.rotate()
    print(r[0], q[0])
    initPos = (deepcopy(r[0]))

    stateStack = []

    while not all(b[v] == 0 for v in b):
        while q[0] not in b:
            if q[0] == initPos and len(stateStack) == 0:
                raise(ValueError("Unsolvable Configuration"))
            q.rotate()

        if q[0] == r[0]:
            if len(stateStack) == 0:
                raise(ValueError("Unsolvable Configuration"))
            state = stateStack.pop()
            r = state[0]
            b = state[1]
            q.rotate()
            continue

        if b[q[0]] == 0:
            q.rotate()
            continue
        if abs(r[0][0] - q[0][0]) + abs(r[0][1] - q[0][1]) > r[1]:
            q.rotate()
            continue

        stateStack.append((deepcopy(r), deepcopy(b)))
        tmp = r[1] - (abs(r[0][0] - q[0][0]) + abs(r[0][1] - q[0][1]))
        r[0] = q[0]
        r[1] = b[q[0]]
        b[q[0]] = tmp
        q.rotate()
    
    return stateStack

def toMoveList(ss):
    moves = []
    for i in range(1, len(ss)):
        for x in range(ss[i - 1][0][0][0], ss[i][0][0][0], (ss[i - 1][0][0][0] - ss[i][0][0][0]) // abs(ss[i - 1][0][0][0] - ss[i][0][0][0])):
            moves.append((x, ))

file = open('D:/OneDrive - WMS-HN.ORG/38-bwinf-runde-2/Aufgabe1/Beispiele/stromralley0.txt', 'r') #open('C:/Users/n.djerfi/OneDrive - WMS-HN.ORG/38-bwinf-runde-2/Aufgabe1/Beispiele/stromralley3.txt', 'r')
lines = file.readlines()

size = int(lines[0])
robotValues = [int(s) for s in lines[1].split(',')]
robot = [(robotValues[0] - 1, robotValues[1] - 1), robotValues[2]]
battCount = int(lines[2])
batteries = dict()
for i in range(battCount):
    values = [int(s) for s in lines[3 + i].split(',')]
    batteries[(values[0] - 1, values[1] - 1)] = values[2]

print(solve(size, robot, batteries))