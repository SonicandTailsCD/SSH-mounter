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

#include "ssh_store.hpp"
#include "console.hpp"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QStandardPaths>

extern Console console;

QJsonObject SSHHost::toJson() const {
    QJsonObject obj;
    obj["name"] = name;
    obj["user"] = user;
    obj["host"] = host;
    obj["remotePath"] = remotePath;
    obj["localPath"] = localPath;
    obj["port"] = port;
    return obj;
}

SSHHost SSHHost::fromJson(const QJsonObject& obj) {
    SSHHost h;
    h.name = obj["name"].toString();
    h.user = obj["user"].toString();
    h.host = obj["host"].toString();
    h.remotePath = obj["remotePath"].toString();
    h.localPath = obj["localPath"].toString();
    h.port = obj["port"].toInt(22);
    return h;
}

SSHStore::SSHStore(QObject* parent) : QObject(parent) {
    QString home = QDir::homePath();
    filePath_ = home + "/.ssh/mounter/hosts.json";
}

QString SSHStore::getFilePath() const {
    return filePath_;
}

void SSHStore::ensureDirectoryExists() {
    QFileInfo info(filePath_);
    QDir dir = info.dir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

bool SSHStore::load() {
    QFile file(filePath_);
    if (!file.exists()) {
        hosts_.clear();
        console.log("No existing hosts file, starting fresh");
        return true; // Empty is fine
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        console.error("Cannot read", filePath_.toStdString());
        emit error("Cannot read " + filePath_);
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        console.error("Invalid JSON format in hosts file");
        emit error("Invalid JSON format");
        return false;
    }
    
    QJsonArray arr = doc.object()["hosts"].toArray();
    hosts_.clear();
    for (const auto& val : arr) {
        hosts_.append(SSHHost::fromJson(val.toObject()));
    }
    
    console.log("Loaded", hosts_.size(), "host(s) from", filePath_.toStdString());
    emit hostsChanged();
    return true;
}

bool SSHStore::save() {
    ensureDirectoryExists();
    
    QJsonArray arr;
    for (const auto& host : hosts_) {
        arr.append(host.toJson());
    }
    
    QJsonObject root;
    root["hosts"] = arr;
    
    QFile file(filePath_);
    if (!file.open(QIODevice::WriteOnly)) {
        console.error("Cannot write to", filePath_.toStdString());
        emit error("Cannot write to " + filePath_);
        return false;
    }
    
    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    console.log("Saved", hosts_.size(), "host(s) to", filePath_.toStdString());
    return true;
}

void SSHStore::addHost(const SSHHost& host) {
    hosts_.append(host);
    emit hostsChanged();
}

void SSHStore::removeHost(int index) {
    if (index >= 0 && index < hosts_.size()) {
        hosts_.removeAt(index);
        emit hostsChanged();
    }
}

void SSHStore::updateHost(int index, const SSHHost& host) {
    if (index >= 0 && index < hosts_.size()) {
        hosts_[index] = host;
        emit hostsChanged();
    }
}

#include "ssh_store.moc"
