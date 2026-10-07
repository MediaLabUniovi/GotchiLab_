/**
 * GotchiLab_ Guided Onboarding Tour (Spotlight Walkthrough)
 * MediaLab_ STEAM Education // Vanilla JS, zero dependencies
 * V4 Refined: Lateral Placement (Right), Fixed Viewport Math, Interactive Cutout
 */

class GotchiTour {
    constructor() {
        this.STORAGE_KEY = 'gotchilab_tour_completed';
        this.currentStep = 0;
        this.isActive = false;
        this.resizeTimeout = null;

        // Flujo guiado con pantalla previa de verificación de montaje
        this.steps = [
            {
                type: 'question',
                badge: 'BIENVENIDO',
                icon: '🐧',
                title: '¿Tienes montado tu pingüino?',
                text: 'Para poder cargar el juego y probarlo, necesitas tener ensambladas las piezas y la placa de tu GotchiLab.',
                target: null
            },
            {
                type: 'modal',
                badge: 'PASO 1 / 4',
                icon: '🔌',
                title: '1. Enchufa tu placa',
                text: 'Conecta la placa al ordenador usando un cable USB. Asegúrate de que sea un cable de datos y no únicamente de carga para que el ordenador pueda comunicarse con el ESP32.',
                target: null
            },
            {
                type: 'spotlight',
                badge: 'PASO 2 / 4',
                icon: '🎛️',
                title: '2. Marca tus piezas',
                text: 'En este recuadro, activa o desactiva los interruptores según los componentes que tengas montados (luz, caricias, voz o sensor de aire). ¡Puedes probar a hacer clic en ellos ahora mismo!',
                target: '.switches-cluster',
                placement: 'right',
                padding: 10
            },
            {
                type: 'spotlight',
                badge: 'PASO 3 / 4',
                icon: '⚡',
                title: '3. Envía el juego a la placa',
                text: 'Haz clic en este botón para iniciar la carga. En la pequeña ventana que abrirá tu navegador, selecciona el puerto COM de tu placa y pulsa "Conectar".',
                target: '#btnStartFlash',
                placement: 'right',
                padding: 8
            },
            {
                type: 'spotlight',
                badge: 'PASO 4 / 4',
                icon: '📊',
                title: '4. Progreso y Terminal',
                text: 'En esta barra verás el avance del 0% al 100% y los 5 pasos completándose en tiempo real. Justo debajo, la terminal de telemetría te mostrará en directo la comunicación y cada bloque grabado en la memoria. ¡Al terminar, la web te avisará para encender la pantalla!',
                target: '#flashMonitoringZone',
                placement: 'right',
                padding: 10
            }
        ];

        this.overlay = null;
        this.spotlightBox = null;
        this.popoverCard = null;

        this.handleKeyDown = this.handleKeyDown.bind(this);
        this.handleResize = this.handleResize.bind(this);

        this.init();
    }

    init() {
        // Enlazar botón manual en la navbar
        const btnManual = document.getElementById('btnOpenTour');
        if (btnManual) {
            btnManual.addEventListener('click', (e) => {
                e.preventDefault();
                this.start(true);
            });
        }

        // Si el usuario hace clic en el botón de flasheo, cerrar el tour inmediatamente
        // para dar paso a la ventana nativa de Web Serial del navegador
        const btnFlash = document.getElementById('btnStartFlash');
        if (btnFlash) {
            btnFlash.addEventListener('click', () => {
                if (this.isActive) {
                    this.close(true);
                }
            }, true);
        }

        // Auto-arranque siempre que abres o recargas la web (sin persistencia de finalización)
        setTimeout(() => {
            this.start(false);
        }, 600);

        // Listeners de teclado silenciosos (Ctrl+H / Escape) sin mostrar texto en pantalla
        window.addEventListener('keydown', (e) => {
            if ((e.ctrlKey || e.metaKey) && (e.key === 'h' || e.key === 'H')) {
                e.preventDefault();
                if (this.isActive) {
                    this.close(true);
                } else {
                    this.start(true);
                }
            }
        });
    }

