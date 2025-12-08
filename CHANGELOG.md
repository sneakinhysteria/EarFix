# Changelog

All notable changes to EarFix will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[Unreleased]: https://github.com/BrighterRealities/EarFix/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/BrighterRealities/EarFix/releases/tag/v1.0.0
