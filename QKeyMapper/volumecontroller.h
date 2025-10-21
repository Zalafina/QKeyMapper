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
    // deviceType: 0 = Playback (default), 1 = Capture
    bool setVolume(float volumePercentage, int deviceType = 0);

    // Get current volume (returns 0.0 to 100.0 percentage)
    // deviceType: 0 = Playback (default), 1 = Capture
    float getCurrentVolume(int deviceType = 0);

    // Increase volume by specified amount (can be negative for decrease)
    // deviceType: 0 = Playback (default), 1 = Capture
    bool adjustVolume(float deltaPercentage, int deviceType = 0);

    // Set mute state (true = muted, false = unmuted)
    // deviceType: 0 = Playback (default), 1 = Capture
    bool setMute(bool muted, int deviceType = 0);

    // Get current mute state (returns true if muted, false if not muted)
    // deviceType: 0 = Playback (default), 1 = Capture
    bool isMuted(int deviceType = 0);

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
    // deviceType: 0 = Playback, 1 = Capture
    bool applyWindowsMuteLogic(float volumePercentage, int deviceType);

    // Update default audio endpoint if the current device has changed
    // This ensures volume operations target the currently active output/input device
    // deviceType: 0 = Playback, 1 = Capture
    bool updateDefaultAudioEndpointIfChanged(int deviceType);

    // Get current default audio endpoint and activate volume interface
    // Helper function to refresh endpoint when device changes
    // deviceType: 0 = Playback, 1 = Capture
    bool refreshAudioEndpoint(int deviceType);

    // QKeyMapper-specific GUID for volume change event context
    // This helps identify volume changes made by QKeyMapper vs other applications
    static const GUID s_QKeyMapperVolumeGUID;

private:
    bool m_isInitialized;
    IMMDeviceEnumerator* m_deviceEnumerator;

    // Playback device (speakers/headphones) interfaces
    IMMDevice* m_defaultDevice;
    IAudioEndpointVolume* m_endpointVolume;
    std::wstring m_currentDeviceId; // Track current device ID to detect changes

    // Capture device (microphone) interfaces
    IMMDevice* m_captureDevice;
    IAudioEndpointVolume* m_captureEndpointVolume;
    std::wstring m_currentCaptureDeviceId; // Track current capture device ID to detect changes
};
