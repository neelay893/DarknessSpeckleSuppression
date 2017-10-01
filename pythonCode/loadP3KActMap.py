import numpy as np
import os, sys
from makeBadPixMask import saveBinImg

actMapTxtPath = "/mnt/data0/speckle_nulling/matlab_code/reflatmapwafflepatterngeneration/act.txt"

actMapSaveDir = os.path.join("../p3kBinFiles")

actMap = np.loadtxt(actMapTxtPath);
saveBinImg(actMap, os.path.join(actMapSaveDir, "actMap.bin"))
