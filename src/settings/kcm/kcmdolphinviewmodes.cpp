/*
 * SPDX-FileCopyrightText: 2008 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kcmdolphinviewmodes.h"

#include "settings/viewmodes/viewsettingstab.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QIcon>
#include <QTabWidget>
#include <QVBoxLayout>

K_PLUGIN_CLASS_WITH_JSON(DolphinViewModesConfigModule, "kcmdolphinviewmodes.json")

DolphinViewModesConfigModule::DolphinViewModesConfigModule(QWidget *parent, const QVariantList &args) :
    KCModule(parent, args),
    m_tabs()
{
    setButtons(KCModule::Default | KCModule::Help);

    QVBoxLayout* topLayout = new QVBoxLayout(this);
    topLayout->setContentsMargins(0, 0, 0, 0);

    QTabWidget* tabWidget = new QTabWidget(this);

    // Initialize 'Icons' tab
    ViewSettingsTab* iconsTab = new ViewSettingsTab(ViewSettingsTab::IconsMode, tabWidget);
    tabWidget->addTab(iconsTab, QIcon::fromTheme(QStringLiteral("view-list-icons")), i18nc("@title:tab", "Icons"));
    connect(iconsTab, &ViewSettingsTab::changed, this, &DolphinViewModesConfigModule::viewModeChanged);

    // Initialize 'Compact' tab
    ViewSettingsTab* compactTab = new ViewSettingsTab(ViewSettingsTab::CompactMode, tabWidget);
    tabWidget->addTab(compactTab, QIcon::fromTheme(QStringLiteral("view-list-details")), i18nc("@title:tab", "Compact"));
    connect(compactTab, &ViewSettingsTab::changed, this, &DolphinViewModesConfigModule::viewModeChanged);

    // Initialize 'Details' tab
    ViewSettingsTab* detailsTab = new ViewSettingsTab(ViewSettingsTab::DetailsMode, tabWidget);
    tabWidget->addTab(detailsTab, QIcon::fromTheme(QStringLiteral("view-list-tree")), i18nc("@title:tab", "Details"));
    connect(detailsTab, &ViewSettingsTab::changed, this, &DolphinViewModesConfigModule::viewModeChanged);

    m_tabs.append(iconsTab);
    m_tabs.append(compactTab);
    m_tabs.append(detailsTab);

    topLayout->addWidget(tabWidget, 0, {});
}

DolphinViewModesConfigModule::~DolphinViewModesConfigModule()
{
}

void DolphinViewModesConfigModule::save()
{
    for (ViewSettingsTab *tab : qAsConst(m_tabs)) {
        tab->applySettings();
    }
    reparseConfiguration();
}

void DolphinViewModesConfigModule::defaults()
{
    for (ViewSettingsTab *tab : qAsConst(m_tabs)) {
        tab->restoreDefaultSettings();
    }
    reparseConfiguration();
}

void DolphinViewModesConfigModule::reparseConfiguration()
{
    QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/KonqMain"),
                                                      QStringLiteral("org.kde.Konqueror.Main"),
                                                      QStringLiteral("reparseConfiguration"));
    QDBusConnection::sessionBus().send(message);
}

void DolphinViewModesConfigModule::viewModeChanged()
{
    markAsChanged();
}

#include "kcmdolphinviewmodes.moc"
