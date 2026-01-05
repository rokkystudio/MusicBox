#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
mid2code.py

MIDI -> uint8_t stream (cmd, val) for MusicBox (новый формат).

Вывод:
    const uint8_t name[] PROGMEM =
    {
        TEMPO, 22, TRANS, 0,      // ~220 BPM
        C4F, L08, D4F, L08, ... ,  // такт 1
        ...
    };

Правила:
 - Моно: в каждый момент времени выбираем самую высокую ноту.
 - Длительность режется по ближайшему MIDI событию (note_on / переход в паузу).
 - TEMPO берём из set_tempo и вставляем TEMPO, tempo10 по позициям.
 - TRANS не извлекаем из MIDI: по умолчанию вставляем TRANS, 0 сразу после первого TEMPO.
 - Длительности раскладываются на "красивые" куски: 16,12,8,6,4,3,2,1 (L01,L2D,L02,L4D,L04,L8D,L08,L16).
 - Конец песни (PAUSE,0) НЕ добавляем (по договорённости).

ВАЖНО (фикс бага):
 - Одинаковые ноты подряд НЕ СКЛЕИВАЕМ в одну длинную ноту.
   Даже если они одинаковые, это должны быть отдельные "триггеры" ноты.
"""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from typing import Dict, List, Tuple, Set

try:
	import mido
except ImportError as e:
	raise SystemExit(
		"Не найден пакет 'mido'. Установи: pip install mido\n"
		f"Текст ошибки: {e}"
	)

#=====================================================================#
# Внутренние структуры
#=====================================================================#

@dataclass
class NoteSeg:
	start: int
	end: int
	note: int			# 0..127 (0 = пауза)

@dataclass
class TempoPoint:
	pos16: int
	tempo10: int
	bpm: float

@dataclass
class NoteMsg:
	t: int
	is_on: bool
	note: int
	vel: int
	ch: int

@dataclass
class OutItem:
	kind: str			# "note" | "tempo" | "trans"
	note_token: str = ""
	dur_token: str = ""
	dur16: int = 0
	tempo10: int = 0
	bpm: float = 0.0
	trans: int = 0


#=====================================================================#
# Общие хелперы
#=====================================================================#

def clamp(v: int, lo: int, hi: int) -> int:
	if v < lo:
		return lo
	if v > hi:
		return hi
	return v

def iter_msgs_with_abs_time(track: "mido.MidiTrack"):
	abs_t = 0
	for msg in track:
		abs_t += msg.time
		yield abs_t, msg

def quantize_tick(t: int, grid: int) -> int:
	return int(round(t / grid) * grid)

def read_time_signature(mid: "mido.MidiFile") -> Tuple[int, int]:
	for tr in mid.tracks:
		abs_t = 0
		for msg in tr:
			abs_t += msg.time
			if msg.type == "time_signature":
				return int(msg.numerator), int(msg.denominator)
	return 4, 4


#=====================================================================#
# MIDI note -> Music.h token (C4F/C4D...)
#=====================================================================#

_NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

def midi_note_to_token(note: int) -> str:
	n = int(note)
	if n <= 0:
		return "PAUSE"

	octave = (n // 12) - 1			# MIDI: C4=60 => octave=4
	sem = n % 12

	# Music.h покрывает C1..B7
	if octave < 1 or octave > 7:
		return str(n)

	name = _NOTE_NAMES[sem]
	if name.endswith("#"):
		base = name[0]
		return f"{base}{octave}D"
	return f"{name}{octave}F"


#=====================================================================#
# dur16 -> token + красивое разбиение
#=====================================================================#

def dur16_to_token(dur16: int) -> str:
	d = int(dur16)
	if d == 16:
		return "L01"
	if d == 12:
		return "L2D"
	if d == 8:
		return "L02"
	if d == 6:
		return "L4D"
	if d == 4:
		return "L04"
	if d == 3:
		return "L8D"
	if d == 2:
		return "L08"
	if d == 1:
		return "L16"
	return str(d)

_PREFERRED_DUR16 = [16, 12, 8, 6, 4, 3, 2, 1]

def split_dur16_pretty(dur16: int) -> List[int]:
	remaining = int(dur16)
	if remaining <= 0:
		return []

	out: List[int] = []
	while remaining > 0:
		for p in _PREFERRED_DUR16:
			if p <= remaining:
				out.append(p)
				remaining -= p
				break
		else:
			out.append(1)
			remaining -= 1
	return out


#=====================================================================#
# Сбор нот из трека
#=====================================================================#

def _append_note_msgs_from_track(track: "mido.MidiTrack",
								 out: List[NoteMsg]) -> None:
	for abs_t, msg in iter_msgs_with_abs_time(track):
		if msg.type not in ("note_on", "note_off"):
			continue

		ch = int(getattr(msg, "channel", 0))
		note = int(msg.note)
		vel = int(getattr(msg, "velocity", 0))

		if msg.type == "note_on" and vel > 0:
			out.append(NoteMsg(t=abs_t, is_on=True, note=note, vel=vel, ch=ch))
		else:
			out.append(NoteMsg(t=abs_t, is_on=False, note=note, vel=0, ch=ch))

def pick_first_track_with_notes(mid: "mido.MidiFile") -> Tuple[int, List[NoteMsg]]:
	for i, tr in enumerate(mid.tracks):
		msgs: List[NoteMsg] = []
		_append_note_msgs_from_track(tr, msgs)
		if msgs:
			# note_off раньше note_on на том же t (чтобы пауза могла начаться ровно в момент off)
			msgs.sort(key=lambda m: (m.t, 0 if not m.is_on else 1, m.note))
			return i, msgs
	return -1, []


#=====================================================================#
# Моно (highest) + сегменты по любому событию
#=====================================================================#

def merge_adjacent_segments(segs: List[NoteSeg],
							note_on_at: Set[Tuple[int, int]]) -> List[NoteSeg]:
	"""
	Склеиваем соседние сегменты одинаковой ноты, НО:
	если в точке стыка (t = cur.end = next.start) был note_on этой же ноты,
	то это реальный "повтор" и склеивать НЕЛЬЗЯ.
	"""
	if not segs:
		return []

	out: List[NoteSeg] = []
	cur = segs[0]

	for s in segs[1:]:
		if s.note == cur.note and s.start == cur.end:
			# Если на границе был note_on этой ноты — это повтор, не склеиваем.
			if (s.start, s.note) not in note_on_at:
				cur = NoteSeg(start=cur.start, end=s.end, note=cur.note)
				continue

		if cur.end > cur.start:
			out.append(cur)
		cur = s

	if cur.end > cur.start:
		out.append(cur)
	return out

def build_timeline_segments_highest(msgs: List[NoteMsg]) -> List[NoteSeg]:
	if not msgs:
		return []

	# Важный набор для фикса "повторов одинаковой ноты":
	# если в момент t есть note_on для ноты N, то два сегмента N, стыкующиеся в t, склеивать нельзя.
	note_on_at: Set[Tuple[int, int]] = set()
	for m in msgs:
		if m.is_on:
			note_on_at.add((m.t, m.note))

	active: Dict[int, int] = {}
	segs: List[NoteSeg] = []

	cur_note = 0
	cur_start = msgs[0].t

	i = 0
	n = len(msgs)

	while i < n:
		t = msgs[i].t

		if t > cur_start:
			segs.append(NoteSeg(start=cur_start, end=t, note=cur_note))
			cur_start = t

		batch: List[NoteMsg] = []
		while i < n and msgs[i].t == t:
			batch.append(msgs[i])
			i += 1

		# сначала off
		for m in batch:
			if not m.is_on:
				if m.note in active:
					del active[m.note]

		# потом on
		for m in batch:
			if m.is_on:
				active[m.note] = m.t

		cur_note = max(active.keys()) if active else 0

	segs = merge_adjacent_segments(segs, note_on_at)

	while segs and segs[0].note == 0:
		segs.pop(0)
	while segs and segs[-1].note == 0:
		segs.pop()

	return segs


#=====================================================================#
# TEMPO из MIDI
#=====================================================================#

def read_tempo_points(mid: "mido.MidiFile",
					  ticks_per_16: float,
					  base_tick: int) -> List[TempoPoint]:
	grid_ticks_i = int(round(ticks_per_16))
	if grid_ticks_i <= 0:
		grid_ticks_i = 1

	raw: List[Tuple[int, int, float]] = []

	for tr in mid.tracks:
		abs_t = 0
		for msg in tr:
			abs_t += msg.time
			if msg.type != "set_tempo":
				continue

			tempo_us = int(getattr(msg, "tempo", 0))
			if tempo_us <= 0:
				continue

			try:
				bpm = float(mido.tempo2bpm(tempo_us))
			except Exception:
				bpm = float(60_000_000.0 / float(tempo_us))

			tempo10 = int(round(bpm / 10.0))
			tempo10 = clamp(tempo10, 1, 25)

			rel_t = int(abs_t - base_tick)
			if rel_t < 0:
				rel_t = 0

			qt = quantize_tick(rel_t, grid_ticks_i)
			pos16 = int(round(float(qt) / float(ticks_per_16)))
			if pos16 < 0:
				pos16 = 0

			raw.append((pos16, tempo10, bpm))

	if not raw:
		return []

	raw.sort(key=lambda x: x[0])

	merged: List[TempoPoint] = []
	for pos16, tempo10, bpm in raw:
		if not merged:
			merged.append(TempoPoint(pos16=pos16, tempo10=tempo10, bpm=bpm))
			continue

		last = merged[-1]
		if pos16 == last.pos16:
			merged[-1] = TempoPoint(pos16=pos16, tempo10=tempo10, bpm=bpm)
			continue

		if tempo10 == last.tempo10:
			continue

		merged.append(TempoPoint(pos16=pos16, tempo10=tempo10, bpm=bpm))

	return merged


#=====================================================================#
# Segments -> events(note,dur16) квантизация по 1/16
#=====================================================================#

def segments_to_events(segs: List[NoteSeg],
					   ticks_per_16: float,
					   base_tick: int) -> List[Tuple[int, int]]:
	"""
	ВАЖНО:
	- Ничего НЕ склеиваем по одинаковым нотам.
	  Одинаковые ноты подряд могут быть "повторами" и должны оставаться отдельными.
	"""
	if not segs:
		return []

	grid_ticks_i = int(round(ticks_per_16))
	if grid_ticks_i <= 0:
		grid_ticks_i = 1

	events: List[Tuple[int, int]] = []

	for s in segs:
		rs = int(s.start - base_tick)
		re = int(s.end - base_tick)
		if rs < 0:
			rs = 0
		if re < 0:
			re = 0

		qs = quantize_tick(rs, grid_ticks_i)
		qe = quantize_tick(re, grid_ticks_i)
		if qe <= qs:
			qe = qs + grid_ticks_i

		dur16 = int(round((qe - qs) / ticks_per_16))
		if dur16 <= 0:
			dur16 = 1

		note = int(s.note)
		if note != 0:
			note = clamp(note, 1, 127)

		events.append((note, dur16))

	return events


#=====================================================================#
# Вставка TEMPO + красивое разбиение длительностей
#=====================================================================#

def append_note_items(out: List[OutItem], note: int, dur16: int) -> None:
	if dur16 <= 0:
		return

	note_token = "PAUSE" if note == 0 else midi_note_to_token(note)

	for d in split_dur16_pretty(dur16):
		out.append(OutItem(kind="note", note_token=note_token, dur_token=dur16_to_token(d), dur16=d))

def insert_tempo_into_events(events: List[Tuple[int, int]],
							 tempos: List[TempoPoint]) -> List[OutItem]:
	out: List[OutItem] = []
	if not events:
		return out

	tempos_sorted = sorted(tempos, key=lambda x: x.pos16)
	ti = 0
	pos = 0

	while ti < len(tempos_sorted) and tempos_sorted[ti].pos16 <= 0:
		tp = tempos_sorted[ti]
		out.append(OutItem(kind="tempo", tempo10=tp.tempo10, bpm=tp.bpm))
		ti += 1

	ei = 0
	while ei < len(events):
		note, dur = events[ei]
		if dur <= 0:
			ei += 1
			continue

		while ti < len(tempos_sorted) and tempos_sorted[ti].pos16 == pos:
			tp = tempos_sorted[ti]
			out.append(OutItem(kind="tempo", tempo10=tp.tempo10, bpm=tp.bpm))
			ti += 1

		next_pos = pos + dur

		if ti < len(tempos_sorted):
			tp = tempos_sorted[ti]
			if pos < tp.pos16 < next_pos:
				left = tp.pos16 - pos
				if left > 0:
					append_note_items(out, note, left)
					pos += left
					dur -= left
				continue

		append_note_items(out, note, dur)
		pos = next_pos
		ei += 1

	return out


#=====================================================================#
# Форматирование C-массива:
#  - первая строка: TEMPO, X, TRANS, 0
#  - такты отдельными строками
#  - TEMPO в середине песни: отдельной строкой
#=====================================================================#

def format_as_c_array(items: List[OutItem],
					  name: str,
					  time_sig: Tuple[int, int],
					  initial_tempo10: int,
					  initial_trans: int) -> str:
	num, den = time_sig
	bar_16_len = int(round(num * (16.0 / float(den))))
	if bar_16_len <= 0:
		bar_16_len = 16

	lines: List[str] = []
	lines.append(f"const uint8_t {name}[] PROGMEM =")
	lines.append("{")

	# Первая строка: TEMPO + TRANS
	lines.append(f"\tTEMPO, {initial_tempo10}, TRANS, {initial_trans},\t// ~{initial_tempo10 * 10} BPM")

	bar_sum = 0
	bar_idx = 1
	row: List[str] = []

	def flush_bar_row() -> None:
		nonlocal row, bar_sum, bar_idx
		if row:
			lines.append("\t" + ", ".join(row) + f",\t// такт {bar_idx}")
			row = []
			bar_sum = 0
			bar_idx += 1

	for it in items:
		if it.kind == "tempo":
			# TEMPO в середине песни: отдельная строка (не внутри такта)
			flush_bar_row()
			lines.append(f"\tTEMPO, {it.tempo10},\t// ~{it.tempo10 * 10} BPM")
			continue

		# note/pause
		if bar_sum > 0 and (bar_sum + it.dur16) > bar_16_len:
			flush_bar_row()

		row.append(f"{it.note_token}, {it.dur_token}")
		bar_sum += it.dur16

		if bar_sum >= bar_16_len:
			flush_bar_row()

	# хвост
	if row:
		lines.append("\t" + ", ".join(row) + f",\t// такт {bar_idx}")

	lines.append("};")
	return "\n".join(lines)


#=====================================================================#
# Inspect
#=====================================================================#

def inspect_midi(mid_path: str) -> None:
	mid = mido.MidiFile(mid_path)
	print(f"MIDI: {mid_path}")
	print(f"ticks_per_beat: {mid.ticks_per_beat}")
	num, den = read_time_signature(mid)
	print(f"time_signature: {num}/{den}")

	tempo_cnt = 0
	for tr in mid.tracks:
		for msg in tr:
			if msg.type == "set_tempo":
				tempo_cnt += 1
	print(f"set_tempo messages: {tempo_cnt}")
	print()

	for ti, tr in enumerate(mid.tracks):
		name = None
		note_cnt = 0
		for msg in tr:
			if msg.type == "track_name":
				name = msg.name
			if msg.type in ("note_on", "note_off"):
				note_cnt += 1
		print(f"[track {ti}] name={name!r}, note_msgs={note_cnt}, total_msgs={len(tr)}")


#=====================================================================#
# main
#=====================================================================#

def sanitize_name_lower(name: str) -> str:
	n = (name or "").strip().lower()
	n = re.sub(r"[^a-z0-9_]", "_", n)
	if not n:
		n = "song0"
	if re.match(r"^[0-9]", n):
		n = "song_" + n
	if re.match(r"^_+$", n):
		n = "song0"
	return n

def main() -> None:
	ap = argparse.ArgumentParser(description="Convert MIDI to uint8_t stream (cmd,val) PROGMEM (mono).")
	ap.add_argument("midi", nargs="?", help="Input MIDI file (.mid)")
	ap.add_argument("--inspect", action="store_true", help="Print MIDI summary and exit.")
	ap.add_argument("--name", type=str, default="song0", help="C array name for output.")

	# Совместимость со старым .bat: принимаем, но игнорируем
	ap.add_argument("--mono", type=str, default=None)
	ap.add_argument("--grid", type=int, default=None)
	ap.add_argument("--bar-per-line", action="store_true")
	ap.add_argument("--track", type=int, default=None)
	ap.add_argument("--channel", type=int, default=None)

	# Если кто-то передаст --transpose, применяем это как initial TRANS, но ноты НЕ СДВИГАЕМ.
	ap.add_argument("--transpose", type=int, default=None)

	args = ap.parse_args()

	if args.inspect:
		if not args.midi:
			raise SystemExit("Нужен путь к MIDI: python mid2code.py file.mid --inspect")
		inspect_midi(args.midi)
		return

	if not args.midi:
		raise SystemExit("Нужен MIDI файл. Пример: python mid2code.py input.mid --name song0")

	mid = mido.MidiFile(args.midi)
	time_sig = read_time_signature(mid)

	ticks_per_beat = int(mid.ticks_per_beat)
	ticks_per_16 = float(ticks_per_beat) / 4.0

	_track_idx, msgs = pick_first_track_with_notes(mid)
	if not msgs:
		raise SystemExit("Не найдено нот (note_on/note_off) ни в одном треке.")

	segs = build_timeline_segments_highest(msgs)
	if not segs:
		raise SystemExit("Ноты найдены, но после моно-таймлайна сегментов не осталось.")

	base_tick = int(segs[0].start)

	events = segments_to_events(
		segs=segs,
		ticks_per_16=ticks_per_16,
		base_tick=base_tick,
	)

	tempos = read_tempo_points(
		mid=mid,
		ticks_per_16=ticks_per_16,
		base_tick=base_tick,
	)

	# initial TEMPO:
	#  - если есть TEMPO в pos16==0, берём его
	#  - иначе дефолт 120 BPM (12)
	initial_tempo10 = 12
	tempos_sorted = sorted(tempos, key=lambda x: x.pos16)
	if tempos_sorted and tempos_sorted[0].pos16 == 0:
		initial_tempo10 = int(tempos_sorted[0].tempo10)
		tempos = tempos_sorted[1:]
	else:
		tempos = tempos_sorted

	# initial TRANS (по умолчанию 0)
	initial_trans = 0
	if args.transpose is not None:
		initial_trans = int(args.transpose)
		if initial_trans < -127:
			initial_trans = -127
		if initial_trans > 127:
			initial_trans = 127

	items = insert_tempo_into_events(events=events, tempos=tempos)

	name = sanitize_name_lower(str(args.name))
	c_code = format_as_c_array(
		items=items,
		name=name,
		time_sig=time_sig,
		initial_tempo10=initial_tempo10,
		initial_trans=initial_trans,
	)
	print(c_code)

if __name__ == "__main__":
	main()
