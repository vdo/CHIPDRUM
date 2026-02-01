# =============================================================================
# BUTTON HANDLER - TECLA
# =============================================================================
import time
import random

# Constants de temps per polsacions llargues - Temps Humà Natural
LONG_PRESS_SUMMARY = 1.5  # 1.5s - Mostrar resum complet (Extra1) - més deliberat
LONG_PRESS_PAUSE = 1.5    # 1.5s - Pausa total/stop (Extra2) - evita stops accidentals

CONFIG_OPTION_COUNT = 7  # Mode, 3 duty i 3 harmònics

# Debounce times globals
_debounce = {}
for i in range(6):
    _debounce[i] = 0.0

def boton_presionado(boton, idx, tiempo_espera=0.08):  # 80ms debounce més natural
    """Verifica botó amb debounce NO BLOQUEJANT"""
    ct = time.monotonic()
    if boton.value and ct - _debounce[idx] > tiempo_espera:
        _debounce[idx] = ct
        return True
    return False

def process_buttons(hw, cfg, rtos, current_time):
    """Processa tots els botons amb prioritats RTOS"""
    
    # MODE CALIBRACIÓ: Botons extra 1 + 2 premuts junts
    if hw.boton_extra_1.value and hw.boton_extra_2.value:
        # Debounce per evitar canvis múltiples
        if current_time - _debounce.get(10, 0) > 0.5:  # 500ms debounce per calibració
            cfg.calibration_mode = not cfg.calibration_mode
            cfg.last_interaction_time = current_time
            _debounce[10] = current_time
        return
    
    # Botó extra 1: Ciclar configuració
    if hw.boton_extra_1.value and not cfg.calibration_mode:
        if cfg.button_hold_times[4] == 0:
            cfg.button_hold_times[4] = current_time
        
        hold_duration = current_time - cfg.button_hold_times[4]
        
        # Polsació llarga: mostrar resum complet
        if hold_duration > LONG_PRESS_SUMMARY and not cfg.button_long_press_triggered[4]:
            cfg.show_full_summary = True
            cfg.button_long_press_triggered[4] = True
    elif cfg.button_hold_times[4] > 0:
        # Deixa anar: Mode normal
        if not cfg.button_long_press_triggered[4]:
            # Ciclar configout (si estem en mode 0, mantenir configout=0)
            if cfg.loop_mode == 0:
                cfg.configout = 0  # Mode 0: només canviar modes
            else:
                cfg.configout = (cfg.configout + 1) % CONFIG_OPTION_COUNT
            cfg.show_config_mode = True
            cfg.config_display_timer = 0
        else:
            cfg.show_full_summary = False
        cfg.button_hold_times[4] = 0
        cfg.button_long_press_triggered[4] = False
        cfg.last_interaction_time = current_time
    
    # Botó extra 2: Curta=enrere config, Llarga(>1s)=pausa/stop
    if hw.boton_extra_2.value and not cfg.calibration_mode:
        if cfg.button_hold_times[5] == 0:
            cfg.button_hold_times[5] = current_time
        
        hold_duration = current_time - cfg.button_hold_times[5]
        
        # Polsació llarga: pausa total
        if hold_duration > LONG_PRESS_PAUSE and not cfg.button_long_press_triggered[5]:
            cfg.loop_mode = 0
            cfg.configout = 0  # Forçar mode de selecció de modes
            rtos.stop_all_notes()
            cfg.iteration = 0
            cfg.caos = 0
            hw.all_leds_off()
            cfg.button_long_press_triggered[5] = True
    elif cfg.button_hold_times[5] > 0:
        # Deixa anar: si no era llarga
        if not cfg.button_long_press_triggered[5]:
            # Rotar enrere configout (SEMPRE mantenir configout=0 si estem en mode 0)
            if cfg.loop_mode == 0:
                cfg.configout = 0  # Mode 0: només canviar modes
            else:
                cfg.configout = (cfg.configout - 1) % CONFIG_OPTION_COUNT
            cfg.show_config_mode = True
            cfg.config_display_timer = 0
        else:
            # Si acabem de fer pausa llarga, assegurar configout=0
            cfg.configout = 0
        cfg.button_hold_times[5] = 0
        cfg.button_long_press_triggered[5] = False
        cfg.last_interaction_time = current_time
    
