# =============================================================================
# CONFIGURACIÓ GLOBAL - TECLA Professional
# =============================================================================

# Control de octava i modes especials
octava = 5
octava_anterior = 0  # Guarda la octava anterior al activar el modo caos
caos = 0
caos_note = 0
rio_base = 64

# Estats del sistema
loop_mode = 0
configout = 0  # 0=mode, 1=duty1, 2=duty2, 3=duty3, 4=harm_base, 5=harm1, 6=harm2
last_interaction_time = 0.0

# Control de polsació llarga botons
button_hold_times = [0.0] * 6  # Temps inici polsació per cada botó
button_long_press_triggered = [False] * 6  # Si polsació llarga activada
show_full_summary = False  # Mostrar resum complet (Extra1 mantingut)

# Control d'harmonies i cicles de treball
duty1 = 50  # Cicle de treball PWM1 (1-99%)
duty2 = 50  # Cicle de treball PWM2 (1-99%)
duty3 = 50  # Cicle de treball PWM3 (1-99%)
freqharm_base = 0  # Harmònic aplicat a la portadora (PWM1)
freqharm1 = 0      # Harmònic aplicat a PWM2
freqharm2 = 0      # Harmònic aplicat a PWM3
cv1_range_config = 0  # CV1: 0=3.3V, 1=2.5V, 2=1.5V, 3=1.0V, 4=0.5V
cv2_range_config = 0  # CV2: 0=3.3V, 1=2.5V, 2=1.5V, 3=1.0V, 4=0.5V

# =============================================================================
# CLOCK INPUT SYSTEM - CV1 pot actuar com entrada de clock extern
# =============================================================================
clock_mode = False           # True quan clock extern detectat, False = tempo intern (pot)
clock_last_state = False     # Estat anterior del clock (HIGH/LOW)
clock_rising_edge = False    # Flanc pujant detectat aquest frame
clock_last_pulse_time = 0.0  # Temps de l'últim pols detectat
clock_threshold_high = 2.0   # Voltatge per sobre = HIGH (rising edge)
clock_threshold_low = 0.5    # Voltatge per sota = LOW (reset per proper rising edge)
CLOCK_TIMEOUT_MS = 2000      # Si no hi ha polsos en 2s, torna a mode intern

# Sistema d'acceleració exponencial per configuració
config_acceleration = 1  # Factor d'acceleració (1, 2, 4, 8)
config_hold_time = 0.0   # Temps que porta premut el botó

# Indicadors dinàmics per LEDs (duty/harm)
led_pulse_counter = 0.0
led_pulse_phase = 0.0
led_last_state = {
    'duty': (50, 50, 50),
    'harmonics': (0, 0, 0)
}
led_mode_step = 0
last_loop_led_time = 0.0

def duty_percent_to_cycle(percent):
    """Converteix duty cycle de percentatge (1-99) a valor PWM (655-64880)"""
    return int((percent / 100.0) * 65535)

# Variables de temps i seqüència
iteration = 0
position = 0
playing_notes = set()
nota_actual = 0
nota_tocada_ara = False  # Per raig caos només quan nota sonag no bloquejant
last_note_time = 0.0
next_note_time = 0.0
last_button_check = 0.0
last_display_update = 0.0
last_input_sample = 0.0  # Nueva variable para muestreo desacoplado
next_calibration_frame = 0.0  # Temps objectiu per refresc de pantalla en mode calibratge
current_sleep_time = 0.3  # Període actual entre notes segons BPM (actualitzat cada iteració)
x, y, z = 0.0, 0.0, 0.0  # Variables globales para inputs
cx, cy = 0.0, 0.0  # Coordenadas para Mandelbrot
bpm_raw = 120  # BPM sense filtratge
filtered_bpm = None  # BPM suavitzat
bpm = 120  # BPM actual (arrodonit)
filtered_sleep_time = 0.25
bpm_smoothing = 0.4  # Més reactiu (abans 0.1 era massa lent)
bpm_voltage_filtered = None
bpm_voltage_smoothing = 0.5  # Transicions més ràpides del slider (abans 0.15)
bpm_min = 20
bpm_max = 220
bpm_curve = 1.0

# Control del bucle principal
loop_sleep_min = 0.0005
loop_sleep_max = 0.005
loop_sleep_factor = 0.05  # Percentatge del període actual aplicat com a descans mínim

# Animació LEDs idle
led_idle_step = 0
last_led_animation_time = 0.0

# Debouncing mejorado (11ms per evitar rebotes)
button_debounce_time = [0] * 6
debounce_delay = 0.011  # 11ms en lloc de 10ms

# Mode calibratge
calibration_mode = False

# CV input ranges (calibrables per CV1 i CV2 independents)
cv1_min, cv1_max = 0.0, 3.3
cv2_min, cv2_max = 0.0, 3.3

