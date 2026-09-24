/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 
 IPlug plug-in -> Standalone app wrapper, using Cockos' SWELL
 
 Oli Larkin 2014-2025
 
 Notes:
 
 App settings are stored in a .ini (text) file. The location is as follows,
 where APP_SETTINGS_SUBPATH defaults to BUNDLE_NAME:
 
 Windows7: C:\Users\USERNAME\AppData\Local\APP_SETTINGS_SUBPATH\settings.ini
 Windows XP/Vista: C:\Documents and Settings\USERNAME\Local Settings\Application Data\APP_SETTINGS_SUBPATH\settings.ini
 macOS: /Users/USERNAME/Library/Application\ Support/APP_SETTINGS_SUBPATH/settings.ini
 OR
 /Users/USERNAME/Library/Containers/BUNDLE_ID/Data/Library/Application Support/APP_SETTINGS_SUBPATH/settings.ini
 
 Define APP_SETTINGS_SUBPATH in config.h to override it. It may name more than
 one folder, e.g. "Company\Product"; the intermediate folders are created.
 
 */

#include <atomic>
#include <cstdlib>
#include <string>
#include <vector>
#include <limits>
#include <memory>
#include <optional>

#include "wdltypes.h"
#include "wdlstring.h"

#include "IPlugPlatform.h"
#include "IPlugConstants.h"

#include "IPlugAPP.h"

#include "config.h"

// Below config.h, so a product's own definition is seen first and this is
// only a fallback. BUNDLE_NAME comes from the same file.
#ifndef APP_SETTINGS_SUBPATH
  #define APP_SETTINGS_SUBPATH BUNDLE_NAME
#endif

#ifdef OS_WIN
  #include <WindowsX.h>
  #include <commctrl.h>
  #include <shlobj.h>
  #define DEFAULT_INPUT_DEV "Default Device"
  #define DEFAULT_OUTPUT_DEV "Default Device"
  // Posted to the main window when the device has closed the stream by itself,
  // which an ASIO driver does when its buffer size or sample rate is changed
  // outside the app. LPARAM is a handle to the thread still closing it, or NULL.
  #define WM_APP_AUDIO_DEVICE_RESET (WM_APP + 1)
  // Drives the reopen that follows WM_APP_AUDIO_DEVICE_RESET.
  #define IDT_AUDIO_RESET_TIMER 1002
#elif defined(OS_MAC)
  #include "IPlugSWELL.h"
  #define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
  #define DEFAULT_INPUT_DEV "Built-in Input"
  #define DEFAULT_OUTPUT_DEV "Built-in Output"
#elif defined(OS_LINUX)
  #include "IPlugSWELL.h"
#endif

#include "RtAudio.h"
#include "RtMidi.h"

#define OFF_TEXT "off"

extern HWND gHWND;
extern HINSTANCE gHINSTANCE;

BEGIN_IPLUG_NAMESPACE

const int kNumBufferSizeOptions = 11;
const std::string kBufferSizeOptions[kNumBufferSizeOptions] = {"32", "64", "96", "128", "192", "256", "512", "1024", "2048", "4096", "8192" };
const int kDeviceDS = 0; const int kDeviceCoreAudio = 0; const int kDeviceAlsa = 0;
const int kDeviceASIO = 1; const int kDeviceJack = 1;
extern UINT gSCROLLMSG;

class IPlugAPP;

/** A class that hosts an IPlug as a standalone app and provides Audio/Midi I/O */
class IPlugAPPHost
{
public:
  
  /** Used to manage changes to app I/O */
  struct AppState
  {
    WDL_String mAudioInDev;
    WDL_String mAudioOutDev;
    WDL_String mMidiInDev;
    WDL_String mMidiOutDev;
    uint32_t mAudioDriverType;
    uint32_t mAudioSR;
    uint32_t mBufferSize;
    uint32_t mMidiInChan;
    uint32_t mMidiOutChan;
    
    uint32_t mAudioInChanL;
    uint32_t mAudioInChanR;
    uint32_t mAudioOutChanL;
    uint32_t mAudioOutChanR;
    
    AppState()
    : mAudioInDev(DEFAULT_INPUT_DEV)
    , mAudioOutDev(DEFAULT_OUTPUT_DEV)
    , mMidiInDev(OFF_TEXT)
    , mMidiOutDev(OFF_TEXT)
    , mAudioDriverType(0) // DirectSound / CoreAudio by default
    , mBufferSize(512)
    , mAudioSR(44100)
    , mMidiInChan(0)
    , mMidiOutChan(0)
    
