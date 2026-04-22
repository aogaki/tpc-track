import pandas as pd


class openCSV:
    def __init__(self, m_filename=""):
        self.m_filename = m_filename
        self.m_data = []

    def setFileName(self, new_filename):
        self.m_filename = new_filename

    def readCSV(self):
        self.m_data = pd.read_csv(self.m_filename, sep=',')

    def printCSV(self):
        print(self.m_data)
