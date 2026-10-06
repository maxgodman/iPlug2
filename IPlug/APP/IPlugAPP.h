/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#ifndef _IPLUGAPI_
#define _IPLUGAPI_

/**
 * @file
 * @copydoc IPlugAPP
 */


#include <atomic>

#include "IPlugPlatform.h"
#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"

BEGIN_IPLUG_NAMESPACE

struct InstanceInfo
{
  void* pAppHost;
};

class IPlugAPPHost;

/** What the audio callback has cost since it was last asked, for a DSP load meter.
 * Each callback must finish within the time its buffer lasts, which is
 * frames / sampleRate (5.33 ms for 256 frames at 48 kHz), or the audio drops out.
 * The load is worstSeconds divided by that time: 1.0 means the slowest callback
 * used all of it. */
struct AppCallbackLoad
{
  /** The longest callback since the last call, in seconds */
  double worstSeconds = 0.;
  /** The callback's length in frames: the device's buffer size. 0 while no stream is open or before its first callback */
  uint32_t frames = 0;
  /** The stream's sample rate */
  double sampleRate = 0.;
  /** How many callbacks ran since the last call. None is ordinary at a large buffer, whose callbacks are further apart than a UI's frames */
  uint32_t callbacks = 0;
};

/** The audio of one callback, as the tap is handed it. See IAppAudioTap.
 * Every pointer in it is valid only for the duration of the call */
struct AppAudioTapBlock
{
  /** One pointer per plug-in input channel */
  const double* const* inputs = nullptr;
  int nInputs = 0;
  /** One pointer per output channel of the buffer the device plays, after
   * APP_MULT and before the open/close fades; the tap may add to them */
  double* const* outputs = nullptr;
  int nOutputs = 0;
  /** The tap's extra device input, or null when the stream carries none */
  const double* extraInput = nullptr;
  int nFrames = 0;
  double sampleRate = 0.;
  /** When the callback began, in steady_clock nanoseconds */
  int64_t callbackStartNs = 0;
  /** Rises with every stream that starts, so a tap can tell one from the next */
  uint32_t stream = 0;
};

/** Receives every audio callback of the standalone app once it starts
 * processing audio, so a plug-in can send its audio somewhere besides the
 * device or mix in audio of its own. OnAppAudio runs on the audio thread,
 * under the same rules as ProcessBlock */
class IAppAudioTap
{
public:
  virtual ~IAppAudioTap() = default;
  virtual void OnAppAudio(const AppAudioTapBlock& block) = 0;
};

/**  Standalone application base class for an IPlug plug-in
*   @ingroup APIClasses */
class IPlugAPP : public IPlugAPIBase
               , public IPlugProcessor
{
public:
  IPlugAPP(const InstanceInfo& info, const Config& config);
  
  //IPlugAPIBase
  void BeginInformHostOfParamChange(int idx) override {};
  void InformHostOfParamChange(int idx, double normalizedValue) override {};
  void EndInformHostOfParamChange(int idx) override {};
  void InformHostOfPresetChange() override {};
  bool EditorResize(int viewWidth, int viewHeight) override;

  //IEditorDelegate
  void SendSysexMsgFromUI(const ISysEx& msg) override;
  
  //IPlugProcessor
  bool SendMidiMsg(const IMidiMsg& msg) override;
  bool SendSysEx(const ISysEx& msg) override;
  
  //IPlugAPP
  void AppProcess(double** inputs, double** outputs, int nFrames);

  /** What the audio callback has cost since the last call. Each call starts
   * a new period. Call on the main thread. See AppCallbackLoad */
  AppCallbackLoad TakeCallbackLoad();

  /** Sends every audio callback to pTap from now on; null stops it. Set it on
   * the main thread, from the constructor, and keep the tap alive while it is
   * set: swapping it does not wait for a callback already running */
  void SetAppAudioTap(IAppAudioTap* pTap);

  /** Opens one more input on the device the plug-in reads from, which the tap
   * is handed as AppAudioTapBlock::extraInput. 1-based; 0 for none. Reopens a
   * running stream, which resets the plug-in. Returns whether the extra input
   * is in place: false for 0, for a channel the device does not have, or when
   * the wider run of inputs will not open. Before the first stream nothing can
   * refuse a request, so it is reported as in place and GetExtraInputChannel
   * says 0 until the stream carries it. Call it on the main thread; asked from
   * the constructor, the first stream opens with it */
  bool SetExtraInputChannel(uint32_t channel);

  /** How many inputs the open device has; 0 while no stream is open */
  uint32_t GetDeviceInputChannels() const;

  /** The extra device input the open stream carries, 1-based; 0 while none is */
  uint32_t GetExtraInputChannel() const;

  /** The first device input the plug-in reads, 1-based; 0 while no stream is
   * open, or the plug-in reads no inputs */
  uint32_t GetPlugInputChannel() const;

private:
  IPlugAPPHost* mAppHost = nullptr;
  /** The plug-in is made inside the host's constructor, before the host's own
   * members exist, so what it asks of the host from its constructor waits here
   * until Init has run */
  bool mHostReady = false;
  /** Set on the main thread, read on the audio thread */
  std::atomic<IAppAudioTap*> mAudioTap {nullptr};
  /** The extra input the plug-in wants. Asked before the first stream opens,
   * the request waits here until it does */
  uint32_t mExtraInputChannel = 0;
  IPlugQueue<IMidiMsg> mMidiMsgsFromCallback {MIDI_TRANSFER_SIZE};
  IPlugQueue<SysExData> mSysExMsgsFromCallback {SYSEX_TRANSFER_SIZE};

  friend class IPlugAPPHost;
};

IPlugAPP* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif
