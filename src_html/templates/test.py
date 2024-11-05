#!/usr/bin/env python
import os

os.makedirs("test", exist_ok=True)

comp = ""
index = ""

with open("comparison.html", "r") as f:
    comp = f.read()

with open("index.html", "r") as f:
    index = f.read()


def replace(c1, c2):
    global comp, index
    comp = comp.replace("{CHOICE1}", c1)
    comp = comp.replace("{CHOICE2}", c2)
    index = index.replace("{CHOICES}", comp)


replace("Cracow University of Technology", "Is better")

with open("test/test.html", "w") as f:
    f.write(index)
