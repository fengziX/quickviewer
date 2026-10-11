#ifndef FULLSCREENCLOSEBUTTON_H
#define FULLSCREENCLOSEBUTTON_H

#include <QtWidgets>

/**
 * @brief The QVCloseButton class
 *
 * A button which looks like the caption close button of Windows applications.
 * It is flat and transparent in the normal state, it turns red while the mouse
 * cursor is over it and it is filled up to its own edges.
 */
class QVCloseButton : public QToolButton
{
public:
    explicit QVCloseButton(QWidget* parent = nullptr) : QToolButton(parent)
    {
        setToolButtonStyle(Qt::ToolButtonIconOnly);
        setFocusPolicy(Qt::NoFocus);
        setCursor(Qt::ArrowCursor);
        setStyleSheet(QStringLiteral("QToolButton{border:none;background:transparent;}"
                                     "QToolButton:hover{background-color:#e81123;}"
                                     "QToolButton:pressed{background-color:#f1707a;}"));
    }

protected:
    bool event(QEvent* event) override
    {
        switch(event->type()) {
        case QEvent::Enter:
        case QEvent::Leave:
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::EnabledChange:
            update();
            break;
        default:
            break;
        }
        return QToolButton::event(event);
    }

    void paintEvent(QPaintEvent* event) override
    {
        QToolButton::paintEvent(event);

        const bool lit = underMouse() || isDown();
        QPen pen(lit ? QColor(255, 255, 255) : QColor(51, 51, 51));
        pen.setWidthF(qMax(qreal(1), height() / 12.0));
        pen.setCapStyle(Qt::FlatCap);

        const qreal margin = qMax(qreal(3), height() * 0.28);
        const QRectF box = QRectF(rect()).adjusted(margin, margin, -margin, -margin);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(pen);
        painter.drawLine(box.topLeft(), box.bottomRight());
        painter.drawLine(box.topRight(), box.bottomLeft());
    }
};

/**
 * @brief The QVMenuBarSpacer class
 *
 * The corner widget of the menu bar which only reserves the space used by the
 * fullscreen button and the close button.
 */
class QVMenuBarSpacer : public QWidget
{
public:
    explicit QVMenuBarSpacer(QWidget* parent = nullptr) : QWidget(parent)
    {
        setAutoFillBackground(false);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

    void setSpacerSize(int width, int height)
    {
        m_size = QSize(width, height);
        updateGeometry();
    }

    QSize sizeHint() const override { return m_size; }

private:
    QSize m_size;
};

/**
 * @brief The QVCloseButtonHost class
 *
 * Takes over the button which is placed in the top right corner of the menu bar
 * (the fullscreen button of QuickViewer) and puts a close button on the right
 * side of it.
 *
 * Both buttons are placed by hand as children of the menu bar, so that they
 * occupy the very top right corner without any margin of the style. The corner
 * widget of the menu bar is used as a spacer which reserves the space of the
 * buttons and keeps the menu items away from them.
 *
 * The close button is shown only while the window is displayed in full screen.
 */
class QVCloseButtonHost : public QObject
{
public:
    QVCloseButtonHost(QMainWindow* mainWindow, QMenuBar* menuBar)
        : QObject(menuBar)
        , m_mainWindow(mainWindow)
        , m_menuBar(menuBar)
        , m_fullscreenButton(nullptr)
        , m_closeButton(nullptr)
        , m_spacer(nullptr)
        , m_buttonHeight(24)
        , m_fullscreenWidth(24)
        , m_closeWidth(36)
        , m_closeButtonVisible(false)
    {
    }

