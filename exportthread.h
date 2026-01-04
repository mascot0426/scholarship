#ifndef EXPORTTHREAD_H
#define EXPORTTHREAD_H

#include <QThread>
#include <QString>
#include <QHash>
#include <QVariant>
#include <QList>

class CsvExporter;

class ExportThread : public QThread
{
    Q_OBJECT

public:
    explicit ExportThread(QObject *parent = nullptr);
    ~ExportThread();
    
    void setExportType(const QString &type); // "registrations" or "statistics"
    void setFilename(const QString &filename);
    void setRegistrationsData(const QList<QHash<QString, QVariant>> &registrations,
                             const QHash<QString, QVariant> &activity);
    void setStatisticsData(const QList<QHash<QString, QVariant>> &statistics);

signals:
    void exportProgress(int percentage);
    void exportFinished(bool success, const QString &message);
    void exportError(const QString &error);

protected:
    void run() override;

private:
    QString exportType;
    QString filename;
    QList<QHash<QString, QVariant>> registrations;
    QHash<QString, QVariant> activity;
    QList<QHash<QString, QVariant>> statistics;
    CsvExporter *exporter;
};

#endif // EXPORTTHREAD_H






