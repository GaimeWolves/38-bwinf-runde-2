import os
import math
import operator

from PIL import Image, ImageDraw, ImageFont

nr = 9
nd = 1

file = open(os.sys.argv[1], 'r')
lines = file.readlines()

size = int(lines[0])
robotValues = [int(s) for s in lines[1].split(',')]
robot = [(robotValues[0] - 1, robotValues[1] - 1), robotValues[2]]
battCount = int(lines[2])
batteries = []
for i in range(battCount):
    values = [int(s) for s in lines[3 + i].split(',')]
    batteries.append([(values[0] - 1, values[1] - 1), values[2]])

def off(p):
    return p * nr * 2 + nr + 15

image = Image.new('RGB', (size * nr * 2 + 15 + 5, size * nr * 2 + 15 + 5), (255, 255, 255))
draw = ImageDraw.Draw(image)
font = ImageFont.load_default()

for x in range(size):
    for y in range(size):
        draw.rectangle((off(x) - nr, off(y) - nr, off(x) + nr, off(y) + nr), None, 0, nd)

for x in range(0, size, 3):
    text = str(x)
    textSize = font.getsize(text)
    draw.text((off(x) - textSize[0] / 2, 8 - textSize[1] / 2), text, (0, 0, 0), font)
    draw.text((8 - textSize[0] / 2, off(x) - textSize[1] / 2), text, (0, 0, 0), font)

draw.rectangle((off(robot[0][0]) - nr + nd, off(robot[0][1]) - nr + nd, off(robot[0][0]) + nr - nd, off(robot[0][1]) + nr - nd), (0, 255, 0))
text = str(robot[1])
textSize = font.getsize(text)
draw.text((off(robot[0][0]) - textSize[0] / 2, off(robot[0][1]) - textSize[1] / 2), text, (0, 0, 0), font)

for b in batteries:
    draw.rectangle((off(b[0][0]) - nr + nd, off(b[0][1]) - nr + nd, off(b[0][0]) + nr - nd, off(b[0][1]) + nr - nd), (255, 128, 0))
    text = str(b[1])
    textSize = font.getsize(text)
    draw.text((off(b[0][0]) - textSize[0] / 2, off(b[0][1]) - textSize[1] / 2), text, (0, 0, 0), font)

image.save(os.sys.argv[2], 'PNG')