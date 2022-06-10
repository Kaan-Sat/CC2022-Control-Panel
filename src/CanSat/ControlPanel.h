/*
 * Copyright (c) 2022 Alex Spataru <https://github.com/alex-spataru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#pragma once

#include <QFile>
#include <QObject>

namespace CanSat
{
class ControlPanel : public QObject
{
    // clang-format off
    Q_OBJECT
    Q_PROPERTY(bool simulationEnabled
               READ simulationEnabled
               WRITE setSimulationMode
               NOTIFY simulationEnabledChanged)
    Q_PROPERTY(bool simulationActivated
               READ simulationActivated
               WRITE setSimulationActivated
               NOTIFY simulationActivatedChanged)
    Q_PROPERTY(bool containerTelemetryEnabled
               READ containerTelemetryEnabled
               WRITE setContainerTelemetryEnabled
               NOTIFY containerTelemetryEnabledChanged)
    Q_PROPERTY(QString currentTime
               READ currentTime
               NOTIFY currentTimeChanged)
    Q_PROPERTY(QString csvFileName
               READ csvFileName
               NOTIFY csvFileNameChanged)
    Q_PROPERTY(bool simulationCsvLoaded
               READ simulationCsvLoaded
               NOTIFY csvFileNameChanged)
    // clang-format on

Q_SIGNALS:
    void currentTimeChanged();
    void csvFileNameChanged();
    void printLn(const QString &data);
    void simulationEnabledChanged();
    void simulationActivatedChanged();
    void containerTelemetryEnabledChanged();

private:
    ControlPanel();
    ControlPanel(ControlPanel &&) = delete;
    ControlPanel(const ControlPanel &) = delete;
    ControlPanel &operator=(ControlPanel &&) = delete;
    ControlPanel &operator=(const ControlPanel &) = delete;

public:
    static ControlPanel &instance();

public:
    bool simulationEnabled() const;
    bool simulationActivated() const;
    bool containerTelemetryEnabled() const;

    QString currentTime() const;
    QString csvFileName() const;
    bool simulationCsvLoaded() const;

public slots:
    void openCsv();
    void updateContainerTime();
    void setSimulationMode(const bool enabled);
    void setSimulationActivated(const bool activated);
    void setContainerTelemetryEnabled(const bool enabled);

private slots:
    void updateCurrentTime();
    void sendSimulatedData();
    void processFrame(const QByteArray &frame);
    void onDataReceived(const QByteArray &data);

private:
    bool sendData(const QString &data);
    bool createCsv(const bool isContainer);

private:
    int m_row;
    QFile m_file;
    QFile m_tempFile;
    QString m_currentTime;
    QByteArray m_dataBuffer;
    QList<QStringList> m_csvData;

    QFile m_payloadCsv;
    QFile m_containerCsv;

    bool m_simulationEnabled;
    bool m_simulationActivated;
    bool m_containerTelemetryEnabled;
};
};
