/*
 * SPDX-FileCopyrightText: 2011 Janardhan Reddy <annapareddyjanardhanreddy@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kfileitemmodelfilter.h"

#include <QRegularExpression>

#include <KFileItem>

KFileItemModelFilter::KFileItemModelFilter() :
    m_useRegExp(false),
    m_regExp(nullptr),
    m_lowerCasePattern(),
    m_pattern()
{
}

KFileItemModelFilter::~KFileItemModelFilter()
{
    delete m_regExp;
    m_regExp = nullptr;
}

void KFileItemModelFilter::setPattern(const QString& filter)
{
    m_pattern = filter;
    m_lowerCasePattern = filter.toLower();

    if (filter.contains('*') || filter.contains('?') || filter.contains('[')) {
        if (!m_regExp) {
            m_regExp = new QRegularExpression();
            m_regExp->setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        }
        m_regExp->setPattern(QRegularExpression::wildcardToRegularExpression(filter));
        m_useRegExp = m_regExp->isValid();
    } else {
        m_useRegExp = false;
    }
}

QString KFileItemModelFilter::pattern() const
{
    return m_pattern;
}

void KFileItemModelFilter::setMimeTypes(const QStringList& types)
{
    m_mimeTypes = types;
}

QStringList KFileItemModelFilter::mimeTypes() const
{
    return m_mimeTypes;
}

bool KFileItemModelFilter::hasSetFilters() const
{
    return (!m_pattern.isEmpty() || !m_mimeTypes.isEmpty());
}


bool KFileItemModelFilter::matches(const KFileItem& item) const
{
    const bool hasPatternFilter = !m_pattern.isEmpty();
    const bool hasMimeTypesFilter = !m_mimeTypes.isEmpty();

    // If no filter is set, return true.
    if (!hasPatternFilter && !hasMimeTypesFilter) {
        return true;
    }

    // If both filters are set, return true when both filters are matched
    if (hasPatternFilter && hasMimeTypesFilter) {
        return (matchesPattern(item) && matchesType(item));
    }

    // If only one filter is set, return true when that filter is matched
    if (hasPatternFilter) {
        return matchesPattern(item);
    }

    return matchesType(item);
}

bool KFileItemModelFilter::matchesPattern(const KFileItem& item) const
{
    if (m_useRegExp) {
        return m_regExp->match(item.text()).hasMatch();
    } else {
        return item.text().toLower().contains(m_lowerCasePattern);
    }
}

bool KFileItemModelFilter::matchesType(const KFileItem& item) const
{
    for (const QString& mimeType : qAsConst(m_mimeTypes)) {
        if (item.mimetype() == mimeType) {
            return true;
        }
    }

    return m_mimeTypes.isEmpty();
}
