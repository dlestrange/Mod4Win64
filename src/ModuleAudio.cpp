#include "ModuleAudio.h"
#include <QAudioFormat>
#include <QFile>
#include <QFileInfo>
#include <QMediaDevices>
#include <QMetaObject>
#include <cstring>
#include <vector>

ModuleAudio::ModuleAudio(QObject* parent) : QIODevice(parent) {}
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
   metadata_=reinterpret_cast<MetadataFn>(library_.resolve("openmpt_module_get_metadata"));
   freeString_=reinterpret_cast<FreeStringFn>(library_.resolve("openmpt_free_string"));
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
 title_=QFileInfo(path).fileName();
 if (metadata_ && freeString_) {
   char* name=metadata_(module_,"title");
   if(name) {
     const QString decoded=QString::fromUtf8(name).trimmed();
     if(!decoded.isEmpty()) title_=decoded;
     freeString_(name);
   }
 }
 emit stateChanged("READY");
 return true;
}

void ModuleAudio::play() {
 if (!module_ || playing_) return;
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
 open(QIODevice::ReadOnly);
 playing_=true; paused_=false;
 sink_->start(this);
 if (sink_->error()!=QtAudio::NoError) {
   stop(); emit stateChanged("AUDIO OUTPUT ERROR"); return;
 }
 emit stateChanged("PLAYING");
}
void ModuleAudio::pauseResume() {
 if(!playing_ || !sink_) return;
 if(paused_) { sink_->resume(); paused_=false; emit stateChanged("PLAYING"); }
 else { sink_->suspend(); paused_=true; emit stateChanged("PAUSED"); }
}
void ModuleAudio::stop() {
 playing_=false; paused_=false;
 if(sink_) { sink_->stop(); sink_.reset(); }
 if(isOpen()) close();
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
 return static_cast<qint64>(produced*4);
}