    void install()
    {
        // The fullscreen button which already sits in the top right corner.
        m_fullscreenButton = m_menuBar->cornerWidget();
        if(m_fullscreenButton)
            m_fullscreenWidth = qMax(20, m_fullscreenButton->sizeHint().width());

        m_buttonHeight = qMax(20, m_fullscreenButton ? m_fullscreenButton->sizeHint().height() : 24);
        m_closeWidth = qRound(m_buttonHeight * 1.5);

        m_spacer = new QVMenuBarSpacer(m_menuBar);
        m_spacer->setSpacerSize(m_fullscreenWidth, m_buttonHeight);
        m_menuBar->setCornerWidget(m_spacer);

        if(m_fullscreenButton)
            m_fullscreenButton->setParent(m_menuBar);

        m_closeButton = new QVCloseButton(m_menuBar);
        m_closeButton->setFixedSize(m_closeWidth, m_buttonHeight);
        // "close(&C)" - the source of this file is kept ASCII only, so the label is
        // embedded as UTF-8 escape sequences instead of using the translation files.
        m_closeButton->setToolTip(QString::fromUtf8("\xe5\x85\xb3\xe9\x97\xad(&C)"));
        m_closeButton->setAccessibleName(QString::fromUtf8("\xe5\x85\xb3\xe9\x97\xad"));
        connect(m_closeButton, &QToolButton::clicked, m_mainWindow, [this]() {
            m_mainWindow->close();
        });

        m_menuBar->installEventFilter(this);

        if(QAction* actionFullscreen = m_mainWindow->findChild<QAction*>(QStringLiteral("actionFullscreen"))) {
            connect(actionFullscreen, &QAction::toggled, this, [this](bool checked) {
                setCloseButtonVisible(checked);
            });
        }

        setCloseButtonVisible(m_mainWindow->isFullScreen());
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if(watched == m_menuBar) {
            switch(event->type()) {
            case QEvent::Resize:
            case QEvent::Show:
            case QEvent::LayoutRequest:
            case QEvent::StyleChange:
                reposition();
                break;
            default:
                break;
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    void setCloseButtonVisible(bool visible)
    {
        m_closeButtonVisible = visible;
        if(m_closeButton)
            m_closeButton->setVisible(visible);
        if(m_spacer) {
            m_spacer->setSpacerSize(m_fullscreenWidth + (visible ? m_closeWidth : 0), m_buttonHeight);
            // Make the menu bar recalculate the geometry of its corner widget.
            m_menuBar->setCornerWidget(m_spacer);
        }
        raiseButtons();
        reposition();
    }

    void raiseButtons()
    {
        if(m_closeButton)
            m_closeButton->raise();
        if(m_fullscreenButton)
            m_fullscreenButton->raise();
    }

    void reposition()
    {
        if(!m_menuBar)
            return;
        const int width = m_menuBar->width();
        const int height = m_menuBar->height();
        if(width <= 0 || height <= 0)
            return;

        int right = width;
        if(m_closeButton && m_closeButtonVisible) {
            m_closeButton->setGeometry(right - m_closeWidth, 0, m_closeWidth, height);
            right -= m_closeWidth;
        }
        if(m_fullscreenButton)
            m_fullscreenButton->setGeometry(right - m_fullscreenWidth, 0, m_fullscreenWidth, height);
    }

    QMainWindow* m_mainWindow;
    QMenuBar* m_menuBar;
    QWidget* m_fullscreenButton;
    QVCloseButton* m_closeButton;
    QVMenuBarSpacer* m_spacer;
    int m_buttonHeight;
    int m_fullscreenWidth;
    int m_closeWidth;
    bool m_closeButtonVisible;
};

/**
 * @brief installFullscreenCloseButton
 *
 * Adds a close button on the right side of the fullscreen button which is placed
 * in the top right corner of the menu bar. The button is shown while the window
 * is displayed in full screen, and the two buttons stick to the top right corner.
 */
inline void installFullscreenCloseButton(QMainWindow* mainWindow)
{
    if(!mainWindow)
        return;
    QMenuBar* menuBar = mainWindow->menuBar();
    if(!menuBar || menuBar->property("qvCloseButtonInstalled").toBool())
        return;
    menuBar->setProperty("qvCloseButtonInstalled", true);
    (new QVCloseButtonHost(mainWindow, menuBar))->install();
}

#endif // FULLSCREENCLOSEBUTTON_H
