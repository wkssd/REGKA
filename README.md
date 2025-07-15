# RE-GKA: Robust and Efficient Group Key Agreement Scheme for UANET

*Version 1.1 – July 2025*

> **RE‑GKA** is the reference implementation and NS‑3 simulation suite accompanying our paper:
> 
> **“RE‑GKA – Robust and Efficient Group Key Agreement Scheme for UANET”**  *(submitted to NDSS 2026).*

## Description

In this paper, our core work is to design a *Robust and Efficient Group Key Agreement Scheme* to improve the success rate of group key agreement in UANET with unstable links.

These codes are to verify the performance of the proposed scheme. Specifically, we use three quantitative analyses that measure: (i) the group key agreement success rate, (ii) the group key agreement delay, and (iii) the communication overhead, defined as the average number of packets transmitted during the group key agreement process.

> **Stochastic note** – Results involve random channel fading & gossip selection; absolute numbers may differ slightly from the paper, but **trends and relative gains remain stable**.

## Repository Layout

```
EAGKA-Ours/
├─ EAGKA-Ours.cc         # Main simulation program entry, contains simulation scenario setup and execution logic
├─ KeyMatrix.cc          # Key matrix implementation for group key management
├─ KeyMatrix.h           # Key matrix class definition and interface declarations
├─ AdhocUdpApplication.cc # Custom UDP application implementation for MANET communication simulation
├─ AdhocUdpApplication.h  # Custom UDP application class definition and interface declarations
└─ allrun.sh             # Script for batch running different scenarios
```

## Dependencies

* ns-3 3.25
* python 2.7.12
* g++ 7.5.0
* c++ c++03
* pandas 0.24.2
* matplotlib 2.2.5
* numpy 1.16.6
* openpyxl 2.6.4

## How to run?

1. Install NS-3 (v3.25) on a Linux machine

```bash
Follow the official [ns-3 build guide](https://www.nsnam.org/docs/release/3.25/) or use your package manager.
```

2. Clone / move the repository into the `scratch` folder

```bash
# Example layout
   /home/<user>/workspace/ns-3.25/scratch/REGKA
```

3. Run the script

```bash
cd ns-3.25/scratch/REGKA
chmod +x run.sh        # one-time, if needed
./run.sh               # fires up the full experiment suite
```

## NS3 Simulation Parameters

In our simulation experiment, to reflect different physical environments and channel conditions, the link quality is configured with two levels—**High** (LoS) and **Low** (long-distance / frequent obstruction). The channel model incorporates a combination of the Friis model, log-distance path loss, Nakagami fading, and random shadowing models to comprehensively simulate multipath fading and shadowing effects in UANET. Relevant parameters are configured according to common UAV hardware settings.

| Parameter                  | Value / Range                                         |
| -------------------------- | ----------------------------------------------------- |
| **Area size**              | 300 × 300 × 80 – 1 000 × 1 000 × 300 m³         |
| **Node number**            | 5 – 65                                               |
| **Flight speed**           | 0 – 20 m s⁻¹                                       |
| **Data transfer rate**     | 10 Mbit s⁻¹                                         |
| **Path-loss model**        | Friis · Log-Distance · Nakagami · Random shadowing |
| **Communication standard** | IEEE 802.11g                                          |
| **Motion model**           | Gauss–Markov                                         |
| **Link-quality levels**    | High / Low                                            |
| **Antenna gain**           | 2 dBi                                                 |
| **Noise figure**           | 7 dB                                                  |

## License

This project is licensed under the **MIT License** – see the [LICENSE](LICENSE) file for details.

---
