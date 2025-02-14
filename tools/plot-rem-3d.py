#! /usr/bin/env python3

# Copyright (c) 2025 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
#
# SPDX-License-Identifier: GPL-2.0-only

import argparse
import csv
import os

import matplotlib.animation as animation
import numpy as np
from matplotlib import pyplot as plt
from matplotlib.colors import Normalize

args_parser = argparse.ArgumentParser()
args_parser.add_argument("rem_file", help="Input nr-rem-*.out path", type=str)
args_parser.add_argument(
    "--output_file", help="Output figure path", type=str, default="plot-rem-3d.gif"
)
args = args_parser.parse_args()

fieldnames = ["x", "y", "z", "avgSnrDb", "avgSinrDb", "avRxPowerDbm", "avgSirDb"]
with open(args.rem_file, "r") as f:
    remOutput = csv.DictReader(f, delimiter="\t", fieldnames=fieldnames)
    remOutput = list(remOutput)

# Normalize values to prune useless values
minSinr = min(list(map(lambda s: float(s["avgSinrDb"]), remOutput)))
maxSinr = max(list(map(lambda s: float(s["avgSinrDb"]), remOutput)))
norm = Normalize(vmin=minSinr, vmax=maxSinr)
normalizedValues = norm(list(map(lambda s: float(s["avgSinrDb"]), remOutput)))

# Discard values lower than 75%
# discardedIndices = list(map(lambda x: x if normalizedValues[x] < 0.75 else -1, range(len(normalizedValues))))
# discardedIndices = list(filter(lambda x: x != -1, discardedIndices))
# discardedIndices.sort(reverse=True)
# for discardedIndex in discardedIndices:
#    remOutput.pop(discardedIndex)

heights = list(sorted(list(set(map(lambda s: float(s["z"]), remOutput)))))

fig = plt.figure()
ax = fig.add_subplot(111, projection="3d")
norm = Normalize(vmin=minSinr, vmax=maxSinr)
ax.view_init(azim=45, elev=20)
ax.set_zlim((heights[0], heights[-1]))


def plotHeight(i):
    remOutputFiltered = list(filter(lambda s: float(s["z"]) == heights[i], remOutput))
    ax.scatter(
        list(map(lambda s: float(s["x"]), remOutputFiltered)),
        list(map(lambda s: float(s["y"]), remOutputFiltered)),
        list(map(lambda s: float(s["z"]), remOutputFiltered)),
        c=list(map(lambda s: float(s["avgSinrDb"]), remOutputFiltered)),
        norm=norm,
    )
    return (ax,)


animatedRem = animation.FuncAnimation(
    fig, plotHeight, frames=range(len(heights)), interval=30, blit=False, repeat=True
)
animatedRem.save(args.output_file, writer="imagemagick", fps=1, dpi=300)
