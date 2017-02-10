#include <QtCore/QtCore>
#include <clocale>
#include <locale>
#include <iostream>
#include "Common/Logger.h"
#include "Common/Path.h"
#include "Common/Connection.h"
#include "Greis/DataChunk.h"
#include "Greis/FileBinaryStream.h"
#include "Greis/ChecksumComputer.h"
#include "Greis/StdMessage/FileIdStdMessage.h"
#include "Greis/StdMessage/MsgFmtStdMessage.h"
#include "Greis/StdMessage/ParamsStdMessage.h"
#include "Greis/StdMessage/SatIndexStdMessage.h"
#include "Greis/StdMessage/SatNumbersStdMessage.h"
#include "Greis/StdMessage/RcvDateStdMessage.h"

using namespace Common;
using namespace Greis;


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
                QRegExp rxOut("^(-o|--output)[=]{0,1}([A-Za-z\\.0-9\\-\\_\\!\\~]+)");

                QDateTime startTime = QDateTime();
                QDateTime stopTime = QDateTime();

                QString inFileName, outFileName;
                bool thoroughMode = false;
                bool accurateMode = false;

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
                        outFileName = rxOut.cap(2);
                    }
                    else if (rxAccurate.indexIn(args.at(i)) != -1)
                    {
                        accurateMode = true;
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

                auto fileId = make_unique<Greis::FileIdStdMessage>(QString("JP055RLOGFJPS GREISTOOLS Receiver log-file").leftJustified(FileIdStdMessageFixedSize,' ').toLatin1().constData(),FileIdStdMessageFixedSize);
                auto msgFmt = make_unique<Greis::MsgFmtStdMessage>("MF009JP010109F", MsgFmtStdMessageFixedSize);
                QByteArray bMsgPMVer = QString("PM024rcv/ver/main=\"2.7.0 Mar,31,2008\",@").toLatin1().constData();
                bMsgPMVer.append(QString::number(Greis::ChecksumComputer::ComputeCs8(bMsgPMVer,bMsgPMVer.size()), 16).toLatin1().constData());
                auto msgPMVer = make_unique<Greis::ParamsStdMessage>(bMsgPMVer,bMsgPMVer.size());

                target->AddMessage(std::move(fileId));
                target->AddMessage(NonStdTextMessage::CreateCarriageReturnMessage());
                target->AddMessage(NonStdTextMessage::CreateNewLineMessage());
                target->AddMessage(std::move(msgFmt));
                target->AddMessage(NonStdTextMessage::CreateCarriageReturnMessage());
                target->AddMessage(NonStdTextMessage::CreateNewLineMessage());
                target->AddMessage(std::move(msgPMVer));
                target->AddMessage(NonStdTextMessage::CreateCarriageReturnMessage());
                target->AddMessage(NonStdTextMessage::CreateNewLineMessage());

                Greis::SatIndexStdMessage::UniquePtr_t lastSatIndexStdMessage;
                Greis::SatNumbersStdMessage::UniquePtr_t lastSatNumbersStdMessage;
                Greis::RcvDateStdMessage::UniquePtr_t lastRcvDateStdMessage;

                do
                {
                    hasMore = file->ReadBody(stream, 1000);
                    for (auto& epoch : file->Body())
                    {
                        bool hasRD = false, hasSI = false, hasNN = false;
                        if (startTime <= epoch->DateTime && epoch->DateTime <= stopTime)
                        {
                            sLogger.Trace(QString("Processing epoch %1").arg(epoch->DateTime.toString(Qt::ISODate)));
                            if(epochCounter==0 && accurateMode){
                                for (auto& msg : epoch->Messages){
                                    auto id = static_cast<Greis::StdMessage*>(msg.get())->Id();
                                    if(id=="SI")hasSI = true;
                                    if(id=="NN")hasNN = true;
                                    if(id=="RD")hasRD = true;
                                }
                            }
                            if(epochCounter==0 && accurateMode){
                                for (auto& msg : epoch->Messages)
                                {
                                    target->AddMessage(std::move(msg));
                                    if(!hasRD && epochCounter == 0)target->AddMessage(std::move(lastRcvDateStdMessage));
                                    if(!hasNN && epochCounter == 0)target->AddMessage(std::move(lastSatNumbersStdMessage));
                                    if(!hasSI && epochCounter == 0)target->AddMessage(std::move(lastSatIndexStdMessage));
                                }
                            }   else {
                                for (auto& msg : epoch->Messages)
                                {
                                    target->AddMessage(std::move(msg));
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
                                                    }
                                                    if(id=="NN"){
                                                        lastSatNumbersStdMessage=make_unique<SatNumbersStdMessage>(msg->ToByteArray(),msg->Size());
                                                    }
                                                    if(id=="RD"){
                                                        lastRcvDateStdMessage=make_unique<RcvDateStdMessage>(msg->ToByteArray(),msg->Size());
                                                    }

                                                }
                                    }
                            }
                            if(!thoroughMode && epochCounter!=0){hasMore=false; break;}
                        }
                    }
                    file->Body().clear();
                } while (hasMore);
                auto ba = target->ToByteArray();
                auto out = File::CreateBinary(outFileName);
                out->write(ba);
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