    , mAudioInChanL(1)
    , mAudioInChanR(2)
    , mAudioOutChanL(1)
    , mAudioOutChanR(2)
    {
    }
    
    AppState (const AppState& obj)
    : mAudioInDev(obj.mAudioInDev.Get())
    , mAudioOutDev(obj.mAudioOutDev.Get())
    , mMidiInDev(obj.mMidiInDev.Get())
    , mMidiOutDev(obj.mMidiOutDev.Get())
    , mAudioDriverType(obj.mAudioDriverType)
    , mBufferSize(obj.mBufferSize)
    , mAudioSR(obj.mAudioSR)
    , mMidiInChan(obj.mMidiInChan)
    , mMidiOutChan(obj.mMidiOutChan)
    
    , mAudioInChanL(obj.mAudioInChanL)
    , mAudioInChanR(obj.mAudioInChanR)
    , mAudioOutChanL(obj.mAudioOutChanL)
    , mAudioOutChanR(obj.mAudioOutChanR)
    {
    }
    
    bool operator==(const AppState& rhs) const {
      return (rhs.mAudioDriverType == mAudioDriverType &&
              rhs.mBufferSize == mBufferSize &&
              rhs.mAudioSR == mAudioSR &&
              rhs.mMidiInChan == mMidiInChan &&
              rhs.mMidiOutChan == mMidiOutChan &&
              (std::string_view(rhs.mAudioInDev.Get()) == mAudioInDev.Get()) &&
              (std::string_view(rhs.mAudioOutDev.Get()) == mAudioOutDev.Get()) &&
              (std::string_view(rhs.mMidiInDev.Get()) == mMidiInDev.Get()) &&
              (std::string_view(rhs.mMidiOutDev.Get()) == mMidiOutDev.Get()) &&
              rhs.mAudioInChanL == mAudioInChanL &&
              rhs.mAudioInChanR == mAudioInChanR &&
              rhs.mAudioOutChanL == mAudioOutChanL &&
              rhs.mAudioOutChanR == mAudioOutChanR
      );
    }
    bool operator!=(const AppState& rhs) const { return !operator==(rhs); }
  };
  
  static IPlugAPPHost* Create();
  static std::unique_ptr<IPlugAPPHost> sInstance;

  /** Set screenshot path for CLI screenshot mode
   * @param path Path to save the screenshot to (empty to disable) */
  void SetScreenshotPath(const char* path) { mScreenshotPath.Set(path); }

  /** Get screenshot path (empty if not in screenshot mode)
   * @return The screenshot path or empty string */
  const char* GetScreenshotPath() const { return mScreenshotPath.Get(); }

  /** Check if in screenshot mode
   * @return true if a screenshot should be taken and app should exit */
  bool IsScreenshotMode() const { return mScreenshotPath.GetLength() > 0; }

  /** Set no-I/O mode (disables audio and MIDI initialization)
   * @param noIO true to disable I/O */
  void SetNoIO(bool noIO) { mNoIO = noIO; }

  /** Check if I/O is disabled
   * @return true if audio and MIDI I/O is disabled */
  bool IsNoIO() const { return mNoIO; }

  void PopulateSampleRateList(HWND hwndDlg, RtAudio::DeviceInfo* pInputDevInfo, RtAudio::DeviceInfo* pOutputDevInfo);
  void PopulateAudioInputList(HWND hwndDlg, RtAudio::DeviceInfo* pInfo);
  void PopulateAudioOutputList(HWND hwndDlg, RtAudio::DeviceInfo* pInfo);
  void PopulateDriverSpecificControls(HWND hwndDlg);
  void PopulateAudioDialogs(HWND hwndDlg);
  bool PopulateMidiDialogs(HWND hwndDlg);
  void PopulatePreferencesDialog(HWND hwndDlg);
  
  IPlugAPPHost();
  ~IPlugAPPHost();
  
  bool OpenWindow(HWND pParent);
  void CloseWindow();

  bool Init();
  bool InitState();
  void UpdateINI();
  
  /** Returns the name of the audio device with a given RTAudio device ID
   * @param deviceID The ID RTAudio has given the audio device
   * @return The device name. Core Audio device names are truncated. */
  std::string GetAudioDeviceName(uint32_t deviceID) const;
  
  /** Returns the a validated audio device ID linked to a particular name
  * @param name The name of the audio device to test
  * @return The ID RTAudio has given the audio device if found */
  std::optional<uint32_t> GetAudioDeviceID(const char* name) const;
  
