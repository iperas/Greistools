#include <QtCore/QtCore>
#include <clocale>
#include <locale>
#include <iostream>
#include "ProjectBase/Logger.h"
#include "ProjectBase/Path.h"
#include "ProjectBase/Connection.h"
#include "JpsFile.h"
#include "MySqlSink.h"
#include "MySqlSource.h"

using namespace ProjectBase;
using namespace Greis;

void parseArguments(const QCoreApplication& a, QString& outFilename, QDateTime& from, QDateTime& to)
{
    auto args = a.arguments();
    auto usageStr = QString("Usage: 'app.exe <output-filename>' [<date-from> [date-to]]\r\n") + 
        QString("\r\n") + 
        QString("Where:\r\n") + 
        QString("    <output-file> - path to output jps file.\r\n") + 
        QString("    <date-from>, <date-to> - date range should be specified in either 'yyyy-MM-dd HH:mm:ss' or `msecs since epoch` format. \r\n") + 
        QString("        Date will be set to [NOW - 1 day, NOW] if omitted. \r\n") + 
        QString("        All dates should be specified in UTC.\r\n") + 
        QString("\r\n");

    if (args.count() < 2)
    {
        std::cout << usageStr.toAscii().data() << std::endl;
        throw Exception();
    }

    outFilename = args[1];

    if (args.count() == 2)
    {
        from = QDateTime::currentDateTimeUtc().addDays(-1);
        to = QDateTime::currentDateTimeUtc();
    } else if (args.count() == 3)
    {
        bool bOk;
        unsigned long long val = args[2].toULongLong(&bOk);
        if (bOk)
        {
            from = QDateTime::fromMSecsSinceEpoch(val);
        } else {
            from = QDateTime::fromString(args[2], "yyyy-MM-dd HH:mm:ss");
            from.setTimeSpec(Qt::UTC);
        }
        if (!from.isValid())
        {
            throw Exception("Invalid `from` argument.");
        }
        to = QDateTime::currentDateTimeUtc();
    } else {
        bool bOk;
        unsigned long long val = args[2].toULongLong(&bOk);
        if (bOk)
        {
            from = QDateTime::fromMSecsSinceEpoch(val);
        } else {
            from = QDateTime::fromString(args[2], "yyyy-MM-dd HH:mm:ss");
            from.setTimeSpec(Qt::UTC);
        }
        if (!from.isValid())
        {
            throw Exception("Invalid `from` argument.");
        }
        val = args[3].toULongLong(&bOk);
        if (bOk)
        {
            to = QDateTime::fromMSecsSinceEpoch(val);
        } else {
            to = QDateTime::fromString(args[3], "yyyy-MM-dd HH:mm:ss");
            to.setTimeSpec(Qt::UTC);
        }
        if (!to.isValid())
        {
            throw Exception("Invalid `to` argument.");
        }
    }
}

int main(int argc, char **argv)
{
    try
    {
        std::setlocale(LC_ALL, "Russian_Russia.1251");
        std::locale::global(std::locale("Russian_Russia.1251"));

        QCoreApplication a(argc, argv);

        QTextCodec* codec = QTextCodec::codecForName("UTF-8");
        QTextCodec* codecMs = QTextCodec::codecForName("Windows-1251");
        QTextCodec::setCodecForCStrings(codecMs);
        QTextCodec::setCodecForLocale(codec);
        QTextCodec::setCodecForTr(codec);

        sLogger.Initialize(Path::Combine(Path::ApplicationDirPath(), "logger.config.xml"));
        sIniSettings.Initialize(Path::Combine(Path::ApplicationDirPath(), "config.ini"));

        QString outFilename;
        QDateTime from;
        QDateTime to;
        parseArguments(a, outFilename, from, to);

        auto connection = Connection::FromSettings("Db");
        sLogger.Info(QString("Connecting to database `%1`...").arg(connection->DatabaseName));
        connection->Connect();

        auto mySqlSource = make_unique<MySqlSource>(connection.get());

        sLogger.Info("Done!");
        sLogger.Info(QString("Reading data for range [`%1`, `%2`] (%3 - %4)...")
            .arg(from.toString("dd.MM.yyyy HH:mm:ss"))
            .arg(to.toString("dd.MM.yyyy HH:mm:ss"))
            .arg(from.toMSecsSinceEpoch())
            .arg(to.toMSecsSinceEpoch()));
        auto jpsFile = mySqlSource->ReadRange(from, to);

        sLogger.Info("Done!");
        sLogger.Info("Binarizing data...");
        auto ba = jpsFile->ToByteArray();

        sLogger.Info("Done!");
        sLogger.Info("Saving data...");
        auto out = File::CreateBinary(outFilename);
        out->write(ba);
        out->close();
        sLogger.Info(QString("Done!\r\nData successfully saved in `%1`.").arg(outFilename));

        return 0;
    }
    catch (Exception& e)
    {
        sLogger.Error(e.what());
        return 1;
    }
}
