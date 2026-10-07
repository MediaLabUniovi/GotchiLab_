import os
import base64
import subprocess
import shutil

# Read penguin image
pingu_path = r"C:\Users\joseev\Downloads\GotchiLab_\docs\pinguino_construccion_transp.png"
with open(pingu_path, "rb") as f:
    pingu_b64 = base64.b64encode(f.read()).decode("utf-8")

# Read MediaLab logo
logo_path = r"C:\Users\joseev\Downloads\GotchiLab_\web\assets\medialab_logo.png"
ml_logo_b64 = ""
if os.path.exists(logo_path):
    with open(logo_path, "rb") as f:
        ml_logo_b64 = base64.b64encode(f.read()).decode("utf-8")

html_content = f"""<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <title>Guía GotchiLab_</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Comfortaa:wght@700&family=Plus+Jakarta+Sans:wght@400;600;700;800&family=JetBrains+Mono:wght@500;700&display=swap');

    @page {{
      size: A4 portrait;
      margin: 0;
    }}

    * {{
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }}

    body {{
      width: 210mm;
      height: 297mm;
      font-family: 'Plus Jakarta Sans', sans-serif;
      background-color: #f8fafc;
      color: #0f172a;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      position: relative;
      overflow: hidden;
      -webkit-print-color-adjust: exact;
      print-color-adjust: exact;
    }}

    /* Top and bottom hazard stripes */
    .hazard-stripe {{
      width: 100%;
      height: 12px;
      background: repeating-linear-gradient(
        45deg,
        #f59e0b,
        #f59e0b 14px,
        #1e293b 14px,
        #1e293b 28px
      );
    }}

    .container {{
      flex: 1;
      padding: 32px 42px 24px 42px;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
      position: relative;
    }}

    /* Background decorative grid */
    .bg-grid {{
      position: absolute;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background-image: 
        linear-gradient(to right, rgba(203, 213, 225, 0.4) 1px, transparent 1px),
        linear-gradient(to bottom, rgba(203, 213, 225, 0.4) 1px, transparent 1px);
      background-size: 24px 24px;
      pointer-events: none;
      z-index: 0;
    }}

    .content-layer {{
      position: relative;
      z-index: 1;
      display: flex;
      flex-direction: column;
      height: 100%;
      justify-content: space-between;
    }}

    /* Header */
    .header {{
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-bottom: 2px solid #e2e8f0;
      padding-bottom: 18px;
    }}

    .brand {{
      display: flex;
      flex-direction: column;
    }}

    .brand-title {{
      font-family: 'Comfortaa', cursive, sans-serif;
      font-size: 28px;
      font-weight: 700;
      letter-spacing: -0.5px;
      color: #090d16;
    }}

    .brand-title span {{
      color: #0891b2;
    }}

    .brand-subtitle {{
      font-family: 'JetBrains Mono', monospace;
      font-size: 11px;
      letter-spacing: 2px;
      color: #64748b;
      font-weight: 700;
      margin-top: 2px;
    }}

    .header-logo img {{
      height: 48px;
      object-fit: contain;
    }}

    /* Main Central Showcase */
    .main-body {{
      display: flex;
      flex-direction: column;
      align-items: center;
      text-align: center;
      margin: auto 0;
    }}

    .mascot-container {{
      position: relative;
      width: 250px;
      height: 250px;
      margin-bottom: 18px;
      display: flex;
      align-items: center;
      justify-content: center;
      background: radial-gradient(circle, rgba(254, 240, 138, 0.45) 0%, rgba(254, 240, 138, 0.08) 60%, transparent 75%);
      border-radius: 50%;
    }}

    .mascot-img {{
      width: 220px;
      height: 220px;
      image-rendering: pixelated;
      image-rendering: -moz-crisp-edges;
      image-rendering: crisp-edges;
      object-fit: contain;
      filter: drop-shadow(0 12px 24px rgba(15, 23, 42, 0.15));
    }}

    .badge-construction {{
      display: inline-flex;
      align-items: center;
      gap: 8px;
      background-color: #fef3c7;
      border: 1.5px solid #f59e0b;
      color: #92400e;
      font-family: 'JetBrains Mono', monospace;
      font-weight: 700;
      font-size: 12px;
      letter-spacing: 1.2px;
      padding: 6px 16px;
      border-radius: 9999px;
      margin-bottom: 16px;
      text-transform: uppercase;
    }}

    .title-construction {{
      font-family: 'Plus Jakarta Sans', sans-serif;
      font-size: 32px;
      font-weight: 800;
      color: #0f172a;
      line-height: 1.25;
      margin-bottom: 14px;
      max-width: 620px;
    }}

    .title-construction span {{
      color: #d97706;
      text-decoration: underline;
      text-underline-offset: 6px;
      text-decoration-color: #fcd34d;
    }}

    .desc-construction {{
      font-size: 15px;
      color: #475569;
      line-height: 1.6;
      max-width: 540px;
      margin-bottom: 28px;
    }}

    /* Information Cards Grid */
    .cards-grid {{
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 16px;
      width: 100%;
      max-width: 620px;
    }}

    .info-card {{
      background: #ffffff;
      border: 1.5px solid #e2e8f0;
      border-radius: 12px;
      padding: 16px 14px;
      text-align: left;
      box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.04);
      display: flex;
      flex-direction: column;
      gap: 6px;
    }}

    .card-icon {{
      font-size: 20px;
    }}

    .card-title {{
      font-size: 12px;
      font-weight: 700;
      color: #1e293b;
    }}

    .card-desc {{
      font-size: 11px;
      color: #64748b;
      line-height: 1.4;
    }}

    /* Notice Banner */
    .notice-box {{
      width: 100%;
      max-width: 620px;
      background: #fffbeb;
      border-left: 4px solid #f59e0b;
      border-radius: 6px;
      padding: 12px 16px;
      display: flex;
      align-items: center;
      gap: 12px;
      margin-top: 18px;
      text-align: left;
    }}

    .notice-icon {{
      font-size: 22px;
      flex-shrink: 0;
    }}

    .notice-text {{
      font-size: 12px;
      color: #78350f;
      line-height: 1.45;
    }}

    /* Footer */
    .footer {{
      display: flex;
      justify-content: space-between;
      align-items: center;
      border-top: 1.5px solid #e2e8f0;
      padding-top: 16px;
      font-size: 11px;
      color: #64748b;
    }}

    .footer-left {{
      display: flex;
      flex-direction: column;
      gap: 2px;
    }}

    .footer-inst {{
      font-weight: 700;
      color: #334155;
    }}

    .footer-web {{
      font-family: 'JetBrains Mono', monospace;
      color: #0891b2;
      font-weight: 600;
    }}

    .footer-stamp {{
      font-family: 'JetBrains Mono', monospace;
      background: #e2e8f0;
      padding: 4px 10px;
      border-radius: 6px;
      font-size: 10px;
      color: #475569;
      font-weight: 600;
    }}
  </style>
</head>
<body>
  <!-- Top Hazard Bar -->
  <div class="hazard-stripe"></div>

  <div class="container">
    <div class="bg-grid"></div>

    <div class="content-layer">
      <!-- Top Brand Header -->
      <header class="header">
        <div class="brand">
          <div class="brand-title">GOTCHI<span>LAB_</span></div>
          <div class="brand-subtitle">GUÍA TÉCNICA Y DIDÁCTICA • TALLERES STEAM</div>
        </div>
        <div class="header-logo">
          {f'<img src="data:image/png;base64,{ml_logo_b64}" alt="MediaLab_">' if ml_logo_b64 else '<span style="font-weight:800; font-family:Comfortaa;">medialab_</span>'}
        </div>
      </header>

      <!-- Main Central Section -->
      <main class="main-body">
        <div class="mascot-container">
          <img class="mascot-img" src="data:image/png;base64,{pingu_b64}" alt="Pingüino GotchiLab_ con casco y martillo">
        </div>

        <div class="badge-construction">
          <span>⚠️</span> Documento en Elaboración <span>⚠️</span>
        </div>

        <h1 class="title-construction">
          Este documento está <span>aún en construcción</span>
        </h1>

        <p class="desc-construction">
          El equipo de <strong>MediaLab_</strong> está ultimando los contenidos oficiales, 
          esquemas eléctricos y pasos de montaje de la mascota interactiva GotchiLab_.
        </p>

        <!-- Information Cards -->
        <div class="cards-grid">
          <div class="info-card">
            <span class="card-icon">🪛</span>
            <span class="card-title">Montaje y Carcasa</span>
            <span class="card-desc">Instrucciones de impresión 3D, inserción de tuercas e integración de piezas.</span>
          </div>

          <div class="info-card">
            <span class="card-icon">⚡</span>
            <span class="card-title">Circuito y Sensores</span>
            <span class="card-desc">Conexionado de ESP32 DevKit, sensor NDIR SCD30 y pantalla OLED 128x64.</span>
          </div>

          <div class="info-card">
            <span class="card-icon">🚀</span>
            <span class="card-title">Flasheo Web Oficial</span>
            <span class="card-desc">Carga inmediata del firmware educativo vía Web Serial sin instalar programas.</span>
          </div>
        </div>

        <!-- Callout Banner -->
        <div class="notice-box">
          <span class="notice-icon">🚧</span>
          <span class="notice-text">
            <strong>Próximamente disponible:</strong> Puedes flashear tu placa mientras tanto desde el navegador en 
            <strong>gotchilab.medialab-uniovi.es</strong> o consultar el repositorio oficial.
          </span>
        </div>
      </main>

      <!-- Footer Info -->
      <footer class="footer">
        <div class="footer-left">
          <span class="footer-inst">MediaLab Universidad de Oviedo • EPI Gijón</span>
          <span>Proyecto de Innovación y Divulgación STEAM</span>
        </div>
        <div class="footer-web">gotchilab.medialab-uniovi.es</div>
        <div class="footer-stamp">DOC-STATUS: DRAFT / WIP</div>
      </footer>
    </div>
  </div>

  <!-- Bottom Hazard Bar -->
  <div class="hazard-stripe"></div>
</body>
</html>
"""

temp_html = r"C:\Users\joseev\Downloads\GotchiLab_\docs\guia_temp.html"
with open(temp_html, "w", encoding="utf-8") as f:
    f.write(html_content)

print("Generated HTML at:", temp_html)
