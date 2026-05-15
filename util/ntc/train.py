import torch
from torch import nn
import copy
import time
import torchvision
from torch.utils.data import DataLoader, TensorDataset
import matplotlib.pyplot as plt
import io
import argparse
import os
from datetime import timedelta
import math
import matplotlib.ticker as ticker
from PIL import Image
import torchvision.transforms.functional as TF
import numpy as np
from enum import Enum
import random
import struct


class QUALITY(Enum):
    BPPC_0_2    = 0
    BPPC_0_5    = 1
    BPPC_1_0    = 2
    BPPC_2_25   = 3

parser = argparse.ArgumentParser()
parser.add_argument('--out', type=str, required=True, help="Output .pt file path")
parser.add_argument('--textures', type=str, nargs='+', required=True, help="List of texture filepaths")
parser.add_argument('--channels', type=int, nargs='+', required=True, help="Number of channels per texture (1 or 3)")
parser.add_argument('--weights', type=float, nargs='+', required=True, help="Loss weights per output channel")
parser.add_argument('--quality', type=int, default=2, help="Quality Enum Value (0-3) \\{ 0 = 0.2BPPC; 1 = 0.5BPPC; 2 = 1.0BPPC; 3 = 2.25BPPC \\}")
parser.add_argument('--hidden_neurons', type=int, default=32, help="Number of hidden neurons per layer in MLP")
parser.add_argument('--epochs', type=int, default=1000, help="Number of training epochs")
parser.add_argument('--flip', action='store_true', help="Flip textures vertically on load")
args = parser.parse_args()


batch_size = 262144
accelerator = torch.accelerator.current_accelerator()
device = accelerator.type if accelerator is not None else "cpu"


def compute_psnr(pred: torch.Tensor, target: torch.Tensor, max_val: float = 1.0):
    mse = torch.mean((pred - target) ** 2)
    if mse.item() == 0:
        return float('inf')
    psnr = 10.0 * torch.log10(torch.tensor(max_val * max_val, dtype=mse.dtype) / mse)
    return psnr.item()


def load_texture(path, grayscale=False, flip=False):
    img = Image.open(path)

    if flip:
        img = img.transpose(Image.FLIP_TOP_BOTTOM)

    if grayscale:
        if img.mode in ("I", "I;16", "I;16B"):
            arr = np.array(img, dtype=np.float32) / 65535.0
            return torch.from_numpy(arr).unsqueeze(0).to(device)  # [1, H, W]
        img = img.convert("L")
    else:
        img = img.convert("RGB")

    return TF.to_tensor(img).to(device)


loaded_textures = []
for path, ch in zip(args.textures, args.channels):
    tex = load_texture(path, grayscale=(ch == 1), flip=args.flip)
    loaded_textures.append(tex)

#Find max resolution and force all textures to match and be square
max_res = max([t.size(1) for t in loaded_textures])
def force_square_res(tex, res):
    if tex.size(1) != res or tex.size(2) != res:
        return torch.nn.functional.interpolate(tex.unsqueeze(0), size=(res, res), mode='bilinear', align_corners=False, antialias=True).squeeze(0)
    return tex
loaded_textures = [force_square_res(t, max_res) for t in loaded_textures]

#Concatenate all textures blindly
current_image = torch.cat(loaded_textures, dim=0)
output_channels = current_image.size(0)

current_image_res = current_image.size(1)
current_image.unsqueeze_(0) #Prepare for first interpolation
base_image_res = current_image_res #Required for latent pyramid setup
num_mips = int(math.log2(base_image_res) - 1) #Go down to 4x4
output_chain = current_image.squeeze(0).permute(1,2,0).reshape(-1,output_channels)

xs = torch.linspace(-1.0, 1.0, current_image_res)
ys = torch.linspace(-1.0, 1.0, current_image_res)
x,y = torch.meshgrid(xs, ys, indexing='xy')
current_uvs = torch.stack([x,y], dim=2).to(device)
current_uvs = current_uvs.reshape(-1,2)
# lod_tensor = torch.full([current_uvs.size(0)], 0.0, dtype=torch.float32).to(device).unsqueeze(1)
# all_inputs = torch.cat([current_uvs, lod_tensor], dim=1)
# input_chain = all_inputs
input_chain = current_uvs
input_chain_lookup = [(0, current_uvs.size(0))] #Stores the offset and size for any given mip level (index by the desired mip)

