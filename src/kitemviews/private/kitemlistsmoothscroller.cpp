/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistsmoothscroller.h"

#include <QApplication>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QStyle>
#include <QWheelEvent>

KItemListSmoothScroller::KItemListSmoothScroller(QScrollBar* scrollBar,
                                                 QObject* parent) :
    QObject(parent),
    m_scrollBarPressed(false),
    m_smoothScrolling(true),
    m_scrollBar(scrollBar),
    m_animation(nullptr)
{
    m_animation = new QPropertyAnimation(this);
    const int animationDuration = m_scrollBar->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, m_scrollBar);
    const bool animationEnabled = (animationDuration > 0);
    m_animation->setDuration(animationEnabled ? animationDuration : 1);
    connect(m_animation, &QPropertyAnimation::stateChanged,
            this, &KItemListSmoothScroller::slotAnimationStateChanged);

    m_scrollBar->installEventFilter(this);
}

KItemListSmoothScroller::~KItemListSmoothScroller()
{
}

void KItemListSmoothScroller::setScrollBar(QScrollBar *scrollBar)
{
    m_scrollBar = scrollBar;
}

QScrollBar* KItemListSmoothScroller::scrollBar() const
{
    return m_scrollBar;
}

void KItemListSmoothScroller::setTargetObject(QObject* target)
{
    m_animation->setTargetObject(target);
}

QObject* KItemListSmoothScroller::targetObject() const
{
    return m_animation->targetObject();
}

void KItemListSmoothScroller::setPropertyName(const QByteArray& propertyName)
{
    m_animation->setPropertyName(propertyName);
}

QByteArray KItemListSmoothScroller::propertyName() const
{
    return m_animation->propertyName();
}

void KItemListSmoothScroller::scrollContentsBy(qreal distance)
{
    QObject* target = targetObject();
    if (!target) {
        return;
    }

    const QByteArray name = propertyName();
    const qreal currentOffset = target->property(name).toReal();
    if (static_cast<int>(currentOffset) == m_scrollBar->value()) {
        // The current offset is already synchronous to the scrollbar
        return;
    }

    const bool animRunning = (m_animation->state() == QAbstractAnimation::Running);
    if (animRunning) {
        // Stopping a running animation means skipping the range from the current offset
        // until the target offset. To prevent skipping of the range the difference
        // is added to the new target offset.
        const qreal oldEndOffset = m_animation->endValue().toReal();
        distance += (currentOffset - oldEndOffset);
    }

    const qreal endOffset = currentOffset - distance;
    if (m_smoothScrolling || animRunning) {
        qreal startOffset = currentOffset;
        if (animRunning) {
            // If the animation was running and has been interrupted by assigning a new end-offset
            // one frame must be added to the start-offset to keep the animation smooth. This also
            // assures that animation proceeds even in cases where new end-offset are triggered
            // within a very short timeslots.
            startOffset += (endOffset - currentOffset) * 1000 / (m_animation->duration() * 60);
            if (currentOffset < endOffset) {
                startOffset = qMin(startOffset, endOffset);
            } else {
                startOffset = qMax(startOffset, endOffset);
            }
        }

        m_animation->stop();
        m_animation->setStartValue(startOffset);
        m_animation->setEndValue(endOffset);
        m_animation->setEasingCurve(animRunning ? QEasingCurve::OutQuad : QEasingCurve::InOutQuad);
        m_animation->start();
        target->setProperty(name, startOffset);
    } else {
        target->setProperty(name, endOffset);
    }
}

void KItemListSmoothScroller::scrollTo(qreal position)
{
    int newValue = position;
    newValue = qBound(0, newValue, m_scrollBar->maximum());

    if (newValue != m_scrollBar->value()) {
        m_smoothScrolling = true;
        m_scrollBar->setValue(newValue);
    }
}

bool KItemListSmoothScroller::requestScrollBarUpdate(int newMaximum)
{
    if (m_animation->state() == QAbstractAnimation::Running) {
        if (newMaximum == m_scrollBar->maximum()) {
            // The value has been changed by the animation, no update
            // of the scrollbars is required as their target state will be
            // reached with the end of the animation.
            return false;
        }

        // The maximum has been changed which indicates that the content
        // of the view has been changed. Stop the animation in any case and
        // update the scrollbars immediately.
        m_animation->stop();
    }
    return true;
}

bool KItemListSmoothScroller::eventFilter(QObject* obj, QEvent* event)
{
    Q_ASSERT(obj == m_scrollBar);

    switch (event->type()) {
    case QEvent::MouseButtonPress:
        m_scrollBarPressed = true;
        m_smoothScrolling = true;
        break;

    case QEvent::MouseButtonRelease:
        m_scrollBarPressed = false;
        m_smoothScrolling = false;
        break;

    case QEvent::Wheel:
        return false; // we're the ones sending them

    default:
        break;
    }

    return QObject::eventFilter(obj, event);
}

void KItemListSmoothScroller::slotAnimationStateChanged(QAbstractAnimation::State newState,
                                                        QAbstractAnimation::State oldState)
{
    Q_UNUSED(oldState)
    if (newState == QAbstractAnimation::Stopped && m_smoothScrolling && !m_scrollBarPressed) {
        m_smoothScrolling = false;
    }
    if (newState == QAbstractAnimation::Stopped) {
        Q_EMIT scrollingStopped();
    }
}

void KItemListSmoothScroller::handleWheelEvent(QWheelEvent* event)
{
    const bool previous = m_smoothScrolling;

    m_smoothScrolling = true;

    QWheelEvent copy = *event;
    QApplication::sendEvent(m_scrollBar, &copy);
    event->setAccepted(copy.isAccepted());

    m_smoothScrolling = previous;
}

