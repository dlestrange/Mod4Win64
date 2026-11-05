#pragma once
#include <QIODevice>
#include <QLibrary>
#include <QByteArray>
#include <QAudioSink>
#include <QTimer>
#include <memory>
#include <QVector>
#include <QStringList>
#include <QQueue>
#include <atomic>
struct EffectSnapshot {
 QString moduleIdentity;
 int channels = 0;
 int speed = 0;
 int bpm = 0;
 int pattern = 0;
 int row = 0;
 int order = 0;
 int orderCount = 0;
 double elapsedSeconds = 0.0;
 QVector<int> usedCommands;
 QVector<int> currentCommands;
 QVector<int> notes;
 QVector<int> instruments;
 QVector<float> vuLeft;
 QVector<float> vuRight;
};

struct ModuleInfo {
 QString title;
 QString format;
 QString tracker;
 QString filePath;
 qint64 fileBytes = 0;
 int channels = 0;
 int instrumentSlots = 0;
 int playableInstrumentCount = -1; // -1: not available for this format
 int sampleSlots = 0;
 int patternCount = 0;
 int orderCount = 0;
 int distinctPatternsInSequence = 0;
 QStringList names;
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
 // Scrub silently while a navigation button is held, then resume on release.
 void beginSeek();
 void seekBy(double seconds);
 void endSeek();
 bool isSeeking() const { return seeking_; }
 bool isPlaying() const;
 bool isPaused() const;
 QString moduleTitle() const { return title_; }
 ModuleInfo moduleInfo() const;
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
 using FreeStringFn = void(*)(const char*);
 using IntFn = int32_t(*)(void*);
 using PatternRowsFn = int32_t(*)(void*,int32_t);
 using PatternCommandFn = uint8_t(*)(void*,int32_t,int32_t,int32_t,int);
 using VuFn = float(*)(void*,int32_t);
 using IndexedNameFn = const char*(*)(void*,int32_t);
 using OrderPatternFn = int32_t(*)(void*,int32_t);
 IntFn numChannels_=nullptr, numPatterns_=nullptr, currentSpeed_=nullptr,
       currentTempo_=nullptr, currentPattern_=nullptr, currentRow_=nullptr,
       currentOrder_=nullptr;
 PatternRowsFn patternRows_=nullptr;
 PatternCommandFn patternCommand_=nullptr;
 VuFn vuLeft_=nullptr, vuRight_=nullptr;
 IntFn numInstruments_=nullptr, numSamples_=nullptr, numOrders_=nullptr;
 IndexedNameFn instrumentName_=nullptr, sampleName_=nullptr;
 OrderPatternFn orderPattern_=nullptr;
 QVector<int> usedCommands_;
 QLibrary library_;
 CreateFn create_=nullptr;
 ReadFn read_=nullptr;
 DestroyFn destroy_=nullptr;
 SeekFn seek_=nullptr;
 PositionFn position_=nullptr;
 PositionFn duration_=nullptr;
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
 bool seeking_=false;
 bool seekWasPlaying_=false;
 bool seekWasPaused_=false;
 // Frame-stamped decoder states; presentation follows the audio sink clock.
 struct TimedEffects { quint64 frame; EffectSnapshot state; };
 QQueue<TimedEffects> effectHistory_;
 EffectSnapshot presentedEffects_;
 EffectSnapshot decoderSnapshot() const;
 std::atomic<quint64> decodedFrames_{0};
 std::atomic<quint64> nonzeroSamples_{0};
};
