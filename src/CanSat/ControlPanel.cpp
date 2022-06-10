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

#include "ControlPanel.h"

#include <QDir>
#include <QTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QFileDialog>
#include <QJsonObject>
#include <QJsonDocument>

#include <qtcsv/reader.h>
#include <Misc/Utilities.h>
#include <Misc/TimerEvents.h>
#include <SerialStudio/Plugin.h>

/**
 * Constructor function
 */
CanSat::ControlPanel::ControlPanel()
{
    // Set default values
    m_row = 0;
    m_currentTime = "";
    m_simulationEnabled = false;
    m_simulationActivated = false;
    m_containerTelemetryEnabled = false;

    // Timer module signals/slots
    auto te = &(Misc::TimerEvents::instance());
    connect(te, &Misc::TimerEvents::timeout20Hz, this,
            &CanSat::ControlPanel::updateCurrentTime);

    // Plugins comm signals/slots
    auto pc = &(SerialStudio::Plugin::instance());
    connect(pc, &SerialStudio::Plugin::dataReceived, this,
            &CanSat::ControlPanel::onDataReceived);
}

/**
 * Returns a pointer to the only instance of the class
 */
CanSat::ControlPanel &CanSat::ControlPanel::instance()
{
    static ControlPanel singleton;
    return singleton;
}

/**
 * Returns @c true if the simulation mode is enabled
 */
bool CanSat::ControlPanel::simulationEnabled() const
{
    return m_simulationEnabled;
}

/**
 * Returns @c true if simulation mode is enabled & active
 */
bool CanSat::ControlPanel::simulationActivated() const
{
    return simulationEnabled() && m_simulationActivated;
}

/**
 * Returns @c true if container telemetry is enabled
 */
bool CanSat::ControlPanel::containerTelemetryEnabled() const
{
    return m_containerTelemetryEnabled;
}

/**
 * Returns @c true if the simulation CSV file has been loaded
 */
bool CanSat::ControlPanel::simulationCsvLoaded() const
{
    return m_file.isOpen();
}

/**
 * Returns current time in hh:mm:ss:zzz format
 */
QString CanSat::ControlPanel::currentTime() const
{
    return m_currentTime;
}

/**
 * Returns the name of the currently loaded CSV file
 */
QString CanSat::ControlPanel::csvFileName() const
{
    if (simulationCsvLoaded())
    {
        auto fileInfo = QFileInfo(m_file.fileName());
        return fileInfo.fileName();
    }

    return tr("No CSV file selected");
}

/**
 * Opens a dialog that allows the user to select a CSV file to load to the application.
 * The CSV file must contain only one column with simulated pressure data.
 */
void CanSat::ControlPanel::openCsv()
{
    // clang-format off
    auto name = QFileDialog::getOpenFileName(Q_NULLPTR,
                                             tr("Select simulation file"),
                                             QDir::homePath());
    // clang-format on

    // User did not select a file, abort
    if (name.isEmpty())
        return;

    // Close current file
    if (m_file.isOpen())
        m_file.close();

    // Open the selected file
    m_file.setFileName(name);
    if (m_file.open(QFile::ReadOnly))
    {
        // Close temp. file
        if (m_tempFile.isOpen())
            m_tempFile.close();

        // Disable simulation mode
        if (simulationActivated())
            setSimulationActivated(false);

        // Generate CSV data
        QString csv;
        QTextStream in(&m_file);
        while (!in.atEnd())
        {
            QString line = in.readLine();
            line.replace(" ", "");
            if (line.startsWith("#") || line.isEmpty())
                continue;
            else
            {
                line.replace("$", "1026");
                csv.append(line);
                csv.append("\n");
            }
        }

        // Save CSV file to temp path
        m_tempFile.setFileName(QDir::tempPath() + "/CC2022_temp.csv");
        if (m_tempFile.open(QFile::WriteOnly))
        {
            m_tempFile.write(csv.toUtf8());
            m_tempFile.close();
        }

        // Read CSV data
        if (m_tempFile.open(QFile::ReadOnly))
        {
            m_row = 1;
            m_csvData = QtCSV::Reader::readToList(m_tempFile);
        }

        // Update UI
        Q_EMIT printLn("[INFO]\tLoaded simulation CSV file from " + m_file.fileName());
        Q_EMIT printLn("[INFO]\tProcessed simulation CSV saved at "
                       + m_tempFile.fileName());
    }

    // Open failure, alert user through a messagebox
    else
        Misc::Utilities::showMessageBox(tr("File open error"), m_file.errorString());

    // Update UI
    emit csvFileNameChanged();
}

/**
 * Sends the current time to the payload with the hh:mm:ss format
 */
void CanSat::ControlPanel::updateContainerTime()
{
    if (SerialStudio::Plugin::instance().isConnected())
    {
        auto time = QDateTime::currentDateTime().toString("hh:mm:ss");
        sendData("CMD,1026,ST," + time + ";");
    }
}

