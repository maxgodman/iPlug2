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

private:
  IPlugAPPHost* mAppHost = nullptr;
  IPlugQueue<IMidiMsg> mMidiMsgsFromCallback {MIDI_TRANSFER_SIZE};
  IPlugQueue<SysExData> mSysExMsgsFromCallback {SYSEX_TRANSFER_SIZE};

  friend class IPlugAPPHost;
};

IPlugAPP* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif
