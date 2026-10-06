/**
 * GotchiLab_ Ambient Web Flasher & Live OLED Experience
 * MediaLab_ STEAM Education // ESP32 DevKit v1
 */

// ============================================================================
// 1. Ambient Background OLED Engine (Continuous Autonomous Loop)
// ============================================================================

class AmbientOledEngine {
    constructor(canvasId = 'liveOledCanvas') {
        this.canvas = document.getElementById(canvasId);
        if (!this.canvas) return;
        this.ctx = this.canvas.getContext('2d', { willReadFrequently: true });
        
        this.width = 128;
        this.height = 64;
        this.scale = 14; // 128x14 = 1792, 64x14 = 896 (Colossal hero size)
        this.canvas.width = this.width * this.scale;
        this.canvas.height = this.height * this.scale;
        
        this.pixelColor = '#00F0FF';
        
        this.animations = window.GOTCHI_ANIMATIONS || {};
        this.sequence = ['EGG', 'BIRTH', 'IDLE', 'FEED', 'IDLE', 'PET', 'SLEEP', 'IDLE'];
        this.sequenceIdx = 0;
        
        this.currentFrame = 0;
        this.fps = 5;
        this.decodedFrames = {};
        this.decodeFrames();
        
        this.startLoop();
    }
    
    decodeFrames() {
        for (const [key, anim] of Object.entries(this.animations)) {
            this.decodedFrames[key] = anim.framesB64.map(b64 => {
                const binStr = atob(b64);
                const bytes = new Uint8Array(binStr.length);
                for (let i = 0; i < binStr.length; i++) {
                    bytes[i] = binStr.charCodeAt(i);
                }
                return bytes;
            });
        }
    }
    
    startLoop() {
        setInterval(() => {
            this.tick();
        }, 1000 / this.fps);
    }
    
    tick() {
        const animKey = this.sequence[this.sequenceIdx];
        const frames = this.decodedFrames[animKey];
        if (!frames || frames.length === 0) return;
        
        this.renderFrame(animKey, frames[this.currentFrame], this.currentFrame);
        
        this.currentFrame++;
        if (this.currentFrame >= frames.length) {
            this.currentFrame = 0;
            this.sequenceIdx = (this.sequenceIdx + 1) % this.sequence.length;
        }
    }
    
    renderFrame(animKey, frameData, frameIdx) {
        const ctx = this.ctx;
        const scale = this.scale;
        const animMeta = this.animations[animKey];
        const yOffset = (animMeta && animMeta.offsets && animMeta.offsets[frameIdx] !== undefined)
            ? animMeta.offsets[frameIdx]
            : 0;
            
        // 100% Transparent Canvas Background - No frame, no box, only the penguin!
        ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
        
        // Active glowing pixels
        ctx.fillStyle = this.pixelColor;
        for (let x = 0; x < this.width; x++) {
            for (let y = 0; y < this.height; y++) {
                const srcY = y - yOffset;
                if (srcY < 0 || srcY >= this.height) continue;
                
                const srcIndex = x + Math.floor(srcY / 8) * this.width;
                const srcBit = 1 << (srcY & 7);
                
                if ((frameData[srcIndex] & srcBit) !== 0) {
                    ctx.fillRect(x * scale, y * scale, scale - 1, scale - 1);
                }
            }
        }
    }
}

// ============================================================================
// 2. Streamlined Hardware Matrix (All Defaults ON)
// ============================================================================

class HardwareConfigurator {
    constructor() {
        this.manifest = null;
        this.activeTarget = null;
        
        this.switches = {
            co2: document.getElementById('sw-co2'),
            light: document.getElementById('sw-light'),
            touch: document.getElementById('sw-touch'),
            buzzer: document.getElementById('sw-buzzer')
        };
        
        this.bindEvents();
    }
    
