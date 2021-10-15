import os
import sys
import rle
from scipy.signal import savgol_filter
from scipy.fftpack import dct, idct
from matplotlib import pyplot as plt
import numpy as np
import cv2
import pyrr
import math

class Config:
    def __init__(self, near, far, sensorWidth, focalLength, offset, wsDir, imgDir):
        self.near = near
        self.far = far
        self.sensorWidth = sensorWidth
        self.focalLength = focalLength
        self.offset = offset
        self.wsDir = imgDir
        self.imgDir = wsDir

def projectNew(near, far, fov, offset, wsFile, imgFile, size, aspect, lightfield):
    im = cv2.cvtColor(cv2.imread(imgFile), cv2.COLOR_BGR2BGRA)
    imd = cv2.cvtColor(cv2.imread(wsFile,  cv2.IMREAD_ANYCOLOR | cv2.IMREAD_ANYDEPTH), cv2.COLOR_BGR2RGB)

    proj = pyrr.matrix44.create_perspective_projection(np.degrees(fov), aspect, near, far)
    view = pyrr.matrix44.create_look_at(pyrr.vector3.create(offset, 0, 0),pyrr.vector3.create(offset, 0, 1),pyrr.vector3.create(0, -1, 0))
    viewProjection = np.matmul(view, proj)

    class Pixel:
        def __init__(self, rgb, z):
            self.rgb = rgb
            self.z = z

    class Point:
        def __init__(self, coords, z):
            self.coords = coords
            self.z = z

    limit = size-1
    def projectPixel(x,y,point):
        wPoint = point*far
        wPoint = np.append(wPoint, 1)
        newPoint =np.matmul(wPoint, viewProjection)
        if abs(newPoint[3])<0.000000000000001:
           return np.array([x,y])
        newPoint /= newPoint[3]
        newCoord = (np.array([newPoint[0], newPoint[1]])+1)*size/2
        newCoord -= 0.5;
        newCoord[0]=np.clip(newCoord[0], 0, limit[0])
        #newCoord[1]=np.clip(newCoord[1], 0, limit[1])
        return Point(coords=np.rint(newCoord).astype(int), z=wPoint[2])

    imn = np.zeros((size[1], size[0]+1000, 4), im.dtype)
    for x in range(size[0]):
        for y in range(size[1]):
            point = projectPixel(x, y, imd[y,x])
            #lightfield[point.coords[0]][point.coords[1]].append(Pixel(im[y,x], point.z))
            imn[point.coords[1], point.coords[0]] = im[y,x]
    cv2.imwrite("/home/ichlubna/Downloads/"+os.path.basename(imgFile), imn)

def projectLightfield(config):
    imgDir = sorted(os.listdir(config.imgDir))
    wsDir = sorted(os.listdir(config.wsDir))
    im = cv2.cvtColor(cv2.imread(os.path.join(config.imgDir, imgDir[0])), cv2.COLOR_BGR2BGRA)
    size = np.array([im.shape[1], im.shape[0]])
    aspect = float(size[0])/size[1]
    fov =  2*np.arctan((config.sensorWidth/aspect)/(config.focalLength*2))

    lightfield = []
    for x in range(size[0]*2):
        lightfield.append([])
        for y in range(size[1]):
            lightfield[x].append([])

    for i in range(len(imgDir)):
        img = os.path.join(config.imgDir, imgDir[i])
        ws = os.path.join(config.wsDir, wsDir[i])
        #TODO check if same
        if os.path.isfile(img):
            if i == 0:
                continue
            projectNew(config.near, config.far, fov, i*config.offset, ws, img, size, aspect, lightfield)
            print(img + " processed")


config = Config(0.1, 100, 36, 50, -0.009339, sys.argv[1], sys.argv[2])
projectLightfield(config)





def colorFunction():
    cx=500
    cy=900
    pixels = [[]]
    pixels.append([])
    pixels.append([])
    directory = sys.argv[1]
    for filename in directory:
        f = os.path.join(directory, filename)
        if os.path.isfile(f):
            im = Image.open(f).convert("YCbCr")
            j=0
            for x in range(cx,cx+2):
                for y in range(cy,cy+1):
                    px = im.load()[x,y]
                    pixels[j].append(px[0])
                j += 1
        print(f)

    #s = savgol_filter(pixels[0], 13, 5)
    c = dct(pixels[0], norm="ortho")
    dec = 10
    clamp = 128
    for i in range(500,len(c)):
        c[i] = 0#max(min(round(c[i]*dec), clamp),-clamp)/dec

    delta = []
    for i in range(0,len(c)):
        delta.append(abs(pixels[0][i]-pixels[1][i-1]))

    fpixels = idct(c, norm="ortho")
    fig, axis = plt.subplots(3)
    axis[0].plot(pixels[0])
    axis[1].plot(pixels[1])
    axis[2].plot(delta)
    plt.show()

    print(*c)
    d = rle.encode(c)
    print(sys.getsizeof(c))
    print(sys.getsizeof(d))
    print(sys.getsizeof(rle.decode(d[0], d[1])))
    print(sys.getsizeof(pixels[0]))

