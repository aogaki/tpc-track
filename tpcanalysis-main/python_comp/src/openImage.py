import numpy as np
from matplotlib import image
from matplotlib import pyplot as plt


class openImage:
    def __init__(self, m_filename=""):
        self.m_filename = m_filename
        self.m_image = []
        self.m_data = []

    def setFileName(self, new_filename):
        self.m_filename = new_filename

    def readImage(self):
        self.m_image = image.imread(self.m_filename)
        self.m_data = np.array(self.m_image)

    def printImageArray(self):
        print(self.m_data)

    def viewImage(self):
        plt.imshow(self.m_data)
        plt.show()

    def trimImage(self):
        threshold = 0.99
        r, g, b = self.m_data[..., 0], self.m_data[..., 1], self.m_data[..., 2]
        rows, cols = np.where(np.all(self.m_data < threshold, axis=-1))
        min_row, max_row = np.min(rows), np.max(rows)
        min_col, max_col = np.min(cols), np.max(cols)
        r, g, b = r[min_row:max_row, min_col:max_col], g[min_row:max_row,
                                                         min_col:max_col], b[min_row:max_row, min_col:max_col]
        self.m_data = np.stack([r, g, b], axis=-1)
