#include "exportthread.h"
#include "csvexporter.h"
#include <QDebug>

ExportThread::ExportThread(QObject *parent)
    : QThread(parent)
    , exporter(new CsvExporter(this))
{
}

ExportThread::~ExportThread()
{
    wait(); // 等待线程完成
}

void ExportThread::setExportType(const QString &type)
{
    exportType = type;
}

void ExportThread::setFilename(const QString &filename)
{
    this->filename = filename;
}

void ExportThread::setRegistrationsData(const QList<QHash<QString, QVariant>> &registrations,
                                       const QHash<QString, QVariant> &activity)
{
    this->registrations = registrations;
    this->activity = activity;
}

void ExportThread::setStatisticsData(const QList<QHash<QString, QVariant>> &statistics)
{
    this->statistics = statistics;
}

void ExportThread::run()
{
    bool success = false;
    QString message;
    
    try {
        if (exportType == "registrations") {
            emit exportProgress(10);
            success = exporter->exportRegistrations(filename, registrations, activity);
            emit exportProgress(100);
            message = success ? "报名名单导出成功！" : "报名名单导出失败！";
        } else if (exportType == "statistics") {
            // 对于大数据量，模拟进度更新
            int total = statistics.size();
            int processed = 0;
            
            emit exportProgress(10);
            
            // 这里可以分批处理数据以显示进度
            // 由于CsvExporter是一次性写入，我们模拟进度
            for (int i = 0; i < total; i += qMax(1, total / 10)) {
                processed = qMin(i + total / 10, total);
                emit exportProgress(10 + (processed * 80 / total));
                msleep(10); // 模拟处理时间
            }
            
            success = exporter->exportStatistics(filename, statistics);
            emit exportProgress(100);
            message = success ? "统计报表导出成功！" : "统计报表导出失败！";
        } else {
            emit exportError("未知的导出类型：" + exportType);
            return;
        }
        
        emit exportFinished(success, message);
    } catch (const std::exception &e) {
        emit exportError(QString("导出异常：%1").arg(e.what()));
    } catch (...) {
        emit exportError("导出过程中发生未知错误");
    }
}



