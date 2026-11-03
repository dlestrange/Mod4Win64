#include "ModuleAudio.h"
#include <QAudioFormat>
#include <QFile>
#include <QFileInfo>
#include <QMediaDevices>
#include <QMetaObject>
#include <cstring>
#include <vector>
#include <algorithm>

ModuleAudio::ModuleAudio(QObject* parent) : QIODevice(parent) {
 pumpTimer_.setInterval(10);
 connect(&pumpTimer_, &QTimer::timeout, this, &ModuleAudio::pumpAudio);
}
ModuleAudio::~ModuleAudio() { stop(); if (module_) destroy_(module_); }

bool ModuleAudio::load(const QString& path, QString* message) {
 stop();
 if (module_) { destroy_(module_); module_=nullptr; }
 if (!library_.isLoaded()) {
   // libopenmpt.dll needs to be in the executable folder or Windows DLL search path.
   library_.setFileName("libopenmpt");
   if (!library_.load()) {
     if (message) *message="libopenmpt.dll not found. Place the official x64 DLL and dependencies beside Mod4Win64.exe. "+library_.errorString();
     return false;
   }
   create_=reinterpret_cast<CreateFn>(library_.resolve("openmpt_module_create_from_memory2"));
   read_=reinterpret_cast<ReadFn>(library_.resolve("openmpt_module_read_interleaved_stereo"));
   destroy_=reinterpret_cast<DestroyFn>(library_.resolve("openmpt_module_destroy"));
   seek_=reinterpret_cast<SeekFn>(library_.resolve("openmpt_module_set_position_seconds"));
   position_=reinterpret_cast<PositionFn>(library_.resolve("openmpt_module_get_position_seconds"));
   metadata_=reinterpret_cast<MetadataFn>(library_.resolve("openmpt_module_get_metadata"));
   freeString_=reinterpret_cast<FreeStringFn>(library_.resolve("openmpt_free_string"));
   numChannels_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_num_channels"));
   numPatterns_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_num_patterns"));
   currentSpeed_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_current_speed"));
   currentTempo_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_current_tempo"));
   currentPattern_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_current_pattern"));
   currentRow_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_current_row"));
   patternRows_=reinterpret_cast<PatternRowsFn>(library_.resolve("openmpt_module_get_pattern_num_rows"));
   patternCommand_=reinterpret_cast<PatternCommandFn>(library_.resolve("openmpt_module_get_pattern_row_channel_command"));
   vuLeft_=reinterpret_cast<VuFn>(library_.resolve("openmpt_module_get_current_channel_vu_left"));
   vuRight_=reinterpret_cast<VuFn>(library_.resolve("openmpt_module_get_current_channel_vu_right"));
   if (!create_ || !read_ || !destroy_) {
     if (message) *message="libopenmpt DLL is missing required API functions.";
     return false;
   }
 }
 QFile file(path);
 if (!file.open(QIODevice::ReadOnly)) {
   if (message) *message=file.errorString();
   return false;
 }
 QByteArray bytes=file.readAll();
 if (bytes.isEmpty()) { if(message) *message="Module file is empty."; return false; }
 module_=create_(bytes.constData(),static_cast<size_t>(bytes.size()),nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr);
 if(!module_) { if(message) *message="Unable to decode this tracker module."; return false; }
 loadedPath_=path;
 title_=QFileInfo(path).fileName();

 if (metadata_ && freeString_) {
   char* name=metadata_(module_,"title");
   if(name) {
     const QString decoded=QString::fromUtf8(name).trimmed();
     if(!decoded.isEmpty()) title_=decoded;
     freeString_(name);
   }
 }
 usedCommands_.clear();
 if (numChannels_ && numPatterns_ && patternRows_ && patternCommand_) {
   const int channels=qBound(0,numChannels_(module_),256);
   const int patterns=qBound(0,numPatterns_(module_),2048);
   for (int pat=0;pat<patterns;++pat) {
     const int rows=qBound(0,patternRows_(module_,pat),1024);
     for(int row=0;row<rows;++row)
       for(int ch=0;ch<channels;++ch) {
         const int command=patternCommand_(module_,pat,row,ch,3);
         if(command>0 && !usedCommands_.contains(command)) usedCommands_.append(command);
       }
   }
   std::sort(usedCommands_.begin(),usedCommands_.end());
 }
 emit stateChanged("READY");
 return true;
}

