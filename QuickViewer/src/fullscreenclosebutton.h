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
        pen.setWidthF(qMax(qreal(1.6), height() / 9.0));
        pen.setCapStyle(Qt::FlatCap);

        const qreal margin = qMax(qreal(5), height() * 0.26);
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
 * occupy the very top right corner of the window without any margin of the
 * style, exactly like the caption buttons of Windows: pushing the mouse against
 * the corner always hits the close button.
 *
 * Because the buttons belong to the menu bar they show up in the toolbar which
 * is displayed on top of the image when the mouse cursor reaches the top edge of
 * the window - in full screen display as well as in a maximized window.
 *
 * A press/release pair inside the area of the close button closes the
 * application even if the event was delivered to another widget, so that the
 * corner of the window always reacts.
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
        , m_buttonHeight(30)
        , m_fullscreenWidth(28)
        , m_closeWidth(56)
        , m_cornerPress(false)
    {
    }

    void install()
    {
        // The fullscreen button which already sits in the top right corner.
        m_fullscreenButton = m_menuBar->cornerWidget();
        if(m_fullscreenButton)
            m_fullscreenWidth = qMax(24, m_fullscreenButton->sizeHint().width());

        // Keep the caption buttons comfortably large so that they are easy to hit.
        m_buttonHeight = qMax(30, m_fullscreenButton ? m_fullscreenButton->sizeHint().height() : 30);
        m_closeWidth = qMax(52, qRound(m_buttonHeight * 1.9));

        m_spacer = new QVMenuBarSpacer(m_menuBar);
        m_spacer->setSpacerSize(m_fullscreenWidth + m_closeWidth, m_buttonHeight);
        m_menuBar->setCornerWidget(m_spacer);

        if(m_fullscreenButton)
            m_fullscreenButton->setParent(m_menuBar);

        m_closeButton = new QVCloseButton(m_menuBar);
        // Only the width is fixed: the height is always the height of the menu bar,
        // so that the button covers the whole corner (see reposition()).
        m_closeButton->setFixedWidth(m_closeWidth);
        m_closeButton->setMinimumHeight(m_buttonHeight);
        // "close(&C)" - the source of this file is kept ASCII only, so the label is
        // embedded as UTF-8 escape sequences instead of using the translation files.
        m_closeButton->setToolTip(QString::fromUtf8("\xe5\x85\xb3\xe9\x97\xad(&C)"));
        m_closeButton->setAccessibleName(QString::fromUtf8("\xe5\x85\xb3\xe9\x97\xad"));
        connect(m_closeButton, &QToolButton::clicked, m_mainWindow, [this]() {
            m_mainWindow->close();
        });

        m_menuBar->installEventFilter(this);
        qApp->installEventFilter(this);

        reposition();
        raiseButtons();
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
                raiseButtons();
                break;
            default:
                break;
            }
            return QObject::eventFilter(watched, event);
        }

        // Safety net: the buttons live in the menu bar, but if anything swallows a
        // click inside the corner area of the window, it is still handled here.
        switch(event->type()) {
        case QEvent::MouseButtonPress: {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            m_cornerPress = mouseEvent->button() == Qt::LeftButton && inCloseArea(mouseEvent->globalPosition().toPoint());
            break;
        }
        case QEvent::MouseButtonRelease: {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            const bool armed = m_cornerPress;
            m_cornerPress = false;
            if(armed && mouseEvent->button() == Qt::LeftButton && inCloseArea(mouseEvent->globalPosition().toPoint()))
                m_mainWindow->close();
            break;
        }
        default:
            break;
        }
        return QObject::eventFilter(watched, event);
    }

private:
    // The close area in global coordinates, a little larger than the button itself.
    QRect closeArea() const
    {
        if(!m_closeButton || !m_closeButton->isVisible() || m_closeButton->width() <= 0)
            return QRect();
        const QPoint topLeft = m_closeButton->mapToGlobal(QPoint(0, 0));
        return QRect(topLeft, m_closeButton->size()).adjusted(-2, 0, 0, 0);
    }

    bool inCloseArea(const QPoint& globalPos) const
    {
        const QRect area = closeArea();
        return area.isValid() && area.contains(globalPos);
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
        if(m_closeButton) {
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
    bool m_cornerPress;
};

/**
 * @brief installFullscreenCloseButton
 *
 * Adds a close button on the right side of the fullscreen button which is placed
 * in the top right corner of the menu bar. Both buttons stick to the top right
 * corner of the window without any margin.
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