/**
 * Enables/disables simulation mode
 */
void CanSat::ControlPanel::setSimulationMode(const bool enabled)
{
    if (SerialStudio::Plugin::instance().isConnected())
    {
        m_simulationActivated = false;
        m_simulationEnabled = enabled;
        emit simulationEnabledChanged();
        emit simulationActivatedChanged();

        QString cmd = "DISABLE";
        if (simulationEnabled())
            cmd = "ENABLE";

        sendData("CMD,1026,SIM," + cmd + ";");
    }
}

/**
 * Activates/deactivates sending simulated pressure readings to the CanSat
 */
void CanSat::ControlPanel::setSimulationActivated(const bool activated)
{
    if (SerialStudio::Plugin::instance().isConnected() && simulationEnabled())
    {
        if (activated && !csvFileName().isEmpty())
        {
            m_simulationActivated = true;
            emit simulationActivatedChanged();
            sendData("CMD,1026,SIM,ACTIVATE;");
            Q_EMIT printLn("[INFO]\tWating 5 seconds before sending data...");
            QTimer::singleShot(5000, this, SLOT(sendSimulatedData()));
        }

        else
            setSimulationMode(false);
    }
}

/**
 * Enables/disables container telemetry
 */
void CanSat::ControlPanel::setContainerTelemetryEnabled(const bool enabled)
{
    if (SerialStudio::Plugin::instance().isConnected())
    {
        m_containerTelemetryEnabled = enabled;
        emit containerTelemetryEnabledChanged();

        QString cmd = "OFF";
        if (enabled)
            cmd = "ON";

        sendData("CMD,1026,CX," + cmd + ";");
    }
}

/**
 * Reads incoming data from Serial Studio
 */
void CanSat::ControlPanel::onDataReceived(const QByteArray &data)
{
    // Append data to buffer
    m_dataBuffer.append(data);

    // Read until start/finish combinations are not found
    auto bytes = 0;
    auto cursor = m_dataBuffer;
    auto start = QString("/*").toUtf8();
    auto finish = QString("*/").toUtf8();
    while (cursor.contains(start) && cursor.contains(finish))
    {
        // Remove the part of the buffer prior to, and including, the start sequence.
        auto sIndex = cursor.indexOf(start);
        cursor = cursor.mid(sIndex + start.length(), -1);
        bytes += sIndex + start.length();

        // Copy a sub-buffer that goes until the finish sequence
        auto fIndex = cursor.indexOf(finish);
        auto frame = cursor.left(fIndex);

        // Process frame data
        processFrame(frame);

        // Remove the data including the finish sequence from the master buffer
        cursor = cursor.mid(fIndex, -1);
        bytes += fIndex + finish.length();
    }

    // Remove parsed data from master buffer
    m_dataBuffer.remove(0, bytes);

    // Clear temp. buffer (e.g. device sends a lot of invalid data)
    if (m_dataBuffer.size() > 1024 * 10 * 10)
        m_dataBuffer.clear();
}

/**
 * Gets the current time in hh:mm:ss:zzz format. This value is used by the user interface,
 * not by the CanSat container.
 */
void CanSat::ControlPanel::updateCurrentTime()
{
    m_currentTime = QDateTime::currentDateTime().toString("hh:mm:ss:zzz");
    emit currentTimeChanged();
}

/**
 * Reads, validates & sends current simulated pressure reading to the CanSat.
 *
 * If we reach the last CSV row, then simulation mode shall be disabled & a message-box
 * shall be shown to the user.
 */
void CanSat::ControlPanel::sendSimulatedData()
{
    // Stop if simulation mode is not active
    if (!simulationActivated() || !SerialStudio::Plugin::instance().isConnected())
        return;

    // Read, validate & send current row data
    if (m_row < m_csvData.count() && m_row >= 0)
    {
        // Get CSV row data
        auto row = m_csvData.at(m_row);

        // Generate row string
        QString cmd = "CMD,1026,SIMP,";
        cmd.append(row.first());
        cmd.append(";");

        // Send command
        if (!cmd.isEmpty())
            sendData(cmd);

        // Column count invalid
        else
            Misc::Utilities::showMessageBox(
                tr("Simulation CSV error"),
                tr("Invalid column count at row %1").arg(m_row));

        // Increment row
        ++m_row;

        // Llamar esta funcion en un segundo
        QTimer::singleShot(1000, this, SLOT(sendSimulatedData()));
    }

    // Show CSV finished box & disable simulation mode
    else
    {
        setSimulationActivated(false);
        Misc::Utilities::showMessageBox(tr("Pressure simulation finished"),
                                        tr("Reached end of CSV file"));
    }
}

/**
 * Writes the data of the given @a frame to its associated CSV file
 */
