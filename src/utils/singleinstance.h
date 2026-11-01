#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H

#include <QObject>
#include <QLocalServer>
#include <QLockFile>
#include <QString>

class SingleInstance : public QObject
{
    Q_OBJECT
public:
    enum class Result { Primary, Activated, Error };

    explicit SingleInstance(const QString &directory, QObject *parent = nullptr);
    Result start();
    QString errorString() const;

signals:
    void activationRequested();

private:
    bool activateExistingInstance();

    QString m_directory;
    QString m_serverName;
    QLockFile m_lock;
    QLocalServer m_server;
    QString m_error;
};

#endif // SINGLEINSTANCE_H