void ModuleAudio::play() {
 if (!module_) { emit stateChanged("NO MODULE LOADED"); return; }
 if (paused_) { pauseResume(); return; }
 if (playing_) { restart(); return; }
 QAudioFormat fmt;
 fmt.setSampleRate(48000);
 fmt.setChannelCount(2);
 fmt.setSampleFormat(QAudioFormat::Int16);
 const auto device=QMediaDevices::defaultAudioOutput();
 if (!device.isFormatSupported(fmt)) {
   emit stateChanged("48 KHZ STEREO NOT SUPPORTED");
   return;
 }
 sink_=std::make_unique<QAudioSink>(device,fmt);
 sink_->setBufferSize(48000 * 2 * 2 / 4);
 finished_=false;
 decodedFrames_=0;
 nonzeroSamples_=0;
 effectHistory_.clear();
 presentedEffects_=decoderSnapshot();
 playing_=true; paused_=false;
 pending_.clear();
 connect(sink_.get(), &QAudioSink::stateChanged, this, [this](QAudio::State state) {
     if (!sink_) return;
     if (state == QAudio::StoppedState && sink_->error() != QtAudio::NoError) {
         const QString problem = QString("AUDIO ERROR %1").arg(static_cast<int>(sink_->error()));
         QMetaObject::invokeMethod(this, [this, problem] { stop(); emit stateChanged(problem); }, Qt::QueuedConnection);
     }
 });
 outputDevice_ = sink_->start();
 if (!outputDevice_ || sink_->error()!=QtAudio::NoError) {
   stop(); emit stateChanged("AUDIO OUTPUT ERROR"); return;
 }
 pumpTimer_.start();
 pumpAudio();
 emit stateChanged("PLAYING");
}
void ModuleAudio::pumpAudio() {
 if (!sink_ || !outputDevice_ || !playing_ || paused_) return;
 // In Qt push mode QAudioSink exposes a writable device; generate only as much PCM
 // as its buffer can accept. Keep any partially written bytes for the next tick.
 int available = sink_->bytesFree();
 while (available >= 4) {
   if (pending_.isEmpty()) {
     const int bytes = qMin(available - (available % 4), 4096);
     if (bytes <= 0) break;
     QByteArray buffer(bytes, '\0');
     const qint64 count = readData(buffer.data(), buffer.size());
     if (count <= 0) break;
     buffer.resize(static_cast<qsizetype>(count));
     pending_ = std::move(buffer);
   }
   const qint64 written = outputDevice_->write(pending_.constData(),
                  qMin(static_cast<qint64>(available), static_cast<qint64>(pending_.size())));
   if (written <= 0) break;
   pending_.remove(0, static_cast<qsizetype>(written));
   available -= static_cast<int>(written);
 }
}
void ModuleAudio::restart() {
 if (!module_) return;
 stop();
 if (seek_) seek_(module_, 0.0);
 play();
}
void ModuleAudio::pauseResume() {
 if(!playing_ || !sink_) return;
 if(paused_) { sink_->resume(); paused_=false; pumpTimer_.start(); emit stateChanged("PLAYING"); }
 else { pumpTimer_.stop(); sink_->suspend(); paused_=true; emit stateChanged("PAUSED"); }
}
void ModuleAudio::stop() {
 playing_=false; paused_=false;
 pumpTimer_.stop(); pending_.clear(); outputDevice_=nullptr;
 effectHistory_.clear();
 presentedEffects_=EffectSnapshot{};
 if(sink_) { sink_->stop(); sink_.reset(); }
 if(isOpen()) close();
 if (module_ && seek_) seek_(module_,0.0);
 emit stateChanged("STOPPED");
}
bool ModuleAudio::isPlaying() const { return playing_; }
bool ModuleAudio::isPaused() const { return paused_; }
qint64 ModuleAudio::readData(char* data,qint64 maxlen) {
 if(!module_ || !playing_ || maxlen<4) return 0;
 const auto frames=static_cast<size_t>(maxlen/4);
 const auto produced=read_(module_,48000,frames,reinterpret_cast<int16_t*>(data));
 if(produced==0 && !finished_) {
   finished_=true;
   QMetaObject::invokeMethod(this,[this] { emit endOfModule(); stop(); },Qt::QueuedConnection);
 }
 const quint64 firstFrame=decodedFrames_.fetch_add(produced);
 // Capture the actual decoder state for each generated PCM block. The state
 // reflects the END of this block; we associate it with the block's first
 // frame, which keeps the visual approximation within one 1024-frame chunk.
 if(produced>0) effectHistory_.enqueue({firstFrame,decoderSnapshot()});
 quint64 audible = 0;
 const auto* samples = reinterpret_cast<const int16_t*>(data);
 for (size_t i = 0; i < produced * 2; ++i) {
     if (samples[i] != 0) ++audible;
 }
 nonzeroSamples_.fetch_add(audible);
 return static_cast<qint64>(produced*4);
}

