import torch
import torch.nn as nn
import numpy as np
import struct

class TinyCNN(nn.Module):
    def __init__(self):
        super().__init__()
        self.conv = nn.Sequential(
            nn.Conv2d(1, 8, 3, padding=1),
            nn.ReLU(),
            nn.MaxPool2d(2),
            nn.Conv2d(8, 16, 3, padding=1),
            nn.ReLU(),
            nn.MaxPool2d(2)
        )
        self.fc = nn.Sequential(
            nn.Linear(16*7*7, 64),
            nn.ReLU(),
            nn.Linear(64, 62)
        )

    def forward(self, x):
        x = x.view(-1, 1, 28, 28)
        x = self.conv(x)
        x = x.view(x.size(0), -1)
        x = self.fc(x)
        return x

def quantize_and_save(model_path, output_path):
    model = TinyCNN()
    model.load_state_dict(torch.load(model_path, map_location="cpu"))
    model.eval()

    layers = [
        model.conv[0], # Conv1
        model.conv[3], # Conv2
        model.fc[0],   # FC1
        model.fc[2]    # FC2
    ]

    with open(output_path, "wb") as f:
        for layer in layers:
            weight = layer.weight.data.numpy()
            bias = layer.bias.data.numpy()

            # Symmetric quantization for weights
            max_val = np.max(np.abs(weight))
            scale = max_val / 127.0 if max_val > 0 else 1.0
            
            # Quantize weights to int8
            weight_q = np.round(weight / scale).astype(np.int8)
            
            # Write weight_q (int8)
            f.write(weight_q.tobytes())
            
            # Write bias (float32)
            f.write(bias.astype(np.float32).tobytes())
            
            # Write scale (float32)
            f.write(np.array([scale], dtype=np.float32).tobytes())

def export_to_header(model_path, header_path):
    model = TinyCNN()
    model.load_state_dict(torch.load(model_path, map_location="cpu"))
    model.eval()

    layers = [
        ("conv1", model.conv[0]),
        ("conv2", model.conv[3]),
        ("fc1", model.fc[0]),
        ("fc2", model.fc[2])
    ]

    with open(header_path, "w") as f:
        f.write("#ifndef MODEL_DATA_H\n#define MODEL_DATA_H\n\n")
        f.write("#include <stdint.h>\n\n")
        
        for name, layer in layers:
            weight = layer.weight.data.numpy()
            bias = layer.bias.data.numpy()

            max_val = np.max(np.abs(weight))
            scale = max_val / 127.0 if max_val > 0 else 1.0
            weight_q = np.round(weight / scale).astype(np.int8)

            # Weight array
            f.write(f"const int8_t {name}_weight[] = {{")
            f.write(", ".join(map(str, weight_q.flatten())))
            f.write("};\n\n")

            # Bias array
            f.write(f"const float {name}_bias[] = {{")
            f.write(", ".join(map(lambda x: f"{x}f", bias.flatten())))
            f.write("};\n\n")

            # Scale
            f.write(f"const float {name}_scale = {scale}f;\n\n")

        f.write("#endif // MODEL_DATA_H\n")

    print(f"Model exported to {header_path}")

if __name__ == "__main__":
    quantize_and_save("mlp_emnist_best.pt", "model_quantized.bin")
    export_to_header("mlp_emnist_best.pt", "model_data.h")
