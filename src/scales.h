#pragma once
float midi_to_freq(float midi);
// Writes e.g. "C#2" into buf (at least 5 bytes)
void note_name(int midi, char *buf);

// Slider position (0..1) to tempo. Two linear segments meeting at BPM_CENTER
// so that 120 BPM lands exactly at the middle of the slider's throw.
float slider_to_bpm(float norm);
