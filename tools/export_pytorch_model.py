#!/usr/bin/env python3
"""
Export a simple PyTorch CNN/MLP into EdgeGlyph C files.

Supported layer types in spec:
  - conv2d_i8
  - depthwise_conv2d_i8
  - maxpool2d
  - avgpool2d
  - global_avgpool2d
  - batchnorm
  - dense_i8
  - relu
  - leaky_relu
  - sigmoid
  - tanh
  - softmax
  - flatten

Example:
  python3 tools/export_pytorch_model.py \
    --checkpoint model.pt \
    --spec tools/specs/simple_cnn.json \
    --out-dir main/models \
    --model-name simple_cnn
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def load_state_dict(path: Path):
    import torch

    obj = torch.load(path, map_location="cpu")
    if isinstance(obj, dict) and "state_dict" in obj and isinstance(obj["state_dict"], dict):
        return obj["state_dict"]
    if isinstance(obj, dict):
        return obj
    raise ValueError("Unsupported checkpoint format: expected state_dict or {'state_dict': ...}")


def sanitize_name(name: str) -> str:
    return name.replace(".", "_").replace("/", "_").replace("-", "_")


def c_array(name: str, values: list[Any], ctype: str, cols: int = 12) -> str:
    chunks = []
    for i in range(0, len(values), cols):
        row = ", ".join(str(v) for v in values[i : i + cols])
        chunks.append(f"    {row}")
    body = ",\n".join(chunks)
    return f"static const {ctype} {name}[{len(values)}] = {{\n{body}\n}};\n"


def tensor_to_list(t):
    return t.detach().cpu().contiguous().view(-1).tolist()


def quantize_int8(weights):
    vals = weights.detach().cpu().float()
    max_abs = float(vals.abs().max().item())
    scale = max_abs / 127.0 if max_abs > 0 else 1.0
    q = (vals / scale).round().clamp(-127, 127).to(dtype=__import__("torch").int8)
    return q, scale


def infer_conv_out(h, w, kh, kw, sh, sw, ph, pw):
    oh = (h + 2 * ph - kh) // sh + 1
    ow = (w + 2 * pw - kw) // sw + 1
    return oh, ow


def generate_files(state_dict, spec: dict, out_dir: Path, model_name: str):
    out_dir.mkdir(parents=True, exist_ok=True)
    layers = spec["layers"]
    c_model_name = sanitize_name(model_name)
    h_path = out_dir / f"{c_model_name}_model.h"
    c_path = out_dir / f"{c_model_name}_model.c"

    arrays: list[str] = []
    layer_defs: list[str] = []

    input_shape = tuple(spec["input_shape"])
    cur_c, cur_h, cur_w = input_shape
    max_elems = cur_c * cur_h * cur_w

    for i, layer in enumerate(layers):
        ltype = layer["type"]
        lname = sanitize_name(layer.get("name", f"layer_{i}"))

        if ltype == "conv2d_i8":
            weight = state_dict[layer["weight_key"]]
            bias = state_dict[layer["bias_key"]] if layer.get("bias_key") else None
            q, scale = quantize_int8(weight)
            out_c, in_c, kh, kw = weight.shape
            sh, sw = layer.get("stride", [1, 1])
            ph, pw = layer.get("padding", [0, 0])
            oh, ow = infer_conv_out(cur_h, cur_w, kh, kw, sh, sw, ph, pw)
            max_elems = max(max_elems, out_c * oh * ow)

            w_name = f"{lname}_w"
            b_name = f"{lname}_b"
            arrays.append(c_array(w_name, tensor_to_list(q), "int8_t"))
            if bias is not None:
                arrays.append(c_array(b_name, [f"{float(x):.9g}f" for x in tensor_to_list(bias)], "float", cols=8))
                b_ref = b_name
            else:
                b_ref = "NULL"

            layer_defs.append(
                "{" +
                f".type = TC_LAYER_CONV2D_I8, .p.conv2d_i8 = {{"
                f".in_shape = {{{cur_c}, {cur_h}, {cur_w}}}, "
                f".out_shape = {{{out_c}, {oh}, {ow}}}, "
                f".kernel_h = {kh}, .kernel_w = {kw}, .stride_h = {sh}, .stride_w = {sw}, "
                f".pad_h = {ph}, .pad_w = {pw}, .weights = {w_name}, .bias = {b_ref}, .weight_scale = {scale:.9g}f"
                "}}}"
            )
            cur_c, cur_h, cur_w = out_c, oh, ow

        elif ltype == "depthwise_conv2d_i8":
            weight = state_dict[layer["weight_key"]]
            bias = state_dict[layer["bias_key"]] if layer.get("bias_key") else None
            q, scale = quantize_int8(weight)
            c, mult, kh, kw = weight.shape
            if mult != 1:
                raise ValueError("Only depth_multiplier=1 is supported")
            sh, sw = layer.get("stride", [1, 1])
            ph, pw = layer.get("padding", [0, 0])
            oh, ow = infer_conv_out(cur_h, cur_w, kh, kw, sh, sw, ph, pw)
            max_elems = max(max_elems, c * oh * ow)

            w_name = f"{lname}_w"
            b_name = f"{lname}_b"
            arrays.append(c_array(w_name, tensor_to_list(q.view(c, kh, kw)), "int8_t"))
            if bias is not None:
                arrays.append(c_array(b_name, [f"{float(x):.9g}f" for x in tensor_to_list(bias)], "float", cols=8))
                b_ref = b_name
            else:
                b_ref = "NULL"

            layer_defs.append(
                "{" +
                f".type = TC_LAYER_DEPTHWISE_CONV2D_I8, .p.depthwise_conv2d_i8 = {{"
                f".in_shape = {{{cur_c}, {cur_h}, {cur_w}}}, .out_shape = {{{c}, {oh}, {ow}}}, "
                f".kernel_h = {kh}, .kernel_w = {kw}, .stride_h = {sh}, .stride_w = {sw}, "
                f".pad_h = {ph}, .pad_w = {pw}, .weights = {w_name}, .bias = {b_ref}, .weight_scale = {scale:.9g}f"
                "}}}"
            )
            cur_c, cur_h, cur_w = c, oh, ow

        elif ltype in {"maxpool2d", "avgpool2d"}:
            kh, kw = layer.get("kernel", [2, 2])
            sh, sw = layer.get("stride", [kh, kw])
            ph, pw = layer.get("padding", [0, 0])
            oh, ow = infer_conv_out(cur_h, cur_w, kh, kw, sh, sw, ph, pw)
            max_elems = max(max_elems, cur_c * oh * ow)
            layer_enum = "TC_LAYER_MAXPOOL2D" if ltype == "maxpool2d" else "TC_LAYER_AVGPOOL2D"
            layer_defs.append(
                "{" +
                f".type = {layer_enum}, .p.pool2d = {{"
                f".in_shape = {{{cur_c}, {cur_h}, {cur_w}}}, .out_shape = {{{cur_c}, {oh}, {ow}}}, "
                f".kernel_h = {kh}, .kernel_w = {kw}, .stride_h = {sh}, .stride_w = {sw}, .pad_h = {ph}, .pad_w = {pw}"
                "}}}"
            )
            cur_h, cur_w = oh, ow

        elif ltype == "global_avgpool2d":
            layer_defs.append(
                "{" +
                f".type = TC_LAYER_GLOBAL_AVGPOOL2D, .p.pool2d = {{"
                f".in_shape = {{{cur_c}, {cur_h}, {cur_w}}}, .out_shape = {{{cur_c}, 1, 1}}, "
                f".kernel_h = {cur_h}, .kernel_w = {cur_w}, .stride_h = 1, .stride_w = 1, .pad_h = 0, .pad_w = 0"
                "}}}"
            )
            cur_h, cur_w = 1, 1

        elif ltype == "batchnorm":
            gamma = state_dict[layer["gamma_key"]]
            beta = state_dict[layer["beta_key"]]
            mean = state_dict[layer["mean_key"]]
            var = state_dict[layer["var_key"]]
            eps = float(layer.get("epsilon", 1e-5))
            g_name = f"{lname}_gamma"
            b_name = f"{lname}_beta"
            m_name = f"{lname}_mean"
            v_name = f"{lname}_var"
            arrays.append(c_array(g_name, [f"{float(x):.9g}f" for x in tensor_to_list(gamma)], "float", cols=8))
            arrays.append(c_array(b_name, [f"{float(x):.9g}f" for x in tensor_to_list(beta)], "float", cols=8))
            arrays.append(c_array(m_name, [f"{float(x):.9g}f" for x in tensor_to_list(mean)], "float", cols=8))
            arrays.append(c_array(v_name, [f"{float(x):.9g}f" for x in tensor_to_list(var)], "float", cols=8))
            layer_defs.append(
                "{" +
                f".type = TC_LAYER_BATCHNORM, .p.batchnorm = {{.channels = {cur_c}, .gamma = {g_name}, .beta = {b_name}, .mean = {m_name}, .var = {v_name}, .epsilon = {eps:.9g}f}}"
                "}"
            )

        elif ltype == "dense_i8":
            weight = state_dict[layer["weight_key"]]
            bias = state_dict[layer["bias_key"]] if layer.get("bias_key") else None
            q, scale = quantize_int8(weight)
            out_features, in_features = weight.shape
            max_elems = max(max_elems, out_features)
            w_name = f"{lname}_w"
            arrays.append(c_array(w_name, tensor_to_list(q), "int8_t"))
            if bias is not None:
                b_name = f"{lname}_b"
                arrays.append(c_array(b_name, [f"{float(x):.9g}f" for x in tensor_to_list(bias)], "float", cols=8))
                b_ref = b_name
            else:
                b_ref = "NULL"
            layer_defs.append(
                "{" +
                f".type = TC_LAYER_DENSE_I8, .p.dense_i8 = {{.in_features = {in_features}, .out_features = {out_features}, .weights = {w_name}, .bias = {b_ref}, .weight_scale = {scale:.9g}f}}"
                "}"
            )
            cur_c, cur_h, cur_w = out_features, 1, 1

        elif ltype == "relu":
            layer_defs.append("{.type = TC_LAYER_RELU}")
        elif ltype == "leaky_relu":
            alpha = float(layer.get("alpha", 0.1))
            layer_defs.append(f"{{.type = TC_LAYER_LEAKY_RELU, .p.leaky_relu = {{.alpha = {alpha:.9g}f}}}}")
        elif ltype == "sigmoid":
            layer_defs.append("{.type = TC_LAYER_SIGMOID}")
        elif ltype == "tanh":
            layer_defs.append("{.type = TC_LAYER_TANH}")
        elif ltype == "softmax":
            layer_defs.append("{.type = TC_LAYER_SOFTMAX}")
        elif ltype == "flatten":
            out_features = cur_c * cur_h * cur_w
            layer_defs.append(
                "{" +
                f".type = TC_LAYER_FLATTEN, .p.flatten = {{.in_shape = {{{cur_c}, {cur_h}, {cur_w}}}, .out_shape = {{{out_features}, 1, 1}}}}"
                "}"
            )
            cur_c, cur_h, cur_w = out_features, 1, 1
        else:
            raise ValueError(f"Unsupported layer type: {ltype}")

    header = f'''#pragma once

#include "inference/tc_inference.h"

#ifdef __cplusplus
extern "C" {{
#endif

extern const tc_model_t {c_model_name}_model;

#ifdef __cplusplus
}}
#endif
'''

    src = [
        '#include <stdint.h>',
        '#include "inference/tc_inference.h"',
        f'#include "{c_model_name}_model.h"',
        '',
    ]
    src.extend(arrays)
    src.append(f"static const tc_layer_t {c_model_name}_layers[] = {{\n    " + ",\n    ".join(layer_defs) + "\n};\n")
    src.append(
        f"const tc_model_t {c_model_name}_model = {{\n"
        f"    .layers = {c_model_name}_layers,\n"
        f"    .layer_count = {len(layer_defs)},\n"
        f"    .input_shape = {{{input_shape[0]}, {input_shape[1]}, {input_shape[2]}}},\n"
        f"    .output_shape = {{{cur_c}, {cur_h}, {cur_w}}},\n"
        f"    .max_tensor_elems = {max_elems},\n"
        f"}};\n"
    )

    h_path.write_text(header, encoding="utf-8")
    c_path.write_text("\n".join(src), encoding="utf-8")
    print(f"Wrote: {h_path}")
    print(f"Wrote: {c_path}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", required=True, type=Path)
    parser.add_argument("--spec", required=True, type=Path)
    parser.add_argument("--out-dir", required=True, type=Path)
    parser.add_argument("--model-name", required=True)
    args = parser.parse_args()

    state_dict = load_state_dict(args.checkpoint)
    spec = json.loads(args.spec.read_text(encoding="utf-8"))
    generate_files(state_dict, spec, args.out_dir, args.model_name)


if __name__ == "__main__":
    main()
