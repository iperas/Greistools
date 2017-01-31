#include <QtCore/QtCore>
//#include <boost/program_options.hpp>
#include <clocale>
#include <locale>
#include <iostream>
#include "Common/Logger.h"
#include "Common/Path.h"
#include "Common/Connection.h"
#include "Greis/DataChunk.h"
#include "Greis/MySqlSink.h"
#include "Greis/MySqlSource.h"

using namespace Common;
using namespace Greis;
//namespace po = boost::program_options;

/* Auxiliary functions for checking input for validity. */

/* Function used to check that 'opt1' and 'opt2' are not specified
   at the same time. */
/*void conflicting_options(const po::variables_map& vm, 
                         const char* opt1, const char* opt2)
{
    if (vm.count(opt1) && !vm[opt1].defaulted() 
        && vm.count(opt2) && !vm[opt2].defaulted())
        throw std::logic_error(std::string("Conflicting options '") 
                          + opt1 + "' and '" + opt2 + "'.");
}

/* Function used to check that of 'for_what' is specified, then
   'required_option' is specified too. */
/*void option_dependency(const po::variables_map& vm,
                        const char* for_what, const char* required_option)
{
    if (vm.count(for_what) && !vm[for_what].defaulted())
        if (vm.count(required_option) == 0 || vm[required_option].defaulted())
            throw std::logic_error(std::string("Option '") + for_what 
                              + "' requires option '" + required_option + "'.");
}

void parseArgsPo(int argc, char **argv)
{
    argc = 2;
    argv = new char*[2];
    argv[0] = "app.exe";
    argv[1] = "--help";
    using namespace boost::program_options;
    using namespace std;
    try
    {
        string ofile;
        string macrofile, libmakfile;
        bool t_given = false;
        bool b_given = false;
        string mainpackage;
        string depends = "deps_file";
        string sources = "src_file";
        string root = ".";

        options_description desc("\nUsage: DatabaseToJps [options]\n");
        desc.add_options()
            // First parameter describes option name/short name
            // The second is parameter to option
            // The third is description
            ("help,h", "print usage message")
            ("output,o", value(&ofile)->required(), "pathname for output")
            ("macrofile,m", value(&macrofile), "full pathname of macro.h")
            ("two,t", bool_switch(&t_given), "preprocess both header and body")
            ("body,b", bool_switch(&b_given), "preprocess body in the header context")
            ("libmakfile,l", value(&libmakfile), 
            "write include makefile for library")
            ("mainpackage,p", value(&mainpackage), 
            "output dependency information")
            ("depends,d", value(&depends), 
            "write dependencies to <pathname>")
            ("sources,s", value(&sources), "write source package list to <pathname>")
            ("root,r", value(&root), "treat <dirname> as project root directory")
            ;

        variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {  
            cout << desc << "\n";
            return;
        }

        conflicting_options(vm, "output", "two");
        conflicting_options(vm, "output", "body");
        conflicting_options(vm, "output", "mainpackage");
        conflicting_options(vm, "two", "mainpackage");
        conflicting_options(vm, "body", "mainpackage");

        conflicting_options(vm, "two", "body");
        conflicting_options(vm, "libmakfile", "mainpackage");
        conflicting_options(vm, "libmakfile", "mainpackage");

        option_dependency(vm, "depends", "mainpackage");
        option_dependency(vm, "sources", "mainpackage");
        option_dependency(vm, "root", "mainpackage");

        cout << "two = " << vm["two"].as<bool>() << "\n";
    }
    catch(exception& e) {
        cerr << e.what() << "\n";
    }
}*/

void parseArguments(const QCoreApplication& a, QString& database, QString& outFilename, QDateTime& from, QDateTime& to)
{
    auto args = a.arguments();
    auto usageStr = QString("Usage: 'app.exe <database> <output-filename>' [<date-from> [date-to]]\r\n") + 
        QString("\r\n") + 
        QString("Where:\r\n") + 
        QString("    <database> - source database name.\r\n") + 
        QString("    <output-file> - path to output jps file.\r\n") + 
        QString("    <date-from>, <date-to> - date range should be specified in either 'yyyy-MM-dd HH:mm:ss' or `msecs since epoch` format. \r\n") + 
        QString("        Date will be set to [NOW - 1 day, NOW] if omitted. \r\n") + 
        QString("        All dates should be specified in UTC.\r\n") + 
        QString("\r\n");

    const int fixedValuesCount = 2;

    if (args.count() < fixedValuesCount + 1)
    {
        std::cout << usageStr.toLatin1().data() << std::endl;
        throw Exception();
    }

    database = args[1];
    outFilename = args[2];

    if (args.count() == fixedValuesCount + 1)
    {
        from = QDateTime::currentDateTimeUtc().addDays(-1);
        to = QDateTime::currentDateTimeUtc();
    } else if (args.count() == fixedValuesCount + 2)
    {
        bool bOk;
        unsigned long long val = args[fixedValuesCount + 1].toULongLong(&bOk);
        if (bOk)
        {
            from = QDateTime::fromMSecsSinceEpoch(val);
        } else {
            from = QDateTime::fromString(args[fixedValuesCount + 1], "yyyy-MM-dd HH:mm:ss");
            from.setTimeSpec(Qt::UTC);
        }
        if (!from.isValid())
        {
            throw Exception("Invalid `from` argument.");
        }
        to = QDateTime::currentDateTimeUtc();
    } else {
        bool bOk;
        unsigned long long val = args[fixedValuesCount + 1].toULongLong(&bOk);
        if (bOk)
        {
            from = QDateTime::fromMSecsSinceEpoch(val);
        } else {
            from = QDateTime::fromString(args[fixedValuesCount + 1], "yyyy-MM-dd HH:mm:ss");
            from.setTimeSpec(Qt::UTC);
        }
        if (!from.isValid())
        {
            throw Exception("Invalid `from` argument.");
        }
        val = args[fixedValuesCount + 2].toULongLong(&bOk);
        if (bOk)
        {
            to = QDateTime::fromMSecsSinceEpoch(val);
        } else {
            to = QDateTime::fromString(args[fixedValuesCount + 2], "yyyy-MM-dd HH:mm:ss");
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
        QCoreApplication a(argc, argv);

        QTextCodec* codec = QTextCodec::codecForName("UTF-8");
        QTextCodec::setCodecForLocale(codec);

        sIniSettings.Initialize(Path::Combine(Path::ApplicationDirPath(), "config.ini"));
        sLogger.Initialize(sIniSettings.value("LogLevel", 5).toInt());

        QString databaseName;
        QString outFilename;
        QDateTime from;
        QDateTime to;
        parseArguments(a, databaseName, outFilename, from, to);

        auto connection = Connection::FromSettings("Db");
        connection->DatabaseName = databaseName;
        sLogger.Info(QString("Connecting to `%1`...").arg(connection->DatabaseName));
        connection->Connect();

        auto mySqlSource = make_unique<MySqlSource>(connection);

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
        sLogger.Info(QString("Done!"));
        sLogger.Info(QString("Data successfully saved into `%1`.").arg(outFilename));

        return 0;
    }
    catch (Exception& e)
    {
        sLogger.Error("An error has occurred");
        sLogger.Error(e.what());
        return 1;
    }
}