for mip in range(1, num_mips):
    current_image_res //= 2
    current_image = torch.nn.functional.interpolate(current_image.cpu(), size=(current_image_res, current_image_res), mode='bilinear', align_corners=False, antialias=True).to(device)
    output_chain = torch.cat([output_chain, current_image.squeeze(0).permute(1,2,0).reshape(-1,output_channels)], dim=0)

    xs = torch.linspace(-1.0, 1.0, current_image_res)
    ys = torch.linspace(-1.0, 1.0, current_image_res)
    x,y = torch.meshgrid(xs, ys, indexing='xy')
    current_uvs = torch.stack([x,y], dim=2).to(device).reshape(-1,2)
    input_chain = torch.cat([input_chain, current_uvs], dim=0)

    prev_input_chain_lookup = input_chain_lookup[-1]
    current_offset = prev_input_chain_lookup[0] + prev_input_chain_lookup[1]
    input_chain_lookup.append((current_offset, current_uvs.size(0)))

input_chain = input_chain.cpu()
output_chain = output_chain.cpu()

training_dataset = TensorDataset(input_chain, output_chain)

#NTC relies on overfitting, no need for an eval dataset


class HardGELU(nn.Module):
    def forward(self, x: torch.Tensor) -> torch.Tensor:
        return torch.where(
            x < -1.5, torch.zeros_like(x), torch.where(
                x > 1.5, x, (x / 3.0) * (x + 1.5)
            )
        )


def pack_4bit(tensor: torch.Tensor) -> torch.Tensor:
    #Packs a tensor of integers in range [0, 15] into half the bytes
    t = tensor.to(torch.uint8).flatten()
    if t.numel() % 2 != 0:
        t = torch.nn.functional.pad(t, (0, 1))
    t = t.view(-1, 2)
    packed = (t[:,0]*16 + t[:,1])
    return packed.to(torch.uint8)

def unpack_4bit(packed: torch.Tensor, original_shape: torch.Size) -> torch.Tensor:
    val1 = packed // 16  #Get upper 4 bits
    val2 = packed % 16   #Get lower 4 bits

    unpacked = torch.stack([val1, val2], dim=-1).flatten()
    #Truncate any padding we added during packing
    unpacked = unpacked[:original_shape.numel()]
    return unpacked.view(original_shape)

def pack_2bit(tensor: torch.Tensor) -> torch.Tensor:
    t = tensor.to(torch.uint8).flatten()
    #Ensure length is a multiple of 4
    if t.numel() % 4 != 0:
        t = torch.nn.functional.pad(t, (0, 4 - (t.numel() % 4)))

    t = t.view(-1, 4)
    #Shift into positions: 64, 16, 4, 1
    packed = (t[:, 0] * 64) + (t[:, 1] * 16) + (t[:, 2] * 4) + t[:, 3]
    return packed.to(torch.uint8)

