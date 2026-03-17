# EdgeGlyph

EdgeGlyph is a handwritten character recognition project that runs a quantized CNN directly on an ESP32-C3, fully on-device, without TensorFlow Lite or any external ML runtime.

The project combines:

- EMNIST training in PyTorch
- INT8 weight quantization
- C header export for deployment
- A custom ESP32-C3 inference pipeline
- Custom display and touch drivers
- A lightweight UI for on-device interaction

## What It Does

The current demo recognizes handwritten characters drawn on a round touchscreen display.

Pipeline:

`touch input -> preprocessing -> CNN inference -> display output`

Everything runs locally on the microcontroller.

## Hardware / Software Stack

- ESP32-C3 (RISC-V)
- ESP-IDF / FreeRTOS-based firmware environment
- Custom display driver
- Custom touch driver
- Handwritten 2-layer CNN inference in C
- INT8 quantized weights exported from PyTorch

## Model

The training pipeline uses the EMNIST ByClass dataset and a small CNN:

- `Conv2d(1 -> 8, 3x3) + ReLU + MaxPool`
- `Conv2d(8 -> 16, 3x3) + ReLU + MaxPool`
- `Linear(16 * 7 * 7 -> 64) + ReLU`
- `Linear(64 -> 62)`

The model predicts 62 classes:

- `0-9`
- `A-Z`
- `a-z`

## Training

The Kaggle training script lives at [training/train_emnist.py](training/train_emnist.py).

Key details:

- Dataset: EMNIST ByClass CSV export
- Optimizer: Adam
- Learning rate: `5e-4`
- Batch size: `256`
- Epochs: `10`
- Validation split: `10%`

The best checkpoint is saved as `mlp_emnist_best.pt`.

## Quantization And Export

1. Train the PyTorch model.
2. Quantize weights to INT8 with [quantize.py](/Users/tensorcraft/Projects/TensorCore/quantize.py).
3. Export model arrays for firmware integration.

Example:

```bash
python3 quantize.py
```

There is also a more general exporter at [tools/export_pytorch_model.py](tools/export_pytorch_model.py).

## Firmware

The embedded app lives in [idf_inference](idf_inference).

Useful commands:

```bash
cd idf_inference
make build
make flash PORT=/dev/cu.usbmodemXXXX
make monitor PORT=/dev/cu.usbmodemXXXX
```

The `Makefile` uses `IDF_PATH` when available, and otherwise falls back to `$HOME/esp/esp-idf/export.sh`.

## Repo Layout

```text
training/            Kaggle/PyTorch training script
quantize.py          INT8 quantization and header export
predict.py           Desktop inference playground
tools/               Export utilities and model specs
idf_inference/       ESP-IDF firmware project
```

## Why This Project Exists

This project is part of a broader effort to understand and rebuild the inference stack end-to-end:

- model training
- quantization
- model export
- embedded inference
- hardware interaction
- runtime integration

The longer-term goal is to push this beyond FreeRTOS and integrate the ML pipeline into a custom OS for tighter control over scheduling, memory, and execution.
