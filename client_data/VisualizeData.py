#!/usr/bin/env python
# -*- coding: utf-8 -*-

import numpy as np
import matplotlib.pyplot as plt


nation1 = np.loadtxt("chr22_GWD.data", delimiter=',')
nation2 = np.loadtxt("chr22_JPT.data", delimiter=',')
#print(nation1)
#print(nation2)

nation1_x = nation1[:,0]
nation1_y = nation1[:,1]
nation2_x = nation2[:,0]
nation2_y = nation2[:,1]


plt.scatter(nation1_x, nation1_y, c="blue", label="GWD", marker=".", s=50)
plt.scatter(nation2_x, nation2_y, c="red",  label="JPT", marker=".", s=50)


plt.title("PCA result")
plt.legend(loc="upper left", fontsize=14)
plt.show()
