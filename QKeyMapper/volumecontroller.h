#pragma once

struct IMMDeviceEnumerator;
struct IMMDevice;
struct IAudioEndpointVolume;

// Volume control class using Windows Core Audio APIs
class VolumeController
{
public:
    VolumeController();
    ~VolumeController();

    // Initialize COM and Core Audio interfaces
    bool initialize();

    // Clean up COM and Core Audio interfaces
    void cleanup();

    // Set absolute volume (0.0 to 100.0 percentage)
    bool setVolume(float volumePercentage);

    // Get current volume (returns 0.0 to 100.0 percentage)
    float getCurrentVolume();

    // Increase volume by specified amount (can be negative for decrease)
    bool adjustVolume(float deltaPercentage);

    // Set mute state (true = muted, false = unmuted)
    bool setMute(bool muted);

    // Get current mute state (returns true if muted, false if not muted)
    bool isMuted();

    // Check if volume controller is properly initialized
    bool isInitialized() const { return m_isInitialized; }

private:
    // Convert percentage (0.0-100.0) to scalar (0.0-1.0)
    float percentageToScalar(float percentage);

    // Convert scalar (0.0-1.0) to percentage (0.0-100.0)
    float scalarToPercentage(float scalar);

    // Clamp volume percentage to valid range
    float clampVolumePercentage(float percentage);

    // Apply Windows-like mute logic when setting volume
    // Automatically mute when setting to 0%, unmute when setting to non-zero
    bool applyWindowsMuteLogic(float volumePercentage);

private:
    bool m_isInitialized;
    IMMDeviceEnumerator* m_deviceEnumerator;
    IMMDevice* m_defaultDevice;
    IAudioEndpointVolume* m_endpointVolume;
};
