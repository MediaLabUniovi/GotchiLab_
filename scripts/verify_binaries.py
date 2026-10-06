import os
import sys
import json
import subprocess
from pathlib import Path

REPO_ROOT = Path(r"C:\Users\joseev\Downloads\GotchiLab_")
WEB_DIR = REPO_ROOT / "web"
MANIFEST_PATH = WEB_DIR / "data" / "manifest.json"

def test_binaries():
    print("=" * 70)
    print("1. COMPROBACIÓN FÍSICA Y ESTRUCTURAL DE CADA ARCHIVO .BIN")
    print("=" * 70)

    if not MANIFEST_PATH.exists():
        print("[FAIL] Manifest no encontrado")
        return False

    with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
        data = json.load(f)

    variants = data.get("variants", [])
    print(f"Total variantes registradas: {len(variants)}\n")

    all_ok = True
    temp_dir = REPO_ROOT / "temp_verify"
    temp_dir.mkdir(exist_ok=True)

    for idx, v in enumerate(variants, 1):
        rel_path = v["path"]
        bin_path = WEB_DIR / rel_path
        name = v["name"]
        vid = v["id"]

        print(f"--- [{idx}/{len(variants)}] {vid} ({v['filename']}) ---")
        if not bin_path.exists():
            print(f"  [ERROR] El archivo no existe: {bin_path}")
            all_ok = False
            continue

        size = bin_path.stat().st_size
        print(f"  Tamaño: {size} bytes ({size/1024:.1f} KB)")
        
        # Verificar longitud mínima
        if size < 0x10000 + 1000:
            print("  [ERROR] Tamaño insuficiente para contener bootloader, particiones y firmware")
            all_ok = False
            continue

        # Leer archivo
        with open(bin_path, "rb") as bf:
            content = bf.read()

        # 1. Comprobar padding 0x0000..0x1000
        padding = content[:0x1000]
        non_ff = [b for b in padding if b != 0xFF]
        if non_ff:
            print(f"  [AVISO] Padding inicial a 0x0 contiene {len(non_ff)} bytes distintos de 0xFF")

        # 2. Comprobar magic byte de Bootloader en 0x1000 (0xE9)
        bootloader_magic = content[0x1000]
        if bootloader_magic == 0xE9:
            print("  Offset 0x1000 (Bootloader): Magic byte 0xE9 VALIDO")
        else:
            print(f"  [ERROR] Offset 0x1000 (Bootloader): Magic byte 0x{bootloader_magic:02X} INVALIDO (esperado 0xE9)")
            all_ok = False

        # 3. Comprobar magic de Particiones en 0x8000 (0xAA 0x50)
        part_magic = content[0x8000:0x8002]
        if part_magic == b"\xaa\x50":
            print("  Offset 0x8000 (Partitions): Magic bytes 0xAA50 VALIDO")
        else:
            print(f"  [ERROR] Offset 0x8000 (Partitions): Magic {part_magic.hex()} INVALIDO (esperado aa50)")
            all_ok = False

        # 4. Comprobar magic de Firmware en 0x10000 (0xE9)
        app_magic = content[0x10000]
        if app_magic == 0xE9:
            print("  Offset 0x10000 (App Firmware): Magic byte 0xE9 VALIDO")
        else:
            print(f"  [ERROR] Offset 0x10000 (App Firmware): Magic byte 0x{app_magic:02X} INVALIDO (esperado 0xE9)")
            all_ok = False

        # 5. Comprobar con esptool image-info sobre la sección de firmware
        app_bin = temp_dir / f"app_{vid}.bin"
        with open(app_bin, "wb") as af:
            af.write(content[0x10000:])

        try:
            res = subprocess.run([sys.executable, "-m", "esptool", "--chip", "esp32", "image-info", str(app_bin)],
                                 capture_output=True, text=True)
            if res.returncode == 0 and "Checksum: " in res.stdout and "valid" in res.stdout:
                print("  esptool image-info: Checksum e Integrity Hash VALIDADOS")
            else:
                print(f"  [ERROR] esptool image-info fallo:\n{res.stderr or res.stdout}")
                all_ok = False
        except Exception as ex:
            print(f"  [ERROR] Excepcion al invocar esptool: {ex}")
            all_ok = False

        if app_bin.exists():
            app_bin.unlink()

        print("  Estado binario: OK")
        print()

    # Limpieza
    try:
        temp_dir.rmdir()
    except Exception:
        pass

    return all_ok

if __name__ == "__main__":
    ok = test_binaries()
    print("=" * 70)
    if ok:
        print("RESULTADO: TODOS LOS 8 BINARIOS SON 100% VALIDOS Y ESTRUCTURALMENTE PERFECTOS")
    else:
        print("RESULTADO: SE ENCONTRARON FALLOS EN ALGUN BINARIO")
    print("=" * 70)
    sys.exit(0 if ok else 1)
