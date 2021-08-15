import numpy as np
import struct
import cv2

def channels_reorder(array):
    a = array[:, 0:len(array[0] - 2):3]
    b = array[:, 1:len(array[0] - 2):3]
    c = array[:, 2:len(array[0] - 2):3]
    a = a[:, :, None]
    b = b[:, :, None]
    c = c[:, :, None]
    m = np.concatenate((a, b, c), axis=2)
    return m

def load_image(image, channels, name, t):
    print(image.shape, channels, name, t)
    if channels == 1:
        dst_img = image
    elif channels == 3:
        dst_img = channels_reorder(image)
    else:
        print("Error: channels == ", channels, "is not supported.")
        return

    # rgbImg=cv2.cvtColor(img,cv2.COLOR_GRAY2BGR)
    print(dst_img.shape)
    cv2.imshow("test", dst_img)
    cv2.waitKey(10)

    bb = [[10, 1011], [10, 2], [2, 56], [3, 3.4]]
    # bb.append([2,34])
    return bb

def get_tuple():
    bb = [[1, 2], [3, 4], [5, 6], [7, 8]]
    return ('Hello world.', 1, "good!", bb)

def test_traceback():
    printf(123)  # NameError

def struct_usage():
    values = (1, b'abc', 2.7)
    s0 = struct.Struct('i3sf')
    packed_data = s0.pack(*values)
    print(packed_data)
    ret0 = struct.unpack('i3sf', packed_data)
    print(ret0)
    
    s = b'bcde'
    st = struct.pack('ii4si', 1, 2, s, 3)
    ret1 = struct.unpack('ii4si', st)
    print(ret1)

def test_struct(input):
    print("Python receive: ", len(input), input)
    ret = struct.unpack(input[0], input[1])
    print("Unpack: ", ret)
    
    buf0 = ret[0] + 1
    buf1 = ret[1] + 1
    buf2 = b'dfds1234'
    buf3 = ret[3] + 1
    buf4 = ret[4] + 1
    
    print("Pack and return.")
    bin_buf_all = struct.pack(input[0], buf0, buf1, buf2, buf3, buf4, 8, 9)
    return bin_buf_all

#if __name__ == '__main__':
#    obj = CTest()
#    print(obj.get_tuple2())