    start(force = false) {
        if (this.isActive) return;
        this.isActive = true;
        this.currentStep = 0;

        document.body.classList.add('tour-active');

        this.createDOM();
        this.bindEvents();
        this.renderStep(this.currentStep);
    }

    createDOM() {
        if (document.getElementById('gotchiTourRoot')) {
            document.getElementById('gotchiTourRoot').remove();
        }

        // Contenedor overlay fijo (pointer-events: none en spotlight para permitir clics)
        this.overlay = document.createElement('div');
        this.overlay.id = 'gotchiTourRoot';
        this.overlay.className = 'tour-backdrop';
        this.overlay.setAttribute('role', 'dialog');
        this.overlay.setAttribute('aria-modal', 'true');
        this.overlay.setAttribute('aria-label', 'Guía de GotchiLab');

        // Marco perimetral iluminado (Spotlight)
        this.spotlightBox = document.createElement('div');
        this.spotlightBox.className = 'tour-spotlight-box';
        this.overlay.appendChild(this.spotlightBox);

        // Tarjeta popover (con pointer-events: auto para que sus botones funcionen)
        this.popoverCard = document.createElement('div');
        this.popoverCard.className = 'tour-card';
        this.overlay.appendChild(this.popoverCard);

        document.body.appendChild(this.overlay);

        requestAnimationFrame(() => {
            this.overlay.classList.add('active');
        });
    }

    bindEvents() {
        window.addEventListener('keydown', this.handleKeyDown);
        window.addEventListener('resize', this.handleResize, { passive: true });
        window.addEventListener('scroll', this.handleResize, { passive: true });
    }

    unbindEvents() {
        window.removeEventListener('keydown', this.handleKeyDown);
        window.removeEventListener('resize', this.handleResize);
        window.removeEventListener('scroll', this.handleResize);
    }

    handleKeyDown(e) {
        if (!this.isActive) return;

        if (e.key === 'Escape') {
            e.preventDefault();
            this.close(true);
            return;
        }

        if (e.key === 'ArrowRight') {
            e.preventDefault();
            this.next();
            return;
        }

        if (e.key === 'ArrowLeft') {
            e.preventDefault();
            this.prev();
            return;
        }
    }

    handleResize() {
        if (!this.isActive) return;
        clearTimeout(this.resizeTimeout);
        this.resizeTimeout = setTimeout(() => {
            this.updatePosition();
        }, 50);
    }

