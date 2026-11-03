#pragma once
#include <QIODevice>
#include <QLibrary>
#include <QByteArray>
#include <QAudioSink>
#include <QTimer>
#include <memory>
#include <QVector>
#include <QQueue>
#include <atomic>
struct EffectSnapshot {
 QString moduleIdentity;
 int channels = 0;
 int speed = 0;
 int bpm = 0;
 int pattern = 0;
 int row = 0;
 double elapsedSeconds = 0.0;
 QVector<int> usedCommands;
 QVector<int> currentCommands;
 QVector<int> notes;
 QVector<int> instruments;
 QVector<float> vuLeft;
 QVector<float> vuRight;
};

class ModuleAudio final : public QIODevice {
 Q_OBJECT
public:
 explicit ModuleAudio(QObject* parent=nullptr);
 ~ModuleAudio() override;
 bool load(const QString& path, QString* message);
 void play();
 void pauseResume();
 void restart();
 void pumpAudio();
 void stop();
 bool isPlaying() const;
 bool isPaused() const;
 QString moduleTitle() const { return title_; }
 EffectSnapshot effectsSnapshot();
 bool isSequential() const override { return true; }
 qint64 readData(char* data, qint64 maxlen) override;
 quint64 decodedFrames() const { return decodedFrames_.load(); }
 quint64 nonzeroSamples() const { return nonzeroSamples_.load(); }
 qint64 writeData(const char*, qint64) override { return -1; }
signals:
 void stateChanged(const QString& state);
 void endOfModule();
private:
 using CreateFn = void*(*)(const void*, size_t, void(*)(const char*,void*), void*, int(*)(int,void*), void*, int*, const char**, const void*);
 using ReadFn = size_t(*)(void*, int32_t, size_t, int16_t*);
 using DestroyFn = void(*)(void*);
 using SeekFn = double(*)(void*, double);
 using PositionFn = double(*)(void*);
 using MetadataFn = char*(*)(void*, const char*);
 using FreeStringFn = void(*)(char*);
 using IntFn = int32_t(*)(void*);
 using PatternRowsFn = int32_t(*)(void*,int32_t);
 using PatternCommandFn = uint8_t(*)(void*,int32_t,int32_t,int32_t,int);
 using VuFn = float(*)(void*,int32_t);
 IntFn numChannels_=nullptr, numPatterns_=nullptr, currentSpeed_=nullptr,
       currentTempo_=nullptr, currentPattern_=nullptr, currentRow_=nullptr;
 PatternRowsFn patternRows_=nullptr;
 PatternCommandFn patternCommand_=nullptr;
 VuFn vuLeft_=nullptr, vuRight_=nullptr;
 QVector<int> usedCommands_;
 QLibrary library_;
 CreateFn create_=nullptr;
 ReadFn read_=nullptr;
 DestroyFn destroy_=nullptr;
 SeekFn seek_=nullptr;
 PositionFn position_=nullptr;
 MetadataFn metadata_=nullptr;
 FreeStringFn freeString_=nullptr;
 void* module_=nullptr;
 QString title_;
 QString loadedPath_;
 std::unique_ptr<QAudioSink> sink_;
 QIODevice* outputDevice_ = nullptr;
 QTimer pumpTimer_;
 QByteArray pending_;
 bool playing_=false;
 bool paused_=false;
 bool finished_=false;
 // Frame-stamped decoder states; presentation follows the audio sink clock.
 struct TimedEffects { quint64 frame; EffectSnapshot state; };
 QQueue<TimedEffects> effectHistory_;
 EffectSnapshot presentedEffects_;
 EffectSnapshot decoderSnapshot() const;
 std::atomic<quint64> decodedFrames_{0};
 std::atomic<quint64> nonzeroSamples_{0};
};
