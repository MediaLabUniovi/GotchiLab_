#!/usr/bin/env python3
"""
GotchiLab_ Build Matrix & Web Deployment Pipeline.

Compiles parameterized firmware targets for different hardware combinations,
merges bootloader, partitions, and application binaries into a single
unified 0x0 offset image, and publishes binaries and metadata to web/.
"""

import os
import sys
import json
import shutil
import subprocess
import datetime
from pathlib import Path

# Paths
REPO_ROOT = Path(__file__).resolve().parent.parent
CODE_DIR = REPO_ROOT / "code"
WEB_DIR = REPO_ROOT / "web"
BIN_DIR = WEB_DIR / "binaries"
DATA_DIR = WEB_DIR / "data"
MANIFEST_PATH = DATA_DIR / "manifest.json"
MANIFEST_COMPAT_PATH = WEB_DIR / "manifest.json"

# Targets definition
VARIANTS = [
    {
        "id": "full",
        "env": "full",
        "filename": "gotchilab_full.bin",
        "name": "GotchiLab Completo (Todos los Sensores)",
        "features": {
            "co2": True,
            "light": True,
            "touch": True,
            "buzzer": True
        },
        "description": "Configuración completa: Sensor óptico NDIR SCD30, fotorresistencia LDR, sensor táctil TTP223 y zumbador piezoeléctrico.",
        "category": "Completo"
    },
    {
        "id": "full_no_touch",
        "env": "full_no_touch",
        "filename": "gotchilab_full_no_touch.bin",
        "name": "GotchiLab Completo Sin Táctil (Auto-Hatch)",
        "features": {
            "co2": True,
            "light": True,
            "touch": False,
            "buzzer": True
        },
        "description": "Sensores ambientales completos (CO2 SCD30, LDR, Zumbador) con eclosión automática sin sensor táctil.",
        "category": "Recomendado"
    },
    {
        "id": "no_co2",
        "env": "no_co2",
        "filename": "gotchilab_no_co2.bin",
        "name": "GotchiLab Sin Sensor de CO2",
        "features": {
            "co2": False,
            "light": True,
            "touch": True,
            "buzzer": True
        },
        "description": "Excluye el driver del sensor SCD30. Mascota interactiva con LDR, TTP223 y Zumbador.",
        "category": "Recomendado"
    },
    {
        "id": "no_co2_silent",
        "env": "no_co2_silent",
        "filename": "gotchilab_no_co2_silent.bin",
        "name": "GotchiLab Silencioso Sin CO2",
        "features": {
            "co2": False,
            "light": True,
            "touch": True,
            "buzzer": False
        },
        "description": "Sin sensor de CO2 ni zumbador piezoeléctrico. Operación silenciosa con LDR y sensor táctil.",
        "category": "Silencioso"
    },
    {
        "id": "no_co2_no_touch",
        "env": "no_co2_no_touch",
        "filename": "gotchilab_no_co2_no_touch.bin",
        "name": "GotchiLab Sin CO2 ni Táctil",
        "features": {
            "co2": False,
            "light": True,
            "touch": False,
            "buzzer": True
        },
        "description": "Sin sensor de CO2 ni táctil. Eclosión automática del huevo por temporizador (auto-hatch).",
        "category": "Estándar"
    },
    {
        "id": "no_co2_no_light",
        "env": "no_co2_no_light",
        "filename": "gotchilab_no_co2_no_light.bin",
        "name": "GotchiLab Sin CO2 ni Luz (Siempre Despierto)",
        "features": {
            "co2": False,
            "light": False,
            "touch": True,
            "buzzer": True
        },
        "description": "Sin sensor de CO2 ni LDR. Mascota despierta de forma continua.",
        "category": "Estándar"
    },
    {
        "id": "minimal",
        "env": "minimal",
        "filename": "gotchilab_minimal.bin",
        "name": "GotchiLab Básico Mínimo (Display + Botón)",
        "features": {
            "co2": False,
            "light": False,
            "touch": False,
            "buzzer": False
        },
        "description": "Montaje básico con hardware esencial: Pantalla OLED SSD1306 y pulsador físico de alimentación.",
        "category": "Mínimo"
    },
    {
        "id": "full_silent",
        "env": "full_silent",
        "filename": "gotchilab_full_silent.bin",
        "name": "GotchiLab Completo Silencioso",
        "features": {
            "co2": True,
            "light": True,
            "touch": True,
            "buzzer": False
        },
        "description": "Sensores ambientales completos (CO2, LDR, TTP223) con zumbador desactivado a nivel de compilación.",
        "category": "Especial"
    }
]

def find_tool(tool_name):
    """Locate tool executable in PATH or standard Python script directories."""
    direct = shutil.which(tool_name)
    if direct:
        return direct

    possible_dirs = [
        Path(sys.executable).parent / "Scripts",
        Path.home() / "AppData" / "Local" / "Programs" / "Python",
        Path.home() / ".platformio" / "penv" / "Scripts",
    ]

    local_pkg_root = Path.home() / "AppData" / "Local" / "Packages"
    if local_pkg_root.exists():
        for p in local_pkg_root.glob("PythonSoftwareFoundation*/**/Scripts"):
            possible_dirs.append(p)

    for p_dir in possible_dirs:
        candidate = p_dir / (tool_name if tool_name.endswith(".exe") else f"{tool_name}.exe")
        if candidate.is_file():
            return str(candidate)
        candidate_py = p_dir / f"{tool_name}.py"
        if candidate_py.is_file():
            return str(candidate_py)

    return None

