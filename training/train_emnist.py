import numpy as np
import pandas as pd
import torch
import torch.nn as nn
import torch.optim as optim

from sklearn.model_selection import train_test_split
from torch.utils.data import DataLoader, TensorDataset
from tqdm import tqdm


DATA_PATH = "../input/datasets/crawford/emnist/"
CHECKPOINT_PATH = "mlp_emnist_best.pt"
EPOCHS = 10
BATCH_SIZE = 256
LEARNING_RATE = 5e-4
VAL_SPLIT = 0.1
RANDOM_SEED = 42


class TinyCNN(nn.Module):
    def __init__(self):
        super().__init__()
        self.conv = nn.Sequential(
            nn.Conv2d(1, 8, 3, padding=1),
            nn.ReLU(),
            nn.MaxPool2d(2),
            nn.Conv2d(8, 16, 3, padding=1),
            nn.ReLU(),
            nn.MaxPool2d(2),
        )
        self.fc = nn.Sequential(
            nn.Linear(16 * 7 * 7, 64),
            nn.ReLU(),
            nn.Linear(64, 62),
        )

    def forward(self, x):
        x = x.view(-1, 1, 28, 28)
        x = self.conv(x)
        x = x.view(x.size(0), -1)
        x = self.fc(x)
        return x


def accuracy(logits, target):
    pred = logits.argmax(dim=1)
    return (pred == target).float().mean().item()


def make_loader(features, labels, shuffle=False):
    return DataLoader(
        TensorDataset(features, labels),
        batch_size=BATCH_SIZE,
        shuffle=shuffle,
        num_workers=2,
    )


def main():
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print("Device:", device)
    print("Loading CSV files...")

    train_df = pd.read_csv(DATA_PATH + "emnist-byclass-train.csv", header=None)
    test_df = pd.read_csv(DATA_PATH + "emnist-byclass-test.csv", header=None)

    print("Train shape:", train_df.shape)
    print("Test shape:", test_df.shape)

    x_train = train_df.iloc[:, 1:].values / 255.0
    y_train = train_df.iloc[:, 0].values
    x_test = test_df.iloc[:, 1:].values / 255.0
    y_test = test_df.iloc[:, 0].values

    train_x, val_x, train_y, val_y = train_test_split(
        x_train,
        y_train,
        test_size=VAL_SPLIT,
        random_state=RANDOM_SEED,
    )

    train_x = torch.tensor(train_x, dtype=torch.float32)
    train_y = torch.tensor(train_y, dtype=torch.long)
    val_x = torch.tensor(val_x, dtype=torch.float32)
    val_y = torch.tensor(val_y, dtype=torch.long)
    x_test = torch.tensor(x_test, dtype=torch.float32)
    y_test = torch.tensor(y_test, dtype=torch.long)

    train_loader = make_loader(train_x, train_y, shuffle=True)
    val_loader = make_loader(val_x, val_y)
    test_loader = make_loader(x_test, y_test)

    print("Train samples:", len(train_x))
    print("Val samples:", len(val_x))
    print("Test samples:", len(x_test))

    model = TinyCNN().to(device)
    print(model)

    criterion = nn.CrossEntropyLoss()
    optimizer = optim.Adam(model.parameters(), lr=LEARNING_RATE)

    best_val = 0.0
    for epoch in range(EPOCHS):
        print("\nEpoch", epoch + 1)
        model.train()

        total_loss = 0.0
        total_acc = 0.0
        count = 0
        pbar = tqdm(train_loader)

        for x, y in pbar:
            x = x.to(device)
            y = y.to(device)

            optimizer.zero_grad()
            out = model(x)
            loss = criterion(out, y)
            loss.backward()
            optimizer.step()

            acc = accuracy(out, y)
            total_loss += loss.item()
            total_acc += acc
            count += 1
            pbar.set_description(f"loss {loss.item():.4f} acc {acc:.4f}")

        print("Train Loss:", total_loss / count)
        print("Train Acc :", total_acc / count)

        model.eval()
        val_acc = 0.0
        count = 0
        with torch.no_grad():
            for x, y in val_loader:
                x = x.to(device)
                y = y.to(device)
                val_acc += accuracy(model(x), y)
                count += 1

        val_acc /= count
        print("Val Acc:", val_acc)

        if val_acc > best_val:
            best_val = val_acc
            torch.save(model.state_dict(), CHECKPOINT_PATH)
            print("Saved best model")

    print("\nTesting...")
    model.eval()
    test_acc = 0.0
    count = 0
    with torch.no_grad():
        for x, y in test_loader:
            x = x.to(device)
            y = y.to(device)
            test_acc += accuracy(model(x), y)
            count += 1

    test_acc /= count
    print("Test Accuracy:", test_acc)


if __name__ == "__main__":
    main()
