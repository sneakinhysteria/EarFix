# Changelog

All notable changes to EarFix will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.2.0] - 2024-12-13

### Added
- **MOSL (Music) correction model**: New music-optimized algorithm based on specific loudness restoration research
  - Preserves spectral balance rather than reshaping for speech
  - Gentle compression (max 1.7:1) to preserve musical dynamics
  - Slower time constants for better sound quality
  - Optional brightness boost for enhanced high-frequency response
  - Configurable bass emphasis
- **Auto-gain feature**: Hold the AUTO GAIN button to automatically match output level to input level
- **Input/Output level meters**: Stereo VU meters showing signal levels before and after processing
- **Signal flow visualization**: Visual arrows showing the audio path through the plugin

### Changed
- Redesigned control panel with improved layout
  - Left 1/3: Model selection and options
  - Right 2/3: Signal flow (Input meters → Strength → Output gain → Output meters → Auto-gain)
  - Vertical faders for finer control
- MOSL gain factors reduced to align output levels with NAL model
- All labels now use consistent font styling
- Improved meter and fader height alignment

### Technical
- 11-band EQ implementation (6 audiogram frequencies + 5 interpolated bands for smoother response)
- Timer-based meter updates at 30Hz
- Atomic level metering for thread-safe UI updates

## [1.0.0] - 2024-12-08

### Added
- Initial release of EarFix
- Half-Gain correction model for simple, transparent hearing correction
- NAL-NL2 correction model with compression for clinical-grade correction
- Interactive audiogram editor for both ears
- Independent left/right ear enable controls
- Correction strength slider (0-100%)
- Output gain control (+/-24 dB)
- NAL-NL2 options: compression speed (Fast/Slow) and experience level (New/Some/Experienced)
- Premium machined aluminum UI design
- Support for AU, VST3, AUv3, and AAX formats
- Universal binary for Apple Silicon and Intel Macs
- macOS 10.13+ support

### Technical
- Built with JUCE 7.x framework
- 6-band parametric EQ implementation
- Real-time parameter automation support
- Full DAW preset save/recall support

---

## Version History

### Planned Features
- Windows VST3 and AAX builds
- Preset library with common audiogram patterns
- Import audiogram from file
- Additional correction models (DSL, Cambridge)
- Frequency response visualization

---

[Unreleased]: https://github.com/sneakinhysteria/EarFix/compare/v1.2.0...HEAD
[1.2.0]: https://github.com/sneakinhysteria/EarFix/releases/tag/v1.2.0
[1.0.0]: https://github.com/sneakinhysteria/EarFix/releases/tag/v1.0.0
