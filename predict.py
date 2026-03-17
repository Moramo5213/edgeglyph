import sys
import numpy as np
import torch
import torch.nn as nn
from PIL import Image

from PyQt5.QtWidgets import QApplication, QWidget, QLabel, QPushButton
from PyQt5.QtGui import QPainter, QPen, QImage
from PyQt5.QtCore import Qt, QPoint


# -----------------------
# Model
# -----------------------

class TinyCNN(nn.Module):

    def __init__(self):
        super().__init__()

        self.conv = nn.Sequential(
            nn.Conv2d(1,8,3,padding=1),
            nn.ReLU(),
            nn.MaxPool2d(2),

            nn.Conv2d(8,16,3,padding=1),
            nn.ReLU(),
            nn.MaxPool2d(2)
        )

        self.fc = nn.Sequential(
            nn.Linear(16*7*7,64),
            nn.ReLU(),
            nn.Linear(64,62)
        )

    def forward(self,x):
        x = x.view(-1,1,28,28)
        x = self.conv(x)
        x = x.view(x.size(0),-1)
        x = self.fc(x)
        return x


model = TinyCNN()
model.load_state_dict(torch.load("mlp_emnist_best.pt", map_location="cpu"))
model.eval()


IDX_TO_CHAR = list(
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
)


# -----------------------
# Preprocess
# -----------------------

def preprocess(img):

    img = np.array(img)

    # invert
    img = 255 - img

    mask = img > 30

    ys, xs = np.where(mask)

    if len(xs) == 0:
        return np.zeros((28,28))

    x0,x1 = xs.min(), xs.max()
    y0,y1 = ys.min(), ys.max()

    crop = img[y0:y1+1, x0:x1+1]

    h,w = crop.shape
    size = max(h,w)

    canvas = np.zeros((size,size))

    canvas[(size-h)//2:(size-h)//2+h,
           (size-w)//2:(size-w)//2+w] = crop

    canvas = Image.fromarray(canvas).resize((28,28))

    x = np.array(canvas).astype(np.float32)

    # EMNIST rotation fix
    x = np.transpose(x)
    # x = np.flip(x, axis=0)


    x = x / 255.0

    return x


# -----------------------
# Canvas
# -----------------------

class Canvas(QWidget):

    def __init__(self):

        super().__init__()

        self.setWindowTitle("EMNIST OCR")

        self.setFixedSize(300,340)

        self.image = QImage(280,280,QImage.Format_RGB32)
        self.image.fill(Qt.white)

        self.last_point = None


        self.label = QLabel(self)
        self.label.move(10,290)
        self.label.resize(280,30)
        self.label.setText("prediction: ")


        btn = QPushButton("Clear",self)
        btn.move(200,290)
        btn.clicked.connect(self.clear)


    def clear(self):

        self.image.fill(Qt.white)
        self.update()
        self.label.setText("prediction: ")


    def paintEvent(self,event):

        painter = QPainter(self)
        painter.drawImage(10,10,self.image)


    def mousePressEvent(self,event):

        if event.button()==Qt.LeftButton:
            self.last_point = event.pos() - self.offset()


    def mouseMoveEvent(self,event):

        if event.buttons() & Qt.LeftButton:

            painter = QPainter(self.image)

            pen = QPen(Qt.black,12,Qt.SolidLine,Qt.RoundCap,Qt.RoundJoin)
            painter.setPen(pen)

            painter.drawLine(self.last_point,event.pos()-self.offset())

            self.last_point = event.pos()-self.offset()

            self.update()


    def mouseReleaseEvent(self,event):

        self.predict()


    def offset(self):

        return self.rect().topLeft() + QPoint(10,10)


    def predict(self):

        ptr = self.image.bits()
        ptr.setsize(self.image.byteCount())

        arr = np.array(ptr).reshape(280,280,4)

        gray = arr[:,:,0]

        x = preprocess(gray)

        x = torch.tensor(x).unsqueeze(0)

        with torch.no_grad():
            y = model(x)

        pred = torch.argmax(y,dim=1).item()

        char = IDX_TO_CHAR[pred]

        self.label.setText(f"prediction: {char}")


# -----------------------
# Main
# -----------------------

app = QApplication(sys.argv)

canvas = Canvas()
canvas.show()

sys.exit(app.exec_())