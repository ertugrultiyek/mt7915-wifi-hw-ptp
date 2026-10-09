# MT7915 Wi-Fi Hardware PTP Porting Project

## Mission
Port the hardware timestamping (PTP over Wi-Fi) mechanism from `references/wifi-ptp` (ath9k)
to MediaTek `work-driver/mt7915_20201106-34067b/`.

## Target Architecture
- Host SoC: MediaTek MT7621 (MIPS architecture).
- Radio: MT7915 / MT7916 PCIe 802.11ax.
- Base System: Vendor OpenWrt SDK (`references/wifi6-sdk`).
- Build Environment: Docker container (`docker/Dockerfile`).

## Workflows & Commands
1. **Analyze Reference:** Read `references/wifi-ptp` to identify TSF math and descriptor hooks.
2. **Modify Driver:** Make edits inside `work-driver/mt7915_20201106-34067b/`.
3. **Compile Test (Driver):** Run `./scripts/build-driver.sh`.
4. **Compile Test (Full Image):** Run `./scripts/build-image.sh`.
5. **Patch Generation:** Produce standard OpenWrt unified diffs in `patches/`.

## Kernel Rules
- Emulate PHC using 64-bit TSF: `time = (tsf * mult) + offset`. Do not touch RF PLL clocks.
- Handle MIPS endianness (`le32_to_cpu`) safely.
- Adhere to Linux spinlock discipline in interrupt / DMA contexts.