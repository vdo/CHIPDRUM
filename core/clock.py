import time

from music.converters import bpm_to_sleep_time, smooth_value


class MasterClock:
    """Clock centralitzat que sincronitza totes les tasques segons BPM."""

    def __init__(self, config, max_catchup_ticks=5):  # Abans 3, ara 5 per millor recuperació
        self.cfg = config
        self.max_catchup_ticks = max_catchup_ticks
        initial_bpm = config.filtered_bpm if config.filtered_bpm else config.bpm
        self.filtered_bpm = max(1.0, float(initial_bpm))
        self.period = bpm_to_sleep_time(self.filtered_bpm)
        now = time.monotonic()
        self.last_tick = now
        self.next_tick = now + self.period

    def update(self, raw_bpm, current_time):
        """Actualitza el període segons el BPM mesurat amb suavitzat."""
        filtered = smooth_value(self.filtered_bpm, raw_bpm, self.cfg.bpm_smoothing)
        if filtered is None:
            filtered = raw_bpm
        filtered = max(1.0, float(filtered))

        self.filtered_bpm = filtered
        self.period = bpm_to_sleep_time(filtered)

        # Actualitzar estat global per a mòduls dependents
        self.cfg.bpm_raw = raw_bpm
        self.cfg.filtered_bpm = filtered
        self.cfg.current_sleep_time = self.period
        # filtered_sleep_time eliminat (redundant)
        self.cfg.bpm = int(round(filtered))

        # Evitar que un canvi brusc deixi la següent nota massa llunyana
        if self.next_tick - current_time > self.period * 2:
            self.next_tick = current_time + self.period

        return self.period

    def consume_ticks(self, current_time, active=True, external_tick=False):
        """Retorna una llista de ticks que s'han de disparar fins al temps actual.

        Args:
            current_time: Temps actual
            active: Si el clock està actiu (False = reset i no ticks)
            external_tick: Si True, retorna un tick immediat (clock extern)
        """
        if not active:
            self.last_tick = current_time
            self.next_tick = current_time + self.period
            return []

        # Mode clock extern: un tick per cada pols rebut
        if external_tick:
            self.last_tick = current_time
            self.next_tick = current_time + self.period  # Mantenir periode per si torna a intern
            return [current_time]

        # Mode intern: ticks segons BPM
        ticks = []
        while current_time >= self.next_tick and len(ticks) < self.max_catchup_ticks:
            tick_time = self.next_tick
            ticks.append(tick_time)
            self.last_tick = tick_time
            self.next_tick = tick_time + self.period

        # Si hem quedat massa enrere, resincronitzar per evitar bucles infinits
        if current_time - self.next_tick > self.period * self.max_catchup_ticks:
            self.last_tick = current_time
            self.next_tick = current_time + self.period

        return ticks

    def idle_sleep(self, current_time):
        """Sleep adaptatiu més agressiu per màxima responsivitat (optimitzat)."""
        remaining = self.next_tick - current_time
        if remaining > 0.002:  # Només sleep si queda >2ms (abans 1ms)
            # Sleep més agressiu: max 1ms, però proporcional al temps restant
            time.sleep(min(0.001, remaining * 0.3))  # Abans: 0.05

