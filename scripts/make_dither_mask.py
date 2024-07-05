#!/usr/bin/env python3

import sys

from PIL import Image


print("{")
line = "   "
with Image.open(sys.argv[1]) as img:
    for y in range(img.height):
        for x in range(img.width):
            val = img.getpixel((x, y))
            old_line = line
            line += " %d," % val
            if len(line) > 70:
                print(old_line)
                line = "    %d," % val
print(line[:-1])
print("};")
