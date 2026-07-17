/*
 * UpdateChecker.h
 *
 * Copyright (C) 2023-2026 Manuel Finessi
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef UPDATECHECKER_H
#define UPDATECHECKER_H

#include <QObject>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

// Checks GitHub Releases (a single HTTPS GET, no authentication) for a
// newer eSchema version than the one currently running. Never downloads or
// runs anything itself - an available update is only ever offered as a link
// to the GitHub release page, opened in the user's own browser if they
// agree. Every failure (no network, a firewall silently dropping the
// connection, a malformed/unexpected response) is reported through
// checkFailed() rather than left to throw/crash, so callers - in
// particular a silent startup check - never need special-casing to stay
// safe with no connection at all.
class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);

    // No-op if a check is already in flight.
    void checkForUpdates();

signals:
    // `version` is the release tag with any leading "v" stripped (e.g.
    // "1.0.4"), `releaseUrl` its GitHub release page.
    void updateAvailable(const QString &version, const QUrl &releaseUrl);
    // The running version already matches (or is newer than) the latest
    // release - only the manual menu check shows anything for this.
    void upToDate();
    // Network or parse error - the silent startup check ignores it, the
    // manual check reports it to the user.
    void checkFailed();

private:
    // Parses the GitHub API JSON reply, compares versions, and emits exactly
    // one of the three signals above.
    void handleReply(QNetworkReply *reply);

    QNetworkAccessManager *m_manager;
    bool m_checking = false;
};

#endif // UPDATECHECKER_H
