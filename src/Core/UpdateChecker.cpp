/*
 * UpdateChecker.cpp
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

#include "UpdateChecker.h"
#include "appversion.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

const char *ReleasesApiUrl = "https://api.github.com/repos/manufino/eSchema/releases/latest";

// Compares two "x.y.z" version strings numerically component-by-component -
// not lexically, so "1.0.10" correctly sorts after "1.0.9". Missing or
// non-numeric components count as 0. Returns >0 if `a` is newer than `b`.
int compareVersions(const QString &a, const QString &b)
{
    const QStringList partsA = a.split(QLatin1Char('.'));
    const QStringList partsB = b.split(QLatin1Char('.'));
    const int count = qMax(partsA.size(), partsB.size());
    for (int i = 0; i < count; ++i) {
        const int va = i < partsA.size() ? partsA.at(i).toInt() : 0;
        const int vb = i < partsB.size() ? partsB.at(i).toInt() : 0;
        if (va != vb)
            return va - vb;
    }
    return 0;
}

} // namespace

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{
}

void UpdateChecker::checkForUpdates()
{
    if (m_checking)
        return;
    m_checking = true;

    QNetworkRequest request{QUrl(QString::fromLatin1(ReleasesApiUrl))};
    // GitHub's REST API rejects requests with no User-Agent header (403) -
    // the exact value doesn't matter, it just has to be present.
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("eSchema-UpdateChecker"));
    request.setRawHeader("Accept", "application/vnd.github+json");
    // Qt itself never throws on a network failure, but with no timeout a
    // firewall that silently drops packets (rather than actively refusing
    // the connection) can leave the request hanging indefinitely.
    request.setTransferTimeout(8000);
    // Redirects, if GitHub ever issues one, must stay on https - never
    // silently follow a downgrade to plain http.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                          QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        handleReply(reply);
    });
}

void UpdateChecker::handleReply(QNetworkReply *reply)
{
    reply->deleteLater();
    m_checking = false;

    if (reply->error() != QNetworkReply::NoError) {
        emit checkFailed();
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject()) {
        emit checkFailed();
        return;
    }

    const QJsonObject obj = doc.object();
    QString tag = obj.value(QStringLiteral("tag_name")).toString();
    const QString htmlUrl = obj.value(QStringLiteral("html_url")).toString();
    if (tag.isEmpty() || htmlUrl.isEmpty()) {
        emit checkFailed();
        return;
    }

    // Release tags are "vX.Y.Z" - strip the leading "v" to match APP_VERSION.
    if (tag.startsWith(QLatin1Char('v'), Qt::CaseInsensitive))
        tag.remove(0, 1);

    // Only ever offer an https://github.com/... link, even though it always
    // comes straight from GitHub's own API over a certificate-verified TLS
    // connection - cheap defense in depth against ever pointing the user
    // somewhere else.
    const QUrl releaseUrl(htmlUrl);
    if (releaseUrl.scheme() != QStringLiteral("https")
            || !releaseUrl.host().endsWith(QStringLiteral("github.com"))) {
        emit checkFailed();
        return;
    }

    if (compareVersions(tag, QString::fromLatin1(APP_VERSION)) > 0)
        emit updateAvailable(tag, releaseUrl);
    else
        emit upToDate();
}
