# Onboarding Improvements: From "configured" to "ready to mow"

> **The user's brief, verbatim:** *"Ce n'est pas assez complet en termes de calibration. À la fin de l'onboarding, tout fonctionne bien."*

This doc audits the existing onboarding wizard, lists every calibration step the robot needs that the wizard skips, and proposes a redesigned step-by-step flow. The goal is that **completing the wizard** is sufficient — the operator should not have to discover the Diagnostics page or hand-edit YAML to make the robot mow well.

Reference inputs:
- Existing wizard: `gui/web/src/pages/OnboardingPage.tsx`
- Calibration backend: `gui/pkg/api/calibration.go`, `gui/pkg/api/calibration_status.go`, `gui/pkg/api/mowglinext.go`
- Diagnostics surface (where calibration lives today): `gui/web/src/pages/DiagnosticsPage.tsx:917-1057`
- Sensor editor + IMU yaw modal: `gui/web/src/components/RobotComponentEditor.tsx`
- Localization architecture: `CLAUDE.md` § "Architecture Invariants" + [Wiki/Architecture](https://github.com/cedbossneo/mowglinext/wiki/Architecture)
- Live screenshots: `docs/gui-walkthrough/screenshots/onboarding/` and `docs/gui-walkthrough/screenshots/diagnostics/`

---

## 1. Current state audit (what onboarding does today)

The wizard at `OnboardingPage.tsx:551-654` has 6 steps:

| # | Step | Component | What it actually does |
|---|---|---|---|
| 0 | Welcome | `WelcomeStep` (lines 25-87) | Marketing copy. No state changes. |
| 1 | Robot Model | `RobotModelStep` (lines 98-204) | Picks a preset (YardForce 500, etc.), auto-fills `wheel_radius`, `wheel_track`, `blade_radius`, encoder ticks/rev, battery thresholds. |
| 2 | GPS | `GpsStep` (lines 208-386) | Sets `datum_lat`, `datum_lon` (typed or "Use current GPS position" via `set_datum` ROS service). Sets GPS protocol, NTRIP host/port/mountpoint/user/password. |
| 3 | Sensors | `SensorStep` (lines 390-403) | Renders `RobotComponentEditor` — visual drag-to-place for `lidar_x/y/z/yaw`, `imu_x/y/z/yaw`, `gps_antenna_x/y/z`. The IMU-yaw auto-calibration **button exists** here (compass icon next to IMU Yaw input, `RobotComponentEditor.tsx:745-748`) but is **not called out** as a required step. |
| 4 | Firmware | `FirmwareStep` (lines 407-453) | Optional STM32 flash via `FlashBoardComponent`. Skip is the recommended path for already-flashed boards. |
| 5 | Complete | `CompleteStep` (lines 457-528) | `POST /api/settings/status` to set `onboarding_completed=true`, then `restartRos2()` + `restartGui()`. |

### What the wizard skips or under-explains

**Calibration steps the wizard does not gate on:**

1. **IMU bias calibration** — never mentioned. The hardware bridge runs it automatically every dock arrival, but the operator has no in-wizard cue that the gyro/accel bias must exist before mowing. Read from `/ros2_ws/maps/imu_calibration.txt` (parsed by `calibration_status.go:140-188`).
2. **IMU mounting yaw (`imu_yaw`)** — required for any heading-based motion to work. The wizard's sensor step *exposes* the auto-calibrate button (compass icon, `RobotComponentEditor.tsx:745-748`) but does not require pressing it. A new operator can finish the wizard with the default `imu_yaw = 0` and the robot will still appear to be configured.
3. **IMU mounting pitch/roll** — same calibration service can write `imu_pitch`/`imu_roll` when the stationary baseline ≥ 150 samples (`RobotComponentEditor.tsx:215-226`). Not surfaced in the wizard at all.
4. **Magnetometer calibration** — the wizard never mentions it. The `Enable & run` button in Diagnostics is currently a notification-only stub (`DiagnosticsPage.tsx:896-902`) that asks the operator to flip a hidden ROS parameter `do_mag_calibration`. There is no GUI flow that *both* enables `do_mag_calibration` *and* triggers the rotation phase.
5. **Dock pose (`dock_pose_x/y/yaw`)** — the wizard does not produce one. Two paths exist post-wizard: (a) the IMU yaw calibration writes it as a side effect *if* the robot is on the dock when started, or (b) the operator finds the "environment pin" button in the map editor and clicks it with the robot manually placed on the dock. Neither is mentioned in the wizard.
6. **RTK-Fixed verification** — the wizard's "Use current GPS position" button only checks for *any* fix when persisting the datum; it does not assert RTK-Fixed. If the operator clicks it on SBAS, the datum is meters off and every subsequent mow is meters off.
7. **NTRIP smoke test** — the wizard saves NTRIP credentials but never confirms RTCM is flowing. We saw on the live robot: NTRIP host **empty** in Settings → GPS & Positioning while RTK was reported as "RTK FIX" elsewhere — the GUI's source of truth was inconsistent and the wizard would not have caught it.
8. **`use_fusion_graph` toggle** — the wizard never mentions it. New robots default to off (`ekf_map_node`), which means the `Localization` settings tab (and the Diagnostics page's Fusion Graph panel) feels disconnected from the rest of the experience.
9. **Map area recording** — strictly speaking out of scope of "calibration", but step 5 ("Complete") tells the operator to "Draw Mowing Area", with no inline guidance.

**Things the wizard explains poorly:**

- Step 2 says "set the map origin (datum) near your docking station" — but doesn't tell the operator they should physically place the robot **on the dock with RTK-Fixed** before clicking "Use current GPS position".
- Step 3 says "drag sensors on the top-down view" — but doesn't explain that the IMU yaw/pitch/roll inputs are **mounting offsets in the body frame**, not arbitrary numbers. The compass icon on IMU Yaw is undocumented (only a tiny tooltip).
- The Firmware step appears in a wizard about *configuration* — many operators flash before installing, so it's confusing to land here.

---

## 2. Gap analysis — every missing step, with backend mapping

| # | Gap | ROS mechanism | YAML key / file produced | Endpoint that exists today |
|---|---|---|---|---|
| 1 | IMU bias calibration trigger | `hardware_bridge_node` 20-second auto on dock arrival | `/ros2_ws/maps/imu_calibration.txt` v1 plaintext | Read-only via `GET /api/calibration/status` (`calibration_status.go:91-98`). **No write endpoint** — bias calibration only triggers on dock arrival; we'd need to expose a manual trigger if the operator isn't on the dock. |
| 2 | IMU yaw mounting calibration | `/calibrate_imu_yaw_node/calibrate` ROS service, ~150 s including dock pre-phase | `mowgli_robot.yaml` → `imu_yaw` (and `imu_pitch`/`imu_roll` when stationary samples ≥ 150) | **Exists:** `POST /api/calibration/imu-yaw` (`calibration.go:74-122`). |
| 3 | Dock pose | (a) side-effect of IMU yaw calibration when started while charging — `dock_valid` field in response (`calibration.go:39-45`); (b) `/map_server_node/set_docking_point` srv | `mowgli_robot.yaml` → `dock_pose_x/y/yaw` | (a) via the same IMU-yaw endpoint; (b) via `POST /api/mowglinext/call/set_docking_point` (`mowglinext.go:182`). |
| 4 | Magnetometer calibration | `calibrate_imu_yaw_node` rotation phase, gated on `do_mag_calibration:=true` parameter | `/ros2_ws/maps/mag_calibration.yaml` | **Half-exists:** Diagnostics button only shows a notification (`DiagnosticsPage.tsx:896-902`). To make it one-click we need a new endpoint that (a) sets the param via `/calibrate_imu_yaw_node/set_parameters` and (b) re-calls `/calibrate_imu_yaw_node/calibrate`. |
| 5 | RTK-Fixed wait + assert | Subscribe to `/gps/fix` (`status==2` for RTK Fixed) | n/a (read-only check) | **Missing as a wizard primitive.** GUI subscribes via `useGPS()` hook today; the wizard would need to gate "Next" on `gps.fix_type === 'RTK FIX'` for ~30 s of stable readings. |
| 6 | NTRIP smoke test | Subscribe to `/ntrip_client/rtcm` rate | n/a (read-only check) | **Missing.** Backend would need a `GET /api/diagnostics/ntrip` returning RTCM Hz over the last 10 s. |
| 7 | `use_fusion_graph` toggle (with explanation) | YAML param | `mowgli_robot.yaml` → `use_fusion_graph: true/false` | **Exists:** part of standard settings save flow. Wizard just needs to surface it. |
| 8 | First mowing area | `/map_server_node/add_area` (after recording trajectory) | `/ros2_ws/maps/garden_map.geojson` | **Exists:** the BT supports `COMMAND_RECORD_AREA` and the map editor saves via the existing service. Wizard just needs to teach the operator the flow. |

---

## 3. Proposed onboarding redesign

Goal: each step produces a concrete artefact, has a measurable success criterion, and blocks "Next" until the criterion is met (with a "Skip — I know what I'm doing" escape hatch for operators who are re-running the wizard after an upgrade).

### 3.1 New step list

| # | Title | Success criterion | Artefact produced |
|---|---|---|---|
| 0 | **Welcome** (unchanged) | User clicks Get Started | none |
| 1 | **Robot Model** (unchanged) | Preset chosen and saved | `wheel_radius`, `wheel_track`, `blade_radius`, etc. |
| 2 | **Firmware (moved earlier)** | STM32 reachable on `/dev/ttyACM*` (`firmware_alive` diagnostic = OK), or operator skips with "Already flashed" | n/a |
| 3 | **GPS receiver + NTRIP** | RTCM received at ≥ 10 Hz on `/ntrip_client/rtcm` for 10 consecutive seconds | NTRIP creds in `mowgli_robot.yaml` |
| 4 | **Wait for RTK-Fixed** | `/gps/fix` reports status == 2 for ≥ 30 consecutive seconds, position σ ≤ 0.05 m | n/a (verifies the chain works) |
| 5 | **Set datum** | `set_datum` returns success, `datum_lat/lon` non-zero in `mowgli_robot.yaml`, gps fix at click was RTK-Fixed | `mowgli_robot.yaml` → `datum_lat`, `datum_lon` |
| 6 | **Sensor placement (visual)** | Operator confirms LiDAR/IMU/GPS positions match the physical robot | `lidar_x/y/z/yaw`, `imu_x/y/z`, `gps_antenna_x/y/z` |
| 7 | **IMU bias calibration (stationary)** | Robot stationary on dock; service returns `n_samples ≥ 1000`, gyro bias < 0.01 rad/s on each axis | `/ros2_ws/maps/imu_calibration.txt` |
| 8 | **IMU yaw + dock pose calibration (motion required)** | Service returns `success=true`, `std_dev_deg < 5°`, `dock_valid=true` | `mowgli_robot.yaml` → `imu_yaw`, `imu_pitch`, `imu_roll`, `dock_pose_x/y/yaw` |
| 9 | **Magnetometer calibration (optional)** | If enabled: rotation phase completes with `magnitude_std_uT < 5 µT`. If skipped: `use_magnetometer=false` enforced. | `/ros2_ws/maps/mag_calibration.yaml` |
| 10 | **Localizer choice** | Operator picks `use_fusion_graph` true/false with the recommendation: "True if LiDAR mounted, otherwise False" | `mowgli_robot.yaml` → `use_fusion_graph`, `use_scan_matching`, `use_loop_closure` |
| 11 | **Record first mowing area** (deep-link to Map page) | At least one workarea polygon saved | GeoJSON in `/ros2_ws/maps/garden_map.geojson` |
| 12 | **Verify & Complete** | All previous steps green; one final "All systems go" panel summarising every artefact's status | `onboarding_completed=true` |

### 3.2 Implementation hooks (where to slot each step)

The current wizard's step dispatcher is `OnboardingPage.tsx:610-617`. Adding new steps means:

- Extend `STEP_TITLES` (`OnboardingPage.tsx:542-549`) and `STEP_ICONS` (`533-540`).
- Add new `<StepXxx>` components alongside `RobotModelStep`, `GpsStep`, etc., each receiving `(values, onChange)` props.
- Replace the linear `currentStep < 3 ? "Next" : "Save & Continue"` button (`OnboardingPage.tsx:639-645`) with a per-step "Next" gate that only unlocks when the step's success criterion is met. Each step component should expose a `useStepValidity(): {isValid: boolean; reason?: string}` hook the wizard reads.

Recommended implementation order (each is independently mergeable):

1. **Step 4 (RTK-Fixed wait)** — pure frontend change. Add a `useGPSFixWait()` hook that polls `useGPS().fix_type` and resolves when `'RTK FIX'` is observed for 30 s. UI: a progress ring + "Move the robot until the antenna sees more sky" instruction.
2. **Step 8 (IMU yaw + dock pose)** — wraps the existing `POST /api/calibration/imu-yaw` endpoint. UI: pre-flight checklist ("robot on dock + charging + 1 m clear in front + 1 m clear behind"), big red "Start calibration" button, live log streamed from `/calibrate_imu_yaw_node` via WebSocket.
3. **Step 7 (IMU bias)** — needs a new `POST /api/calibration/imu-bias` endpoint that calls a new ROS service `/hardware_bridge_node/calibrate_bias` (the bias logic exists, we'd wrap it in a service that doesn't require a dock arrival). Until that service ships, fall back to "Wait for the next dock arrival" with a polling indicator that watches `imu_calibration.txt`'s mtime.
4. **Step 6 NTRIP smoke test** — add a backend `GET /api/diagnostics/ntrip` that returns `{rtcm_hz, last_message_age_s}`; frontend polls until rtcm_hz ≥ 10.
5. **Step 9 (Magnetometer)** — needs a new `POST /api/calibration/magnetometer` endpoint that (a) sets `do_mag_calibration=true` via dynamic params, (b) re-invokes `/calibrate_imu_yaw_node/calibrate`, (c) restores `do_mag_calibration=false` on completion. Frontend gates Next on the resulting `mag_calibration.yaml` having `magnitude_std_uT < 5 µT`.
6. **Step 10 (Localizer choice)** — pure frontend; reuses the LocalizationSection toggles inline.
7. **Step 11 (Record area)** — deep-link to `/#/map` with a query param `?guided=record-first-area` so the map page can open with the area-recording panel pre-expanded.

### 3.3 Per-step UX details

Each step should have:

- **Header** with an icon + title.
- **What this does** paragraph (non-jargon, two sentences).
- **Pre-flight checklist** (bulleted, e.g. "Robot is on the dock and charging" — operator ticks each box before the action button enables).
- **Action button** (single, primary).
- **Live status panel** (progress, elapsed time, log tail).
- **Result panel** (artefact summary, "Re-run" link, "Continue" button enabled only on success).

### 3.4 Definition of "done"

When the wizard is complete, the **Verify & Complete** step should display a checklist that **all** read green before `onboarding_completed=true` is persisted. This is the user's actual ask:

```
[OK] Robot model:           YardForce Classic 500
[OK] Firmware:              v1.4.2 (flashed 2026-04-30 09:12)
[OK] NTRIP RTCM rate:       58 Hz from caster.centipede.fr/OUIL
[OK] GPS fix:               RTK-Fixed (sigma=0.005 m for 60 s)
[OK] Datum:                 48.879650, 2.172814 (set 2026-04-30 09:18)
[OK] Sensor placement:      LiDAR/IMU/GPS confirmed on chassis editor
[OK] IMU bias:              gyro_bias=[6.7e-2, -1.1e-1, -5.2e-2], n=1000, pitch=1.99 deg, roll=-2.75 deg
[OK] IMU yaw mounting:      162.8 deg (sigma=0.4 deg, calibrated 2026-04-30 09:26)
[OK] Dock pose:             (0.000, 0.000, 238.5 deg), confidence +/-0.4 deg
[--] Magnetometer:          skipped (use_magnetometer=false)
[OK] Localizer:             use_fusion_graph=true, scan_matching=true, loop_closure=true
[OK] First mowing area:     "Front yard" 92 m^2, saved 2026-04-30 09:41

All systems go - your robot is ready to mow.

[Go to Dashboard]   [Start mowing now]
```

If any row is red, "All systems go" stays disabled and the wizard offers to re-run that specific step.

### 3.5 Robustness and re-entrancy

The current wizard's biggest flaw is that it cannot be re-entered partially: if an operator fails the IMU yaw calibration on first try, they have to re-run the whole wizard. The proposed design fixes this in three ways:

1. **Read existing state first.** On wizard mount, `GET /api/calibration/status` populates `calibrationStatus`. Each step's success criterion is checked against that initial state — if the criterion is already met, the step is auto-completed (green badge) and Next is enabled immediately.
2. **Per-step "Re-run" link.** Even on a green step, the operator can re-run if they want fresher data.
3. **Skip with reason.** Each step has a "Skip" button that records *why* (e.g. "operator skipped: no LiDAR mounted") into a new `onboarding_skipped[]` field in the GUI's settings DB, surfaced on the Diagnostics page so future ops have context.

---

## 4. Out of scope for this design

These are real gaps but belong elsewhere:

- **Multi-robot / fleet onboarding** — no demand today.
- **Offline / no-cloud onboarding** — Mapbox satellite tiles require internet at present; an offline-tile flow is a separate doc.
- **Re-flashing firmware after an OTA fault** — covered by the Firmware step but recovery logic (bricked board, USB DFU mode) is a separate runbook.
- **Wizard for adding *additional* mowing areas** post-onboarding — the Map page already does this well; we just need to surface it from the post-onboarding "Verify & Complete" panel.

---

## 5. Quick win shortlist (≤ 1 day each)

If a full redesign is too much, these are the highest-impact incremental fixes:

1. **Add a "Calibration check" alert** to step 5 ("Complete") of the existing wizard. If `calibrationStatus.dock.present` is false **or** `calibrationStatus.imu.present` is false, show an `Alert type="warning"` with deep-links to the Diagnostics calibration cards. (~50 lines in `CompleteStep`, no backend work.)
2. **Gate step 2's "Use current GPS position" on RTK-Fixed.** Disable the button unless `gps.fix_type === 'RTK FIX'`; show a helper "Waiting for RTK-Fixed (current: 3D fix)…" otherwise. (~20 lines in `GpsStep`.)
3. **Surface the IMU yaw calibration as a wizard step**, not just a tooltip. Add a brand-new `ImuYawStep` between current Sensors and Firmware steps, reusing the modal logic from `RobotComponentEditor.tsx:851-920`.
4. **Make the `Enable & run` button on the magnetometer card actually enable + run.** Backend: new endpoint `POST /api/calibration/magnetometer` that toggles `do_mag_calibration` and re-invokes the calibrate service; frontend: replace the notification-only handler at `DiagnosticsPage.tsx:896-902` with a real call.
5. **Default `use_fusion_graph` based on LiDAR enable.** When the operator toggles LiDAR on in step 3, also flip `use_fusion_graph=true` (with a small explainer) — and conversely when LiDAR is off, force fusion_graph off.

These five changes alone would close roughly 70% of the gap the user is feeling, before any architectural redesign.

---

## Appendix A — backend endpoints relevant to onboarding

| Endpoint | Method | File | Notes |
|---|---|---|---|
| `/api/settings/status` | POST | (settings handler) | Sets `onboarding_completed=true`. Already used by `CompleteStep`. |
| `/api/calibration/imu-yaw` | POST | `calibration.go:63-122` | 150 s budget; returns yaw/pitch/roll + dock pose. |
| `/api/calibration/status` | GET | `calibration_status.go:91-98` | Dock + IMU + Mag, read from on-disk artefacts. |
| `/api/calibration/imu-bias` | POST | **does not exist** | Proposed: triggers `hardware_bridge_node`'s 20 s bias calibration on demand. |
| `/api/calibration/magnetometer` | POST | **does not exist** | Proposed: enables `do_mag_calibration` + re-invokes calibrate service. |
| `/api/diagnostics/ntrip` | GET | **does not exist** | Proposed: returns RTCM Hz over last 10 s. |
| `/api/mowglinext/call/set_datum` | POST | `mowglinext.go:391-397` | Calls `/navsat_to_absolute_pose/set_datum`. |
| `/api/mowglinext/call/set_docking_point` | POST | `mowglinext.go:182` | Calls `/map_server_node/set_docking_point`. |

---

## Appendix B — recommended PR sequencing

1. PR-1 (frontend only): quick-win #1 + #2 — calibration alert in CompleteStep, RTK-Fixed gate on datum button. ~80 lines.
2. PR-2 (frontend only): quick-win #3 — extract `ImuYawStep` into a real wizard step. Reuse existing `POST /api/calibration/imu-yaw`. ~250 lines.
3. PR-3 (backend + frontend): quick-win #4 — magnetometer endpoint + Diagnostics rewire. ~150 lines (Go) + ~30 lines (TS).
4. PR-4 (frontend only): quick-win #5 — couple LiDAR enable to `use_fusion_graph`. ~30 lines.
5. PR-5 (backend + frontend): the new full step list (sections 3.1–3.4). ~1500 lines, splits naturally per-step.