    renderStep(index) {
        if (index < 0 || index >= this.steps.length) return;
        this.currentStep = index;
        const step = this.steps[index];
        const isQuestion = step.type === 'question';
        const isFirst = index === 0;
        const isLast = index === this.steps.length - 1;

        // Si es el paso de pregunta previa (¿Tienes montado tu pingüino?)
        if (isQuestion) {
            this.popoverCard.innerHTML = `
                <div class="tour-card-header">
                    <div class="tour-badge-row">
                        <span class="tour-step-badge">${step.badge}</span>
                    </div>
                    <button class="tour-btn-close" id="tourBtnClose" aria-label="Cerrar tutorial" title="Cerrar">✕</button>
                </div>

                <div class="tour-card-body">
                    <div class="tour-icon-wrap">${step.icon}</div>
                    <div class="tour-text-wrap">
                        <h3 class="tour-title">${step.title}</h3>
                        <p class="tour-text">${step.text}</p>
                        
                        <div class="tour-branch-options">
                            <button class="tour-btn-branch tour-btn-branch-yes" id="tourBtnAnswerYes">
                                <span>¡Sí, ya está montado!</span> ➔
                            </button>
                            <button class="tour-btn-branch tour-btn-branch-no" id="tourBtnAnswerNo">
                                <span>No, todavía no</span> 🛠️
                            </button>
                        </div>
                    </div>
                </div>

                <div class="tour-card-footer">
                    <span style="font-size:0.75rem; color:#64748b;">Selecciona una opción para comenzar</span>
                    <div class="tour-actions">
                        <button class="tour-btn tour-btn-ghost" id="tourBtnSkip">Saltar</button>
                    </div>
                </div>

                <div class="tour-arrow" id="tourArrow"></div>
            `;

            const btnYes = this.popoverCard.querySelector('#tourBtnAnswerYes');
            const btnNo = this.popoverCard.querySelector('#tourBtnAnswerNo');
            const btnSkip = this.popoverCard.querySelector('#tourBtnSkip');
            const btnClose = this.popoverCard.querySelector('#tourBtnClose');

            if (btnYes) btnYes.addEventListener('click', () => this.renderStep(1));
            if (btnNo) btnNo.addEventListener('click', () => this.renderAssemblyGuideScreen());
            if (btnSkip) btnSkip.addEventListener('click', () => this.close());
            if (btnClose) btnClose.addEventListener('click', () => this.close());

            this.overlay.classList.add('tour-mode-modal');
            this.positionCentered();
            return;
        }

        // Construir contenido limpio de los pasos 1 a 4
        // (Excluimos el paso 0 de las bolitas para que muestre exactamente 4 pasos)
        const dotIndex = index - 1;
        const totalDots = this.steps.length - 1;

        this.popoverCard.innerHTML = `
            <div class="tour-card-header">
                <div class="tour-badge-row">
                    <span class="tour-step-badge">${step.badge}</span>
                </div>
                <button class="tour-btn-close" id="tourBtnClose" aria-label="Cerrar tutorial" title="Cerrar">✕</button>
            </div>

            <div class="tour-card-body">
                <div class="tour-icon-wrap">${step.icon}</div>
                <div class="tour-text-wrap">
                    <h3 class="tour-title">${step.title}</h3>
                    <p class="tour-text">${step.text}</p>
                </div>
            </div>

            <div class="tour-card-footer">
                <div class="tour-dots">
                    ${Array.from({ length: totalDots }).map((_, i) => `<span class="tour-dot ${i === dotIndex ? 'active' : ''}"></span>`).join('')}
                </div>
                <div class="tour-actions">
                    <button class="tour-btn tour-btn-secondary" id="tourBtnPrev">⬅ Anterior</button>
                    <button class="tour-btn tour-btn-primary" id="tourBtnNext">
                        ${isLast ? '¡Entendido! ✨' : 'Siguiente ➔'}
                    </button>
                </div>
            </div>

            <div class="tour-arrow" id="tourArrow"></div>
        `;

        const btnNext = this.popoverCard.querySelector('#tourBtnNext');
        const btnPrev = this.popoverCard.querySelector('#tourBtnPrev');
        const btnClose = this.popoverCard.querySelector('#tourBtnClose');

        if (btnNext) btnNext.addEventListener('click', () => this.next());
        if (btnPrev) btnPrev.addEventListener('click', () => this.prev());
        if (btnClose) btnClose.addEventListener('click', () => this.close());

        if (step.type === 'modal' || !step.target) {
            // Paso 1: Modo modal centrado con fondo bloqueante
            this.overlay.classList.add('tour-mode-modal');
            this.positionCentered();
        } else {
            // Pasos 2, 3 y 4: Modo spotlight interactivo (permite clics en los componentes)
            this.overlay.classList.remove('tour-mode-modal');
            const targetEl = document.querySelector(step.target);
            if (targetEl) {
                this.positionSpotlight(targetEl, step.padding || 8, step.placement || 'auto');
            } else {
                this.positionCentered();
            }
        }
    }

