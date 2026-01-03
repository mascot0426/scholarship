#ifndef CSVEXPORTER_H
#define CSVEXPORTER_H

#include <QObject>
#include <QHash>
#include <QVariant>
#include <QList>

class CsvExporter : public QObject
{
    Q_OBJECT

public:
    explicit CsvExporter(QObject *parent = nullptr);
    
    bool exportRegistrations(const QString &filename, 
                            const QList<QHash<QString, QVariant>> &registrations,
                            const QHash<QString, QVariant> &activity);
    
    bool exportStatistics(const QString &filename,
                         const QList<QHash<QString, QVariant>> &statistics);

private:
    QString escapeCsvField(const QString &field);
};

#endif // CSVEXPORTER_H

