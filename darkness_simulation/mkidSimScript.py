import os, sys, time, struct
import numpy as np
import scipy.special as spfun
import scipy.signal as signal
import matplotlib.pyplot as plt
import mkid_processing as mkp
from SpeckleSimulator.SpeckleSim import SpeckleSim

def saveImgFile(image, imageLoc='./images'):
    timestamp = int(time.time()*10)
    print 'saving darkness image at', imageLoc
    image = image.transpose()
    darkImArr = image.flatten()
    darkImStr = struct.pack('{}{}'.format(len(darkImArr), 'H'), *darkImArr)
    #imgFn = os.path.join(imageLoc, str(timestamp)+'.img')
    imgFn = os.path.join(imageLoc, '12345.img')
    imgFile = open(imgFn, 'wb')
    imgFile.write(darkImStr)
    imgFile.close()
 
if __name__=='__main__':
    imgDir = './images'
    simDir = '.'
    kvecFn = 'kvecs.txt'
    speckleType = ['probe', 'corr']
    bPlot = True
    
    simParamDict = {'logNormMu':0.7, 'logNormSigma':0.5, 'sSigmaUniform':50, 'dt':0.001}
    imgParamDict = {'integrationTime':0.050, 'nMkidPixPerLD':2.7, 'psfCoords':np.array([40, 30]), 'addPoissonNoise':True, 'unFlatCal':True}
    beammapFile = '/home/neelay/SpeckleNulling/SpeckleSimulator/calFiles/finalMap_20180524.txt'
    flatCalFile = '/home/neelay/SpeckleNulling/SpeckleSimulator/calFiles/DomeJFlat1.npz'
    controlRegionX = np.array([-20, 20])
    controlRegionY = np.array([0, 45])

    controlRegionX += imgParamDict['psfCoords'][0]
    controlRegionY += imgParamDict['psfCoords'][1]

    speckleSim = SpeckleSim(nMkidPixPerLD=imgParamDict['nMkidPixPerLD'], beammapFile=beammapFile, flatCalFile=flatCalFile, dt=simParamDict['dt'])

    ## Dynamic Speckle Sim
    #speckleSim.setUniformLogNormTCorr(simParamDict['logNormMu'], simParamDict['logNormSigma'])
    #speckleSim.setCAmpToRadSin(20, 0.5, 0)
    #speckleSim.setUniformSSigma(simParamDict['sSigmaUniform'])
    #speckleSim.incrementClock(5)

    ## Static Single Speckle Sim
    speckleSim.curTCorr[:] = 10000
    speckleSim.setUniformSSigma(0)
    speckleSim.createSpeckle(2*np.pi*2.407407407, 2*np.pi*14.259259, 800, 0, 'corr')
    print 'Generating test speckle at ', 2.407407407, 14.259259, ' with amplitude ', 2500
    speckleSim.createSpeckle(2*np.pi*2.222222222, 2*np.pi*7.7777777, 800, np.pi/2, 'corr')
    speckleSim.createSpeckle(2*np.pi*-4, 2*np.pi*10, 800, np.pi/2, 'corr')

    ## Static Speckle Field Sim
    # speckleSim.setCAmpToRadSin(20, 0.5, 0)
    # speckleSim.setUniformSSigma(50)
    # speckleSim.setStaticRawS()


    image = speckleSim.getNextImage(imgParamDict['integrationTime'], imgParamDict['psfCoords'], imgParamDict['addPoissonNoise'], imgParamDict['unFlatCal'])
    image2 = speckleSim.getNextImage(imgParamDict['integrationTime'], imgParamDict['psfCoords'], imgParamDict['addPoissonNoise'], False)
    saveImgFile(image)
    np.savetxt(os.path.join(imgDir, 'IMG_READY'), [])
    imgSumList = []

    if bPlot == True:
        plt.ion()
        fig = plt.figure()
        fig2 = plt.figure()
        ax1 = fig.add_subplot(121)
        ax2 = fig.add_subplot(122)
        ax3 = fig2.add_subplot(111)
    
    while True:
        while(not os.path.isfile(os.path.join(simDir, 'KVECS_READY'))):
            pass
        os.remove(os.path.join(simDir, 'KVECS_READY'))
        kx, ky, amplitudes, phases, isFinal = np.loadtxt((os.path.join(simDir, kvecFn)), unpack=True, ndmin=1)
        if kx.shape==():
            kx = np.asarray([kx])
            ky = np.asarray([ky])
            amplitudes = np.asarray([amplitudes])
            phases = np.asarray([phases])
            isFinal = np.asarray([isFinal])
        os.remove(os.path.join(simDir, kvecFn))
        speckleSim.clearProbeSpeckles()
        for i in range(len(kx)):
            speckleSim.createSpeckle(kx[i], ky[i], amplitudes[i], phases[i], speckleType[int(isFinal[i])], width=6)
            print 'generating speckle at', kx[i]/(2*np.pi), ky[i]/(2*np.pi), 'amplitude:', amplitudes[i], 'phase:', phases[i], speckleType[int(isFinal[i])]
        
        prevImage = image
        image = speckleSim.getNextImage(imgParamDict['integrationTime'], imgParamDict['psfCoords'], imgParamDict['addPoissonNoise'], imgParamDict['unFlatCal'])
        speckleCoords = np.unravel_index(np.argmax(speckleSim.totalAmp), speckleSim.totalAmp.shape)
        saveImgFile(image)
        np.savetxt(os.path.join(imgDir, 'IMG_READY'), [])

        # if bPlot:
        #     fig = plt.figure()
        #     ax1 = fig.add_subplot(121)
        #     ax2 = fig.add_subplot(122)
        #     img1 = ax1.imshow(speckleSim.totalAmp.real[speckleCoords[0]-10:speckleCoords[0]+10, speckleCoords[1]-10:speckleCoords[1]+10])
        #     img2 = ax2.imshow(speckleSim.totalAmp.imag[speckleCoords[0]-10:speckleCoords[0]+10, speckleCoords[1]-10:speckleCoords[1]+10])
        #     plt.colorbar(img1, ax=ax1)
        #     plt.colorbar(img2, ax=ax2)
        #     plt.show() 

        if bPlot:
            #fig = plt.figure()
            #ax1 = fig.add_subplot(121)
            #ax2 = fig.add_subplot(122)
            ax1.clear()
            ax2.clear()
            ax3.clear()
            imgSumList.append(np.sum(image[controlRegionX[0]:controlRegionX[1], controlRegionY[0]:controlRegionY[1]]))
            img1 = ax1.imshow(np.transpose(prevImage))
            img2 = ax2.imshow(np.transpose(image))
            ax3.plot(imgSumList)
            plt.draw()
            plt.pause(0.01)
            # plt.colorbar(img1, ax=ax1)
            # plt.colorbar(img2, ax=ax2)
            # plt.show() 
    
 

