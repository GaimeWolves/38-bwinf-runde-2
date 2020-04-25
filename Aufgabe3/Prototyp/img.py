import os
import math
import operator

from PIL import Image, ImageDraw, ImageFont

nr = 25
nd = nr * 3

file = open(os.sys.argv[1])
lines = file.readlines()

cnt = int(lines[0][:-1])

start = (int(lines[1].split(',')[0][1:]), int(lines[1].split(',')[1][:-2]))
end = (int(lines[2].split(',')[0][1:]), int(lines[2].split(',')[1][:-2]))

edgesPoints = []

for i in range(3, 3 + cnt):
    edgesPoints.append([(int(lines[i].split()[0].split(',')[0][1:]), int(lines[i].split()[0].split(',')[1][:-1])), (int(lines[i].split()[1].split(',')[0][1:]), int(lines[i].split()[1].split(',')[1][:-1]))])

edges = []
nodes = []

for e in edgesPoints:
    if e[0] not in nodes:
        nodes.append(e[0])

    if e[1] not in nodes:
        nodes.append(e[1])

    edges.append((e[0], e[1]))

maxX = 0
maxY = 0
for n in nodes:
    if n[0] > maxX:
        maxX = n[0]

    if n[1] > maxY:
        maxY = n[1]

image = Image.new('RGB', (maxX * nd + nr * 4, maxY * nd + nr * 4))
draw = ImageDraw.Draw(image)
font = ImageFont.load_default()

def off(p):
    return p * nd + nr * 2

for e in edges:
    draw.line((off(e[0][0]), off(e[0][1]), off(e[1][0]), off(e[1][1])), (0, 255, 0), 4)

for n in nodes:
    draw.ellipse((off(n[0]) - nr, off(n[1]) - nr, off(n[0]) + nr, off(n[1]) + nr), (255, 255, 0) if n == start or n == end else (255, 0, 0))
    text = str(n)
    textSize = font.getsize(text)
    draw.text((off(n[0]) - textSize[0] / 2, off(n[1]) - textSize[1] / 2), text, (0, 0, 255) if n == start or n == end else (0, 255, 255), font)

del draw
image.save(os.sys.argv[2], 'PNG')