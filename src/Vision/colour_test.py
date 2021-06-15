import cv2

image = cv2.imread(r'gamer.jpeg')

height, width, depth = image.shape

# things only true for balls
#  0 >= 160
#  1 >= 232
#  2 >= 216

# blue
#  image[i,j][0] > (image[i,j][1]+16)

# green
#  (image[i,j][0]<(image[i,j][1]+32)) and (image[i,j][0]>(image[i,j][1]-24)) and (image[i,j][2]<160) and (image[i,j][1]>=128)

# pink
#  image[i,j][2] > (image[i,j][1]+96)

# yellow
#  ((grey >= 128) and (image[i,j][2]<(image[i,j][1]+32)) and (image[i,j][2]>(image[i,j][1]-24)) and (image[i,j][0]<64)) or ((image[i,j][2]>=240) and (image[i,j][1]>=240) and (image[i,j][0]<176))

for i in range(0, height):
    for j in range(0, width):
        """
        grey = (image[i,j][2]>>1)+(image[i,j][1]>>2)+(image[i,j][0]>>2)
        if image[i,j][0] > (image[i,j][1]+16):
            image[i,j] = [255,0,0]
        elif (image[i,j][0]<(image[i,j][1]+32)) and (image[i,j][0]>(image[i,j][1]-24)) and (image[i,j][2]<160) and (image[i,j][1]>=128):
            image[i,j] = [0,255,0]
        elif image[i,j][2] > (image[i,j][1]+96):
            image[i,j] = [128,0,128]
        elif ((grey >= 128) and (image[i,j][2]<(image[i,j][1]+32)) and (image[i,j][2]>(image[i,j][1]-24)) and (image[i,j][0]<64)) or ((image[i,j][2]>=240) and (image[i,j][1]>=240) and (image[i,j][0]<176)):
            image[i,j] = [0,128,128]
            """
        if (image[i,j][2]>=128) and (image[i,j][1]<128) and (image[i,j][0]<128):
            image[i,j] = [0,0,255]
        else:
            image[i,j] = [0,0,0]

cv2.imshow("Image", image)
cv2.waitKey(0)