# Botó cruceta 1: ↑ Pujar octava
    if boton_presionado(hw.boton_crueta_1, 0):
        # NO actualitzar last_interaction_time (no canvia pantalla)
        # Pujar octava
        if cfg.octava < 8:
            cfg.octava += 1
            cfg.caos = 0
        else:
            # Al activar/desactivar el modo caos, guardar/restaurar la octava
            if cfg.caos == 0:
                # Guardar la octava actual y activar modo caos
                cfg.octava_anterior = cfg.octava
                cfg.octava = random.randint(0, 8)  # Octava aleatoria inicial
                cfg.caos = 1
            else:
                # Restaurar la octava anterior y desactivar modo caos
                cfg.octava = cfg.octava_anterior
                cfg.caos = 0
    
    # Botó cruceta 2: ↓ Baixar octava
    if boton_presionado(hw.boton_crueta_2, 1):
        # NO actualitzar last_interaction_time (no canvia pantalla)
        # Baixar octava
        if cfg.octava > 0:
            cfg.octava -= 1
            cfg.caos = 0
        else:
            # Al activar/desactivar el modo caos, guardar/restaurar la octava
            if cfg.caos == 0:
                # Guardar la octava actual y activar modo caos
                cfg.octava_anterior = cfg.octava
                cfg.octava = random.randint(0, 8)  # Octava aleatoria inicial
                cfg.caos = 1
            else:
                # Restaurar la octava anterior y desactivar modo caos
                cfg.octava = cfg.octava_anterior
                cfg.caos = 0
    
    # Botó cruceta 3: Decrementar valor amb acceleració
    if hw.boton_crueta_3.value and not cfg.calibration_mode:
        first_press = (cfg.button_debounce_time[2] == 0)
        if first_press:
            cfg.button_debounce_time[2] = current_time
            cfg.config_hold_time = 0.0

        # Calcular acceleració exponencial - Progressió més natural
        cfg.config_hold_time = current_time - cfg.button_debounce_time[2]
        if cfg.config_hold_time > 2.0:      # Acceleració màxima més tard
            cfg.config_acceleration = 8
        elif cfg.config_hold_time > 1.5:    # Progressió més gradual
            cfg.config_acceleration = 4
        elif cfg.config_hold_time > 0.8:    # Temps més natural abans d'accelerar
            cfg.config_acceleration = 2
        else:
            cfg.config_acceleration = 1

        cfg.last_interaction_time = current_time

        if cfg.configout == 0:
            # MODE CANVI: Actua immediatament al primer press, després repeteix cada 150ms
            if first_press or (current_time - cfg.button_debounce_time[2] > 0.15):
                cfg.loop_mode = (cfg.loop_mode - 1) if cfg.loop_mode > 1 else 14
                cfg.configout = 0  # Mantenir en mode selecció de modes
                rtos.stop_all_notes()
                if not first_press:
                    cfg.button_debounce_time[2] = current_time  # Reset debounce for repeats
        elif cfg.configout == 1:
            cfg.duty1 = max(1, cfg.duty1 - cfg.config_acceleration)
            rtos.stop_all_notes()  # Apagar gate al canviar duty
        elif cfg.configout == 2:
            cfg.duty2 = max(1, cfg.duty2 - cfg.config_acceleration)
            rtos.stop_all_notes()  # Apagar gate al canviar duty
        elif cfg.configout == 3:
            cfg.duty3 = max(1, cfg.duty3 - cfg.config_acceleration)
            rtos.stop_all_notes()  # Apagar gate al canviar duty
        elif cfg.configout == 4:
            cfg.freqharm_base = (cfg.freqharm_base - 1) % 13
            rtos.stop_all_notes()  # Apagar gate al canviar harmònic
        elif cfg.configout == 5:
            cfg.freqharm1 = (cfg.freqharm1 - 1) % 13
            rtos.stop_all_notes()  # Apagar gate al canviar harmònic
        elif cfg.configout == 6:
            cfg.freqharm2 = (cfg.freqharm2 - 1) % 13
            rtos.stop_all_notes()  # Apagar gate al canviar harmònic
        
        cfg.show_config_mode = True
        cfg.config_display_timer = 0
    else:
        cfg.button_debounce_time[2] = 0
        cfg.config_acceleration = 1
    
    # Botó cruceta 4: Incrementar valor amb acceleració
    if hw.boton_crueta_4.value and not cfg.calibration_mode:
        first_press = (cfg.button_debounce_time[3] == 0)
        if first_press:
            cfg.button_debounce_time[3] = current_time
            cfg.config_hold_time = 0.0

        # Calcular acceleració exponencial - Progressió més natural
        cfg.config_hold_time = current_time - cfg.button_debounce_time[3]
        if cfg.config_hold_time > 2.0:      # Acceleració màxima més tard
            cfg.config_acceleration = 8
        elif cfg.config_hold_time > 1.5:    # Progressió més gradual
            cfg.config_acceleration = 4
        elif cfg.config_hold_time > 0.8:    # Temps més natural abans d'accelerar
            cfg.config_acceleration = 2
        else:
            cfg.config_acceleration = 1

        cfg.last_interaction_time = current_time

        if cfg.configout == 0:
            # MODE CANVI: Actua immediatament al primer press, després repeteix cada 150ms
            if first_press or (current_time - cfg.button_debounce_time[3] > 0.15):
                cfg.loop_mode = (cfg.loop_mode + 1) if cfg.loop_mode < 14 else 1
                cfg.configout = 0  # Mantenir en mode selecció de modes
                rtos.stop_all_notes()
                if not first_press:
                    cfg.button_debounce_time[3] = current_time  # Reset debounce for repeats
        elif cfg.configout == 1:
            cfg.duty1 = min(99, cfg.duty1 + cfg.config_acceleration)
            rtos.stop_all_notes()  # Apagar gate al canviar duty
        elif cfg.configout == 2:
            cfg.duty2 = min(99, cfg.duty2 + cfg.config_acceleration)
            rtos.stop_all_notes()  # Apagar gate al canviar duty
        elif cfg.configout == 3:
            cfg.duty3 = min(99, cfg.duty3 + cfg.config_acceleration)
            rtos.stop_all_notes()  # Apagar gate al canviar duty
        elif cfg.configout == 4:
            cfg.freqharm_base = (cfg.freqharm_base + 1) % 13
            rtos.stop_all_notes()  # Apagar gate al canviar harmònic
        elif cfg.configout == 5:
            cfg.freqharm1 = (cfg.freqharm1 + 1) % 13
            rtos.stop_all_notes()  # Apagar gate al canviar harmònic
        elif cfg.configout == 6:
            cfg.freqharm2 = (cfg.freqharm2 + 1) % 13
            rtos.stop_all_notes()  # Apagar gate al canviar harmònic
        
        cfg.show_config_mode = True
        cfg.config_display_timer = 0
    else:
        cfg.button_debounce_time[3] = 0
        cfg.config_acceleration = 1
