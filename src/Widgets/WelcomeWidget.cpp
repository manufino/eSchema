/*
 * WelcomeWidget.cpp
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

#include "WelcomeWidget.h"
#include "SettingsManager.h"
#include "DrawingThumbnails.h"
#include "appversion.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QPainter>
#include <QFileInfo>
#include <QDir>
#include <QEvent>
#include <QDragEnterEvent>

WelcomeWidget::WelcomeWidget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 16, 24, 16);
    layout->setSpacing(6);

    // Header row: title on the left, a small close X on the right.
    auto *headerRow = new QHBoxLayout;
    auto *title = new QLabel(tr("Welcome to eSchema"), this);
    QFont titleFont = title->font();
    titleFont.setPointSizeF(titleFont.pointSizeF() * 1.5);
    titleFont.setBold(true);
    title->setFont(titleFont);
    headerRow->addWidget(title);
    headerRow->addStretch();
    auto *closeButton = new QToolButton(this);
    closeButton->setText(QStringLiteral("✕"));
    closeButton->setAutoRaise(true);
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setToolTip(tr("Close"));
    connect(closeButton, &QToolButton::clicked, this, &WelcomeWidget::dismiss);
    headerRow->addWidget(closeButton, 0, Qt::AlignTop);
    layout->addLayout(headerRow);

    auto *version = new QLabel(tr("Version %1").arg(QStringLiteral(APP_VERSION)), this);
    version->setEnabled(false); // rendered dimmed, as secondary text
    layout->addWidget(version);
    layout->addSpacing(8);

    auto *recentTitle = new QLabel(tr("Recent files"), this);
    QFont sectionFont = recentTitle->font();
    sectionFont.setBold(true);
    recentTitle->setFont(sectionFont);
    layout->addWidget(recentTitle);
    buildRecentFiles(layout);
    layout->addSpacing(8);

    auto *tipsTitle = new QLabel(tr("Quick tips"), this);
    tipsTitle->setFont(sectionFont);
    layout->addWidget(tipsTitle);
    const QStringList tips = {
        tr("Single-letter shortcuts pick the drawing tools (L line, R rectangle, T text)"),
        tr("Drag a macro from the Libraries panel straight onto the sheet"),
        tr("Ctrl+Shift+P searches and runs any command"),
        tr("Ctrl+Tab switches between open drawings"),
    };
    for (const QString &tip : tips) {
        auto *tipLabel = new QLabel(QStringLiteral("• ") + tip, this);
        tipLabel->setWordWrap(true);
        layout->addWidget(tipLabel);
    }
    layout->addSpacing(8);

    // Unchecking persists immediately - the card still stays up until
    // dismissed, so the choice can be changed back on the spot.
    auto *showAgain = new QCheckBox(tr("Show this screen on new drawings"), this);
    const QVariant enabledVal = SettingsManager::getInstance().loadSetting("welcome_enabled");
    showAgain->setChecked(!enabledVal.isValid() || enabledVal.toBool());
    connect(showAgain, &QCheckBox::toggled, this, [](bool on) {
        SettingsManager::getInstance().saveSetting("welcome_enabled", on);
    });
    layout->addWidget(showAgain);

    setMaximumWidth(440);
    setAcceptDrops(true); // only to dismiss and hand the drag over - see dragEnterEvent()
    if (parent) {
        parent->installEventFilter(this);
        recenter();
    }
    raise();
}

void WelcomeWidget::buildRecentFiles(QVBoxLayout *layout)
{
    const QStringList recents =
            SettingsManager::getInstance().loadSetting("recent_files").toStringList();
    int shown = 0;
    for (const QString &path : recents) {
        if (shown >= 5)
            break;
        if (!QFileInfo::exists(path))
            continue;
        // One row per file: a small rendering of the drawing (see
        // DrawingThumbnails - cached, so rebuilding cards stays cheap)
        // next to the clickable name.
        auto *row = new QHBoxLayout;
        row->setSpacing(10);
        const QPixmap thumb = DrawingThumbnails::thumbnail(path, QSize(64, 48));
        if (!thumb.isNull()) {
            auto *preview = new QLabel(this);
            preview->setPixmap(thumb);
            preview->setFrameShape(QFrame::Box);
            row->addWidget(preview);
        }
        auto *link = new QLabel(QStringLiteral("<a href=\"%1\">%2</a>")
                .arg(path.toHtmlEscaped(), QFileInfo(path).fileName().toHtmlEscaped()), this);
        link->setToolTip(QDir::toNativeSeparators(path));
        link->setTextInteractionFlags(Qt::TextBrowserInteraction);
        connect(link, &QLabel::linkActivated, this, [this](const QString &href) {
            emit openFileRequested(href);
        });
        row->addWidget(link);
        row->addStretch();
        layout->addLayout(row);
        ++shown;
    }
    if (shown == 0) {
        auto *none = new QLabel(tr("No recent files yet"), this);
        none->setEnabled(false); // dimmed secondary text
        layout->addWidget(none);
    }
}

void WelcomeWidget::dismiss()
{
    hide();
}

void WelcomeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QColor background = palette().color(QPalette::Window);
    background.setAlpha(240); // a hint of the sheet showing through
    painter.setPen(QPen(palette().color(QPalette::Mid)));
    painter.setBrush(background);
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 10, 10);
}

void WelcomeWidget::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    dismiss();
}

void WelcomeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    // Never accepted here: hiding the card mid-drag means the very next
    // drag move targets the canvas underneath instead, which then handles
    // the drop normally (see SheetView's drag handlers).
    dismiss();
    event->ignore();
}

bool WelcomeWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize)
        recenter();
    return QWidget::eventFilter(watched, event);
}

void WelcomeWidget::recenter()
{
    if (!parentWidget())
        return;
    adjustSize();
    const QSize parentSize = parentWidget()->size();
    move((parentSize.width() - width()) / 2, (parentSize.height() - height()) / 2);
}
