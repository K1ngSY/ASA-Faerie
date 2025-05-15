#ifndef EXPIRATIONDETECTOR_H
#define EXPIRATIONDETECTOR_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QTimer>

class ExpirationDetector : public QObject
{
    Q_OBJECT
public:
    explicit ExpirationDetector(QObject *parent = nullptr, QDateTime expDT = QDateTime());

    void setExpDateTime(QDateTime expDT);
    ~ExpirationDetector();
public slots:
    void start();
    void stop();

signals:
    void logMessage(const QString &msg);
    void expirated();

private:
    QDateTime m_expDateTime;
    QTimer *m_expireTimer;

private slots:
    void checkExpiration();
};

#endif // EXPIRATIONDETECTOR_H
