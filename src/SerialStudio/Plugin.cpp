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

#include <SerialStudio/Plugin.h>

#include <QTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QHostAddress>
#include <QJsonDocument>
#include <Misc/Utilities.h>
#include <Misc/TimerEvents.h>

/*
 * Set TCP port to communicate with Serial Studio
 */
#define SERIAL_STUDIO_PLUGINS_PORT 7777

/**
 * Constructor function
 */
SerialStudio::Plugin::Plugin()
{
    // Connect socket signals/slots
    connect(&m_socket, &QTcpSocket::readyRead, this, &Plugin::onDataReceived);
    connect(&m_socket, &QTcpSocket::disconnected, &m_socket, &QTcpSocket::close);
    connect(&m_socket, &QTcpSocket::connected, this, &Plugin::onConnectedChanged);
    connect(&m_socket, &QTcpSocket::disconnected, this, &Plugin::onConnectedChanged);

    // Timer module signals/slots
    auto te = &(Misc::TimerEvents::instance());
    connect(te, &Misc::TimerEvents::timeout1Hz, this, &Plugin::tryConnection);
}

/**
 * Returns a pointer to the only instance of the class
 */
SerialStudio::Plugin &SerialStudio::Plugin::instance()
{
    static Plugin singleton;
    return singleton;
}

/**
 * Returns @c true if the application is connected to the Serial Studio TCP server
 */
bool SerialStudio::Plugin::isConnected() const
{
    return m_socket.state() == QTcpSocket::ConnectedState;
}

/**
 * Sends the given @a data string to Serial Studio, which in turn sends the data through
 * the serial port.
 */
bool SerialStudio::Plugin::write(const QByteArray &data)
{
    return m_socket.write(data) == data.length();
}

/**
 * Tries to establish a connection with Serial Studio's TCP server
 */
void SerialStudio::Plugin::tryConnection()
{
    if (!isConnected())
    {
        m_socket.abort();
        m_socket.connectToHost(QHostAddress::LocalHost, SERIAL_STUDIO_PLUGINS_PORT);
    }
}

/**
 * Reads incoming data from the TCP socket
 */
void SerialStudio::Plugin::onDataReceived()
{
    auto json = QJsonDocument::fromJson(m_socket.readAll());
    if (!json.isEmpty())
    {
        auto base64 = json.object().value("data").toString();
        auto data = QByteArray::fromBase64(base64.toUtf8());
        Q_EMIT dataReceived(data);
    }
}

/**
 * Waits 500 ms and notifies the UI if the TCP connection with Serial Studio has been
 * established.
 *
 * We need to wait in order to avoid 'flickering' in the UI when the plugin system of
 * Serial Studio is disabled.
 */
void SerialStudio::Plugin::onConnectedChanged()
{
    QTimer::singleShot(500, this, &Plugin::connectedChanged);
}

/**
 * Displays any socket errors with a message-box
 */
void SerialStudio::Plugin::onErrorOccurred(const QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    Misc::Utilities::showMessageBox(tr("TCP socket error"), m_socket.errorString());
}