EffectSnapshot ModuleAudio::effectsSnapshot() {
 // QAudioSink::processedUSecs reports how much audio has passed through its
 // backend since start, excluding suspended time. Unlike decoder position,
 // this clock follows the sound being rendered instead of queued PCM.
 if (sink_ && playing_ && !effectHistory_.isEmpty()) {
   const qint64 us=std::max<qint64>(0,sink_->processedUSecs());
   const quint64 played=static_cast<quint64>(us)*48000ULL/1000000ULL;
   while (!effectHistory_.isEmpty() && effectHistory_.head().frame<=played)
     presentedEffects_=effectHistory_.dequeue().state;
   return presentedEffects_;
 }
 if (sink_ && playing_) return presentedEffects_;
 return decoderSnapshot();
}

EffectSnapshot ModuleAudio::decoderSnapshot() const {
 EffectSnapshot result;
 if (!module_ || !numChannels_) return result;
 result.moduleIdentity=loadedPath_;
 result.channels=qBound(0,numChannels_(module_),256);
 result.usedCommands=usedCommands_;
 result.currentCommands.fill(0,result.channels);
 result.notes.fill(0,result.channels);
 result.instruments.fill(0,result.channels);
 result.vuLeft.fill(0.0f,result.channels);
 result.vuRight.fill(0.0f,result.channels);
 if(currentSpeed_) result.speed=currentSpeed_(module_);
 if(currentTempo_) result.bpm=currentTempo_(module_);
 if(currentPattern_) result.pattern=currentPattern_(module_);
 if(currentRow_) result.row=currentRow_(module_);
 if(position_) result.elapsedSeconds=position_(module_);
 if(patternCommand_ && result.pattern>=0 && result.row>=0 &&
    patternRows_ && result.row<patternRows_(module_,result.pattern)) {
   for(int ch=0;ch<result.channels;++ch) {
     result.currentCommands[ch]=patternCommand_(module_,result.pattern,result.row,ch,3);
     result.notes[ch]=patternCommand_(module_,result.pattern,result.row,ch,0);
     result.instruments[ch]=patternCommand_(module_,result.pattern,result.row,ch,1);
   }
 }
 for(int ch=0;ch<result.channels;++ch) {
   if(vuLeft_) result.vuLeft[ch]=qBound(0.0f,vuLeft_(module_,ch),1.0f);
   if(vuRight_) result.vuRight[ch]=qBound(0.0f,vuRight_(module_,ch),1.0f);
 }
 return result;
}
