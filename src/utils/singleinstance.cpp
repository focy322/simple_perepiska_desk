#include "singleinstance.h"

#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QLocalSocket>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

SingleInstance::SingleInstance(const QString &directory, QObject *parent)
    : QObject(parent)
    , m_directory(QDir(directory).absolutePath())
    , m_serverName("Vent-activation-" + QString::fromLatin1(
          QCryptographicHash::hash(m_directory.toUtf8(), QCryptographicHash::Sha256).toHex()))
    , m_lock(QDir(m_directory).filePath("instance.lock"))
{
    m_lock.setStaleLockTime(0);
    m_server.setSocketOptions(QLocalServer::UserAccessOption);
    connect(&m_server, &QLocalServer::newConnection, this, [this]() {
        while (QLocalSocket *socket = m_server.nextPendingConnection()) {
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            emit activationRequested();
            socket->write("activated\n");
            socket->disconnectFromServer();
            if (socket->state() == QLocalSocket::UnconnectedState)
                socket->deleteLater();
        }
    });
}

SingleInstance::Result SingleInstance::start()
{
    if (m_server.isListening())
        return Result::Primary;

    if (!QDir().mkpath(m_directory)) {
        m_error = tr("Не удалось создать папку для блокировки запуска.");
        return Result::Error;
    }

    if (!m_lock.tryLock(0)) {
        if (m_lock.error() == QLockFile::LockFailedError) {
            if (activateExistingInstance())
                return Result::Activated;
            m_error = tr("Vent уже запущен, но не ответил на запрос показа окна.");
        } else {
            m_error = tr("Не удалось создать блокировку запуска.");
        }
        return Result::Error;
    }

    // Удаляем оставшийся после сбоя адрес только после получения блокировки.
    QLocalServer::removeServer(m_serverName);
    if (!m_server.listen(m_serverName)) {
        m_error = tr("Не удалось запустить локальный канал активации: %1").arg(m_server.errorString());
        m_lock.unlock();
        return Result::Error;
    }
    return Result::Primary;
}

QString SingleInstance::errorString() const
{
    return m_error;
}

bool SingleInstance::activateExistingInstance()
{
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 3000) {
#ifdef Q_OS_WIN
        qint64 pid = 0;
        QString hostname;
        QString appname;
        if (m_lock.getLockInfo(&pid, &hostname, &appname) && pid > 0)
            AllowSetForegroundWindow(static_cast<DWORD>(pid));
#endif
        QLocalSocket socket;
        socket.connectToServer(m_serverName);
        if (socket.waitForConnected(200)) {
            QByteArray response;
            while (timer.elapsed() < 3000) {
                response += socket.readAll();
                if (response.contains('\n'))
                    return response == "activated\n";
                if (!socket.waitForReadyRead(qMax(1, 3000 - static_cast<int>(timer.elapsed()))))
                    break;
            }
        }
        // Первый процесс мог получить блокировку, но ещё не успеть открыть канал.
        QThread::msleep(50);
    }
    return false;
}