# Presets de rangs CV segons cv1/cv2_range_config
CV_RANGE_PRESETS = {
    0: (0.0, 3.3, "0-3.3V"),   # Full range
    1: (0.0, 2.5, "0-2.5V"),   # Eurorack
    2: (0.0, 1.5, "0-1.5V"),   # Low voltage
    3: (0.0, 1.0, "0-1.0V"),   # Very low
    4: (0.0, 0.5, "0-0.5V"),   # Minimal
}

def apply_cv1_range_preset():
    """Aplica el preset de rang CV1"""
    global cv1_min, cv1_max
    min_v, max_v, _ = CV_RANGE_PRESETS[cv1_range_config]
    cv1_min = min_v
    cv1_max = max_v

def apply_cv2_range_preset():
    """Aplica el preset de rang CV2"""
    global cv2_min, cv2_max
    min_v, max_v, _ = CV_RANGE_PRESETS[cv2_range_config]
    cv2_min = min_v
    cv2_max = max_v

# Control de visualització optimitzat
show_config_mode = False
config_display_timer = 0

# Estats per algorismes específics
state_harmony = {
    'previous_note': 60,
    'last_profile': 0,
    'last_tension': 0,
    'initialized': False
}

# Estat específic per al mode Euclidia simplificat
state_euclid = {
    'initialized': False,
    'pattern': [],
    'accent_map': [],
    'pulses': -1,
    'position': 0,
    'degree': 0,
    'direction': 1,
    'previous_note': None,
    'accent_level': -1,
}

# Estat específic per al mode Espiral
state_espiral = {
    'initialized': False,
    'transposicio': 0,
    'cicle_counter': 0,
}

# Estat específic per al mode Contrapunt
state_contrapunt = {
    'initialized': False,
    'beat_counter': 0,
    'degree': 0,
}

# Estat específic per al mode Narval
state_narval = {
    'initialized': False,
    'grau_escala': 0,         # Posició actual a l'escala pentatònica
    'nota_base': 60,          # Nota MIDI base (Do central)
}


# Rangs i escales per conversió de voltatge
pot_min, pot_max = 0.0, 3.3
step = (pot_max - pot_min) / 127.0
step_melo = (pot_max - pot_min) / 10.0
step_escala = (pot_max - pot_min) / 6.0
step_control = (pot_max - pot_min) / 50.0
step_nota = (pot_max - pot_min) / 23.0
step_ritme = (pot_max - pot_min) / 36.0

# Sistema RTOS - Control de Gate/Trigger temporal
gate_active = False
gate_off_time = 0.0
gate_duration = 0.020  # Duració variable segons mode
note_off_schedule = {}  # Mapa nota->temps_off per NoteOff programats

# Duracions segures per a NoteOff (clamp per mantenir consistència)
NOTE_OFF_MIN_DURATION = 0.02
NOTE_OFF_MAX_DURATION = 1.0
NOTE_OFF_DEFAULT_RATIO = 0.7  # 70% del gate (abans 90%)

# Gate percentatges per cada loop mode (% del període entre notes)
# Això assegura que el gate és proporcional al BPM
GATE_PERCENTAGES = {
    0: 0.05,   # Pausa - 5% del període
    1: 0.10,   # Fractal - 10% (atac ràpid)
    2: 0.20,   # Riu - 20% (fluid, sostingut)
    3: 0.08,   # Tempesta - 8% (molt ràpid, percussiu)
    4: 0.15,   # Harmonia - 15% (mig, musical)
    5: 0.18,   # Bosc - 18% (orgànic)
    6: 0.12,   # Escala - 12% (precís)
    7: 0.10,   # Euclidia - 10% (rítmic)
    8: 0.14,   # Cosmos - 14% (espacial)
}

# Límits de gate per seguretat (en segons)
GATE_MIN_DURATION = 0.005  # 5ms mínim (sempre perceptible)
GATE_MAX_DURATION = 0.200  # 200ms màxim (evita gates eternament llargs a BPM baixos)

def get_gate_duration_for_mode(mode, gate_ms):
    """
    Retorna la duració del gate en segons.
    
    Args:
        mode: Mode musical actiu (0-8) [no utilitzat, mantingut per compatibilitat]
        gate_ms: Duració del gate en mil·lisegons (ja calculada pels modes)
    
    Returns:
        Duració del gate en segons, limitada entre 10ms i 2000ms per seguretat
    """
    # Convertir ms a segons amb límits de seguretat
    gate_seconds = gate_ms / 1000.0
    return max(0.01, min(2.0, gate_seconds))  # Mínim 10ms, màxim 2s

# Estadística i gestió d'errors
last_midi_error = None
midi_error_count = 0

# Control de pausa després d'errors crítics
error_pause_until = 0.0

# Paràmetres de refresc per calibratge (evita sleeps bloquejants)
calibration_frame_interval = 0.05
