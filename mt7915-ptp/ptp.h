/* SPDX-License-Identifier: ISC */
/*
 * MT7915/MT7916 virtual PTP Hardware Clock (PHC) on top of the 802.11 TSF.
 *
 * The TSF is a free-running 64-bit microsecond counter. We never touch the
 * oscillator or PLL; frequency and phase corrections are applied purely in
 * software, ported from the ath9k cyclecounter/timecounter model:
 *
 *     phc_ns(tsf) = offset + ((tsf - tsf_base) * mult) >> MT7915_PTP_SHIFT
 *
 * i.e. the piecewise-linear form of  time = tsf * mult + offset,  re-anchored
 * at (tsf_base, offset) whenever mult changes so that the clock stays
 * continuous across frequency adjustments.
 */
#ifndef __MT7915_PTP_H
#define __MT7915_PTP_H

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/math64.h>
#include <linux/time64.h>
#include <linux/ptp_clock_kernel.h>
#include <asm/byteorder.h>

struct mt7915_dev;

/* Fixed-point scale. Same shift as the ath9k reference (ATH9K_PTP_FAKE_SHIFT). */
#define MT7915_PTP_SHIFT		21
/* 1 TSF tick = 1 us = 1000 ns -> 1000 << 21 = 2097152000 (fits in u32) */
#define MT7915_PTP_NOMINAL_MULT		((u32)(NSEC_PER_USEC << MT7915_PTP_SHIFT))
/* Max frequency adjustment in ppb (same as the reference). Headroom check:
 * NOMINAL_MULT * (1 + 1e-3) = 2099249152 < U32_MAX.
 */
#define MT7915_PTP_MAX_ADJ		1000000
/* Refresh interval of the cached 64-bit TSF used to extend 32-bit
 * descriptor timestamps. The 32-bit us counter wraps every ~71.6 min and
 * extension is valid within +/- 2^31 us (~35.8 min) of the cache.
 */
#define MT7915_PTP_RESYNC_INTERVAL	(5 * HZ)

struct mt7915_ptp {
	struct ptp_clock_info info;
	struct ptp_clock *clock;

	/*
	 * Protects every timekeeping field below. Taken with
	 * spin_lock_irqsave() so the conversion helpers are safe from any
	 * context (NAPI, tasklet, hard IRQ). Never held across MMIO.
	 */
	spinlock_t lock;
	u32 mult;		/* current ns-per-us scale, Q.MT7915_PTP_SHIFT */
	u32 nominal_mult;	/* unadjusted scale; ppb is applied against this */
	u64 tsf_base;		/* TSF (us) at which @offset is valid */
	u64 offset;		/* PHC time (ns) at @tsf_base */
	u64 last_tsf;		/* latest full 64-bit TSF read from hardware */

	/* TSF instance this PHC follows (LPON band / HW BSSID index) */
	u8 band;
	u8 tsf_idx;

	struct delayed_work resync_work;
};

/*
 * Lock-free core conversion. Caller must hold ptp->lock.
 * Handles tsf < tsf_base (frame captured before the last re-anchor).
 * mul_u64_u32_shr() keeps a 96-bit intermediate, so no overflow for
 * any realistic TSF delta, and is cheap on 32-bit MIPS.
 */
static inline u64 __mt7915_ptp_tsf_to_ns(const struct mt7915_ptp *ptp, u64 tsf)
{
	if (tsf >= ptp->tsf_base)
		return ptp->offset +
		       mul_u64_u32_shr(tsf - ptp->tsf_base, ptp->mult,
				       MT7915_PTP_SHIFT);

	return ptp->offset -
	       mul_u64_u32_shr(ptp->tsf_base - tsf, ptp->mult,
			       MT7915_PTP_SHIFT);
}

/* Extend a 32-bit TSF sample to 64 bits around a recent full read. */
static inline u64 __mt7915_ptp_tsf32_extend(u64 ref, u32 tsf_lo)
{
	return ref + (s64)(s32)(tsf_lo - lower_32_bits(ref));
}

#if IS_REACHABLE(CONFIG_PTP_1588_CLOCK)

int mt7915_ptp_init(struct mt7915_dev *dev);
void mt7915_ptp_cleanup(struct mt7915_dev *dev);

/* Hot path: full 64-bit TSF (us) -> PHC time (ns). Any context. */
u64 mt7915_ptp_tsf_to_phc_time(struct mt7915_dev *dev, u64 tsf);

/*
 * Hot path: 32-bit TSF from a descriptor, already in CPU byte order
 * (e.g. status->timestamp, FIELD_GET(MT_TXS4_TIMESTAMP, ...)). Any context.
 */
u64 mt7915_ptp_tsf32_to_phc_time(struct mt7915_dev *dev, u32 tsf_lo);

int mt7915_ptp_index(struct mt7915_dev *dev);

#else /* !CONFIG_PTP_1588_CLOCK */

static inline int mt7915_ptp_init(struct mt7915_dev *dev)
{
	return 0;
}

static inline void mt7915_ptp_cleanup(struct mt7915_dev *dev)
{
}

static inline u64 mt7915_ptp_tsf_to_phc_time(struct mt7915_dev *dev, u64 tsf)
{
	return 0;
}

static inline u64 mt7915_ptp_tsf32_to_phc_time(struct mt7915_dev *dev,
					       u32 tsf_lo)
{
	return 0;
}

static inline int mt7915_ptp_index(struct mt7915_dev *dev)
{
	return -1;
}

#endif /* CONFIG_PTP_1588_CLOCK */

/*
 * Same as mt7915_ptp_tsf32_to_phc_time() but takes the raw little-endian
 * DMA descriptor word (RXD group 2 DW0, TXS DW4). This is where the MIPS
 * endianness conversion belongs: descriptors live in DMA memory written by
 * the device in LE order. MMIO reads via mt76_rr() are already CPU-order.
 */
static inline u64 mt7915_ptp_le_tsf32_to_phc_time(struct mt7915_dev *dev,
						  __le32 raw)
{
	return mt7915_ptp_tsf32_to_phc_time(dev, le32_to_cpu(raw));
}

#endif /* __MT7915_PTP_H */
