# OpenMower Hardware Bridge — Research & Design

Status: design only, no code in this PR. Implementation is sequenced in §8.

References (resolved 2026-04-30):
- OpenMower LL firmware @ `4f11e68` — `ClemensElflein/OpenMower` (RP2040 / Arduino, MIT-CC-BY-NC-SA).
  - Protocol header: [`Firmware/LowLevel/src/datatypes.h`](https://github.com/ClemensElflein/OpenMower/blob/main/Firmware/LowLevel/src/datatypes.h)
  - LL main loop: [`Firmware/LowLevel/src/main.cpp`](https://github.com/ClemensElflein/OpenMower/blob/main/Firmware/LowLevel/src/main.cpp)
  - PlatformIO env: [`Firmware/LowLevel/platformio.ini`](https://github.com/ClemensElflein/OpenMower/blob/main/Firmware/LowLevel/platformio.ini) (`board = pico`)
- OpenMower ROS1 stack @ `d9a0f38` — `ClemensElflein/open_mower_ros`.
  - Legacy bridge: [`src/mower_comms_v1/src/mower_comms.cpp`](https://github.com/ClemensElflein/open_mower_ros/blob/main/src/mower_comms_v1/src/mower_comms.cpp) (closest analog to MowgliNext's `hardware_bridge_node.cpp`)
  - LL header copy: [`src/mower_comms_v1/src/ll_datatypes.h`](https://github.com/ClemensElflein/open_mower_ros/blob/main/src/mower_comms_v1/src/ll_datatypes.h)
- MowgliNext firmware contract:
  - [`firmware/stm32/ros_usbnode/include/mowgli_protocol.h`](../firmware/stm32/ros_usbnode/include/mowgli_protocol.h)
  - [`ros2/src/mowgli_hardware/include/mowgli_hardware/ll_datatypes.hpp`](../ros2/src/mowgli_hardware/include/mowgli_hardware/ll_datatypes.hpp)
  - [`ros2/src/mowgli_hardware/src/hardware_bridge_node.cpp`](../ros2/src/mowgli_hardware/src/hardware_bridge_node.cpp)

---

## 1. Goals & Non-Goals

**Goals**
- Run MowgliNext on an unmodified, stock-flashed OpenMower board. **No reflashing of the operator's hardware.** The bridge speaks OpenMower's LL wire protocol natively.
- The rest of the MowgliNext ROS2 stack stays bit-identical at the topic / service level: `~/status`, `~/emergency`, `~/power`, `~/imu/data_raw`, `~/wheel_odom`, `~/cmd_vel`, `~/mower_control`, `~/emergency_stop`, `/battery_state` (see `hardware_bridge_node.cpp:25-46`). Nav2, BT, fusion_graph, GUI are untouched.
- Reorganize `firmware/` into `firmware/yf500x/` (current YardForce/Mowgli-native firmware) and `firmware/openmower/` (bridge + scripts to talk to a stock OpenMower board).

**Non-goals**
- Reusing Clemens's Pico LL firmware on a YardForce board (the OpenMower board is a custom RP2040 PCB; the YardForce board is STM32F103 — they are not interchangeable).
- Forking xESC firmware or replacing `xesc_driver`.
- Changing Mowgli safety semantics: STM32-side blade gating is replaced by **xESC duty-cycle gating in the bridge** plus heartbeat-driven LL emergency latch. Firmware-of-record is still the sole blade authority — it's just two firmwares now (OpenMower LL for chassis safety, mow xESC for motor stop on heartbeat loss).

---

## 2. OpenMower Firmware Contract

**Hardware**: Raspberry Pi Pico (RP2040) running Arduino-Pico via PlatformIO (`platformio.ini:18-23`). USB-CDC serial to host. Drive + blade motors are **not** on the LL board — they are 3 independent xESC controllers on the host's USB hubs, driven via `xesc_driver` (`mower_comms.cpp:23,108-115`). The LL board only owns: battery, charging, USS×5, IMU+mag, rain, sound, UI buttons, stop/lift emergency latching.

**Wire framing** (identical to MowgliNext's, by design — Mowgli forked it):
- COBS-encoded payload, terminated by `0x00` (`mower_comms.cpp:154-156` uses `cobs.encode()` then writes `0x00`).
- Payload = packed struct (with leading 1-byte `type` field) + `uint16_t crc` (CRC-16 CCITT, big-endian on the wire — see `mower_comms.cpp:259-260`: `buf[size-1] = (crc >> 8) & 0xFF; buf[size-2] = crc & 0xFF`). **Endianness diff vs Mowgli — see §4.**
- Library: `bakercp/PacketSerial` on firmware, `boost/crc.hpp` on ROS host.

**Packet IDs** (`datatypes.h:23-29`): `PACKET_ID_LL_STATUS=0x01`, `LL_IMU=0x02`, `LL_UI_EVENT=0x03`, `LL_HIGH_LEVEL_CONFIG_REQ=0x11`, `LL_HIGH_LEVEL_CONFIG_RSP=0x12`, `LL_HEARTBEAT=0x42`, `LL_HIGH_LEVEL_STATE=0x43`. **No `LL_ODOMETRY`, no `LL_CMD_VEL`, no `LL_BLADE_STATUS`, no `LL_CMD_BLADE`.**

**Structs** (`datatypes.h:97-167` — sizes match Mowgli's):
- `ll_status` (38 B): `type, status_bitmask, uss_ranges_m[5], emergency_bitmask, v_charge, v_battery, charging_current, batt_percentage, crc`. Bits identical to Mowgli (`LL_STATUS_BIT_*`, `LL_EMERGENCY_BIT_*`).
- `ll_imu` (41 B): `type, dt_millis, acceleration_mss[3], gyro_rads[3], mag_uT[3], crc`. **Field-for-field identical** to `pkt_imu_t`.
- `ll_heartbeat` (5 B): `type, emergency_requested, emergency_release_requested, crc`. Identical.
- `ll_high_level_state` (5 B): `type, current_mode, gps_quality, crc`. Mode encoding differs — see §4 (mode is `mode_5bits | submode_2bits<<6`, with submodes for AUTONOMOUS={MOWING,DOCKING,UNDOCKING} and RECORDING={OUTLINE,OBSTACLE}).
- `ll_ui_event` (5 B): identical.
- `ll_high_level_config` (variable, ~58 B): `ConfigOptions, rain_threshold, v_charge_cutoff, i_charge_cutoff, v_battery_cutoff, v_battery_empty, v_battery_full, lift_period, tilt_period, shutdown_esc_max_pitch, language[2], volume, hall_configs[10]`. **Bidirectional, flexible-length.** Mowgli has no equivalent — see §4 row "config".

**Wheel odometry**: not on the wire. `mower_comms.cpp:286-318` reads tachos directly from `xesc_driver` (`left_status.state.tacho_absolute`, `rpm`, `current_input`, `temperature_pcb`, `direction`, `duty_cycle`) and integrates m/s + tick deltas itself.

**Motor commands**: bridge calls `left_xesc_interface->setDutyCycle(speed_l)`, `right_xesc_interface->setDutyCycle(-speed_r)`, `mow_xesc_interface->setDutyCycle(speed_mow)` (`mower_comms.cpp:131-135`, note the right-wheel sign flip and the duty-cycle-not-velocity unit). Speed limit + emergency-zeroing + 1-s/25-s timeouts are enforced in `publishActuators()`.

**Heartbeat**: 100 ms cadence in v1 (`mower_comms.cpp:137-160`); LL latches emergency if heartbeat is absent for ~500 ms.

---

## 3. MowgliNext Firmware Contract

**Hardware**: STM32F103 on YardForce 500-class PCB, USB-CDC. Drive motors + blade motor controlled directly by the STM32 (no xESC, no separate motor MCU).

**Framing**: same COBS+CRC-16 envelope (`mowgli_protocol.h:14-21`). Mowgli's CRC trailer is little-endian on the wire (`uint16_t crc` final field of each `__attribute__((packed))` struct, written/read via `std::memcpy` on a little-endian Cortex-M3 / x86_64 host — `ll_datatypes.hpp:222`). **This is the only protocol-level binary diff** vs OpenMower (which writes the high byte first, see §4).

**Packet IDs** (`mowgli_protocol.h:43-83` and `ll_datatypes.hpp:41-54`): the 7 OpenMower IDs, plus 3 Mowgli extensions: `PKT_ID_ODOMETRY=0x04`, `PKT_ID_BLADE_STATUS=0x05`, `PKT_ID_CMD_VEL=0x50`, `PKT_ID_CMD_BLADE=0x51`.

**Structs not in OpenMower**:
- `pkt_odometry_t` / `LlOdometry` (17 B, `mowgli_protocol.h:204-213`): `dt_millis, left_ticks (int32 signed), right_ticks (int32 signed), left_velocity_mm_s (int16 signed), right_velocity_mm_s (int16 signed)`. Firmware-side dt = hardware timer (jitter-free).
- `pkt_blade_status_t` / `LlBladeStatus` (16 B, `mowgli_protocol.h:293-301`): `is_active, rpm, power_watts, temperature, error_count`.
- `pkt_cmd_vel_t` / `LlCmdVel` (11 B): `linear_x [m/s], angular_z [rad/s]` — full geometry_msgs/Twist semantics, firmware does the differential-drive split.
- `pkt_cmd_blade_t` / `LlCmdBlade` (5 B): `blade_on, blade_dir`.

**Mowgli HL mode** (`mowgli_protocol.h:244-248`): single `uint8_t current_mode` ∈ {NULL=0, IDLE=1, AUTONOMOUS=2, RECORDING=3, MANUAL_MOWING=4}. **No submode bits.** `MANUAL_MOWING` does not exist upstream.

**Wheel kinematics**: declared in `mowgli_robot.yaml`, fed to `hardware_bridge_node.cpp:137-138` (`wheel_track`, `ticks_per_metre`). The STM32 also knows ticks-per-metre via its `board.h` — that's the only place it needs it (for the `int16 mm_s` velocity field). On OpenMower, ticks-per-metre is a host-side parameter only.

---

## 4. Gap Analysis

Legend: 🟢 1:1 wire-compatible; 🟡 conversion needed (units, bit layout, semantic translation); 🔴 missing on one side — bridge must synthesise it.

| Capability | OpenMower (`datatypes.h` / `mower_comms.cpp`) | MowgliNext (`mowgli_protocol.h` / `hardware_bridge_node.cpp`) | Status | Bridge action |
|---|---|---|---|---|
| Framing (COBS + 0x00 delimiter) | `cobs.encode` then `0x00` | `packet_handler_.encode_packet` → COBS + 0x00 | 🟢 | reuse `cobs.{c,h}`, `crc16.{c,h}` from `ros2/src/mowgli_hardware/firmware/` |
| CRC-16 algorithm | CCITT (`boost::crc_ccitt_type`, init unspecified — verify init=0xFFFF, poly=0x1021) | CCITT, same | 🟢 | shared CRC code |
| CRC byte order on wire | **big-endian**: `buf[size-1]=hi; buf[size-2]=lo` (`mower_comms.cpp:259-260`) | **little-endian** (struct field `uint16_t crc` on LE host) | 🟡 | bridge byte-swaps the CRC trailer when encoding/decoding (1-line shim) |
| `ll_status` (38 B) | `v_battery` field name | `v_system` field name (same offset, same float) | 🟢 | byte-for-byte copy; rename in handler only |
| `ll_status` ESC-power bit | `LL_STATUS_BIT_FREE = 1<<3` (literal "Bit 3: ESC power" in struct comment, `datatypes.h:99-110`) | bit 3 reserved/free, `Status.esc_power` derived from blade telemetry | 🟡 | bridge maps OM bit-3 → `Status.esc_power` |
| Emergency bitmask | OM has BOTH a "standard" set (latch=0, stop=1, lift=2) **and** a CoverUI set (latch=0, stop1=1, stop2=2, lift=3, bump=4, liftx=5, rbump=6) selected at runtime by board variant | latch=0, stop=1, lift=2 only | 🟡 | bridge only supports the standard set in v1; CoverUI variant gated to a future PR |
| `ll_imu` (41 B) | identical layout, identical units | identical | 🟢 | direct memcpy |
| `ll_ui_event` (5 B) | identical | identical (button event maps to `~/ui_event`) | 🟢 | direct |
| `ll_heartbeat` (5 B) | identical, recommended at 100 ms | identical, default 250 ms | 🟢 | bridge sends at 100 ms (faster cadence is forward-compatible with Mowgli's 500 ms LL timeout) |
| `ll_high_level_state` mode encoding | `mode_5bits \| (submode<<6)` — RECORDING has submodes OUTLINE/OBSTACLE, AUTONOMOUS has MOWING/DOCKING/UNDOCKING | flat `uint8_t` 0..4 with `MANUAL_MOWING=4` | 🟡 | bridge translates Mowgli HL mode → OM `(mode\|submode<<6)`. `MANUAL_MOWING=4` → upstream OM treats this as "unknown"; we map it to `RECORDING\|OUTLINE` (drives, no blade enforcement at LL) so the LL doesn't reject the heartbeat |
| `ll_high_level_config` (~58 B, bidirectional) | required handshake — LL requests config on boot via `PACKET_ID_LL_HIGH_LEVEL_CONFIG_REQ`; host responds with `_RSP` carrying battery cutoffs, lift/tilt periods, language, volume, hall configs | no equivalent — Mowgli's STM32 reads these from compiled-in `board.h` | 🔴 | **bridge MUST implement this handshake or the LL board will refuse to leave init.** Map fields from `mowgli_robot.yaml` (battery thresholds, lift/tilt) plus sensible defaults. New file: `firmware/openmower/config/openmower_hl_config.yaml`. |
| Wheel encoder ticks | xESC tachos via `xesc_driver` (USB serial, separate transport) | `pkt_odometry_t` from STM32 over the same USB | 🔴 | bridge must spawn / link `xesc_driver` to talk to the 2 drive xESCs and synthesise `pkt_odometry_t`-equivalent values (`int32 ticks`, `int16 mm/s`). Output: `~/wheel_odom` `nav_msgs/Odometry` exactly as today. |
| Drive cmd_vel | host computes `speed_l/speed_r` (duty cycle, ±1.0) and calls `xesc.setDutyCycle()` | bridge sends `pkt_cmd_vel_t` with `linear_x, angular_z` to STM32, which does the kinematic split | 🔴 | **major behavioral change.** Bridge takes Mowgli's `~/cmd_vel` Twist, splits into `speed_l/r` using `wheel_track` + `wheel_radius`, converts m/s to xESC duty-cycle, then calls `xesc_driver`. Add a top-speed clamp (existing safety property). |
| Blade motor cmd | `pkt_cmd_blade_t` (on/off, dir) | `target_speed_mow` duty-cycle to mow xESC | 🔴 | bridge converts Mowgli's `MowerControl` service request → mow xESC duty cycle. **Heartbeat-driven blade-stop is preserved**: if `~/cmd_vel` is silent for >25 s OR LL/HL emergency latches, all 3 xESCs are zeroed (matches `mower_comms.cpp:131-135`). |
| Blade telemetry | `mow_xesc_interface->getStatus()` → rpm, current, temp | `pkt_blade_status_t` from STM32 (rpm, watts, temp, error_count) | 🔴 | bridge synthesises `Status.mower_motor_rpm`, `mower_motor_temperature`, `mower_esc_current` from `xesc_msgs/XescState` |
| Wheel-tick units (over wire) | tachos = absolute uint32, host divides by `wheel_ticks_per_m` ROS param | int32 signed cumulative ticks scaled by `ticks_per_metre` from `mowgli_robot.yaml` | 🟡 | bridge uses the **same** `mowgli_robot.yaml` value, just wires xESC tacho deltas through it; sign comes from `state.direction` |
| Right-wheel inversion | `setDutyCycle(-speed_r)` and direction flip in tick decode (`mower_comms.cpp:299-300`) | not applicable — STM32 firmware handles motor wiring | 🔴 | bridge must replicate the inversion (or expose it as a parameter `right_wheel_invert: true`) |
| ROS distro | OpenMower upstream is **ROS1 Noetic** (`ros::Publisher`, `dynamic_reconfigure`) | ROS2 Kilted | 🔴 | bridge is a from-scratch **ROS2 C++ rclcpp node**, not a fork. xESC needs a ROS2 driver — see §5. |

---

## 5. Proposed Bridge Architecture

**Language: C++17 (rclcpp)**. Justification: matches `mowgli_hardware` package layout exactly — we get to reuse `cobs.cpp`, `crc16.cpp`, `serial_port.cpp`, `packet_handler.cpp` (`ros2/src/mowgli_hardware/src/`) verbatim. A Python implementation would force us to re-port + revalidate the COBS/CRC code, and to wrap a Python xESC driver that doesn't yet exist for ROS2. The bridge is also on the cmd_vel hot path (50 Hz+) — C++ keeps latency in the same ballpark as the existing bridge.

**Package layout** (new ROS2 package, same workspace as `mowgli_hardware`):

```
ros2/src/openmower_bridge/
├── CMakeLists.txt
├── package.xml
├── include/openmower_bridge/
│   ├── om_datatypes.hpp        # ports OM datatypes.h, fixes CRC byte order in pack/unpack helpers
│   ├── om_packet_handler.hpp   # subclass / sibling of mowgli_hardware::PacketHandler with BE-CRC trailer
│   └── xesc_adapter.hpp        # thin abstract over xesc_driver: setDutyCycle / getStatus
├── src/
│   ├── openmower_bridge_node.cpp  # main node — same public ROS interface as hardware_bridge_node
│   ├── om_packet_handler.cpp
│   ├── xesc_adapter.cpp           # talks to ros2-port of xesc_driver (or wraps a thread-pool over libserial)
│   └── hl_config_loader.cpp       # reads openmower_hl_config.yaml, builds ll_high_level_config, sends RSP
├── config/
│   └── openmower_hl_config.yaml   # battery cutoffs, lift/tilt periods, hall configs
└── launch/
    └── openmower_bridge.launch.py
```

**Public ROS interface** (must match `hardware_bridge_node.cpp:25-46` byte-for-byte):

| Topic / Service | Source on OpenMower hardware |
|---|---|
| pub `~/status` | `ll_status` + xESC status |
| pub `~/emergency` | `ll_status.emergency_bitmask` |
| pub `~/power` | `ll_status` voltages + currents |
| pub `~/imu/data_raw` | `ll_imu` (acceleration, gyro) |
| pub `~/imu/mag_raw` | `ll_imu.mag_uT` (with µT→Tesla conversion same as today) |
| pub `~/wheel_odom` | left/right xESC tacho integration |
| pub `~/dock_heading` | unchanged — derived from yaw at charging-rising-edge |
| pub `/battery_state` | `ll_status` % + `is_charging` |
| sub `~/cmd_vel` (`TwistStamped`) | split into `speed_l/r` duty cycle, push to xESC |
| srv `~/mower_control` | mow xESC duty cycle |
| srv `~/emergency_stop` | next heartbeat sets `emergency_requested` / `emergency_release_requested` |
| sub `/behavior_tree_node/high_level_status` | translate Mowgli HL mode → OM `mode\|submode<<6`, send `ll_high_level_state` |

**Threading**: same shape as `hardware_bridge_node.cpp:280-314` — one timer for serial drain, one for heartbeat (100 ms), one for HL state (500 ms). xESC drivers run their own background threads.

**xESC driver dependency**: upstream `xesc_ros` is ROS1 only. We have two options, ranked:
1. (preferred) port `xesc_driver` to ROS2 as `xesc_ros2` — small package, mostly serial framing + state struct. Track in PR 4 of §8.
2. wrap the xESC serial protocol directly inside `xesc_adapter.cpp` (more code, no upstream sync).

---

## 6. GUI Flashing Flow Update

`gui/web/src/components/FlashBoardComponent.tsx` currently has a single "Flash Firmware" form with `boardType` ∈ {`BOARD_VERMUT_YARDFORCE500`, `BOARD_YARDFORCE500`, `BOARD_YARDFORCE500B`, `BOARD_LUV1000RI`} (`FlashBoardComponent.tsx:262-281`). Backend at `gui/pkg/providers/firmware.go:73-79` switches on `boardType` and runs `flashVermut` (OpenOCD/RP2040 — current "no-flash for OpenMower-the-board" path) or `flashMowgli` (PlatformIO/STM32, paths under `firmware/stm32/ros_usbnode/...` — `firmware.go:108-129`).

**Changes needed**:
1. **Frontend** — add a top-level mode selector "I want to use…" before the existing form:
   - Option A: *Flash YF500x firmware (Mowgli-native STM32)* → existing `flashMowgli` path
   - Option B: *Flash Vermut OpenMower firmware on this Pico/PCB* → existing `flashVermut` path
   - Option C: ★ NEW ★ *Use my existing OpenMower board (no flash)* → no flash; just write a runtime toggle.
   File to edit: `gui/web/src/components/FlashBoardComponent.tsx` — add the `hardwareMode` enum field, hide the entire flash form when `hardwareMode === "openmower-no-flash"`, replace it with a "Save and continue" button.
2. **Backend** — new endpoint `POST /api/setup/setHardwareBackend` writing `gui.hardware.backend ∈ {"yf500x","openmower"}` to the existing key-value DB (same store used at `firmware.go:70` for `gui.firmware.config`). File to edit: `gui/pkg/providers/firmware.go` (add `SetHardwareBackend`); register the route in the corresponding api file (search for `flashBoard` registration in `gui/pkg/api/`).
3. **ROS2 bringup** — `mowgli.launch.py` reads `gui.hardware.backend` (already mounted via the GUI's config file or via a ROS param `hardware_backend`) and conditionally launches `mowgli_hardware/hardware_bridge` (yf500x) **or** `openmower_bridge/openmower_bridge` (openmower). The two are mutually exclusive — same `~/status`, `~/cmd_vel`, etc., so downstream nodes do not care.
4. **Field validation** — when `hardwareMode === "openmower-no-flash"`, the existing flash-form fields (`maxChargeVoltage` etc) become an `openmower_hl_config.yaml` template instead of `board.h`. Reuse the same Formily form, retarget the submit handler.

---

## 7. Migration Plan for `firmware/` Reorg

Repo-wide grep for `firmware/stm32/ros_usbnode` (sources of truth that hard-code the path):

| File | Reference | Action |
|---|---|---|
| `firmware/scripts/sync_ros_lib.py:21` | `OUT_DIR = REPO_ROOT / "firmware" / "stm32" / "ros_usbnode" / "src" / "ros" / "ros_lib" / "mower_msgs"` | update to `firmware/yf500x/stm32/ros_usbnode/...`; move script to `firmware/yf500x/scripts/` (it's YF-specific) |
| `gui/pkg/providers/firmware.go:108,113,129` | three hard-coded paths under the cloned tempdir | update to `firmware/yf500x/stm32/ros_usbnode/include/board.h(.template)` and `cmd.Dir = .../firmware/yf500x/stm32/ros_usbnode` |
| `.github/workflows/firmware-ci.yml:7,9,39` | path filters + `working-directory` | update both path filters and working directory to `firmware/yf500x/stm32/ros_usbnode` |
| `CONTRIBUTING.md` | docs | rewrite to mention both `yf500x/` and `openmower/` |
| `CLAUDE.md` (root) | "Quick Commands" → `cd firmware/stm32/ros_usbnode && pio run` and "Code Generation Workflow" | update path; add OpenMower-bridge section |
| `ros2/src/mowgli_bringup/config/mowgli_robot.yaml`, `install/config/mowgli/mowgli_robot.yaml` | reference comments only | docstring-only update |

**Move command** (a single `git mv` in PR 1):
```bash
mkdir -p firmware/yf500x
git mv firmware/stm32 firmware/yf500x/stm32
git mv firmware/scripts firmware/yf500x/scripts
mkdir -p firmware/openmower   # placeholder, populated by PR 2
```

---

## 8. Effort Estimate & PR Sequencing

Six PRs, each scoped to ≤1 day of focused work, each independently mergeable + revertible.

| # | PR title | Scope | Risk |
|---|---|---|---|
| 1 | `refactor(firmware): split firmware/ into yf500x/ and openmower/` | `git mv` from §7 + path updates in `firmware-ci.yml`, `firmware.go`, `sync_ros_lib.py`, `CLAUDE.md`, `CONTRIBUTING.md`. No protocol or runtime changes. | Low — pure rename, CI catches paths |
| 2 | `feat(openmower_bridge): scaffold ROS2 package + COBS/CRC unit tests` | Create `ros2/src/openmower_bridge/`, port `om_datatypes.hpp` from upstream `datatypes.h`, write `om_packet_handler.cpp` with byte-swapped CRC trailer. Pure encode/decode unit tests against vectors captured from upstream `mower_comms.cpp` test buffers. **No serial I/O.** | Low — pure logic + GoogleTest |
| 3 | `feat(openmower_bridge): implement status/imu/heartbeat path on real serial` | Open USB-serial, parse `ll_status` + `ll_imu`, send `ll_heartbeat` at 100 ms, send `ll_high_level_state`. Implement `ll_high_level_config` REQ/RSP handshake (PR-blocker for any OpenMower board). Publishes `~/status`, `~/emergency`, `~/power`, `~/imu/data_raw`. Test on a real stock OpenMower board, robot wheels OFF. | Medium — first contact with real hardware |
| 4 | `feat(xesc_ros2): port xesc_driver to ROS2 Kilted` | Standalone PR — port `ClemensElflein/xesc_ros` `xesc_driver` (just the serial driver, not the simulator) to ROS2. New repo or vendored under `ros2/src/xesc_driver/`. | Medium — upstream is ROS1, mostly mechanical port |
| 5 | `feat(openmower_bridge): wire cmd_vel + odometry + blade through xESC` | Add `xesc_adapter.cpp` that calls into `xesc_driver` (PR 4). Subscribe `~/cmd_vel`, split to L/R duty cycle, apply right-wheel inversion. Integrate tachos → `~/wheel_odom`. Service `~/mower_control` → mow xESC. Reuse Mowgli's 1 s / 25 s cmd_vel timeouts and emergency-zero logic. | High — motors move |
| 6 | `feat(gui): hardware backend selector + E2E on stock OpenMower` | `FlashBoardComponent.tsx` mode selector (§6), backend route, conditional bringup in `mowgli.launch.py`. Full integration: undock → coverage → dock on a stock OpenMower YardForce, with fusion_graph + Nav2 unchanged. | High — full-system test |

Total: ~5 working days of implementation + 1 day of hardware bring-up.

---

## 9. Headline risks (must read before PR 5)

1. **CRC byte order**: OpenMower writes the high CRC byte at the *last* position of the encoded payload (`mower_comms.cpp:259-260`), i.e. CRC trailer is big-endian on the wire. Mowgli's `pkt_*` structs treat the trailing `uint16_t crc` as a native (little-endian) field. PR 2's unit tests must include a byte-for-byte vector captured from a running OpenMower board to confirm.
2. **`ll_high_level_config` is a soft-required handshake**: upstream LL boots in init mode and won't run the chassis safety state machine cleanly until it has received a config RSP. Skipping this in PR 3 will look like flaky LIFT/STOP behavior.
3. **Dual emergency authority**: the OpenMower LL board's stop/lift bits are the same wire format, but the blade no longer sits on the LL board — it's on a mow xESC over a *different* serial connection. The bridge must zero the mow xESC duty cycle on every LL emergency edge **and** on heartbeat-loss to the LL. This is the only place where Mowgli's "firmware is the sole blade safety authority" invariant (CLAUDE.md root §Safety) needs a careful re-statement: with OpenMower hardware, **two firmwares jointly own blade safety** (LL latches the chassis state, mow xESC enforces duty=0 on serial timeout).
4. **Right-wheel sign + tach direction**: Mowgli's STM32 hides this; the bridge has to replicate `mower_comms.cpp:131,299-300` exactly. Add unit tests with synthetic xESC state.
