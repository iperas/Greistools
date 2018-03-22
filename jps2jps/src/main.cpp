#include <QtCore/QtCore>
#include <clocale>
#include <locale>
#include <iostream>
#include "common/Logger.h"
#include "common/Path.h"
#include "common/Connection.h"
#include "greis/DataChunk.h"
#include "greis/FileBinaryStream.h"
#include "greis/ChecksumComputer.h"
#include "greis/StdMessage/FileIdStdMessage.h"
#include "greis/StdMessage/MsgFmtStdMessage.h"
#include "greis/StdMessage/ParamsStdMessage.h"
#include "greis/StdMessage/SatIndexStdMessage.h"
#include "greis/StdMessage/ExtSatIndexStdMessage.h"
#include "greis/StdMessage/SatNumbersStdMessage.h"
#include "greis/StdMessage/RcvDateStdMessage.h"

using namespace Common;
using namespace Greis;

    void insertCRLF(DataChunk * dataChunk)
    {
        dataChunk->AddMessage(NonStdTextMessage::CreateCarriageReturnMessage());
        dataChunk->AddMessage(NonStdTextMessage::CreateNewLineMessage());
    }

    int main(int argc, char** argv)
    {
        try {
                QCoreApplication a(argc, argv);
                QTextCodec* codec = QTextCodec::codecForName("UTF-8");
                QTextCodec::setCodecForLocale(codec);

                int LogLevel = 3;
                sLogger.Initialize(LogLevel);

                sLogger.Info("greistools\\jps2jps data manipulation component");
                sLogger.Info("Schmidt Institute of Physics of the Earth RAS");
                sLogger.Info("PROVIDED AS IS, NO WARRANTY, FOR SCIENTIFIC USE");

                QStringList args = a.arguments();
                QRegExp rxStart("--start[=]{0,1}([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z)");
                QRegExp rxStop("--stop[=]{0,1}([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z)");
                QRegExp rxIn("([A-Za-z\\.0-9\\-\\_\\!\\~]+)");
                QRegExp rxX("^-X$|--debug");
                QRegExp rxAccurate("^-A$|--accurate");
                QRegExp rxT("^-t$|--thorough");
                QRegExp rxCRLF("^--crlf");
                QRegExp rxOut("(?:--output|-O)[=]{0,1}([/A-Za-z\\.0-9\\-\\_\\!\\~]+)");

                QDateTime startTime = QDateTime();
                QDateTime stopTime = QDateTime();

                QString inFileName, outFileName;
                bool thoroughMode = false;
                bool accurateMode = false;
                bool crlfMode = false;

                if(rxIn.indexIn(args.at(args.size()-1)) != -1){
                    inFileName=rxIn.cap(1);
                } else {
                    throw 1;
                }

                for (int i = 1; i < args.size(); ++i)
                {
                    if (rxStart.indexIn(args.at(i)) != -1)
                    {
                        startTime = QDateTime::fromString(rxStart.cap(1),Qt::ISODate);
                        
                    }
                    else if (rxStop.indexIn(args.at(i)) != -1)
                    {
                        stopTime = QDateTime::fromString(rxStop.cap(1),Qt::ISODate);
                    }
                    else if (rxX.indexIn(args.at(i)) != -1)
                    {
                        sLogger.Initialize(5);
                    }
                    else if (rxT.indexIn(args.at(i)) != -1)
                    {
                        thoroughMode = true;
                    }
                    else if (rxOut.indexIn(args.at(i)) != -1)
                    {
                        outFileName = rxOut.cap(1);
                    }
                    else if (rxAccurate.indexIn(args.at(i)) != -1)
                    {
                        accurateMode = true;
                    }
                    else if (rxCRLF.indexIn(args.at(i)) != -1)
                    {
                        crlfMode = true;
                    }
                }

                if(outFileName.isEmpty())outFileName=QString("%1-%2").arg(QTime::currentTime().toString("HHmm")).arg(inFileName);


                sLogger.Info(QString("Processing file %1").arg(inFileName));
                sLogger.Info(QString("Output file %1").arg(outFileName));
                if(startTime.isValid())sLogger.Info(QString("Windowing start: %1").arg(startTime.toString(Qt::ISODate))); else startTime = QDateTime::fromMSecsSinceEpoch(0);
                if(stopTime.isValid())sLogger.Info(QString("Windowing stop: %1").arg(stopTime.toString(Qt::ISODate))); else stopTime = QDateTime::fromMSecsSinceEpoch(932838457459459);
                
                auto file = make_unique<DataChunk>();
                auto target = make_unique<DataChunk>();
                GreisMessageStream stream(std::make_shared<FileBinaryStream>(inFileName), true);
                file->ReadHead(stream);
                bool hasMore; int epochCounter = 0;

                const int FileIdStdMessageFixedSize = 90;
                const int MsgFmtStdMessageFixedSize = 14;

                auto fileId = make_unique<Greis::FileIdStdMessage>(QString("JP055RLOGF JPS GREISTOOLS Receiver Log File").leftJustified(FileIdStdMessageFixedSize,' ').toLatin1().constData(),FileIdStdMessageFixedSize);
                auto msgFmt = make_unique<Greis::MsgFmtStdMessage>("MF009JP010109F", MsgFmtStdMessageFixedSize);
                QByteArray bMsgPMVer = QString("PM024rcv/ver/main=\"3.7.2 Oct,11,2017\",@").toLatin1().constData();
                bMsgPMVer.append(QString("%1").arg(Greis::ChecksumComputer::ComputeCs8(bMsgPMVer,bMsgPMVer.size()), 2, 16, QChar('0')).toUpper().toLatin1().constData());
                auto msgPMVer = make_unique<Greis::ParamsStdMessage>(bMsgPMVer,bMsgPMVer.size());

                target->AddMessage(std::move(fileId));
                insertCRLF(target.get());
                target->AddMessage(std::move(msgFmt));
                insertCRLF(target.get());
                target->AddMessage(std::move(msgPMVer));
                insertCRLF(target.get());

                Greis::SatIndexStdMessage::UniquePtr_t lastSatIndexStdMessage; bool savedSI = false;
                Greis::ExtSatIndexStdMessage::UniquePtr_t lastExtSatIndexStdMessage; bool savedSX = false;
                Greis::SatNumbersStdMessage::UniquePtr_t lastSatNumbersStdMessage; bool savedNN = false;
                Greis::RcvDateStdMessage::UniquePtr_t lastRcvDateStdMessage; bool savedRD = false;

                auto out = File::CreateBinary(outFileName);
                do
                {
                    hasMore = file->ReadBody(stream, 1000);
                    for (auto& epoch : file->Body())
                    {
                        bool hasRD = false, hasSI = false, hasNN = false, hasSX = false;
                        if (startTime <= epoch->DateTime && epoch->DateTime <= stopTime)
                        {
                            sLogger.Trace(QString("Processing epoch %1").arg(epoch->DateTime.toString(Qt::ISODate)));
                            if(epochCounter==0 && accurateMode){
                                for (auto& msg : epoch->Messages){
                                    auto id = static_cast<Greis::StdMessage*>(msg.get())->Id();
                                    if(id=="SX"){hasSX = true; sLogger.Trace(QString("First epoch of interest already has [SX]"));}
                                    if(id=="SI"){hasSI = true; sLogger.Trace(QString("First epoch of interest already has [SI]"));}
                                    if(id=="NN"){hasNN = true; sLogger.Trace(QString("First epoch of interest already has [NN]"));}
                                    if(id=="RD"){hasRD = true; sLogger.Trace(QString("First epoch of interest already has [RD]"));}
                                }
                            }
                            if(epochCounter==0 && accurateMode){
                                for (auto& msg : epoch->Messages)
                                {
                                    auto id = static_cast<Greis::StdMessage*>(msg.get())->Id();
                                    target->AddMessage(std::move(msg));
                                    if(crlfMode)insertCRLF(target.get());
                                    if(id=="~~"){
                                        if(!hasRD && savedRD){
                                            sLogger.Trace(QString("Injecting [RD]..."));
                                            target->AddMessage(std::move(lastRcvDateStdMessage));
                                            if(crlfMode)insertCRLF(target.get());
                                            }
                                        if((!hasSI && !hasSX) && savedSI){
                                            sLogger.Trace(QString("Injecting [SI]..."));
                                            target->AddMessage(std::move(lastSatIndexStdMessage));
                                            if(crlfMode)insertCRLF(target.get());
                                        }
                                        if((!hasSX) && savedSX){
                                            sLogger.Trace(QString("Injecting [SX]..."));
                                            target->AddMessage(std::move(lastExtSatIndexStdMessage));
                                            if(crlfMode)insertCRLF(target.get());
                                        }
                                        if(!hasNN && savedNN){
                                            sLogger.Trace(QString("Injecting [NN]..."));
                                            target->AddMessage(std::move(lastSatNumbersStdMessage));
                                            if(crlfMode)insertCRLF(target.get());
                                        }
                                    }
                                }
                            }   else {
                                for (auto& msg : epoch->Messages)
                                {
                                    auto id = static_cast<Greis::StdMessage*>(msg.get())->Id();
                                    sLogger.Trace(QString("Copying %1").arg(QString::fromStdString(id)));
                                    target->AddMessage(std::move(msg));
                                    insertCRLF(target.get());
                                }   
                            }
                            epochCounter++;
                        } else {
                            sLogger.Trace(QString("Skipping epoch %1").arg(epoch->DateTime.toString(Qt::ISODate)));
                            if(epochCounter==0 && accurateMode){
                                for (auto& msg : epoch->Messages)
                                    {
                                            if(msg.get()->Kind()==Greis::EMessageKind::StdMessage)
                                                {
                                                    auto id = static_cast<Greis::StdMessage*>(msg.get())->Id();
                                                    if(id=="SI"){
                                                        lastSatIndexStdMessage=make_unique<SatIndexStdMessage>(msg->ToByteArray(),msg->Size());
                                                        savedSI = true;
                                                    }
                                                    if(id=="SX"){
                                                        lastExtSatIndexStdMessage=make_unique<ExtSatIndexStdMessage>(msg->ToByteArray(),msg->Size());
                                                        savedSX = true;
                                                    }
                                                    if(id=="NN"){
                                                        lastSatNumbersStdMessage=make_unique<SatNumbersStdMessage>(msg->ToByteArray(),msg->Size());
                                                        savedNN = true;
                                                    }
                                                    if(id=="RD"){
                                                        lastRcvDateStdMessage=make_unique<RcvDateStdMessage>(msg->ToByteArray(),msg->Size());
                                                        savedRD = true;
                                                    }

                                                }
                                    }
                            }
                            if(!thoroughMode && epochCounter!=0){hasMore=false; break;}
                        }
                    }
                    auto ba = QByteArray();
                    if (out->pos() == 0){
                        sLogger.Trace(QString("Writing header and data..."));
                        ba = target->ToByteArray();
                    } else {
                        sLogger.Trace(QString("Writing data..."));
                        for (const auto& targetepoch : target->Body())
                        {
                            for (const auto& targetmsg : targetepoch->Messages)
                            {
                                ba.append(targetmsg->ToByteArray());
                            }
                        }
                    }
                    out->write(ba);
                    target->Body().clear();
                    file->Body().clear();
                } while (hasMore);
                out->close();
                return 0;
            }
            catch (Exception& e)
            {
                sLogger.Error("An error has occurred");
                sLogger.Error(e.what());
                return 1;
            }
}
