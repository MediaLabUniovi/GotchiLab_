import json
from pathlib import Path
import itertools

REPO_ROOT = Path(r"C:\Users\joseev\Downloads\GotchiLab_")
MANIFEST_PATH = REPO_ROOT / "web" / "data" / "manifest.json"

with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
    data = json.load(f)

variants = data["variants"]

print("=" * 85)
print("2. ANÁLISIS DE LA MATRIZ COMPLETA DE SELECCIÓN WEB (16 COMBINACIONES POSIBLES)")
print("=" * 85)
print(f"{'#':<3} | {'CO2':<5} | {'LUZ':<5} | {'TOUCH':<5} | {'BUZZ':<5} | {'BINARIO SELECCIONADO':<30} | {'COINCIDENCIA'}")
print("-" * 85)

switches = ["co2", "light", "touch", "buzzer"]
combinations = list(itertools.product([True, False], repeat=4))

perfect_count = 0

for i, (co2, light, touch, buzzer) in enumerate(combinations, 1):
    flags = {"co2": co2, "light": light, "touch": touch, "buzzer": buzzer}
    
    # Algoritmo de scoring de app.js (10 pts por coincidencia)
    best_target = None
    highest_score = -1
    
    for v in variants:
        score = 0
        if v["features"]["co2"] == flags["co2"]: score += 10
        if v["features"]["light"] == flags["light"]: score += 10
        if v["features"]["touch"] == flags["touch"]: score += 10
        if v["features"]["buzzer"] == flags["buzzer"]: score += 10
        
        if score > highest_score:
            highest_score = score
            best_target = v
            
    matches = []
    diffs = []
    for sw in switches:
        if best_target["features"][sw] == flags[sw]:
            matches.append(sw)
        else:
            diffs.append(f"{sw} (pediste {flags[sw]}, bin tiene {best_target['features'][sw]})")
            
    is_perfect = len(matches) == 4
    if is_perfect:
        perfect_count += 1
        status = "100% EXACTO (4/4)"
    else:
        status = f"3/4 (Difiere: {', '.join(diffs)})"
        
    c_s = "ON" if co2 else "OFF"
    l_s = "ON" if light else "OFF"
    t_s = "ON" if touch else "OFF"
    b_s = "ON" if buzzer else "OFF"
    
    print(f"{i:<3} | {c_s:<5} | {l_s:<5} | {t_s:<5} | {b_s:<5} | {best_target['filename']:<30} | {status}")

print("=" * 85)
print(f"Total combinaciones exactas (100% match): {perfect_count} / 16")
print("=" * 85)