    renderAssemblyGuideScreen() {
        if (!this.popoverCard) return;

        this.popoverCard.innerHTML = `
            <div class="tour-card-header">
                <div class="tour-badge-row">
                    <span class="tour-step-badge" style="color:#ffb300; background:rgba(255,179,0,0.12); border-color:rgba(255,179,0,0.4);">GUÍA DE MONTAJE</span>
                </div>
                <button class="tour-btn-close" id="tourBtnClose" aria-label="Cerrar tutorial" title="Cerrar">✕</button>
            </div>

            <div class="tour-card-body">
                <div class="tour-icon-wrap" style="border-color:rgba(255,179,0,0.4); box-shadow:0 0 15px rgba(255,179,0,0.15);">🛠️</div>
                <div class="tour-text-wrap">
                    <h3 class="tour-title">¡Móntalo antes que nada!</h3>
                    <p class="tour-text">
                        No te preocupes. Hemos preparado una guía visual paso a paso para que aprendas cómo colocar cada sensor, el zumbador y conectar la pantalla a tu placa ESP32.
                    </p>
                    
                    <div class="tour-assembly-callout">
                        <div class="assembly-callout-header">
                            <span class="assembly-callout-icon">📖</span>
                            <span>Accede a la guía detallada con todos los esquemas y conexiones:</span>
                        </div>
                        <a href="docs/guia.pdf" target="_blank" rel="noopener noreferrer" class="tour-btn-guide" id="tourBtnOpenGuide">
                            <span>Ver Guía de Montaje</span> ➔
                        </a>
                    </div>
                </div>
            </div>

            <div class="tour-card-footer">
                <button class="tour-btn tour-btn-secondary" id="tourBtnBackToQuestion">⬅ Volver</button>
                <div class="tour-actions">
                    <button class="tour-btn tour-btn-ghost" id="tourBtnSkip">Cerrar y montar luego</button>
                </div>
            </div>

            <div class="tour-arrow" id="tourArrow"></div>
        `;

        const btnBack = this.popoverCard.querySelector('#tourBtnBackToQuestion');
        const btnSkip = this.popoverCard.querySelector('#tourBtnSkip');
        const btnClose = this.popoverCard.querySelector('#tourBtnClose');

        if (btnBack) btnBack.addEventListener('click', () => this.renderStep(0));
        if (btnSkip) btnSkip.addEventListener('click', () => this.close());
        if (btnClose) btnClose.addEventListener('click', () => this.close());

        this.overlay.classList.add('tour-mode-modal');
        this.positionCentered();
    }

    positionCentered() {
        if (!this.spotlightBox || !this.popoverCard) return;

        this.spotlightBox.style.opacity = '0';
        this.spotlightBox.style.display = 'none';

        const arrow = this.popoverCard.querySelector('#tourArrow');
        if (arrow) arrow.style.display = 'none';

        this.popoverCard.className = 'tour-card tour-card-centered';
        this.popoverCard.style.top = '';
        this.popoverCard.style.left = '';
        this.popoverCard.style.transform = '';
    }

