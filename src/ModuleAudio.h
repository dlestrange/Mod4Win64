#pragma once
#include <QIODevice>
#include <QLibrary>
#include <QByteArray>
#include <QAudioSink>
#include <memory>
class ModuleAudio final : public QIODevice {
 Q_OBJECT
public:
 explicit ModuleAudio(QObject* parent=nullptr);
 ~ModuleAudio() override;
 bool load(const QString& path, QString* message);
 void play();
 void pauseResume();
 void restart();
 void stop();
 bool isPlaying() const;
 bool isPaused() const;
 QString moduleTitle() const { return title_; }
 qint64 readData(char* data, qint64 maxlen) override;
 qint64 writeData(const char*, qint64) override { return -1; }
signals:
 void stateChanged(const QString& state);
 void endOfModule();
private:
 using CreateFn = void*(*)(const void*, size_t, void(*)(const char*,void*), void*, int(*)(int,void*), void*, int*, const char**, const void*);
 using ReadFn = size_t(*)(void*, int32_t, size_t, int16_t*);
 using DestroyFn = void(*)(void*);
 using SeekFn = double(*)(void*, double);
 using MetadataFn = char*(*)(void*, const char*);
 using FreeStringFn = void(*)(char*);
 QLibrary library_;
 CreateFn create_=nullptr;
 ReadFn read_=nullptr;
 DestroyFn destroy_=nullptr;
 SeekFn seek_=nullptr;
 MetadataFn metadata_=nullptr;
 FreeStringFn freeString_=nullptr;
 void* module_=nullptr;
 QString title_;
 std::unique_ptr<QAudioSink> sink_;
 bool playing_=false;
 bool paused_=false;
 bool finished_=false;
};
