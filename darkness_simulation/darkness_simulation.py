import os, sys, time, struct
import numpy as np
import scipy.special as spfun
import scipy.signal as signal
import matplotlib.pyplot as plt
import mkid_processing as mkp

class OpticsSim:
    
    def __init__(self, xWidth=70, yWidth=70, nLDperPix=8, darkNLD=2, beammapFile=None):
        '''
        INPUTS: 
            xWidth, yWidth: image size in lambda/D
        '''
        self.speckleFieldImage = np.zeros((xWidth*nLDperPix, yWidth*nLDperPix), dtype=np.complex128)
        self.probeSpeckleImage = np.zeros((xWidth*nLDperPix, yWidth*nLDperPix), dtype=np.complex128)  
        self.imageCoords = np.mgrid[-xWidth/2:xWidth/2:1./nLDperPix, -yWidth/2:yWidth/2:1./nLDperPix]
        self.nLD = nLDperPix
        self.darkNLD = darkNLD
        self.xWidth = xWidth
        self.yWidth = yWidth

        x0 = np.argmin(np.abs(self.imageCoords[0,:,0])) #location of central PSF
        y0 = np.argmin(np.abs(self.imageCoords[1,0,:]))
        self.psfCoords = np.array([x0, y0])
        self.badPixelMask = np.zeros((80,125))

        if not beammapFile is None:
            self.badPixelMask = np.array(mkp.getBeammappedPixels(beammapFile), dtype=np.int32)

        self.badPixels = np.where(self.badPixelMask)

    def createSpeckle(self, kx, ky, amplitude, phase, source):
        xLoc = kx/(2*np.pi)
        yLoc = ky/(2*np.pi)
        distCoords = np.copy(self.imageCoords)
        distCoords[0] -= xLoc
        distCoords[1] -= yLoc
        distCoords = np.sqrt(distCoords[0]**2+distCoords[1]**2)
        if np.any(distCoords==0):
            distCoords[np.where(distCoords==0)]=0.01
        speckle = amplitude*2*np.exp(phase*1j)*spfun.jn(1, np.pi*distCoords)/(np.pi*distCoords)

        if source=='probe':
            self.probeSpeckleImage += speckle

        elif source=='real':
            self.speckleFieldImage += speckle

        else:
            raise Exception('Specify speckle source: probe or real')

    def clearProbeSpeckles(self):
        self.probeSpeckleImage = np.zeros((self.xWidth*self.nLD, self.yWidth*self.nLD), dtype=np.complex128)  

    def addProbeSpecklesToImage(self):
        self.speckleFieldImage += self.probeSpeckleImage
        self.clearProbeSpeckles()

    def generateDarknessImage(self, darkPSFLoc):
        '''
        INPUTS
            psfLoc - location of PSF on DARKNESS image, in pixel units
        '''
        image = self.probeSpeckleImage + self.speckleFieldImage
        darkPSFCoords = np.array(darkPSFLoc*int(self.nLD/self.darkNLD), dtype=np.int32)
        darkImSize = np.array([80, 125])*int(self.nLD/self.darkNLD)
        imgOffs = self.psfCoords - darkPSFCoords
        print 'psfCoords', self.psfCoords
        print 'imgOffs', imgOffs
        print 'imgOffs+darkImSize', imgOffs+darkImSize
        darkIm = image[imgOffs[0]:darkImSize[0]+imgOffs[0], imgOffs[1]:darkImSize[1]+imgOffs[1]]
        darkIm = np.array(np.abs(darkIm)**2, dtype=np.float64)
        
        downSampFilt = np.ones((int(self.nLD/self.darkNLD), int(self.nLD/self.darkNLD)))
        darkIm = signal.correlate(darkIm, downSampFilt, 'valid')
        self.darkIm = darkIm[0:-1:int(self.nLD/self.darkNLD), 0:-1:int(self.nLD/self.darkNLD)]
        darkImShape = np.shape(self.darkIm)
        self.darkIm = np.pad(self.darkIm, ((0,80-darkImShape[0]),(0,125-darkImShape[1])), 'edge')
        self.darkIm[self.badPixels] = 0

    def saveDarknessImage(self, imageLoc='./images'):
        timestamp = int(time.time()*10)
        print 'saving darkness image at', imageLoc
        darkImArr = self.darkIm.flatten()
        darkImStr = struct.pack('{}{}'.format(len(darkImArr), 'H'), *darkImArr)
        #imgFn = os.path.join(imageLoc, str(timestamp)+'.img')
        imgFn = os.path.join(imageLoc, '12345.img')
        imgFile = open(imgFn, 'wb')
        imgFile.write(darkImStr)
        imgFile.close()

    def plotImage(self, ax=None, fig=None):
        image = self.probeSpeckleImage + self.speckleFieldImage
        if ax is None:
            plt.imshow(np.transpose(np.abs(image)**2))
            plt.show()

        else:
            if fig is None:
                raise Exception('Must pass a figure object!')
            cax = ax.imshow(np.transpose(np.abs(image)**2))
            #fig.colorbar(cax, ax)

    def plotDarkImage(self, ax=None, fig=None):
        if ax is None:
            plt.imshow(np.transpose(self.darkIm))
            plt.show()

        else:
            if fig is None:
                raise Exception('Must pass a figure object!')
            cax = ax.imshow(np.transpose(self.darkIm))
            #fig.colorbar(cax, ax)
    