def get_pio_cmd():
    pio_path = find_tool("pio")
    if pio_path:
        return [pio_path]
    return [sys.executable, "-m", "platformio"]

def get_esptool_cmd():
    esptool_path = find_tool("esptool")
    if esptool_path:
        return [esptool_path]
    return [sys.executable, "-m", "esptool"]

def extract_animations():
    extract_script = REPO_ROOT / "scripts" / "extract_animations.py"
    if extract_script.exists():
        print("[EXTRACT] Extracting OLED animations to web/js/animations.js...")
        res = subprocess.run([sys.executable, str(extract_script)], capture_output=True, text=True)
        if res.returncode == 0:
            print("[EXTRACT] OK: Animations updated.")
        else:
            print(f"[EXTRACT] Warning: {res.stderr}")

def build_variant(variant, pio_cmd, esptool_cmd):
    env_name = variant["env"]
    filename = variant["filename"]
    out_file = BIN_DIR / filename

    print(f"\n=======================================================")
    print(f"[BUILD] Target: {variant['name']} ({env_name})")
    print(f"=======================================================")

    cmd = pio_cmd + ["run", "-e", env_name]
    result = subprocess.run(cmd, cwd=CODE_DIR)
    if result.returncode != 0:
        raise RuntimeError(f"PlatformIO build failed for {env_name} (code {result.returncode})")

    build_dir = CODE_DIR / ".pio" / "build" / env_name
    bootloader = build_dir / "bootloader.bin"
    partitions = build_dir / "partitions.bin"
    firmware = build_dir / "firmware.bin"

    for f_path, label in [(bootloader, "bootloader"), (partitions, "partitions"), (firmware, "firmware")]:
        if not f_path.is_file():
            raise FileNotFoundError(f"Missing {label} binary at: {f_path}")

    print(f"[MERGE] Generating unified 0x0 binary: {out_file.name}")
    merge_cmd = esptool_cmd + [
        "--chip", "esp32",
        "merge_bin",
        "--flash_mode", "dio",
        "--flash_freq", "40m",
        "--flash_size", "4MB",
        "-o", str(out_file),
        "0x1000", str(bootloader),
        "0x8000", str(partitions),
        "0x10000", str(firmware)
    ]
    merge_res = subprocess.run(merge_cmd, capture_output=True, text=True)
    if merge_res.returncode != 0:
        merge_cmd[2] = "merge-bin"
        merge_res = subprocess.run(merge_cmd, capture_output=True, text=True)
        if merge_res.returncode != 0:
            raise RuntimeError(f"esptool merge_bin failed:\n{merge_res.stderr}\n{merge_res.stdout}")

    size_bytes = out_file.stat().st_size
    size_kb = size_bytes / 1024.0
    print(f"[MERGE] Successfully created {out_file.name} ({size_kb:.1f} KB)")

    return {
        "id": variant["id"],
        "name": variant["name"],
        "category": variant["category"],
        "filename": filename,
        "path": f"binaries/{filename}",
        "sizeBytes": size_bytes,
        "sizeFormatted": f"{size_kb:.1f} KB",
        "features": variant["features"],
        "description": variant["description"]
    }

def main():
    print("===============================================================")
    print("   GotchiLab_ Multi-Variant Build & Deployment Pipeline        ")
    print("===============================================================")

    BIN_DIR.mkdir(parents=True, exist_ok=True)
    DATA_DIR.mkdir(parents=True, exist_ok=True)

    pio_cmd = get_pio_cmd()
    esptool_cmd = get_esptool_cmd()

    print(f"PlatformIO command: {' '.join(pio_cmd)}")
    print(f"esptool command:    {' '.join(esptool_cmd)}")

    # Update OLED animations
    extract_animations()

    built_targets = []
    build_timestamp = datetime.datetime.now(datetime.timezone.utc).isoformat()

    for variant in VARIANTS:
        try:
            info = build_variant(variant, pio_cmd, esptool_cmd)
            built_targets.append(info)
        except Exception as e:
            print(f"[ERROR] Target {variant['id']} failed: {e}")
            raise

    # Write manifest.json
    manifest = {
        "project": "GotchiLab_",
        "targetChip": "ESP32",
        "flashOffset": "0x00000000",
        "flashSize": "4MB",
        "flashMode": "dio",
        "flashFreq": "40m",
        "buildTimestamp": build_timestamp,
        "totalVariants": len(built_targets),
        "variants": built_targets,
        "defaults": {
            "co2": True,
            "light": True,
            "touch": True,
            "buzzer": True
        }
    }

    for p in [MANIFEST_PATH, MANIFEST_COMPAT_PATH]:
        with open(p, "w", encoding="utf-8") as f:
            json.dump(manifest, f, indent=2, ensure_ascii=False)

    print("\n===============================================================")
    print(f"[SUCCESS] All {len(built_targets)} variants compiled and unified!")
    print(f"[DEPLOY]  Binaries written to: {BIN_DIR}")
    print(f"[DEPLOY]  Manifest written to: {MANIFEST_PATH}")
    print("===============================================================\n")

if __name__ == "__main__":
    main()
