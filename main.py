# =============================================================================
# TECLA - SISTEMA MODULAR PROFESSIONAL
# Arquitectura optimitzada amb RTOS, CV calibrable i gate variable
# =============================================================================
print("🚀 TECLA Professional - Iniciant...")

import time
import random
from core.hardware import TeclaHardware
from core import config as cfg
from core.rtos import RTOSManager
from core.midi_handler import MidiHandler
from core import button_handler, calibration
from core.clock import MasterClock
from display.screens import ScreenManager
from display.animations import Animations
from music.converters import (
    voltage_to_bpm,
    map_value,
    midi_to_note_name,
    get_voltage_calibrated,
    smooth_value,
)
from modes.loader import ModeLoader

print("✅ Mòduls importats")

# =============================================================================
# INICIALITZACIÓ
# =============================================================================
try:
    hw = TeclaHardware()
    print("✅ Hardware inicialitzat")
    
    rtos = RTOSManager(hw, cfg)
    midi_handler = MidiHandler(hw, cfg)
    screen = ScreenManager(hw, cfg)
    anim = Animations(hw, cfg)
    mode_loader = ModeLoader(hw, cfg, midi_handler)
    clock = MasterClock(cfg)
    print("✅ Gestors creats")
    
    # Temps inicials
    cfg.last_note_time = time.monotonic()
    cfg.next_note_time = cfg.last_note_time
    cfg.last_display_update = cfg.last_note_time
    cfg.last_button_check = cfg.last_note_time
    cfg.last_interaction_time = cfg.last_note_time
    cfg.last_input_sample = cfg.last_note_time
    cfg.next_calibration_frame = cfg.last_note_time
    
    # ========================================================================
    # PROVA INICIAL DE LEDS - Verificar funcionament
    # ========================================================================
    print("🔦 Prova LEDs: Encenent tots els LEDs...")
    hw.all_leds_on()
    time.sleep(1.0)  # 1 segon amb tots encesos
    hw.all_leds_off()
    time.sleep(0.3)
    print("✅ Prova LEDs completada")
    
    # ========================================================================
    # ANIMACIÓ ÈPICA D'INICI - 3 SEGONS
    # ========================================================================
    hw.led_startup_animation()
    
    # FASE 1: EXPLOSIÓ DE PARTÍCULES (1s)
    for frame in range(12):
        hw.display.fill(0)
        progress = frame / 12.0
        
        # Partícules que surten del centre
        import random
        for _ in range(int(progress * 40)):
            angle = random.uniform(0, 6.28)  # 2π
            import math
            distance = progress * 60
            px = int(64 + distance * math.cos(angle))
            py = int(32 + distance * math.sin(angle))
            if 0 <= px < 128 and 0 <= py < 64:
                hw.display.pixel(px, py, 1)
                # Esteles de moviment
                px2 = int(64 + distance * 0.7 * math.cos(angle))
                py2 = int(32 + distance * 0.7 * math.sin(angle))
                if 0 <= px2 < 128 and 0 <= py2 < 64:
                    hw.display.pixel(px2, py2, 1)
        
        # Text "TECLA" apareix gradualment
        if progress > 0.5:
            alpha = (progress - 0.5) / 0.5
            if alpha > 0.2:
                hw.display.text("TECLA", 49, 28, 1)
        
        hw.display.show()
        time.sleep(0.08)  # 12×0.08 = 0.96s
    
    # FASE 2: RAIG ELÈCTRIC (0.6s)
    for frame in range(6):
        hw.display.fill(0)
        
        # Raigs que creuen tota la pantalla
        import random
        for r in range(3):
            x, y = 64, random.randint(5, 25)
            for i in range(8):
                dx = random.randint(-8, 8)
                dy = random.randint(4, 8)
                hw.display.line(x, y, x + dx, y + dy, 1)
                x, y = x + dx, y + dy
                if y > 60:
                    break
        
        # Flash en frames parells
        if frame % 2 == 0:
            hw.display.text("TECLA", 49, 28, 1)
            # Cercle d'impacte
            hw.display.circle(64, 32, 15 + frame * 3, 1)
        
        hw.display.show()
        time.sleep(0.1)  # 6×0.1 = 0.6s
    
    # FASE 3: ONES EXPANSIVES + "CHIPTUNE" (1s)
    for frame in range(15):
        hw.display.fill(0)
        progress = frame / 15.0
        
        # Ones concèntriques que s'expandeixen
        import math
        for ring in range(5):
            radius = int((progress + ring * 0.2) * 40)
            if radius < 50 and radius > 0:
                # Cercle amb punts
                for angle in range(0, 360, 10):
                    rad = math.radians(angle)
                    px = int(64 + radius * math.cos(rad))
                    py = int(32 + radius * math.sin(rad))
                    if 0 <= px < 128 and 0 <= py < 64:
                        hw.display.pixel(px, py, 1)
        
        # Text "CHIPTUNE" amb efecte d'escaneig
        if progress > 0.3:
            hw.display.text("CHIPTUNE", 40, 28, 1)
            # Línia d'escaneig
            scan_y = int(20 + progress * 30)
            hw.display.hline(0, scan_y, 128, 1)
        
        hw.display.show()
        time.sleep(0.067)  # 15×0.067 = 1.0s
    
    # FASE 4: FADE OUT AMB ESTRELLES (0.4s)
    for frame in range(5):
        hw.display.fill(0)
        
        # Estrelles aleatòries
        import random
        for _ in range(20):
            sx = random.randint(0, 127)
            sy = random.randint(0, 63)
            hw.display.pixel(sx, sy, 1)
        
        # Text final
        if frame < 3:
            hw.display.text("CHIPTUNE", 40, 28, 1)
        
        hw.display.show()
        time.sleep(0.08)  # 5×0.08 = 0.4s
    
    time.sleep(0.04)  # Pausa final
    
    print("✅ Sistema preparat - Arquitectura Modular Activa")
    print("")
    
