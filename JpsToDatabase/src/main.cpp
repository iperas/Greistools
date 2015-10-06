#include <QtCore/QtCore>
#include <clocale>
#include <locale>
#include <iostream>
#include "Common/Logger.h"
#include "Common/Path.h"
#include "Common/Connection.h"
#include "Greis/DataChunk.h"
#include "Greis/MySqlSink.h"

using namespace Common;
using namespace Greis;

void applyArguments(QStringList& args, Connection* connection)
{
    const QString databaseName =    "--database";
    const QString username =        "--username";
    const QString password =        "--password";
    const QString port =            "--port";
    const QString hostname =        "--host";
    QVector<QString> argumentName;
    argumentName << databaseName;
    argumentName << username;
    argumentName << password;
    argumentName << port;
    argumentName << hostname;

    std::cout << "Reading console parameters:" << std::endl;
    bool waitingForValue = false;
    QString key;
    QString value;
    int parameterArgsCount = 0;
    foreach (QString arg, args)
    {
        if (argumentName.contains(arg) && !waitingForValue)
        {
            key = arg;
            waitingForValue = true;
            parameterArgsCount++;
        }
        else if (waitingForValue)
        {
            value = arg;
            std::cout << key.toStdString() << " = " << value.toStdString() << std::endl;
            if (key == databaseName)
            {
                connection->DatabaseName = value;
            }
            else if (key == username)
            {
                connection->Username = value;
            }
            else if (key == password)
            {
                connection->Password = value;
            }
            else if (key == port)
            {
                connection->Port = value.toInt();
            }
            else if (key == hostname)
            {
                connection->Hostname = value;
            }
            else
            {
                std::cout << "Invalid parameter key " << key.toStdString() << std::endl;
            }
            
            waitingForValue = false;
            parameterArgsCount++;
        }
        else
        {
            break;
        }
    }

    for (int i = 0; i < parameterArgsCount; i++)
    {
        args.pop_front();
    }

    std::cout << "Done!" << std::endl;
}

int main(int argc, char **argv)
{
    bool wrapIntoTransaction;
    Connection::SharedPtr_t connection;
    try
    {
        QCoreApplication a(argc, argv);

        QTextCodec* codec = QTextCodec::codecForName("UTF-8");
        QTextCodec::setCodecForLocale(codec);

        sIniSettings.Initialize(Path::Combine(Path::ApplicationDirPath(), "config.ini"));
        sLogger.Initialize(sIniSettings.value("LogLevel", 5).toInt());
        
        auto args = a.arguments();        
        if (args.size() == 2 && args[1] == "--help" || args[1] == "-h" || args.size() == 1)
        {
            
            auto usageStr = QString("Usage: 'JpsToDatabase.exe [--database databaseName] [--username userName] [--password password] [--port port] [--host hostName] <input-filename> [<other-input-filenames>]\r\n") + 
                QString("\r\n") + 
                QString("Where:\r\n") + 
                QString("    <input-file> - path to input jps file.\r\n") + 
                QString("\r\n");
            std::cout << usageStr.toStdString();
            return 10022;
        }
        args.pop_front();

        // Connecting to database
        wrapIntoTransaction = sIniSettings.value("WrapIntoTransaction", false).toBool();
        int inserterBatchSize = sIniSettings.value("inserterBatchSize", 10000).toInt();
        connection = Connection::FromSettings("LocalDatabase");
        applyArguments(args, connection.get());
        connection->Connect();
        
        auto _dbHelper = connection->DbHelper();
        /*auto query = _dbHelper->ExecuteQuery("SET autocommit=0");
        DatabaseHelper::ThrowIfError(query);
        query = _dbHelper->ExecuteQuery("SET unique_checks=0");
        DatabaseHelper::ThrowIfError(query);
        query = _dbHelper->ExecuteQuery("SET foreign_key_checks=0");
        DatabaseHelper::ThrowIfError(query);*/

        foreach (QString filename, args)
        {
            //QFile file2(filename);
            //file2.open(QIODevice::ReadOnly);
            //while (!file2.atEnd()) {
            //    QVariantList tests;
            //    int i = 0;
            //    while (!file2.atEnd() && i < 1000) {
            //        QByteArray data = file2.read(1000);
            //        tests << data;
            //        i++;
            //    }

            //    if (i == 0)
            //    {
            //        break;
            //    }

            //    QSqlQuery query = _dbHelper->ExecuteQuery("");

            //    auto _insertQuery = QString("INSERT INTO `test_blob` (test,test2) VALUES ");
            //    for (int j = 0; j < i - 1; j++)
            //    {
            //        _insertQuery.append("(?,?),");
            //    }
            //    _insertQuery.append("(?,?)");

            //    sLogger.Debug(_insertQuery);
            //    sLogger.Debug(QString::number(i));
            //    query.prepare(_insertQuery);
            //    DatabaseHelper::ThrowIfError(query);

            //    foreach (QVariant v, tests)
            //    {
            //        query.addBindValue(v);
            //        query.addBindValue(123);
            //    }

            //    //query.addBindValue(tests);
            //    //if (i >= 5)
            //    {
            //        //query.execBatch();
            //        query.exec();
            //        DatabaseHelper::ThrowIfError(query);
            //    }
            //}
            
                /*auto query = _dbHelper->ExecuteQuery("COMMIT");
                DatabaseHelper::ThrowIfError(query);
                query = _dbHelper->ExecuteQuery("SET foreign_key_checks=1");
                DatabaseHelper::ThrowIfError(query);
                query = _dbHelper->ExecuteQuery("SET unique_checks=1");
                DatabaseHelper::ThrowIfError(query);
                query = _dbHelper->ExecuteQuery("SET autocommit=1");
                DatabaseHelper::ThrowIfError(query);*/

            sLogger.Info(QString("Reading file `%1`...").arg(filename));
            auto file = DataChunk::FromFile(filename);

            sLogger.Info(QString("Writing data into database `%1`...").arg(connection->DatabaseName));
            if (wrapIntoTransaction)
            {
                sLogger.Info("Starting a new transaction...");
                connection->Database().transaction();
            }
            {
                auto sink = make_unique<MySqlSink>(connection, inserterBatchSize);
                sink->AddJpsFile(file.get());
                sink->Flush();
            }
            if (wrapIntoTransaction)
            {
                connection->Database().commit();
                sLogger.Info("Transaction has been committed.");
            }
            sLogger.Info(QString("Data from `%1` has been successfully imported into database.").arg(filename));
        }
        return 0;
    }
    catch (Exception& e)
    {
        if (wrapIntoTransaction)
        {
            connection->Database().rollback();
            sLogger.Info("Transaction has been rolled back.");
        }
        sLogger.Error(e.what());
        return 1;
    }
}