    async init() {
        try {
            // Check both organized data path and root fallback
            let resp = await fetch('data/manifest.json');
            if (!resp.ok) resp = await fetch('manifest.json');
            if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
            this.manifest = await resp.json();
        } catch (e) {
            console.warn('[CONFIG] Usando manifiesto de reserva local:', e);
            this.manifest = {
                variants: [
                    { id: "full", filename: "gotchilab_full.bin", name: "GotchiLab Completo (Todos los Sensores)", path: "binaries/gotchilab_full.bin", features: { co2: true, light: true, touch: true, buzzer: true } },
                    { id: "full_no_touch", filename: "gotchilab_full_no_touch.bin", name: "GotchiLab Completo Sin Táctil", path: "binaries/gotchilab_full_no_touch.bin", features: { co2: true, light: true, touch: false, buzzer: true } },
                    { id: "no_co2", filename: "gotchilab_no_co2.bin", name: "GotchiLab Sin CO2", path: "binaries/gotchilab_no_co2.bin", features: { co2: false, light: true, touch: true, buzzer: true } },
                    { id: "no_co2_no_touch", filename: "gotchilab_no_co2_no_touch.bin", name: "GotchiLab Sin CO2 ni Táctil", path: "binaries/gotchilab_no_co2_no_touch.bin", features: { co2: false, light: true, touch: false, buzzer: true } },
                    { id: "no_co2_silent", filename: "gotchilab_no_co2_silent.bin", name: "GotchiLab Silencioso Sin CO2", path: "binaries/gotchilab_no_co2_silent.bin", features: { co2: false, light: true, touch: true, buzzer: false } },
                    { id: "minimal", filename: "gotchilab_minimal.bin", name: "GotchiLab Básico Mínimo", path: "binaries/gotchilab_minimal.bin", features: { co2: false, light: false, touch: false, buzzer: false } }
                ]
            };
        }
        
        // Ensure ALL switches start checked (true) as requested
        Object.values(this.switches).forEach(sw => {
            if (sw) sw.checked = true;
        });
        
        this.resolveTarget();
    }
    
    bindEvents() {
        // Permitir clic en el cuadrado entero o directamente en la barrita
        document.querySelectorAll('.switch-pill').forEach(card => {
            card.addEventListener('click', (e) => {
                if (e.target.closest('.toggle-track')) return;
                const input = card.querySelector('input[type="checkbox"]');
                if (input) {
                    input.checked = !input.checked;
                    this.resolveTarget();
                }
            });
        });
        
        Object.values(this.switches).forEach(sw => {
            if (sw) {
                sw.addEventListener('change', () => this.resolveTarget());
            }
        });
    }
    
    getFlags() {
        return {
            co2: this.switches.co2 ? this.switches.co2.checked : true,
            light: this.switches.light ? this.switches.light.checked : true,
            touch: this.switches.touch ? this.switches.touch.checked : true,
            buzzer: this.switches.buzzer ? this.switches.buzzer.checked : true
        };
    }
    
    resolveTarget() {
        const flags = this.getFlags();
        
        // Actualizar clase active en los cuadrados de los sensores
        document.querySelectorAll('.switch-pill').forEach(card => {
            const input = card.querySelector('input[type="checkbox"]');
            if (input && input.checked) {
                card.classList.add('active');
            } else if (input) {
                card.classList.remove('active');
            }
        });
        
        if (!this.manifest || !this.manifest.variants) return;
        
        // Score best matching firmware target
        let bestTarget = null;
        let highestScore = -1;
        
        for (const variant of this.manifest.variants) {
            let score = 0;
            if (variant.features.co2 === flags.co2) score += 10;
            if (variant.features.light === flags.light) score += 10;
            if (variant.features.touch === flags.touch) score += 10;
            if (variant.features.buzzer === flags.buzzer) score += 10;
            
            if (score > highestScore) {
                highestScore = score;
                bestTarget = variant;
            }
        }
        
        this.activeTarget = bestTarget;
        
        // Update DOM elements
        const profileTag = document.getElementById('targetProfileLabel');
        const pillText = document.getElementById('pillProfileText');
        const downloadLink = document.getElementById('btnDownloadRawBin');
        
        if (profileTag) profileTag.textContent = bestTarget.name;
        if (pillText) pillText.textContent = `Firmware: ${bestTarget.filename}`;
        if (downloadLink) {
            downloadLink.href = bestTarget.path;
            downloadLink.download = bestTarget.filename;
        }
    }
}

// ============================================================================
// 3. Central Web Serial Flashing Hub
// ============================================================================

class FlashHub {
    constructor(configurator) {
        this.configurator = configurator;
        this.port = null;
        this.transport = null;
        this.esploader = null;
        this.busy = false;
        
        this.btnFlash = document.getElementById('btnStartFlash');
        this.progressBar = document.getElementById('flashProgressBar');
        this.progressPct = document.getElementById('flashProgressPct');
        this.progressStatus = document.getElementById('flashStatusLabel');
        this.telemetry = document.getElementById('telemetryConsole');
        this.modal = document.getElementById('flashSuccessModal');
        this.btnModalClose = document.getElementById('btnModalClose');
        
        this.verifyBrowserSupport();
        this.bindEvents();
    }
    