  /** @param direction Either kInput or kOutput
   * @param name The name of the midi device
   * @return An integer specifying the output port number, where 0 means any */
  int GetMIDIPortNumber(ERoute direction, const char* name) const;
  
  void ProbeAudioIO();
  void ProbeMidiIO();
  bool InitMidi();
  void CloseAudio();
  bool InitAudio(uint32_t inId, uint32_t outId, uint32_t sr, uint32_t iovs);

  /** Clamp a route's stored channel selection in mState so the plugin's run of
   * channels fits the device, deriving the R channel from the L one.
   * @param route Either kInput or kOutput
   * @param nDeviceChannels The number of channels the device has on that route
   * @return The zero-based offset to pass to RtAudio as firstChannel */
  uint32_t ClampAudioChans(ERoute route, uint32_t nDeviceChannels);
  bool AudioSettingsInStateAreEqual(AppState& os, AppState& ns);
  bool MIDISettingsInStateAreEqual(AppState& os, AppState& ns);

  bool TryToChangeAudioDriverType();

  /** Open the audio stream on the devices named in mState
   * @param followDevice For ASIO, open at the sample rate and buffer size the
   * driver is already running at, rather than the ones in mState, and store
   * those in mState. Launching and a driver reset use this, so the app only
   * changes a device setting when it is changed in the app's own dialog.
   * Ignored for other driver types.
   * @return true if the stream opened and started */
  bool TryToChangeAudio(bool followDevice = false);
  bool SelectMIDIDevice(ERoute direction, const char* portName);

#ifdef OS_WIN
  /** Handles WM_APP_AUDIO_DEVICE_RESET on the main thread, and takes ownership
   * of the handle */
  void OnAudioDeviceReset(HANDLE closingThread);
  /** Handles IDT_AUDIO_RESET_TIMER on the main thread */
  void OnAudioResetTimer();
  /** Set when a reset arrives before the main window exists to be told */
  static std::atomic<bool> sAudioResetBeforeWindow;
#endif
  
  static int AudioCallback(void* pOutputBuffer, void* pInputBuffer, uint32_t nFrames, double streamTime, RtAudioStreamStatus status, void* pUserData);
  static void MIDICallback(double deltatime, std::vector<uint8_t>* pMsg, void* pUserData);
  static void ErrorCallback(RtAudioErrorType type, const std::string& errorText);

  static WDL_DLGRET PreferencesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

  IPlugAPP* GetPlug() { return mIPlug.get(); }
private:
  std::unique_ptr<IPlugAPP> mIPlug = nullptr;
  std::unique_ptr<RtAudio> mDAC = nullptr;
  std::unique_ptr<RtMidiIn> mMidiIn = nullptr;
  std::unique_ptr<RtMidiOut> mMidiOut = nullptr;
  int mMidiOutChannel = -1;
  int mMidiInChannel = -1;

  AppState mState;
  /** When the preferences dialog is opened the existing state is cached here, and restored if cancel is pressed */
  AppState mTempState;
  /** When the audio driver is started the current state is copied here so that if OK is pressed after APPLY nothing is changed */
  AppState mActiveState;
  
  double mSampleRate = 44100.;
  uint32_t mSamplesElapsed = 0;
  uint32_t mVecWait = 0;
  uint32_t mBufferSize = 512;
  uint32_t mBufIndex = 0; // index for signal vector, loops from 0 to mSigVS
  bool mExiting = false;
  bool mAudioEnding = false;
  bool mAudioDone = false;
  bool mNoIO = false;

  /** The ID of the operating system's default input device if detected */
  std::optional<uint32_t> mDefaultInputDev;
  /** The ID of the operating system's default output device if detected */
  std::optional<uint32_t> mDefaultOutputDev;

#ifdef OS_WIN
  /** The thread RtAudio is closing the stream on after a driver reset.
   * The stream cannot be reopened until it has finished. */
  HANDLE mAudioResetThread = NULL;
  /** Timer ticks spent reopening after a driver reset, so that a device which
   * has gone for good is not retried forever */
  int mAudioResetTicks = 0;
#endif

  WDL_String mINIPath;
  WDL_String mScreenshotPath;

  std::vector<uint32_t> mAudioInputDevIDs;
  std::vector<uint32_t> mAudioOutputDevIDs;
  std::vector<std::string> mMidiInputDevNames;
  std::vector<std::string> mMidiOutputDevNames;
  
  WDL_PtrList<double> mInputBufPtrs;
  WDL_PtrList<double> mOutputBufPtrs;
  
  friend class IPlugAPP;
};

END_IPLUG_NAMESPACE
