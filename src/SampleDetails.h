#pragma once
// Read-only inspection of classic XM / S3M sample headers. This code never
// edits module files and is deliberately independent of the playback decoder.
#include <QByteArray>
#include <QFile>
#include <QString>
#include <QtGlobal>
#include <cmath>

struct SampleDetails {
    bool supported=false;
    bool hasSample=false;
    QString explanation;
    quint32 length=0;       // sample frames, not bytes
    quint32 loopStart=0;
    quint32 loopLength=0;
    int fineTune=0;
    int defaultVolume=0;
    int resolution=8;
    quint32 c2Tuning=0;
    int sampleCount=0;      // multi-sample XM instruments show first sample
};

namespace SampleInspection {
inline quint32 u16(const QByteArray& b,qsizetype p) {
    if(p<0 || p>b.size()-2) return 0;
    auto* v=reinterpret_cast<const unsigned char*>(b.constData()+p);
    return quint32(v[0]) | (quint32(v[1])<<8);
}
inline quint32 u32(const QByteArray& b,qsizetype p) {
    if(p<0 || p>b.size()-4) return 0;
    auto* v=reinterpret_cast<const unsigned char*>(b.constData()+p);
    return quint32(v[0]) | (quint32(v[1])<<8) |
           (quint32(v[2])<<16) | (quint32(v[3])<<24);
}
inline int byte(const QByteArray& b,qsizetype p) {
    return p>=0 && p<b.size()?static_cast<unsigned char>(b.at(p)):0;
}
inline SampleDetails inspectXm(const QByteArray& b,int index) {
    SampleDetails d;
    d.supported=true;
    if(b.size()<80 || !b.startsWith("Extended Module: ")) {
        d.explanation="Invalid XM header"; return d;
    }
    const quint32 header=u32(b,60), patterns=u16(b,70), instruments=u16(b,72);
    if(index<0 || quint32(index)>=instruments || header<20 ||
       patterns>2048 || instruments>512) {
        d.explanation="No such instrument"; return d;
    }
    qsizetype at=60+qsizetype(header);
    if(at>b.size()) { d.explanation="Truncated XM header"; return d; }
    for(quint32 i=0;i<patterns;++i) {
        if(at>b.size()-9) { d.explanation="Truncated pattern header"; return d; }
        const quint32 ph=u32(b,at), packed=u16(b,at+7);
        if(ph<9 || ph>quint32(b.size()-at)) {
            d.explanation="Invalid pattern header"; return d;
        }
        at+=ph;
        if(packed>quint32(b.size()-at)) {
            d.explanation="Truncated pattern data"; return d;
        }
        at+=packed;
    }
    for(int i=0;i<=index;++i) {
        if(at>b.size()-29) { d.explanation="Truncated instrument"; return d; }
        const quint32 ih=u32(b,at), count=u16(b,at+27);
        if(ih<29 || ih>4096 || count>512 || ih>quint32(b.size()-at)) {
            d.explanation="Invalid instrument header"; return d;
        }
        const qsizetype sampleHeaders=at+ih;
        if(!count) {
            if(i==index) {
                d.explanation="This instrument slot contains no samples";
                return d;
            }
            at=sampleHeaders;
            continue;
        }
        if(ih<33) { d.explanation="Truncated sample header size"; return d; }
        const quint32 sh=u32(b,at+29);
        if(sh<40 || sh>1024 ||
           qsizetype(count)*sh>b.size()-sampleHeaders) {
            d.explanation="Invalid sample headers"; return d;
        }
        qsizetype totalBytes=0;
        for(quint32 j=0;j<count;++j) {
            const qsizetype p=sampleHeaders+qsizetype(j)*sh;
            const quint32 size=u32(b,p);
            if(qsizetype(size)>b.size()-totalBytes) {
                d.explanation="Sample length exceeds file"; return d;
            }
            totalBytes+=size;
        }
        const qsizetype next=sampleHeaders+qsizetype(count)*sh;
        if(totalBytes>b.size()-next) {
            d.explanation="Truncated sample data"; return d;
        }
        if(i==index) {
            const qsizetype p=sampleHeaders;
            const int flags=byte(b,p+14);
            const bool sixteen=(flags & 0x10)!=0;
            const quint32 unit=sixteen?2u:1u;
            d.resolution=sixteen?16:8;
            d.length=u32(b,p)/unit;
            d.loopStart=u32(b,p+4)/unit;
            d.loopLength=(flags & 0x03)?u32(b,p+8)/unit:0;
            d.defaultVolume=byte(b,p+12);
            d.fineTune=static_cast<qint8>(byte(b,p+13));
            const int relative=static_cast<qint8>(byte(b,p+16));
            d.c2Tuning=quint32(std::lround(8363.0*std::pow(2.0,
                (relative+double(d.fineTune)/128.0)/12.0)));
            d.sampleCount=int(count);
            d.hasSample=d.length>0;
            if(!d.hasSample) d.explanation="The first sample contains no audio";
            return d;
        }
        at=next+totalBytes;
    }
    d.explanation="Instrument not found"; return d;
}
inline SampleDetails inspectS3m(const QByteArray& b,int index) {
    SampleDetails d;
    d.supported=true;
    if(b.size()<0x60 || b.mid(0x2c,4)!="SCRM") {
        d.explanation="Invalid S3M header"; return d;
    }
    const quint32 orders=u16(b,0x20), instruments=u16(b,0x22);
    if(index<0 || quint32(index)>=instruments || instruments>512 ||
       orders>4096) {
        d.explanation="No such sample slot"; return d;
    }
    const qsizetype ptr=0x60+qsizetype(orders)+qsizetype(index)*2;
    if(ptr>b.size()-2) { d.explanation="Truncated S3M pointer table"; return d; }
    const quint32 paragraph=u16(b,ptr);
    if(paragraph==0) { d.explanation="Empty instrument slot"; return d; }
    const qsizetype at=qsizetype(paragraph)*16;
    if(at>b.size()-0x50) { d.explanation="Truncated S3M instrument"; return d; }
    if(byte(b,at)!=1) {
        d.explanation="Not a PCM sample (FM/AdLib or empty slot)";
        return d;
    }
    const quint32 length=u32(b,at+0x10), loopBegin=u32(b,at+0x14),
                  loopEnd=u32(b,at+0x18);
    const int flags=byte(b,at+0x1f);
    d.length=length;    // S3M positions are sample frames
    d.loopStart=(flags & 1)?loopBegin:0;
    d.loopLength=(flags & 1 && loopEnd>=loopBegin)?loopEnd-loopBegin:0;
    d.defaultVolume=byte(b,at+0x1c);
    d.fineTune=0;       // S3M stores C-2 frequency, not XM-style fine tune
    d.resolution=(flags & 4)?16:8;
    d.c2Tuning=u32(b,at+0x20);
    d.sampleCount=1;
    d.hasSample=d.length>0;
    if(!d.hasSample) d.explanation="This sample slot contains no audio";
    return d;
}
inline SampleDetails inspect(const QString& path,int zeroBasedIndex) {
    QFile file(path);
    SampleDetails d;
    if(!file.open(QIODevice::ReadOnly)) {
        d.explanation="Cannot open module file"; return d;
    }
    const QByteArray b=file.readAll();
    const QString ext=path.section('.',-1).toLower();
    if(ext=="xm") return inspectXm(b,zeroBasedIndex);
    if(ext=="s3m") return inspectS3m(b,zeroBasedIndex);
    d.explanation="Sample details not yet supported for this format";
    return d;
}
} // namespace SampleInspection