def unpack_2bit(packed: torch.Tensor, original_shape: torch.Size) -> torch.Tensor:
    val1 = packed // 64
    val2 = (packed // 16) % 4
    val3 = (packed // 4) % 4
    val4 = packed % 4

    unpacked = torch.stack([val1, val2, val3, val4], dim=-1).flatten()
    unpacked = unpacked[:original_shape.numel()]
    return unpacked.view(original_shape)


#Index with mip to get corresponding feature level
FeatureLevelLookup = [0,0,0,0,1,1,2,2,3,3,3]

#Key with quality level to get array that can be indexed with feature level to get corresponding g0 grid resolution (to be scaled by image_res)
G0ResolutionLookup = {
    QUALITY.BPPC_0_2:  [1/4, 1/16, 1/64, 1/256],
    QUALITY.BPPC_0_5:  [1/4, 1/16, 1/64, 1/256],
    QUALITY.BPPC_1_0:  [1/2,  1/8, 1/32, 1/128],
    QUALITY.BPPC_2_25: [1/2,  1/8, 1/32, 1/128],
}

#Key with quality level to get num latents and quant levels for g0 and g1 (same for all feature levels)
LatentAndQuantLookup = {
    QUALITY.BPPC_0_2: ((8, 4), (12, 16)),
    QUALITY.BPPC_0_5: ((12, 16), (20, 16)),
    QUALITY.BPPC_1_0: ((12, 4), (10, 16)),
    QUALITY.BPPC_2_25: ((16, 16), (12, 16)),
}


class NTCNetwork(nn.Module):
    def __init__(self, image_res: int, quality: QUALITY, hidden_neurons: int, output_channels: int, quantise_latents_during_training: bool):
        super().__init__()

        #Latent pyramid setup
        self.g0_resolutions = G0ResolutionLookup[quality]
        g0_latent_quant, g1_latent_quant = LatentAndQuantLookup[quality]
        self.g0_channels, self.g0_quant_levels = g0_latent_quant
        self.g1_channels, self.g1_quant_levels = g1_latent_quant

        g0_textures = [] #indexed by feature level
        g1_textures = [] #indexed by feature level
        for i in range(4): #4 feature levels
            g0_res = int(self.g0_resolutions[i] * image_res)
            g1_res = g0_res // 2
            g0_textures.append(torch.rand(1, g0_res, g0_res, self.g0_channels))
            g1_textures.append(torch.rand(1, g1_res, g1_res, self.g1_channels))
        self.g0_textures = nn.ParameterList(g0_textures)
        self.g1_textures = nn.ParameterList(g1_textures)
        self.quantise_latents_during_training = quantise_latents_during_training
        self.latents_frozen = False


        #Determine number of octaves for frequency encoding
        #This is used to represent frequencies higher than the Nyquist limit of the latent texture
        #Therefore, the number of octaves is log_2(k) where k is the upsampling factor from the lowest res (g1) latent texture to the reference texture
        lowest_res_latent_tex = self.g0_resolutions[3] * image_res * 0.5
        self.tile_size = math.ceil(image_res / lowest_res_latent_tex) #k
        self.num_octaves = math.ceil(math.log2(self.tile_size))
        self.image_res = image_res

        #Precompute freq encoding scales
        self.register_buffer("scales", torch.tensor([2**i * math.pi for i in range(self.num_octaves)], dtype=torch.float32))


        #MLP
        self.linearReluStack = nn.Sequential(
            nn.Linear(4 * self.num_octaves + 1 + 4 * self.g0_channels + self.g1_channels, hidden_neurons),
            HardGELU(),
            nn.Linear(hidden_neurons, hidden_neurons),
            HardGELU(),
            nn.Linear(hidden_neurons, output_channels),
        )


    def forward(self, x, mip):
        x = self.positional_encode(x, mip)
        x = self.linearReluStack(x)
        return x


    #Input coords shape: [N, 2]
    #Output coords shape: [N, 4 * num_octaves + 1 + g0_channels + g1_channels]
    def positional_encode(self, coords, mip):
        normalised_lod = mip / (num_mips - 1)
        lod_feature = torch.full((coords.shape[0], 1), normalised_lod, device=coords.device, dtype=coords.dtype)

        #Frequency encoding
        px = ((coords[:,0] + 1) * 0.5) * self.image_res #[-1,1] -> [0, image_res)
        py = ((coords[:,1] + 1) * 0.5) * self.image_res #[-1,1] -> [0, image_res)
        local_x = torch.remainder(px, self.tile_size) / self.tile_size
        local_y = torch.remainder(py, self.tile_size) / self.tile_size
        local_coords = torch.stack([local_x, local_y], dim=1) * 2.0 - 1.0

        encodings = []
        for scale in self.scales:
            encodings.append(torch.sin(scale * local_coords))
            encodings.append(torch.cos(scale * local_coords))
        encodings = torch.cat(encodings, dim=1)


        #Latents
        feature_level = FeatureLevelLookup[mip]
        g0, g1 = self.get_latents_for_sampling(feature_level)

        #G0: learned interpolation
        W = H = self.g0_resolutions[feature_level] * self.image_res
        C = self.g0_channels
        px = (coords[:,0] * 0.5 + 0.5) * (W-1) #[0, W)
        py = (coords[:,1] * 0.5 + 0.5) * (H-1) #[0, H)
        x0 = torch.floor(px).clamp(min=0, max=W-1).to(torch.int64)
        x1 = (x0+1).clamp(min=0, max=W-1).to(torch.int64)
        y0 = torch.floor(py).clamp(min=0, max=H-1).to(torch.int64)
        y1 = (y0+1).clamp(min=0, max=H-1).to(torch.int64)
        s00 = g0[0, y0, x0, :]
        s01 = g0[0, y1, x0, :]
        s10 = g0[0, y0, x1, :]
        s11 = g0[0, y1, x1, :]
        g0_output = torch.cat([s00, s01, s10, s11], dim=-1) #[N,4*C]

        g1_res = g1.shape[1]
        g1_px = (coords[:, 0] * 0.5 + 0.5) * (g1_res - 1)
        g1_py = (coords[:, 1] * 0.5 + 0.5) * (g1_res - 1)

        g1_x0 = torch.floor(g1_px).clamp(0, g1_res - 1).to(torch.int64)
        g1_x1 = (g1_x0 + 1).clamp(0, g1_res - 1)
        g1_y0 = torch.floor(g1_py).clamp(0, g1_res - 1).to(torch.int64)
        g1_y1 = (g1_y0 + 1).clamp(0, g1_res - 1)

        wx = (g1_px - g1_x0.float()).unsqueeze(1)  #[N, 1]
        wy = (g1_py - g1_y0.float()).unsqueeze(1)  #[N, 1]

        s00 = g1[0, g1_y0, g1_x0, :]    #[N, C]
        s01 = g1[0, g1_y1, g1_x0, :]
        s10 = g1[0, g1_y0, g1_x1, :]
        s11 = g1[0, g1_y1, g1_x1, :]

        g1_output = (1 - wx) * (1 - wy) * s00 + \
                    (1 - wx) * wy * s01 + \
                    wx * (1 - wy) * s10 + \
                    wx * wy * s11  #[N, C]


        encodings = torch.cat((encodings, lod_feature, g0_output, g1_output), dim=1)
        return encodings


    def clamp_latents_(self):
        with torch.no_grad():
            for g0 in self.g0_textures:
                g0.data.clamp_(0.0, 1.0)
            for g1 in self.g1_textures:
                g1.data.clamp_(0.0, 1.0)

    def fake_quantise_latent(self, latent: torch.Tensor, quant_levels: int) -> torch.Tensor:
        noise = torch.empty_like(latent).uniform_(-0.5 / (quant_levels - 1), 0.5 / (quant_levels - 1))
        return latent + noise

    def hard_quantise_latents_(self):
        with torch.no_grad():
            self.clamp_latents_()
            n0 = self.g0_quant_levels - 1
            for g0 in self.g0_textures:
                g0.data.copy_(torch.round(g0.data * n0) / n0)
            n1 = self.g1_quant_levels - 1
            for g1 in self.g1_textures:
                g1.data.copy_(torch.round(g1.data * n1) / n1)

    def freeze_latents_(self):
        for g0 in self.g0_textures:
            g0.requires_grad_(False)
        for g1 in self.g1_textures:
            g1.requires_grad_(False)
        self.latents_frozen = True

    def get_latents_for_sampling(self, feature_level: int) -> torch.Tensor:
        if self.latents_frozen:
            return (self.g0_textures[feature_level], self.g1_textures[feature_level])

        if self.training and self.quantise_latents_during_training:
            g0_q = self.fake_quantise_latent(self.g0_textures[feature_level], self.g0_quant_levels)
            g1_q = self.fake_quantise_latent(self.g1_textures[feature_level], self.g1_quant_levels)
        else:
            n0 = self.g0_quant_levels - 1
            n1 = self.g1_quant_levels - 1
            g0_q = torch.round(self.g0_textures[feature_level] * n0) / n0
            g1_q = torch.round(self.g1_textures[feature_level] * n1) / n1

        return (g0_q, g1_q)

quality = QUALITY(args.quality)
hidden_neurons = args.hidden_neurons
epochs = args.epochs
channel_weights = torch.tensor(args.weights, device=device, dtype=torch.float32)

model = NTCNetwork(base_image_res, quality, hidden_neurons, output_channels, True).to(device)
model = torch.compile(model)
loss_fn = nn.MSELoss()
optimiser = torch.optim.Adam([
    {"params": [g0 for g0 in model.g0_textures], "lr": 0.01},
    {"params": [g1 for g1 in model.g1_textures], "lr": 0.01},
    {"params": model.linearReluStack.parameters(), "lr": 0.005}
])
hard_quant_epoch = int(epochs * 0.95)
scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(optimiser, T_max=epochs)
scaler = torch.amp.GradScaler(device)

start_time = time.time()

avg_losses = []

steps_per_epoch = input_chain_lookup[0][1] // batch_size
for epoch in range(epochs):
    model.train()
    avg_loss = 0

    for step in range(steps_per_epoch):
        #Determine mip level
        if random.uniform(0, 1) <= 0.05:
            #5% chance of uniform distribution
            mip = random.randint(0, num_mips - 1)
        else:
            #95% chance of exponential distribution
            X = random.uniform(1e-10, 1)
            mip = math.floor(-math.log(X, 4))
            mip = min(mip, num_mips - 1)

        offset, size = input_chain_lookup[mip]
        idx = torch.randint(offset, offset + size, (batch_size,), device=input_chain.device)
        x = input_chain[idx].to(device)
        y = output_chain[idx].to(device)
        optimiser.zero_grad()

        with torch.autocast(device_type=device, dtype=torch.bfloat16):
            pred = model(x, mip)
            loss = loss_fn(pred * channel_weights, y * channel_weights)

        loss_val = loss.item()
        avg_loss += loss_val

        scaler.scale(loss).backward()
        scaler.step(optimiser)
        scaler.update()

        model.clamp_latents_()

    scheduler.step()
    avg_loss /= steps_per_epoch
    avg_losses.append(avg_loss)

    #Calculate ETA
    elapsedTime = time.time() - start_time
    avgTimePerEpoch = elapsedTime / (epoch+1)
    totalETA = avgTimePerEpoch * epochs

    print(f"({epoch+1}/{epochs}) Loss: {avg_loss}\t\t(ETA: {timedelta(seconds=int(elapsedTime))} / {timedelta(seconds=int(totalETA))} - Time Remaining: {timedelta(seconds=int(totalETA - elapsedTime))})")

    if epoch + 1 == hard_quant_epoch:
        print(f"Hard-quantising latent texture and freezing it...")
        model.hard_quantise_latents_()
        model.freeze_latents_()

        #Rebuild the optimiser so it only updates decoder weights
        optimiser = torch.optim.Adam([
            {"params": model.linearReluStack.parameters(), "lr": 0.0005}
        ])
        scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(optimiser, T_max = epochs - hard_quant_epoch)


def save_ntc_binary(model, filepath, quality, base_image_res, hidden_neurons):
    with open(filepath, 'wb') as f:
        #Header
        f.write(b'NTC\x00')
        f.write(struct.pack('<I', 1)) #version
        f.write(struct.pack('<I', base_image_res))
        f.write(struct.pack('<I', num_mips))
        f.write(struct.pack('<I', quality.value))
        f.write(struct.pack('<I', hidden_neurons))
        f.write(struct.pack('<I', model.g0_channels))
        f.write(struct.pack('<I', model.g1_channels))
        f.write(struct.pack('<I', model.g0_quant_levels))
        f.write(struct.pack('<I', model.g1_quant_levels))
        f.write(struct.pack('<I', len([l for l in model.linearReluStack if isinstance(l, nn.Linear)]))) #Number of linear layers in mlp
        f.write(struct.pack('<I', model.num_octaves))
        f.write(struct.pack('<I', model.tile_size))

        #Latent textures
        for fl in range(4): #4 feature levels
            g0 = model.g0_textures[fl].detach().cpu().clamp(0.0, 1.0)
            g1 = model.g1_textures[fl].detach().cpu().clamp(0.0, 1.0)

            #G0
            g0_res = g0.shape[1]
            g0_int = torch.round(g0 * (model.g0_quant_levels - 1)).to(torch.uint8)
            g0_numel = g0_int.numel() #store original element count before packing (in case of padding)
            if model.g0_quant_levels <= 4:
                g0_packed = pack_2bit(g0_int)
            else:
                g0_packed = pack_4bit(g0_int)
            f.write(struct.pack('<I', g0_res))
            f.write(struct.pack('<I', g0_numel))
            f.write(struct.pack('<I', g0_packed.numel()))
            f.write(g0_packed.numpy().tobytes())

            #G1
            g1_res = g1.shape[1]
            g1_int = torch.round(g1 * (model.g1_quant_levels - 1)).to(torch.uint8)
            g1_numel = g1_int.numel() #store original element count before packing (in case of padding)
            if model.g1_quant_levels <= 4:
                g1_packed = pack_2bit(g1_int)
            else:
                g1_packed = pack_4bit(g1_int)
            f.write(struct.pack('<I', g1_res))
            f.write(struct.pack('<I', g1_numel))
            f.write(struct.pack('<I', g1_packed.numel()))
            f.write(g1_packed.numpy().tobytes())

        #MLP
        for module in model.linearReluStack:
            if isinstance(module, nn.Linear):
                f.write(struct.pack('<I', module.in_features))
                f.write(struct.pack('<I', module.out_features))
                w = module.weight.detach().cpu().half().numpy()
                b = module.bias.detach().cpu().half().numpy()
                f.write(w.tobytes())
                f.write(b.tobytes())

with torch.no_grad():
    save_ntc_binary(model, args.out, quality, base_image_res, hidden_neurons)