    verifyBrowserSupport() {
        const banner = document.getElementById('browserAlertBanner');
        const hasSerial = ('serial' in navigator);
        const isFileProtocol = (window.location.protocol === 'file:');
        
        if (!hasSerial || isFileProtocol) {
            if (banner) {
                banner.classList.add('active');
                const text = banner.querySelector('.banner-text');
                if (text) {
                    if (isFileProtocol) {
                        text.innerHTML = `
                            <strong>Ejecutando desde archivo local (protocolo file://)</strong>
                            Por seguridad, los navegadores (Chrome, Brave, Edge) exigen un servidor local o HTTPS para permitir el uso de Web Serial.<br>
                            Ejecuta <code>iniciar_web.bat</code> en la raíz del proyecto o abre una terminal con <code>python -m http.server 8000</code> y entra en <a href="http://localhost:8000/web/" target="_blank" style="color: #00f0ff; text-decoration: underline;">http://localhost:8000/web/</a>.
                        `;
                    } else {
                        text.innerHTML = `
                            <strong>Navegador sin soporte para Web Serial API</strong>
                            Para flashear tu ESP32 por USB directamente desde la web, abre esta página en <strong>Google Chrome</strong> o <strong>Microsoft Edge</strong>.<br>
                            En <strong>Brave</strong>, asegúrate de tener permitido el acceso a puertos serie en <code>brave://settings/content/serialPorts</code>.
                        `;
                    }
                }
            }
            if (this.btnFlash) {
                this.btnFlash.disabled = true;
                this.btnFlash.innerHTML = '⚠️ NAVEGADOR SIN SOPORTE WEB SERIAL';
            }
            this.log('Web Serial no disponible en este contexto.', 'log-warn');
        } else {
            if (banner) {
                banner.classList.remove('active');
                banner.style.display = 'none';
            }
            if (this.btnFlash) {
                this.btnFlash.disabled = false;
                this.btnFlash.innerHTML = `
                    <span class="btn-flash-glare"></span>
                    <span class="btn-flash-icon">⚡</span>
                    <span class="btn-flash-label">INICIAR CARGA AL ESP32</span>
                `;
            }
            this.log('[HARDWARE] Web Serial API disponible y lista para conexión.', 'log-cyan');
        }
    }
    
    bindEvents() {
        if (this.btnFlash) {
            this.btnFlash.addEventListener('click', () => this.runFlashSequence());
        }
        const btnClear = document.getElementById('btnClearLog');
        if (btnClear) {
            btnClear.addEventListener('click', () => {
                if (this.telemetry) this.telemetry.innerHTML = '';
            });
        }
        if (this.btnModalClose) {
            this.btnModalClose.addEventListener('click', () => this.closeModal());
        }
        if (this.modal) {
            this.modal.addEventListener('click', (e) => {
                if (e.target === this.modal) this.closeModal();
            });
        }
        window.addEventListener('keydown', (e) => {
            if (e.key === 'Escape' && this.modal && this.modal.classList.contains('active')) {
                this.closeModal();
            }
        });
    }

    openModal() {
        if (this.modal) {
            this.modal.classList.add('active');
            this.modal.setAttribute('aria-hidden', 'false');
        }
    }

    closeModal() {
        if (this.modal) {
            this.modal.classList.remove('active');
            this.modal.setAttribute('aria-hidden', 'true');
        }
    }
    
    log(message, typeClass = 'log-info') {
        if (!this.telemetry) return;
        const line = document.createElement('div');
        line.className = typeClass;
        const time = new Date().toLocaleTimeString();
        line.textContent = `[${time}] ${message}`;
        this.telemetry.appendChild(line);
        this.telemetry.scrollTop = this.telemetry.scrollHeight;
    }
    
    setProgress(percentage, statusText) {
        const clamped = Math.max(0, Math.min(100, Math.round(percentage)));
        if (this.progressBar) this.progressBar.style.width = `${clamped}%`;
        if (this.progressPct) this.progressPct.textContent = `${clamped}%`;
        if (this.progressStatus && statusText) this.progressStatus.textContent = statusText;
    }
    