    positionSpotlight(targetEl, padding = 8, preferredPlacement = 'auto') {
        if (!targetEl || !this.spotlightBox || !this.popoverCard) return;

        // Medir exactamente en el viewport actual (sin tocar el scroll)
        const rect = targetEl.getBoundingClientRect();

        // 1. Posicionar marco perimetral (position: fixed, coordenadas exactas de pantalla)
        const boxTop = rect.top - padding;
        const boxLeft = rect.left - padding;
        const boxWidth = rect.width + (padding * 2);
        const boxHeight = rect.height + (padding * 2);

        this.spotlightBox.style.display = 'block';
        this.spotlightBox.style.opacity = '1';
        this.spotlightBox.style.top = `${boxTop}px`;
        this.spotlightBox.style.left = `${boxLeft}px`;
        this.spotlightBox.style.width = `${boxWidth}px`;
        this.spotlightBox.style.height = `${boxHeight}px`;

        // 2. Medir dimensiones de tarjeta y viewport
        this.popoverCard.className = 'tour-card tour-card-floating';
        const cardWidth = Math.min(420, window.innerWidth * 0.92);
        this.popoverCard.style.width = `${cardWidth}px`;

        const arrow = this.popoverCard.querySelector('#tourArrow');
        if (arrow) arrow.style.display = 'block';

        const cardHeight = this.popoverCard.offsetHeight || 210;
        const viewportWidth = window.innerWidth;
        const viewportHeight = window.innerHeight;

        let cardTop = 0;
        let cardLeft = 0;
        let arrowClass = '';
        let arrowLeft = '';
        let arrowTop = '';

        // Comprobar si hay espacio a la derecha
        const spaceRight = viewportWidth - (boxLeft + boxWidth);

        if (preferredPlacement === 'right' && spaceRight >= (cardWidth + 20)) {
            // POSICIONAMIENTO A LA DERECHA (Evita corte superior y deja el panel despejado)
            cardLeft = boxLeft + boxWidth + 16;
            
            // Centrado vertical respecto al elemento resaltado
            const targetCenterY = boxTop + (boxHeight / 2);
            cardTop = targetCenterY - (cardHeight / 2);
            
            // Clamping vertical estricto: NUNCA se cortará por arriba (< 16px) ni por abajo
            cardTop = Math.max(16, Math.min(viewportHeight - cardHeight - 16, cardTop));
            
            arrowClass = 'arrow-left';
            arrowLeft = '-10px';
            
            // Alinear la flecha verticalmente con el centro del elemento
            const relativeArrowY = targetCenterY - cardTop;
            const clampedArrowY = Math.max(22, Math.min(cardHeight - 22, relativeArrowY));
            arrowTop = `${clampedArrowY}px`;
        } else {
            // Posicionamiento vertical arriba/abajo (Fallback en pantallas estrechas)
            const targetCenterY = rect.top + (rect.height / 2);
            const placeAbove = targetCenterY > (viewportHeight / 2);

            if (placeAbove) {
                cardTop = boxTop - cardHeight - 14;
                if (cardTop < 10) {
                    cardTop = boxTop + boxHeight + 14;
                    arrowClass = 'arrow-top';
                } else {
                    arrowClass = 'arrow-bottom';
                }
            } else {
                cardTop = boxTop + boxHeight + 14;
                if (cardTop + cardHeight > viewportHeight - 10) {
                    cardTop = boxTop - cardHeight - 14;
                    arrowClass = 'arrow-bottom';
                } else {
                    arrowClass = 'arrow-top';
                }
            }

            const targetCenterX = boxLeft + (boxWidth / 2);
            cardLeft = targetCenterX - (cardWidth / 2);
            cardLeft = Math.max(16, Math.min(viewportWidth - cardWidth - 16, cardLeft));

            arrowClass = placeAbove ? 'arrow-bottom' : 'arrow-top';
            const relativeArrowX = targetCenterX - cardLeft;
            const clampedArrowX = Math.max(20, Math.min(cardWidth - 20, relativeArrowX));
            arrowLeft = `${clampedArrowX}px`;
            arrowTop = '';
        }

        this.popoverCard.style.top = `${cardTop}px`;
        this.popoverCard.style.left = `${cardLeft}px`;
        this.popoverCard.style.transform = 'none';

        if (arrow) {
            arrow.className = `tour-arrow ${arrowClass}`;
            arrow.style.left = arrowLeft;
            arrow.style.top = arrowTop;
            arrow.style.right = '';
            arrow.style.bottom = '';
        }
    }

    updatePosition() {
        const step = this.steps[this.currentStep];
        if (!step) return;

        if (step.type === 'modal' || !step.target) {
            this.positionCentered();
        } else {
            const targetEl = document.querySelector(step.target);
            if (targetEl) {
                this.positionSpotlight(targetEl, step.padding || 8, step.placement || 'auto');
            } else {
                this.positionCentered();
            }
        }
    }

    next() {
        if (this.currentStep < this.steps.length - 1) {
            this.renderStep(this.currentStep + 1);
        } else {
            this.close(true);
        }
    }

    prev() {
        if (this.currentStep > 0) {
            this.renderStep(this.currentStep - 1);
        }
    }

    close() {
        if (!this.isActive) return;

        // Limpieza de cualquier estado previo para asegurar que siempre salte de nuevo
        try {
            localStorage.removeItem(this.STORAGE_KEY);
        } catch (e) {
            // Ignorar restricciones de storage
        }

        document.body.classList.remove('tour-active');
        this.unbindEvents();

        if (this.overlay) {
            this.overlay.classList.remove('active');
            setTimeout(() => {
                if (this.overlay) {
                    this.overlay.remove();
                    this.overlay = null;
                    this.spotlightBox = null;
                    this.popoverCard = null;
                }
            }, 200);
        }

        this.isActive = false;

        const btnManual = document.getElementById('btnOpenTour');
        if (btnManual) btnManual.focus();
    }
}

// Inicialización global
document.addEventListener('DOMContentLoaded', () => {
    window.gotchiTourInstance = new GotchiTour();
});
