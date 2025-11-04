/*
 * SSH Mounter - Simple GUI for SSHFS mounts
 * Copyright (C) 2025 SonicandTailsCD
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 */

#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

struct SSHHost {
    QString name;
    QString user;
    QString host;
    QString remotePath;
    QString localPath;
    int port = 22;
    bool usePublicKey = false;  // If true, use public key auth only. If false, use password auth.
    
    QJsonObject toJson() const;
    static SSHHost fromJson(const QJsonObject& obj);
};

class SSHStore : public QObject {
    Q_OBJECT
public:
    explicit SSHStore(QObject* parent = nullptr);
    
    bool load();
    bool save();
    
    QList<SSHHost> getHosts() const { return hosts_; }
    void addHost(const SSHHost& host);
    void removeHost(int index);
    void updateHost(int index, const SSHHost& host);
    
    QString getFilePath() const;
    
signals:
    void hostsChanged();
    void error(const QString& message);
    
private:
    QList<SSHHost> hosts_;
    QString filePath_;
    
    void ensureDirectoryExists();
};