    setStep(stepNumber) {
        document.querySelectorAll('.step-chip').forEach((node, i) => {
            node.classList.remove('active', 'done');
            if (i + 1 < stepNumber) {
                node.classList.add('done');
            } else if (i + 1 === stepNumber) {
                node.classList.add('active');
            }
        });
    }
    
    async loadEsptool() {
        try {
            // First check local organized bundle, then unpkg CDN
            return await import('../lib/esptool-bundle.js').catch(() => {
                return import('https://unpkg.com/esptool-js@0.5.4/bundle.js');
            });
        } catch (e) {
            throw new Error(`Fallo cargando esptool-js: ${e.message}`);
        }
    }
    
    async runFlashSequence() {
        if (this.busy) return;
        const target = this.configurator.activeTarget;
        if (!target) {
            this.log('Error: No se ha seleccionado una variante válida.', 'log-error');
            return;
        }
        
        this.busy = true;
        this.btnFlash.disabled = true;
        this.btnFlash.innerHTML = '<span class="pill-dot" style="background:#000;"></span> CARGANDO FIRMWARE...';
        this.setProgress(5, 'Solicitando puerto serie...');
        this.setStep(1);
        
        try {
            this.log('Inicializando motor de flasheo esptool-js...', 'log-cyan');
            const { ESPLoader, Transport } = await this.loadEsptool();
            
            this.log('Selecciona el puerto COM de tu ESP32 en el diálogo del navegador...', 'log-info');
            this.port = await navigator.serial.requestPort({});
            
            this.setStep(2);
            this.setProgress(15, 'Sincronizando con ESP32...');
            this.log('Puerto serie abierto. Conectando a 460800 baudios...', 'log-info');
            
            this.transport = new Transport(this.port, true);
            
            const term = {
                clean: () => {},
                writeLine: (t) => this.log(t, 'log-cyan'),
                write: (t) => {
                    const clean = t.replace(/[\r\n]+$/, '');
                    if (clean) this.log(clean, 'log-cyan');
                }
            };
            
            this.esploader = new ESPLoader({
                transport: this.transport,
                baudrate: 460800,
                terminal: term,
                romBaudrate: 115200
            });
            
            const chip = await this.esploader.main();
            this.log(`Conexión exitosa. Microcontrolador: ${chip}`, 'log-success');
            
            this.setStep(3);
            this.setProgress(30, `Descargando ${target.filename}...`);
            this.log(`Descargando binario unificado (${target.path})...`, 'log-info');
            
            const resp = await fetch(target.path);
            if (!resp.ok) throw new Error(`HTTP ${resp.status} al descargar el binario`);
            const buf = await resp.arrayBuffer();
            
            this.log(`Imagen recibida: ${(buf.byteLength / 1024).toFixed(1)} KB (Offset 0x00000000)`, 'log-success');
            
            // Conversión segura 1:1 de ArrayBuffer a binary string (evita la reasignación WHATWG latin1/windows-1252 en 0x80-0x9F)
            const bytes = new Uint8Array(buf);
            let binaryString = '';
            const chunkSize = 0x8000;
            for (let i = 0; i < bytes.length; i += chunkSize) {
                binaryString += String.fromCharCode.apply(null, bytes.subarray(i, i + chunkSize));
            }
            
            this.setStep(4);
            this.setProgress(40, 'Escribiendo en memoria Flash...');
            this.log('Escribiendo bloques a offset 0x0 con verificación...', 'log-info');
            
            await this.esploader.writeFlash({
                fileArray: [{ data: binaryString, address: 0x0000 }],
                flashSize: 'keep',
                flashMode: 'dio',
                flashFreq: '40m',
                eraseAll: false,
                compress: true,
                reportProgress: (fileIdx, written, total) => {
                    const pct = 40 + (written / total) * 55;
                    this.setProgress(pct, `Transfiriendo: ${Math.round((written / total) * 100)}%`);
                }
            });
            
            this.setStep(5);
            this.setProgress(98, 'Reiniciando microcontrolador...');
            this.log('Flash completado con éxito. Ejecutando reinicio por hardware...', 'log-success');
            
            await this.esploader.after();
            
            this.setStep(6);
            this.setProgress(100, '¡GotchiLab_ cargado con éxito!');
            this.log('====================================================', 'log-success');
            this.log('¡CARGA COMPLETADA! Pulsa EN (RST) para iniciar el OLED.', 'log-success');
            this.log('====================================================', 'log-success');
            
            this.openModal();
            
        } catch (err) {
            console.error(err);
            this.setProgress(0, 'Fallo en la operación');
            this.log(`ERROR: ${err.message || err}`, 'log-error');
            alert(`No se pudo completar la carga: ${err.message || err}`);
        } finally {
            this.busy = false;
            this.btnFlash.disabled = false;
            this.btnFlash.innerHTML = '⚡ INICIAR CARGA AL ESP32';
            if (this.transport) {
                try { await this.transport.disconnect(); } catch (e) {}
            }
        }
    }
}

