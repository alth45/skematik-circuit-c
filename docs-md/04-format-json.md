## `docs/04-format-json.md`


# Format JSON

[← Kembali ke readme](../readme.md)

UTF-8, contoh (rangkaian RC–LED dengan probe):

```json
{
  "app": "schematic-c",
  "version": 1,
  "components": [
    {"id": 1, "type": "acsource", "x": 120, "y": 160, "rot": 0, "on": 0, "wave": 0, "value": 220},
    {"id": 2, "type": "resistor", "x": 220, "y": 160, "rot": 0, "on": 0, "wave": 0, "value": 220},
    {"id": 3, "type": "inductor", "x": 320, "y": 160, "rot": 0, "on": 0, "wave": 0, "value": 0.1},
    {"id": 4, "type": "led",      "x": 420, "y": 160, "rot": 0, "on": 0, "wave": 0, "value": 2.0},
    {"id": 5, "type": "oscprobe", "x": 320, "y": 240, "rot": 0, "on": 0, "wave": 0, "value": 0},
    {"id": 6, "type": "ammeter",  "x": 370, "y": 160, "rot": 0, "on": 0, "wave": 0, "value": 0},
    {"id": 7, "type": "ldr",      "x": 220, "y": 260, "rot": 0, "on": 1, "wave": 0, "value": 1000000},
    {"id": 8, "type": "buzzer",   "x": 420, "y": 260, "rot": 0, "on": 0, "wave": 0, "value": 1000}
  ],
  "wires": [
    {"a": {"id": 1, "pin": 0}, "b": {"id": 2, "pin": 0}},
    {"a": {"id": 2, "pin": 1}, "b": {"id": 3, "pin": 0}},
    {"a": {"id": 3, "pin": 1}, "b": {"id": 4, "pin": 0}}
  ]
}
```

## Field `type`

`resistor | switch | battery | led | ground | capacitor | transistor | diode | capolar | acsource | voltmeter | ammeter | ldr | buzzer | oscprobe | inductor | zener | mosfet | potentiometer`.

Alias juga diterima:
- `R` / `SW` / `BAT` / `LED` / `GND` / `CAP` / `TR`
- `Diode` / `D`
- `Capolar` / `capacitor_polar` / `electrolytic` / `CP`
- `ACSource` / `ac_source` / `ac` / `AC` / `SumberAC`
- `Voltmeter` / `VM`
- `Ammeter` / `AM`
- `LDR` / `photoresistor`
- `Buzzer` / `BZR`
- `OSCProbe` / `OSC`
- `Inductor` / `IND` / `L`
- `Zener` / `Z` / `ZD` / `zenerdiode`
- `MOSFET` / `nmos` / `NMOS` / `FET`
- `POT` / `trimmer` / `rheostat`

## Field `value` (satuan SI)

| Tipe | Satuan |
|---|---|
| `resistor` | Ω |
| `battery` | V |
| `acsource` | VAC RMS |
| `led` / `diode` | $V_f$ (V) |
| `capacitor` / `capolar` | F |
| `ldr` | $R_{\text{gelap}}$ (Ω) |
| `buzzer` | frekuensi (Hz) |
| `inductor` | H |
| `zener` | $V_Z$ (V) |
| `mosfet` | $V_{th}$ (V) |
| `potentiometer` | $R$ total (Ω) |

- **Potentiometer**: posisi wiper 0–100 disimpan di field **`on`** (bukan `value`).
- Voltmeter & Ammeter tidak memakai `value` (selalu 0).
- Key lama `"v"` juga terbaca.

## Field `wave` (khusus `acsource`)

- `0` = sinus (default)
- `1` = kotak
- `2` = segitiga

Diabaikan untuk tipe lain.

## Field `on`

Umumnya `0`/`1`:
- **Switch**: 1 = tertutup, 0 = terbuka.
- **LDR**: 1 = TERANG ($R_{\text{efektif}} \approx 1\% \, R_{\text{gelap}}$), 0 = GELAP.
- **Potentiometer**: posisi wiper **0–100** (persen; di-load di-clamp ke rentang ini).
- Tipe lain: diabaikan.

## Field `rot`

`0..3` (= 0/90/180/270 derajat).

## Field `pin`

- 2-pin → `0` = kiri, `1` = kanan (mengikuti rotasi).
- `ground` → `0`.
- `transistor` NPN → `0` = Kolektor, `1` = Emitor, `2` = Basis.
- `mosfet` → 3 pin (gate/source/drain).
- `potentiometer` → `0` = A, `1` = B, `2` = W (wiper).
- `acsource` → `0` = Live, `1` = Netral.

## Batas

256 komponen, 512 kabel, file maks 4 MB. Kabel yang id-nya tidak ada dibuang saat load.

---
