#include "ModuleAudio.h"
#include <QAudioFormat>
#include <QFile>
#include <QFileInfo>
#include <QMediaDevices>
#include <QMetaObject>
#include <cstring>
#include <vector>
#include <algorithm>
#include <QSet>

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
   numInstruments_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_num_instruments"));
   numSamples_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_num_samples"));
   numOrders_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_num_orders"));
   instrumentName_=reinterpret_cast<IndexedNameFn>(library_.resolve("openmpt_module_get_instrument_name"));
   sampleName_=reinterpret_cast<IndexedNameFn>(library_.resolve("openmpt_module_get_sample_name"));
   orderPattern_=reinterpret_cast<OrderPatternFn>(library_.resolve("openmpt_module_get_order_pattern"));
   currentSpeed_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_current_speed"));
   currentTempo_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_current_tempo"));
   currentPattern_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_current_pattern"));
   currentRow_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_current_row"));
   currentOrder_=reinterpret_cast<IntFn>(library_.resolve("openmpt_module_get_current_order"));
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
 if(currentOrder_) result.order=currentOrder_(module_);
 if(numOrders_) result.orderCount=qBound(0,numOrders_(module_),4096);
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


namespace {
// FastTracker XM stores optional text-only instrument slots. Mod4Win's
// "with N instruments" statistic counts slots containing actual sample data,
// not all names. Parse headers defensively; never change playback data.
int countXmPlayableInstruments(const QByteArray& bytes) {
    if(bytes.size()<80 || !bytes.startsWith("Extended Module: ")) return -1;
    const qsizetype length=bytes.size();
    const auto u16=[&](qsizetype at)->quint32 {
        if(at<0 || at>length-2) return 0;
        const auto* p=reinterpret_cast<const unsigned char*>(bytes.constData()+at);
        return quint32(p[0]) | (quint32(p[1])<<8);
    };
    const auto u32=[&](qsizetype at)->quint32 {
        if(at<0 || at>length-4) return 0;
        const auto* p=reinterpret_cast<const unsigned char*>(bytes.constData()+at);
        return quint32(p[0]) | (quint32(p[1])<<8) |
               (quint32(p[2])<<16) | (quint32(p[3])<<24);
    };
    const quint32 headerSize=u32(60);
    const quint32 patterns=u16(70);
    const quint32 instruments=u16(72);
    if(headerSize<20 || patterns>2048 || instruments>512) return -1;
    qsizetype pos=60+qsizetype(headerSize);
    if(pos>length) return -1;
    for(quint32 i=0;i<patterns;++i) {
        if(pos>length-9) return -1;
        const quint32 header=u32(pos);
        const quint32 packed=u16(pos+7);
        if(header<9 || qsizetype(header)>length-pos) return -1;
        pos+=qsizetype(header);
        if(qsizetype(packed)>length-pos) return -1;
        pos+=qsizetype(packed);
    }
    int populated=0;
    for(quint32 i=0;i<instruments;++i) {
        if(pos>length-29) return -1;
        const quint32 header=u32(pos);
        const quint32 samples=u16(pos+27);
        if(header<29 || header>4096 || samples>512 || qsizetype(header)>length-pos)
            return -1;
        const qsizetype sampleHeaders=pos+qsizetype(header);
        if(samples==0) { pos=sampleHeaders; continue; }
        if(header<33) return -1;
        const quint32 sampleHeaderSize=u32(pos+29);
        if(sampleHeaderSize<40 || sampleHeaderSize>1024 ||
           qsizetype(samples)*sampleHeaderSize>length-sampleHeaders) return -1;
        qsizetype sampleDataSize=0;
        bool containsAudio=false;
        for(quint32 j=0;j<samples;++j) {
            const qsizetype sampleAt=sampleHeaders+qsizetype(j)*sampleHeaderSize;
            const quint32 sampleBytes=u32(sampleAt);
            if(sampleBytes>0) containsAudio=true;
            if(qsizetype(sampleBytes)>length-sampleDataSize) return -1;
            sampleDataSize+=qsizetype(sampleBytes);
        }
        const qsizetype next=sampleHeaders+qsizetype(samples)*sampleHeaderSize;
        if(sampleDataSize>length-next) return -1;
        pos=next+sampleDataSize;
        if(containsAudio) ++populated;
    }
    return populated;
}
} // namespace

ModuleInfo ModuleAudio::moduleInfo() const {
 ModuleInfo info;
 if(!module_) return info;
 info.title=title_;
 info.filePath=loadedPath_;
 info.fileBytes=QFileInfo(loadedPath_).size();
 if(loadedPath_.endsWith(".xm",Qt::CaseInsensitive)) {
   QFile source(loadedPath_);
   if(source.open(QIODevice::ReadOnly))
     info.playableInstrumentCount=countXmPlayableInstruments(source.readAll());
 }
 info.channels=numChannels_?qBound(0,numChannels_(module_),256):0;
 info.instrumentSlots=numInstruments_?qBound(0,numInstruments_(module_),512):0;
 info.sampleSlots=numSamples_?qBound(0,numSamples_(module_),512):0;
 info.patternCount=numPatterns_?qBound(0,numPatterns_(module_),2048):0;
 info.orderCount=numOrders_?qBound(0,numOrders_(module_),4096):0;
 auto metadataValue=[this](const char* key)->QString {
   if(!metadata_ || !freeString_) return {};
   const char* ptr=metadata_(module_,key);
   if(!ptr) return {};
   const QString result=QString::fromUtf8(ptr).trimmed();
   freeString_(ptr);
   return result;
 };
 info.format=metadataValue("type_long");
 info.tracker=metadataValue("tracker");
 // XM and IT have a separate instrument layer. S3M and MOD conventionally
 // store names in sample slots; do not discard text used as composer notes.
 const bool useInstruments=info.instrumentSlots>0 && instrumentName_;
 const int count=useInstruments?info.instrumentSlots:info.sampleSlots;
 const IndexedNameFn getName=useInstruments?instrumentName_:sampleName_;
 for(int i=0;i<count;++i) {
   QString name;
   if(getName && freeString_) {
     const char* p=getName(module_,i);
     if(p) { name=QString::fromUtf8(p); freeString_(p); }
   }
   info.names.append(name);
 }
 if(orderPattern_) {
   QSet<int> distinct;
   for(int order=0;order<info.orderCount;++order) {
     const int pat=orderPattern_(module_,order);
     if(pat>=0 && pat<info.patternCount) distinct.insert(pat);
   }
   info.distinctPatternsInSequence=distinct.size();
 }
 return info;
}