// ============================================================================
// 4. Ambient Retro Pixel Universe (Full Screen Atmospheric Depth)
// ============================================================================

class PixelUniverseEngine {
    constructor(canvasId = 'ambientPixelCanvas') {
        this.canvas = document.getElementById(canvasId);
        if (!this.canvas) return;
        this.ctx = this.canvas.getContext('2d');
        
        this.particles = [];
        this.numParticles = 75;
        this.colors = ['#00F0FF', '#00FF9D', '#38BDF8', '#818CF8', '#FFB800'];
        
        this.resize();
        window.addEventListener('resize', () => this.resize());
        this.initParticles();
        this.loop();
    }
    
    resize() {
        this.width = window.innerWidth;
        this.height = window.innerHeight;
        this.canvas.width = this.width;
        this.canvas.height = this.height;
    }
    
    initParticles() {
        this.particles = [];
        for (let i = 0; i < this.numParticles; i++) {
            this.particles.push(this.createParticle(true));
        }
    }
    
    createParticle(randomY = false) {
        return {
            x: Math.random() * this.width,
            y: randomY ? Math.random() * this.height : this.height + 10,
            size: Math.random() < 0.2 ? 4 : (Math.random() < 0.5 ? 3 : 2),
            speedX: (Math.random() - 0.5) * 0.35,
            speedY: -0.2 - Math.random() * 0.45,
            alpha: 0.12 + Math.random() * 0.45,
            pulseSpeed: 0.01 + Math.random() * 0.02,
            pulseDir: 1,
            color: this.colors[Math.floor(Math.random() * this.colors.length)],
            isCross: Math.random() < 0.15
        };
    }
    
    loop() {
        const ctx = this.ctx;
        ctx.clearRect(0, 0, this.width, this.height);
        
        for (let i = 0; i < this.particles.length; i++) {
            const p = this.particles[i];
            
            p.x += p.speedX;
            p.y += p.speedY;
            
            // Alpha pulse
            p.alpha += p.pulseSpeed * p.pulseDir;
            if (p.alpha > 0.65) p.pulseDir = -1;
            else if (p.alpha < 0.12) p.pulseDir = 1;
            
            ctx.fillStyle = p.color;
            ctx.globalAlpha = p.alpha;
            
            if (p.isCross) {
                // Retro '+' pixel shape (5 subpixels)
                const s = p.size;
                ctx.fillRect(p.x, p.y, s, s);
                ctx.fillRect(p.x - s, p.y, s, s);
                ctx.fillRect(p.x + s, p.y, s, s);
                ctx.fillRect(p.x, p.y - s, s, s);
                ctx.fillRect(p.x, p.y + s, s, s);
            } else {
                // Square crisp retro pixel
                ctx.fillRect(p.x, p.y, p.size, p.size);
            }
            
            // Reset if out of viewport
            if (p.y < -15 || p.x < -15 || p.x > this.width + 15) {
                this.particles[i] = this.createParticle(false);
            }
        }
        
        ctx.globalAlpha = 1.0;
        requestAnimationFrame(() => this.loop());
    }
}

// ============================================================================
// Initialization
// ============================================================================

document.addEventListener('DOMContentLoaded', async () => {
    // 1. Start Ambient Retro Pixel Universe across full screen
    const universe = new PixelUniverseEngine('ambientPixelCanvas');

    // 2. Start Giant OLED Penguin loop
    const oled = new AmbientOledEngine('liveOledCanvas');
    
    // 3. Start Hardware Configuration (defaults all ON)
    const config = new HardwareConfigurator();
    await config.init();
    
    // 4. Start Flash Hub
    const flasher = new FlashHub(config);
});