except Exception as e:
    print(f"❌ Error inicialització: {e}")
    import traceback
    traceback.print_exception(e)
    while True:
        time.sleep(1)

# =============================================================================
# BUCLE PRINCIPAL - ARQUITECTURA RTOS AMB PRIORITATS
# =============================================================================
print("🔄 Bucle principal actiu")
iteration_count = 0

while True:
    try:
        current_time = time.monotonic()
        
        # ===== PRIORITAT MÀXIMA: RTOS (Gate temporal + NoteOff) =====
        rtos.update(current_time)  # Passar current_time (optimització: evita crida redundant)
        
        # ===== PRIORITAT ALTA: Lectura inputs usuari =====
        # Pins (NOU MAPATGE):
        #   Slider (GP28): x - Paràmetre 1 (NO calibrat, sempre 0-3.3V)
        #   CV1/Pote (GP26): Tempo (pot) o Clock input (jack extern)
        #   CV2/LDR (GP27): y - Paràmetre 2 (calibrat amb cv2_min/max)

        x_raw = hw.get_voltage(hw.slider)      # Slider (GP28) - Param 1
        tempo_raw = hw.get_voltage(hw.cv1_pote)  # CV1 (GP26) - Tempo/Clock
        y_raw = hw.get_voltage(hw.cv2_ldr)     # CV2 (GP27) - Param 2

        # Aplicar calibració al CV2/LDR, slider és paràmetre 1 sense calibrar
        x = x_raw / 3.3  # Normalitzar slider a 0-1 per modes
        y = get_voltage_calibrated(y_raw, cfg.cv2_min, cfg.cv2_max)

        # ===== DETECCIÓ CLOCK EXTERN a CV1 =====
        # Si detectem flancs pujants, estem rebent clock extern
        cfg.clock_rising_edge = False
        cv1_high = tempo_raw > cfg.clock_threshold_high
        cv1_low = tempo_raw < cfg.clock_threshold_low

        # Detectar flanc pujant (LOW→HIGH)
        if cv1_high and not cfg.clock_last_state:
            cfg.clock_rising_edge = True
            cfg.clock_last_pulse_time = current_time
            cfg.clock_mode = True  # Un cop detectat clock, quedem en mode clock

        # Actualitzar estat (amb histèresi)
        if cv1_high:
            cfg.clock_last_state = True
        elif cv1_low:
            cfg.clock_last_state = False

        # Mode clock: un cop activat, NO torna a mode intern
        # Si no hi ha polsos, simplement no avancem (esperem clock)

        # BPM calculations amb CV1 (només si NO estem en mode clock)
        if not cfg.clock_mode:
            cfg.bpm_voltage_raw = tempo_raw
            thr_filter = smooth_value(cfg.bpm_voltage_filtered, tempo_raw, cfg.bpm_voltage_smoothing)
            cfg.bpm_voltage_filtered = thr_filter
            raw_bpm = voltage_to_bpm(
                thr_filter,
                pot_min=0.0,
                pot_max=3.3,
                bpm_min=cfg.bpm_min,
                bpm_max=cfg.bpm_max,
                curve=cfg.bpm_curve,
            )
            sleep_time = clock.update(raw_bpm, current_time)
        else:
            # En mode clock extern, mantenir l'últim sleep_time per gate duration
            sleep_time = cfg.current_sleep_time

        # Guardar voltatges per als modes
        cfg.x, cfg.y, cfg.z = x, y, tempo_raw

        # Coordenades fractals: x normalitzat (0-1), y calibrat
        cx = map_value(x, 0.0, 1.0, -1.5, 1.5)
        cy = map_value(y, cfg.cv2_min, cfg.cv2_max, -1.5, 1.5)
        cfg.cx, cfg.cy = cx, cy
        
        # Variables aleatòries per caos
        cfg.caos_note = random.randint(0, 1)
        
        # ===== PRIORITAT ALTA: Detecció botons (cada 5ms) =====
        if current_time - cfg.last_button_check > 0.005:
            cfg.last_button_check = current_time
            button_handler.process_buttons(hw, cfg, rtos, current_time)
        
        error_block_active = current_time < cfg.error_pause_until

        # ===== PRIORITAT ALTA: Execució modes musicals =====
        if cfg.loop_mode == 0 or error_block_active:
            # Mode parada o pausa per error
            if cfg.playing_notes:
                midi_handler.all_notes_off()
            hw.pwm1.duty_cycle = 0
            hw.pwm2.duty_cycle = 0
            hw.pwm3.duty_cycle = 0
        elif cfg.loop_mode > 0:
            # Consumir ticks segons mode:
            # - Mode clock extern: només un tick per cada pols (rising edge)
            # - Mode intern (pot): ticks segons BPM
            if cfg.clock_mode:
                # Mode clock extern: només avancem si hi ha rising edge
                if cfg.clock_rising_edge:
                    ticks = [current_time]
                else:
                    ticks = []  # Esperem clock, no avancem
            else:
                # Mode intern: ticks segons BPM del pot
                ticks = clock.consume_ticks(current_time)

            for tick_time in ticks:
                cfg.next_note_time = tick_time + sleep_time
                mode_loader.execute_mode(cfg.loop_mode, x, y, sleep_time, cx, cy)
                if cfg.loop_mode not in [6, 8]:
                    cfg.iteration = (cfg.iteration + 1) % 60000
        
        # Actualitzar LEDs de configuració (sense animacions dinàmiques)
        hw.update_config_led_indicators(cfg)
        
        # ===== PRIORITAT BAIXA: Actualització display =====
        if cfg.calibration_mode:
            calibration.procesar_calibracion(hw, cfg)
            if current_time >= cfg.next_calibration_frame:
                cfg.next_calibration_frame = current_time + cfg.calibration_frame_interval
                screen.mostrar_calibracion_cv()
            
        elif current_time - cfg.last_display_update > 0.15:  # Optimitzat: 150ms (abans 100ms)
            inactive_time = current_time - cfg.last_interaction_time
            
            if cfg.show_full_summary:
                screen._mostrar_resum_complet()
            elif cfg.caos == 1 and cfg.nota_tocada_ara:
                screen.mostrar_info_loop_mode()
                anim.dibujar_rayo_simple()
                hw.display.show()
                cfg.nota_tocada_ara = False
            elif cfg.loop_mode == 0:
                anim.animacion_ojo()
            elif cfg.loop_mode > 0 and inactive_time > 999999:  # DESACTIVAT (abans 5.0s)
                # Animacions idle desactivades per performance
                screen.mostrar_info_loop_mode()  # Mostrar info normal
            else:
                screen.mostrar_info_loop_mode()
            
            cfg.last_display_update = current_time
        
        # Sleep mínim CPU (0.5ms per màxima responsivitat)
        clock.idle_sleep(current_time)
        
        # Debug cada 2000 iteracions (~4 segons)
        iteration_count += 1
        if iteration_count % 2000 == 0:
            try:
                note_name = midi_to_note_name(cfg.nota_actual)
            except Exception:
                note_name = "---"
            print(
                f"✅ {iteration_count} it | Mode:{cfg.loop_mode} Oct:{cfg.octava} "
                f"BPM:{cfg.bpm} Gate:{cfg.gate_duration*1000:.1f}ms Nota:{note_name}"
            )
        
    except KeyboardInterrupt:
        print("\n⚠️  Interrupció manual - Netejant...")
        rtos.stop_all_notes()
        midi_handler.all_notes_off()
        hw.all_leds_off()
        hw.display.fill(0)
        hw.display.text("STOPPED", 35, 28, 1)
        hw.display.show()
        break
        
    except Exception as e:
        print(f"❌ Error bucle: {e}")
        import traceback
        traceback.print_exception(e)
        
        # Mostrar error en pantalla
        hw.display.fill(0)
        hw.display.text("ERROR!", 40, 15, 1)
        error_str = str(e)[:20]
        hw.display.text(error_str, 0, 30, 1)
        hw.display.text("Check console", 10, 45, 1)
        hw.display.show()
        
        # Neteja i espera
        rtos.stop_all_notes()
        midi_handler.all_notes_off()
        cfg.error_pause_until = max(cfg.error_pause_until, time.monotonic() + 5.0)

print("🛑 TECLA Professional finalitzat")
 