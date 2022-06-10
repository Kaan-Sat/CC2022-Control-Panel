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

#include <QObject>
#include <QTcpSocket>

namespace SerialStudio
{
class Plugin : public QObject
{
    // clang-format off
    Q_OBJECT
    Q_PROPERTY(bool isConnected
               READ isConnected
               NOTIFY connectedChanged)
    // clang-format on

Q_SIGNALS:
    void connectedChanged();
    void dataReceived(const QByteArray &data);

private:
    Plugin();
    Plugin(Plugin &&) = delete;
    Plugin(const Plugin &) = delete;
    Plugin &operator=(Plugin &&) = delete;
    Plugin &operator=(const Plugin &) = delete;

public:
    static Plugin &instance();

public:
    bool isConnected() const;
    bool write(const QByteArray &data);

public slots:
    void tryConnection();

private slots:
    void onDataReceived();
    void onConnectedChanged();
    void onErrorOccurred(const QAbstractSocket::SocketError socketError);

private:
    QTcpSocket m_socket;
};
};
