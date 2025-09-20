#include <mmdeviceapi.h>
#include <endpointvolume.h>

#include <QDebug>
#include "volumecontroller.h"

#include "qkeymapper_constants.h"

using namespace QKeyMapperConstants;

// QKeyMapper-specific GUID for volume change event context
// Generated GUID: {B8A9F2E5-1D4C-4A7B-9F3E-2C8B5A9D7E6F}
// This helps identify volume changes made by QKeyMapper vs other applications
const GUID VolumeController::s_QKeyMapperVolumeGUID =
    { 0xB8A9F2E5, 0x1D4C, 0x4A7B, { 0x9F, 0x3E, 0x2C, 0x8B, 0x5A, 0x9D, 0x7E, 0x6F } };

// VolumeController class implementation
VolumeController::VolumeController()
    : m_isInitialized(false)
    , m_deviceEnumerator(nullptr)
    , m_defaultDevice(nullptr)
    , m_endpointVolume(nullptr)
{
}

VolumeController::~VolumeController()
{
    cleanup();
}

bool VolumeController::initialize()
{
    if (m_isInitialized) {
        return true; // Already initialized
    }

    HRESULT hr;

    // Create device enumerator
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator),
                          reinterpret_cast<void**>(&m_deviceEnumerator));
    if (FAILED(hr)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[VolumeController::initialize] Failed to create device enumerator, hr =" << QString::number(hr, 16);
#endif
        return false;
    }

    // Get default audio render device
    hr = m_deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_defaultDevice);
    if (FAILED(hr)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[VolumeController::initialize] Failed to get default audio endpoint, hr =" << QString::number(hr, 16);
#endif
        cleanup();
        return false;
    }

    // Get audio endpoint volume interface
    hr = m_defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL,
                                   nullptr, reinterpret_cast<void**>(&m_endpointVolume));
    if (FAILED(hr)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[VolumeController::initialize] Failed to activate audio endpoint volume, hr =" << QString::number(hr, 16);
#endif
        cleanup();
        return false;
    }

    m_isInitialized = true;

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[VolumeController::initialize] Volume controller initialized successfully";
#endif
    return true;
}

void VolumeController::cleanup()
{
    if (m_endpointVolume) {
        m_endpointVolume->Release();
        m_endpointVolume = nullptr;
    }

    if (m_defaultDevice) {
        m_defaultDevice->Release();
        m_defaultDevice = nullptr;
    }

    if (m_deviceEnumerator) {
        m_deviceEnumerator->Release();
        m_deviceEnumerator = nullptr;
    }

    m_isInitialized = false;

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[VolumeController::cleanup] Volume controller cleaned up";
#endif
}

bool VolumeController::setVolume(float volumePercentage)
{
    if (!m_isInitialized || !m_endpointVolume) {
        return false;
    }

    float clampedPercentage = clampVolumePercentage(volumePercentage);
    float scalar = percentageToScalar(clampedPercentage);

    // Set the volume level first
    // Use QKeyMapper-specific GUID to identify this volume change event
    // This allows other applications to distinguish between QKeyMapper volume changes
    // and system/user-initiated changes, but does NOT control Windows volume OSD display
    HRESULT hr = m_endpointVolume->SetMasterVolumeLevelScalar(scalar, &s_QKeyMapperVolumeGUID);
    if (FAILED(hr)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[VolumeController::setVolume] Failed to set volume to" << clampedPercentage << "%, hr =" << QString::number(hr, 16);
#endif
        return false;
    }

    // Apply Windows-like mute logic: mute at 0%, unmute at non-zero
    if (!applyWindowsMuteLogic(clampedPercentage)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[VolumeController::setVolume] Failed to apply mute logic for volume" << clampedPercentage << "%";
#endif
        // Don't return false here as volume was set successfully, mute operation is supplementary
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[VolumeController::setVolume] Volume set to" << clampedPercentage << "%";
#endif
    return true;
}

float VolumeController::getCurrentVolume()
{
    if (!m_isInitialized || !m_endpointVolume) {
        return 0.0f;
    }

    float scalar = 0.0f;
    HRESULT hr = m_endpointVolume->GetMasterVolumeLevelScalar(&scalar);
    if (FAILED(hr)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[VolumeController::getCurrentVolume] Failed to get current volume, hr =" << QString::number(hr, 16);
#endif
        return 0.0f;
    }

    return scalarToPercentage(scalar);
}

bool VolumeController::adjustVolume(float deltaPercentage)
{
    float currentVolume = getCurrentVolume();
    if (currentVolume < 0.0f) {
        return false; // Failed to get current volume
    }

    float newVolume = currentVolume + deltaPercentage;
    return setVolume(newVolume);
}

float VolumeController::percentageToScalar(float percentage)
{
    return percentage / 100.0f;
}

float VolumeController::scalarToPercentage(float scalar)
{
    float percentage = scalar * 100.0f;

    // Round to specified decimal precision (e.g., 2 decimal places: 0.01)
    float precision = std::pow(10.0f, static_cast<float>(VOLUME_DECIMAL_PRECISION));
    return std::round(percentage * precision) / precision;
}

float VolumeController::clampVolumePercentage(float percentage)
{
    if (percentage < VOLUME_MIN_PERCENTAGE) {
        percentage = VOLUME_MIN_PERCENTAGE;
    }
    else if (percentage > VOLUME_MAX_PERCENTAGE) {
        percentage = VOLUME_MAX_PERCENTAGE;
    }

    // Round to specified decimal precision (e.g., 2 decimal places: 0.01)
    float precision = std::pow(10.0f, static_cast<float>(VOLUME_DECIMAL_PRECISION));
    return std::round(percentage * precision) / precision;
}

bool VolumeController::setMute(bool muted)
{
    if (!m_isInitialized || !m_endpointVolume) {
        return false;
    }

    // Use QKeyMapper-specific GUID to identify this mute change event
    HRESULT hr = m_endpointVolume->SetMute(muted ? TRUE : FALSE, &s_QKeyMapperVolumeGUID);
    if (FAILED(hr)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[VolumeController::setMute] Failed to set mute state to" << muted << ", hr =" << QString::number(hr, 16);
#endif
        return false;
    }

#ifdef DEBUG_LOGOUT_ON
    qDebug() << "[VolumeController::setMute] Mute state set to" << muted;
#endif
    return true;
}

bool VolumeController::isMuted()
{
    if (!m_isInitialized || !m_endpointVolume) {
        return false;
    }

    BOOL muted = FALSE;
    HRESULT hr = m_endpointVolume->GetMute(&muted);
    if (FAILED(hr)) {
#ifdef DEBUG_LOGOUT_ON
        qDebug() << "[VolumeController::isMuted] Failed to get mute state, hr =" << QString::number(hr, 16);
#endif
        return false;
    }

    return (muted == TRUE);
}

bool VolumeController::applyWindowsMuteLogic(float volumePercentage)
{
    if (!m_isInitialized || !m_endpointVolume) {
        return false;
    }

    // Apply Windows-like mute logic:
    // - Setting volume to 0% should mute the system
    // - Setting volume to any non-zero value should unmute the system
    bool shouldMute = (volumePercentage <= VOLUME_EPSILON);

    return setMute(shouldMute);
}
