
import cv2
import numpy as np
import argparse
import os
import re

DEFAULT_OFFSETS = [0, -1, -1, -2, -2, -1, -1, 0, 1, 1, 2, 1, 1, 0, 0]

def sanitize_name(name: str) -> str:
    name = os.path.splitext(os.path.basename(name))[0]
    name = re.sub(r'[^a-zA-Z0-9_]', '_', name)
    name = re.sub(r'_+', '_', name).strip('_')
    if not name:
        name = "video"
    if name[0].isdigit():
        name = "_" + name
    return name.lower()

def extract_frames(video_path, frame_count):
    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        raise RuntimeError(f"No se pudo abrir el video: {video_path}")

    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    if total_frames <= 0:
        cap.release()
        raise RuntimeError("No se pudieron leer frames del video.")

    indices = np.linspace(0, total_frames - 1, frame_count, dtype=int)
    frames = []

    for idx in indices:
        cap.set(cv2.CAP_PROP_POS_FRAMES, int(idx))
        ret, frame = cap.read()
        if ret and frame is not None:
            frames.append(frame)

    cap.release()

    if not frames:
        raise RuntimeError("No se pudo extraer ningun frame del video.")

    return frames

def process_frame(frame, width, height):
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    resized = cv2.resize(gray, (width, height), interpolation=cv2.INTER_AREA)
    return resized

def pack_mono_vertical(gray_frame, threshold=128):
    """
    Convierte un frame en escala de grises a 1 bit por pixel
    empaquetado verticalmente (formato típico SSD1306)
    """
    height, width = gray_frame.shape

    if height % 8 != 0:
        raise ValueError("La altura debe ser multiplo de 8")

    # INVERTIDO (negro = 1, blanco = 0)
    binary = (gray_frame < threshold).astype(np.uint8)

    pages = height // 8
    packed = []

    for page in range(pages):
        y_base = page * 8

        for x in range(width):
            value = 0

            for bit in range(8):
                if binary[y_base + bit, x]:
                    value |= (1 << bit)

            packed.append(value)

    return packed

def format_c_byte_array(byte_list, indent="    ", per_line=16):
    lines = []

    for i in range(0, len(byte_list), per_line):
        chunk = byte_list[i:i + per_line]
        lines.append(indent + ", ".join(f"0x{b:02X}" for b in chunk))

    return ",\n".join(lines)

def write_header_file(header_path, macro_name, array_name, frames, width, height, fps, seconds, frame_size, offsets_name):

    with open(header_path, "w", encoding="utf-8") as f:

        f.write("#pragma once\n")
        f.write("#include <stdint.h>\n\n")

        f.write(f"#define {macro_name}_WIDTH {width}\n")
        f.write(f"#define {macro_name}_HEIGHT {height}\n")
        f.write(f"#define {macro_name}_FPS {fps}\n")
        f.write(f"#define {macro_name}_SECONDS {seconds}\n")
        f.write(f"#define {macro_name}_FRAMES {frames}\n")
        f.write(f"#define {macro_name}_FRAME_SIZE {frame_size}\n\n")

        f.write(f"extern const uint8_t {array_name}[{macro_name}_FRAMES][{macro_name}_FRAME_SIZE];\n")
        f.write(f"extern const int {offsets_name}[{macro_name}_FRAMES];\n")

def write_source_file(source_path, header_filename, macro_name, array_name, packed_frames, offsets_name, offsets):

    with open(source_path, "w", encoding="utf-8") as f:

        f.write(f'#include "{header_filename}"\n\n')

        f.write(f"const uint8_t {array_name}[{macro_name}_FRAMES][{macro_name}_FRAME_SIZE] = {{\n")

        for i, frame_bytes in enumerate(packed_frames):

            f.write(f"    // frame {i+1}\n")
            f.write("    {\n")

            f.write(format_c_byte_array(frame_bytes, indent="        "))

            f.write("\n    }")

            if i < len(packed_frames) - 1:
                f.write(",")

            f.write("\n")

        f.write("};\n\n")

        f.write(f"const int {offsets_name}[{macro_name}_FRAMES] = {{ ")
        f.write(", ".join(str(v) for v in offsets))
        f.write(" };\n")

def main():

    parser = argparse.ArgumentParser(description="Convertir MP4 a animacion C (.h + .c) 128x64 1bit")

    parser.add_argument("input", help="Video mp4 de entrada")
    parser.add_argument("--width", type=int, default=128)
    parser.add_argument("--height", type=int, default=64)
    parser.add_argument("--fps", type=int, default=5)
    parser.add_argument("--seconds", type=int, default=3)
    parser.add_argument("--frames", type=int, default=15)
    parser.add_argument("--threshold", type=int, default=128)
    parser.add_argument("--base-name", default=None)
    parser.add_argument("--prefix", default="penguin")
    parser.add_argument("--offsets", default=None)

    args = parser.parse_args()

    source_video_name = sanitize_name(args.input)
    base_name = sanitize_name(args.base_name) if args.base_name else source_video_name
    prefix_name = sanitize_name(args.prefix)

    macro_name = f"{prefix_name}_{base_name}".upper()
    array_name = f"{prefix_name}_{base_name}_anim"
    offsets_name = f"{prefix_name}_{base_name}_offsets"

    header_filename = f"{prefix_name}_{base_name}_anim.h"
    source_filename = f"{prefix_name}_{base_name}_anim.c"

    if args.offsets:
        offsets = [int(x.strip()) for x in args.offsets.split(",")]
    else:
        offsets = DEFAULT_OFFSETS

    raw_frames = extract_frames(args.input, args.frames)

    processed = [process_frame(f, args.width, args.height) for f in raw_frames]

    packed_frames = [pack_mono_vertical(f, args.threshold) for f in processed]

    frame_size = args.width * args.height // 8

    write_header_file(
        header_filename,
        macro_name,
        array_name,
        args.frames,
        args.width,
        args.height,
        args.fps,
        args.seconds,
        frame_size,
        offsets_name
    )

    write_source_file(
        source_filename,
        header_filename,
        macro_name,
        array_name,
        packed_frames,
        offsets_name,
        offsets
    )

    print("Archivos generados:")
    print(header_filename)
    print(source_filename)

if __name__ == "__main__":
    main()
