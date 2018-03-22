#include <QtCore/QtCore>
#include <clocale>
#include <locale>
#include <iostream>
#include "common/Logger.h"
#include "common/Path.h"
#include "common/Connection.h"
#include "greis/DataChunk.h"
#include "extras/SkyPeek.h"
#include "greis/FileBinaryStream.h"
#include "greis/ChecksumComputer.h"
#include "greis/StdMessage/RcvDateStdMessage.h"
#include "greis/StdMessage/EpochTimeStdMessage.h"
#include "greis/StdMessage/FileIdStdMessage.h"
#include "greis/StdMessage/MsgFmtStdMessage.h"
#include "greis/StdMessage/ParamsStdMessage.h"

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

                sLogger.Info("Greistools\\jpsdump data examination component");
                sLogger.Info("Schmidt Institute of Physics of the Earth RAS");
                sLogger.Info("PROVIDED AS IS, NO WARRANTY, FOR SCIENTIFIC USE");

                QStringList args = a.arguments();
                QRegExp rxStart("--start[=]{0,1}([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z)");
                QRegExp rxStop("--stop[=]{0,1}([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z)");
                QRegExp rxIn("([A-Za-z\\.0-9\\-\\_\\!\\~]+)");
                QRegExp rxX("^-X$|--debug");
                QRegExp rxV("^-v$|--verbose");
                QRegExp rxS("^--skypeek");
                QRegExp rxStat("^-s$|--statistics");
                QRegExp rxT("^-t$|--thorough");

                QDateTime startTime = QDateTime();
                QDateTime stopTime = QDateTime();

                QString inFileName;
                bool thoroughMode = false;
                bool verboseMode = false;
                bool showStatistics = false;
                bool skyPeekMode = false;

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
                    else if (rxV.indexIn(args.at(i)) != -1)
                    {
                        verboseMode = true;
                    }
                    else if (rxStat.indexIn(args.at(i)) != -1)
                    {
                        showStatistics = true;
                    } 
                    else if (rxS.indexIn(args.at(i)) != -1)
                    {
                        skyPeekMode = true;
                    }
                    else if (rxT.indexIn(args.at(i)) != -1)
                    {
                        thoroughMode = true;
                    }
                }

                sLogger.Info(QString("Processing file %1").arg(inFileName));
                if(startTime.isValid())sLogger.Info(QString("Windowing start: %1").arg(startTime.toString(Qt::ISODate))); else startTime = QDateTime::fromMSecsSinceEpoch(0);
                if(stopTime.isValid())sLogger.Info(QString("Windowing stop: %1").arg(stopTime.toString(Qt::ISODate))); else stopTime = QDateTime::fromMSecsSinceEpoch(932838457459459);
                
                auto skyPeek = make_unique<SkyPeek>();
                auto file = make_unique<DataChunk>();
                auto target = make_unique<DataChunk>();
                GreisMessageStream stream(std::make_shared<FileBinaryStream>(inFileName), true);
                file->ReadHead(stream);
                bool hasMore; int epochCounter = 0;

                do
                {
                    hasMore = file->ReadBody(stream, 1000);
                    for (auto& epoch : file->Body())
                    {
                        if (startTime <= epoch->DateTime && epoch->DateTime <= stopTime)
                        {
                             std::cout << QString("Epoch %1:{").arg(epoch->DateTime.toString(Qt::ISODate)).toStdString();
                                    for (auto& msg : epoch->Messages)
                                        {
                                            auto sMsg = static_cast<Greis::StdMessage*>(msg.get());
                                            std::cout << QString("[%1]").arg(sMsg->Id().c_str()).toStdString();

                                            if(skyPeekMode){
                                                skyPeek->AddMessage(msg.get());
                                            }

                                            if(verboseMode){
                                                if(sMsg->Id()=="~~")
                                                {
                                                    auto eTMsg = static_cast<Greis::EpochTimeStdMessage*>(msg.get());
                                                    std::cout << QString(":{Tod: %1 ms, Cs: %2, isCorrect: %3}").arg(eTMsg->Tod()).arg(eTMsg->Cs()).arg(eTMsg->IsCorrect()).toStdString();
                                                }
                                                else if(sMsg->Id()=="RD")
                                                {
                                                    auto rDMsg = static_cast<Greis::RcvDateStdMessage*>(msg.get());
                                                    std::cout << QString(":{Year: %1, Month: %2, Day: %3, Base: %4, Cs: %5, isCorrect: %6}").arg(rDMsg->Year()).arg(rDMsg->Month()).arg(rDMsg->Day()).arg(rDMsg->Base()).arg(rDMsg->Cs()).arg(rDMsg->IsCorrect()).toStdString();
                                                }
                                                else 
                                                {
                                                    std::cout << QString(":{Size: %1, isCorrect: %2}").arg(sMsg->Size()).arg(sMsg->IsCorrect()).toStdString();
                                                }
                                                std::cout << std::endl;
                                            }
                                        }
                            std::cout << "}";
                            std::cout << std::endl;
                            if(skyPeekMode){
                                std::cout << QString("SkyPeek:{").toStdString();
                                std::cout << std::endl;
                                for (int i=0;i< skyPeek->SVs.size(); i++)
                                {
                                    SkyPeek::SV SV = skyPeek->SVs[i];
                                    std::cout << QString("{SSID: %1, SVID: %2, USI: %3}:{").arg(SV.ESI.SSID).arg(SV.ESI.SVID).arg(SV.USI).toStdString();
                                    std::cout << QString("Asys: %1, Ksys: %2, ").arg(SV.Asys).arg(SV.Ksys).toStdString();
                                    std::cout << QString("PR: {CA/L1: %2, P/L1: %3, ").arg(SV.Pseudorange[SkyPeek::Signals::caL1]).arg(SV.Pseudorange[SkyPeek::Signals::pL1]).toStdString();
                                    std::cout << QString("P/L2: %1, CA/L2: %2, P/L5: %3}, ").arg(SV.Pseudorange[SkyPeek::Signals::pL2]).arg(SV.Pseudorange[SkyPeek::Signals::caL2]).arg(SV.Pseudorange[SkyPeek::Signals::L5]).toStdString();
                                    std::cout << QString("CP:{CA/L1: %1, P/L1: %2, P/L2: %3, ").arg(SV.CarrierPhase[SkyPeek::Signals::caL1]).arg(SV.CarrierPhase[SkyPeek::Signals::pL1]).arg(SV.CarrierPhase[SkyPeek::Signals::pL2]).toStdString();
                                    std::cout << QString("CP CA/L2: %1, CP P/L5: %2}, ").arg(SV.CarrierPhase[SkyPeek::Signals::caL2]).arg(SV.CarrierPhase[SkyPeek::Signals::L5]).toStdString();
                                    std::cout << QString("AZ: %2, EL: %1},").arg(SV.Elevation).arg(SV.Azimuth).toStdString();
                                    std::cout << std::endl;
                                }
                                
                                std::cout << QString("}").toStdString();
                                std::cout << std::endl;
                            }
                            epochCounter++;
                        } else {
                           sLogger.Trace(QString("Skipping epoch %1").arg(epoch->DateTime.toString(Qt::ISODate)));
                           if(!thoroughMode && epochCounter!=0){hasMore=false; break;}
                        }
                    }
                    file->Body().clear();
                } while (hasMore);
                return 0;
            }
            catch (Exception& e)
            {
                sLogger.Error("An error has occurred");
                sLogger.Error(e.what());
                return 1;
            }
}