if __name__=='__main__':
    imgDir = './images'
    simDir = '.'
    kvecFn = 'kvecs.txt'
    speckleType = ['probe', 'real']
    
    nSpeckles = 3
    amplitudeRange = 5
    xRange = 15 #lambda/D
    yRange = 20
    psfLoc = np.array([40, 60])

    phases = np.pi*np.random.rand(nSpeckles)
    amplitudes = amplitudeRange*np.random.rand(nSpeckles)
    kxList = 2*xRange*np.pi*np.random.rand(nSpeckles)
    kyList = 2*yRange*np.pi*np.random.rand(nSpeckles)
    opticsSim = OpticsSim()
    for i in range(nSpeckles):
        opticsSim.createSpeckle(kxList[i], kyList[i], amplitudes[i], phases[i], 'real')
        print 'generating speckle at', kxList[i]/(2*np.pi), kyList[i]/(2*np.pi), 'amplitude:', amplitudes[i], 'phase:', phases[i]
    
    opticsSim.generateDarknessImage(psfLoc)
    #plt.ioff()
    #fig1 = plt.figure()
    #fig2 = plt.figure()
    #ax1 = fig1.add_subplot(111)
    #ax2 = fig2.add_subplot(111)
    #opticsSim.plotImage(ax1, fig1)
    #opticsSim.plotDarkImage()
    #plt.show()
    opticsSim.saveDarknessImage(imgDir)
    np.savetxt(os.path.join(imgDir, 'IMG_READY'), [])
    
    while True:
        while(not os.path.isfile(os.path.join(simDir, 'KVECS_READY'))):
            pass
        os.remove(os.path.join(simDir, 'KVECS_READY'))
        kx, ky, amplitudes, phases, isFinal = np.loadtxt((os.path.join(simDir, kvecFn)), unpack=True)
        # kx = np.asarray([kx])
        # ky = np.asarray([ky])
        # amplitudes = np.asarray([amplitudes])
        # phases = np.asarray([phases])
        # isFinal = np.asarray([isFinal])
        os.remove(os.path.join(simDir, kvecFn))
        opticsSim.clearProbeSpeckles()
        for i in range(len(kx)):
            opticsSim.createSpeckle(kx[i], ky[i], amplitudes[i], phases[i], speckleType[int(isFinal[i])])
            print 'generating speckle at', kx[i]/(2*np.pi), ky[i]/(2*np.pi), 'amplitude:', amplitudes[i], 'phase:', phases[i], speckleType[int(isFinal[i])]
        
        opticsSim.generateDarknessImage(psfLoc)
        opticsSim.plotDarkImage()
        plt.show()
        opticsSim.saveDarknessImage(imgDir)
        np.savetxt(os.path.join(imgDir, 'IMG_READY'), [])
    
    

    

