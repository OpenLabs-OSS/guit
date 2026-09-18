#pragma once

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace Guit
{

// Small non-modal completion notice shown over the main window corner.
// Replaces itself on rapid operations; never steals focus.
class Toast : public QWidget
{
    Q_OBJECT

public:
    explicit Toast(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_label(new QLabel(this))
        , m_timer(new QTimer(this))
    {
        setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_ShowWithoutActivating);
        m_label->setWordWrap(true);
        m_label->setMargin(10);
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_label);
        m_timer->setSingleShot(true);
        m_timer->setInterval(4000);
        connect(m_timer, &QTimer::timeout, this, &QWidget::hide);
        hide();
    }

    void showMessage(const QString &message)
    {
        m_label->setText(message);
        adjustSize();
        if (parentWidget() != nullptr) {
            const QRect geo = parentWidget()->geometry();
            move(geo.right() - width() - 24, geo.bottom() - height() - 48);
        }
        show();
        raise();
        m_timer->start();
    }

private:
    QLabel *m_label = nullptr;
    QTimer *m_timer = nullptr;
};

} // namespace Guit