void CanSat::ControlPanel::processFrame(const QByteArray &frame)
{
    // Validate frame
    if (frame.isEmpty())
        return;

    // Frame begins with 6026
    if (frame.startsWith(QString("6026").toUtf8()))
    {
        // File is not open, create it
        if (!m_payloadCsv.isOpen())
        {
            if (!createCsv(false))
            {
                Misc::Utilities::showMessageBox(tr("Error while creating payload CSV"),
                                                m_payloadCsv.errorString());
                return;
            }
        }

        // Escribir datos al CSV
        m_payloadCsv.write(frame);
        m_payloadCsv.write("\n");
    }

    // Frame begins with 1026
    else if (frame.startsWith(QString("1026").toUtf8()))
    {
        // File is not open, create it
        if (!m_containerCsv.isOpen())
        {
            if (!createCsv(true))
            {
                Misc::Utilities::showMessageBox(tr("Error while creating container CSV"),
                                                m_containerCsv.errorString());
                return;
            }
        }

        // Escribir datos al CSV
        m_containerCsv.write(frame);
        m_containerCsv.write("\n");
    }

    // Update user interface
    Q_EMIT printLn("  [RX]\t" + QString::fromUtf8(frame));
}

/**
 * Sends the given @a data string to Serial Studio, which in turn sends the data through
 * the serial port.
 */
bool CanSat::ControlPanel::sendData(const QString &data)
{
    // Data is empty, abort
    if (data.isEmpty())
        return false;

    // We are not connected to Serial Studio, abort transmission
    if (!SerialStudio::Plugin::instance().isConnected())
        return false;

    // Define Xbee 64-bit address
    QByteArray address64bit;
    address64bit.append((quint8)0x00); // 0x7D in XCTU
    address64bit.append((quint8)0x13); // 0x33 in XCTU
    address64bit.append((quint8)0xA2);
    address64bit.append((quint8)0x00);
    address64bit.append((quint8)0x41);
    address64bit.append((quint8)0xB1);
    address64bit.append((quint8)0x8C);
    address64bit.append((quint8)0x8D);

    // Define Xbee 16-bit address
    QByteArray address16bit;
    address16bit.append((quint8)0xFF);
    address16bit.append((quint8)0xFE);

    // Begin constructing Xbee API frame
    QByteArray frame;
    frame.append((quint8)0x10); // Transmit request
    frame.append((quint8)0x00); // No acknowledgement
    frame.append((quint8)0x00); // Frame ID
    frame.append(address64bit); // 64-bit destination address
    frame.append(address16bit); // 16-bit destination address
    frame.append((quint8)0x00); // Broadcast radio
    frame.append((quint8)0x00); // Options

    // Add data to frame
    frame.append(data.toUtf8());

    // Calculate sum
    quint16 sum = 0;
    for (auto i = 0; i < frame.length(); ++i)
        sum += (quint8)frame[i];

    // Calculate checksum
    quint8 crc = 0xff - ((sum)&0xFF);

    // Calculate frame length
    quint16 length = frame.length() - 1;

    // Replace first two bytes of 64-bit address after the CRC is calculated,
    // for some unknown reason, the CRC needs to be calculated with non-MSB bytes
    QByteArray msbAddress;
    msbAddress.append((quint8)0x7D);
    msbAddress.append((quint8)0x33);
    frame = frame.replace(3, 2, msbAddress);

    // Generate full frame
    QByteArray apiFrame;
    apiFrame.append((quint8)0x7E);
    apiFrame.append((quint8)((length) >> 8) & 0xff);
    apiFrame.append((quint8)((length) >> 0) & 0xff);
    apiFrame.append(frame);
    apiFrame.append((quint8)crc);

    // Send data to Serial Studio
    Q_EMIT printLn("  [TX]\t" + data);
    return SerialStudio::Plugin::instance().write(apiFrame);
}

/**
 * Creates a new CSV file with current date/time, the filaname of the created
 * CSV file depends on the value of @a createContainerCsv
 */
bool CanSat::ControlPanel::createCsv(const bool createContainerCsv)
{
    // Get current date time
    const auto dateTime = QDateTime::currentDateTime();

    // Get file name
    const QString title = createContainerCsv ? "Container" : "Payload";
    const QString fileName = title + "_" + dateTime.toString("HH-mm-ss") + ".csv";

    // Get path
    const QString format = dateTime.toString("yyyy/MMM/dd/");
    const QString path = QString("%1/Documents/%2/%3")
                             .arg(QDir::homePath(), qApp->applicationName(), format);

    // Generate file path if required
    QDir dir(path);
    if (!dir.exists())
        dir.mkpath(".");

    // Update UI
    Q_EMIT printLn("[INFO]\tCreating new CSV file at " + dir.filePath(fileName));

    // Create container CSV file
    if (createContainerCsv)
    {
        m_containerCsv.close();
        m_containerCsv.setFileName(dir.filePath(fileName));
        return m_containerCsv.open(QFile::WriteOnly);
    }

    // Create payload CSV file
    else
    {
        m_payloadCsv.close();
        m_payloadCsv.setFileName(dir.filePath(fileName));
        return m_payloadCsv.open(QFile::WriteOnly);
    }
}
