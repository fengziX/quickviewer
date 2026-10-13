#ifndef FULLSCREENCLOSEBUTTON_H
#define FULLSCREENCLOSEBUTTON_H

#include <QtWidgets>

/**
 * @brief The QVCaptionRole enum
 * The three caption buttons of a window, in the order of Windows 11.
 */
enum QVCaptionRole {
    QVCaptionMinimize,
    QVCaptionMaximize,
    QVCaptionRestore,
    QVCaptionClose
};

/**
 * @brief The QVCaptionButton class
 *
 * A caption button drawn in the style of Windows 11: flat and transparent in the
 * normal state, a light grey background while the mouse cursor is over it and a
 * red one for the close button. The glyph is drawn with a thin line instead of
 * the icon font of the system, so that it looks the same on every system.
 *
 * The hover state can be forced from the outside (setForcedHot) so that the
 * highlight is also shown while the mouse cursor is a few pixels away from the
 * button, at the very corner of the screen.
 */
class QVCaptionButton : public QToolButton
{
public:
    QVCaptionButton(QVCaptionRole role, QWidget* parent = nullptr)
        : QToolButton(parent)
        , m_role(role)
        , m_forcedHot(false)
    {
        setToolButtonStyle(Qt::ToolButtonIconOnly);
        setFocusPolicy(Qt::NoFocus);
        setCursor(Qt::ArrowCursor);
        setMouseTracking(true);
    }

    void setRole(QVCaptionRole role)
    {
        if(m_role == role)
            return;
        m_role = role;
        update();
    }

    void setForcedHot(bool hot)
    {
        if(m_forcedHot == hot)
            return;
        m_forcedHot = hot;
        update();
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
        case QEvent::MouseMove:
            update();
            break;
        default:
            break;
        }
        return QToolButton::event(event);
    }

    void paintEvent(QPaintEvent*) override
    {
        const bool lit = m_forcedHot || underMouse() || isDown();

        QColor background(Qt::transparent);
        if(m_role == QVCaptionClose) {
            if(isDown())
                background = QColor(0xD1, 0x4A, 0x3D);
            else if(lit)
                background = QColor(0xC4, 0x2B, 0x1C);
        } else {
            if(isDown())
                background = QColor(0xD6, 0xD6, 0xD6);
            else if(lit)
                background = QColor(0xE5, 0xE5, 0xE5);
        }

        QPainter painter(this);
        if(background.alpha() > 0)
            painter.fillRect(rect(), background);

        const QColor color = (m_role == QVCaptionClose && lit) ? QColor(Qt::white)
                                                               : QColor(0x1C, 0x1C, 0x1C);
        paintGlyph(painter, color);
    }

private:
    void paintGlyph(QPainter& painter, const QColor& color)
    {
        const qreal s = qMax(qreal(0.75), height() / 32.0);
        const qreal cx = width() / 2.0;
        const qreal cy = height() / 2.0;

        QPen pen(color);
        pen.setWidthF(qMax(qreal(1.0), s));
        pen.setCapStyle(Qt::FlatCap);
        pen.setJoinStyle(Qt::MiterJoin);

        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        switch(m_role) {
        case QVCaptionMinimize:
            painter.drawLine(QPointF(cx - 5 * s, cy), QPointF(cx + 5 * s, cy));
            break;
        case QVCaptionMaximize:
            painter.drawRect(QRectF(cx - 5 * s, cy - 5 * s, 10 * s, 10 * s));
            break;
        case QVCaptionRestore: {
            // the square in the back (upper right), only the visible part of it
            QPainterPath path;
            path.moveTo(cx + 4.5 * s, cy - 1.5 * s);
            path.lineTo(cx + 6.5 * s, cy - 1.5 * s);
            path.lineTo(cx + 6.5 * s, cy - 6.5 * s);
            path.lineTo(cx - 1.5 * s, cy - 6.5 * s);
            path.lineTo(cx - 1.5 * s, cy - 1.5 * s);
            painter.drawPath(path);
            // the square in the front (lower left)
            painter.drawRect(QRectF(cx - 3.5 * s, cy - 1.5 * s, 8 * s, 8 * s));
            break;
        }
        case QVCaptionClose:
            painter.drawLine(QPointF(cx - 5 * s, cy - 5 * s), QPointF(cx + 5 * s, cy + 5 * s));
            painter.drawLine(QPointF(cx + 5 * s, cy - 5 * s), QPointF(cx - 5 * s, cy + 5 * s));
            break;
        default:
            break;
        }
    }

    QVCaptionRole m_role;
    bool m_forcedHot;
};

/**
 * @brief The QVMenuBarSpacer class
 *
 * The corner widget of the menu bar which only reserves the space used by the
 * buttons in the top right corner, so that the menu items do not run under them.
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
 * @brief The QVCaptionButtonHost class
 *
 * Replaces the fullscreen button of QuickViewer by the three caption buttons of
 * Windows 11 - minimize, maximize/restore and close - in the top right corner of
 * the toolbar which is displayed on top of the image when the mouse cursor
 * reaches the top edge of the window.
 *
 * The buttons are only shown while the window is displayed in full screen. In a
 * normal or maximized window the title bar of Windows already provides them, so
 * the fullscreen button of QuickViewer is kept there instead.
 *
 * They are children of the window (not of the menu bar), so that they always
 * cover the very corner of the screen, whatever the size of the menu bar is.
 * Their hover state is derived from the position of the cursor which is polled,
 * so that the highlight does not disappear in the last pixels of the screen.
 */
class QVCaptionButtonHost : public QObject
{
public:
    QVCaptionButtonHost(QMainWindow* mainWindow, QMenuBar* menuBar)
        : QObject(menuBar)
        , m_mainWindow(mainWindow)
        , m_menuBar(menuBar)
        , m_fullscreenButton(nullptr)
        , m_minimizeButton(nullptr)
        , m_maximizeButton(nullptr)
        , m_closeButton(nullptr)
        , m_spacer(nullptr)
        , m_barHeight(30)
        , m_captionWidth(49)
        , m_buttonsVisible(false)
        , m_cornerPress(false)
        , m_restoreFullScreen(false)
    {
        m_timer.setInterval(40);
        connect(&m_timer, &QTimer::timeout, this, [this]() { updateState(); });
    }

    void install()
    {
        m_fullscreenButton = m_menuBar->cornerWidget();

        m_barHeight = qMax(30, m_menuBar->fontMetrics().height() + 14);
        m_captionWidth = qBound(40, qRound((m_barHeight + 4) * 1.44), 72);

        m_spacer = new QVMenuBarSpacer(m_menuBar);
        m_spacer->setSpacerSize(fullscreenWidth(), m_barHeight);
        m_menuBar->setCornerWidget(m_spacer);

        m_minimizeButton = new QVCaptionButton(QVCaptionMinimize, m_mainWindow);
        m_maximizeButton = new QVCaptionButton(QVCaptionMaximize, m_mainWindow);
        m_closeButton = new QVCaptionButton(QVCaptionClose, m_mainWindow);

        // the labels are embedded as UTF-8 escape sequences because the source of
        // this file is kept ASCII only
        m_minimizeButton->setToolTip(QString::fromUtf8("\xe6\x9c\x80\xe5\xb0\x8f\xe5\x8c\x96(&N)"));
        m_maximizeButton->setToolTip(QString::fromUtf8("\xe6\x9c\x80\xe5\xa4\xa7\xe5\x8c\x96(&X)"));
        m_closeButton->setToolTip(QString::fromUtf8("\xe5\x85\xb3\xe9\x97\xad(&C)"));
        m_minimizeButton->setAccessibleName(QString::fromUtf8("\xe6\x9c\x80\xe5\xb0\x8f\xe5\x8c\x96"));
        m_maximizeButton->setAccessibleName(QString::fromUtf8("\xe6\x9c\x80\xe5\xa4\xa7\xe5\x8c\x96"));
        m_closeButton->setAccessibleName(QString::fromUtf8("\xe5\x85\xb3\xe9\x97\xad"));

        connect(m_minimizeButton, &QToolButton::clicked, this, &QVCaptionButtonHost::onMinimize);
        connect(m_maximizeButton, &QToolButton::clicked, this, &QVCaptionButtonHost::onMaximize);
        connect(m_closeButton, &QToolButton::clicked, m_mainWindow, [this]() {
            m_mainWindow->close();
        });

        setButtonsVisible(false);

        m_menuBar->installEventFilter(this);
        m_mainWindow->installEventFilter(this);
        qApp->installEventFilter(this);

        updateState();
        m_timer.start();
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if(watched == m_menuBar) {
            switch(event->type()) {
            case QEvent::Resize:
            case QEvent::Show:
            case QEvent::Hide:
            case QEvent::LayoutRequest:
            case QEvent::ParentChange:
            case QEvent::StyleChange:
                updateState();
                break;
            default:
                break;
            }
            return QObject::eventFilter(watched, event);
        }

        if(watched == m_mainWindow && event->type() == QEvent::Resize)
            updateState();

        switch(event->type()) {
        case QEvent::WindowStateChange:
            if(watched == m_mainWindow) {
                updateState();
                // Keep the full screen display when the window was minimized and is
                // restored again, like other full screen applications do.
                if(m_restoreFullScreen && !(m_mainWindow->windowState() & Qt::WindowMinimized)) {
                    m_restoreFullScreen = false;
                    if(!m_mainWindow->isFullScreen())
                        m_mainWindow->showFullScreen();
                }
            }
            break;
        case QEvent::MouseButtonPress: {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            m_cornerPress = mouseEvent->button() == Qt::LeftButton && inCloseArea(mouseEvent->globalPosition().toPoint());
            break;
        }
        case QEvent::MouseButtonRelease: {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            const bool armed = m_cornerPress;
            m_cornerPress = false;
            // Safety net: handle a click in the corner of the screen even when the
            // event was delivered to another widget.
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
    int fullscreenWidth() const
    {
        return m_fullscreenButton ? qMax(24, m_fullscreenButton->sizeHint().width()) : 28;
    }

    void onMinimize()
    {
        m_restoreFullScreen = m_mainWindow->isFullScreen();
        m_mainWindow->showMinimized();
    }

    void onMaximize()
    {
        // The middle caption button switches the full screen display, which is the
        // behaviour of the button which was replaced by it. The action of the
        // application is used so that the window state is restored correctly.
        if(QAction* actionFullscreen = m_mainWindow->findChild<QAction*>(QStringLiteral("actionFullscreen")))
            actionFullscreen->trigger();
        updateState();
    }

    void setButtonsVisible(bool visible)
    {
        m_buttonsVisible = visible;
        m_minimizeButton->setVisible(visible);
        m_maximizeButton->setVisible(visible);
        m_closeButton->setVisible(visible);
        if(m_fullscreenButton)
            m_fullscreenButton->setVisible(!visible);
        m_spacer->setSpacerSize(visible ? m_captionWidth * 3 : fullscreenWidth(), m_barHeight);
        // make the menu bar recalculate the geometry of its corner widget
        m_menuBar->setCornerWidget(m_spacer);
        if(!visible) {
            m_minimizeButton->setForcedHot(false);
            m_maximizeButton->setForcedHot(false);
            m_closeButton->setForcedHot(false);
        }
        raiseButtons();
        reposition();
    }

    void updateState()
    {
        // The caption buttons of the window are only used for the toolbar which is
        // shown on top of the image in full screen display.
        const bool wanted = m_mainWindow->isFullScreen() && m_menuBar->parentWidget() != m_mainWindow;

        if(wanted != m_buttonsVisible)
            setButtonsVisible(wanted);

        if(m_maximizeButton) {
            const bool fullscreen = m_mainWindow->isFullScreen();
            m_maximizeButton->setRole(fullscreen ? QVCaptionRestore : QVCaptionMaximize);
            const QString tip = fullscreen
                ? QString::fromUtf8("\xe5\x90\x91\xe4\xb8\x8b\xe8\xbf\x98\xe5\x8e\x9f(&R)")
                : QString::fromUtf8("\xe6\x9c\x80\xe5\xa4\xa7\xe5\x8c\x96(&X)");
            if(m_maximizeButton->toolTip() != tip)
                m_maximizeButton->setToolTip(tip);
        }

        if(m_buttonsVisible) {
            updateHot();
        }
        reposition();
    }

    // The area of the close button, a little larger than the button itself, so
    // that the last pixels of the screen still count as the button.
    QRect closeArea() const
    {
        if(!m_buttonsVisible || !m_closeButton || m_closeButton->width() <= 0)
            return QRect();
        const QRect button(m_closeButton->mapToGlobal(QPoint(0, 0)), m_closeButton->size());
        return button.adjusted(-4, -2, 4, 2).intersected(QRect(m_mainWindow->mapToGlobal(QPoint(0, 0)),
                                                              m_mainWindow->size()));
    }

    bool inCloseArea(const QPoint& globalPos) const
    {
        const QRect area = closeArea();
        return area.isValid() && area.contains(globalPos);
    }

    void updateHot()
    {
        const QRect area = closeArea();
        const bool hot = area.isValid() && area.contains(QCursor::pos());
        m_closeButton->setForcedHot(hot);
    }

    void raiseButtons()
    {
        if(!m_buttonsVisible)
            return;
        QWidget* buttons[] = { m_minimizeButton, m_maximizeButton, m_closeButton };
        for(QWidget* button : buttons) {
            if(button)
                button->raise();
        }
    }

    void place(QWidget* widget, const QRect& rect)
    {
        if(widget && widget->geometry() != rect)
            widget->setGeometry(rect);
    }

    void reposition()
    {
        if(!m_mainWindow)
            return;

        int barHeight = m_barHeight;
        if(m_menuBar->height() > 0)
            barHeight = m_menuBar->height();
        if(barHeight <= 0)
            return;

        if(m_buttonsVisible) {
            const int width = m_mainWindow->width();
            if(width <= 0)
                return;
            int right = width;
            QVCaptionButton* buttons[] = { m_closeButton, m_maximizeButton, m_minimizeButton };
            for(QVCaptionButton* button : buttons) {
                if(!button)
                    continue;
                place(button, QRect(right - m_captionWidth, 0, m_captionWidth, barHeight));
                right -= m_captionWidth;
            }
        } else if(m_fullscreenButton) {
            // the fullscreen button of the application stays in the corner of the menu bar
            const int width = m_menuBar->width();
            if(width <= 0)
                return;
            place(m_fullscreenButton, QRect(width - fullscreenWidth(), 0, fullscreenWidth(), barHeight));
        }
    }

    QMainWindow* m_mainWindow;
    QMenuBar* m_menuBar;
    QWidget* m_fullscreenButton;
    QVCaptionButton* m_minimizeButton;
    QVCaptionButton* m_maximizeButton;
    QVCaptionButton* m_closeButton;
    QVMenuBarSpacer* m_spacer;
    QTimer m_timer;
    int m_barHeight;
    int m_captionWidth;
    bool m_buttonsVisible;
    bool m_cornerPress;
    bool m_restoreFullScreen;
};

/**
 * @brief installCaptionButtons
 *
 * Replaces the fullscreen button of the menu bar by the three caption buttons of
 * Windows 11 (minimize, maximize/restore, close) which are shown in the top right
 * corner while the window is in full screen display.
 */
inline void installCaptionButtons(QMainWindow* mainWindow)
{
    if(!mainWindow)
        return;
    QMenuBar* menuBar = mainWindow->menuBar();
    if(!menuBar || menuBar->property("qvCaptionButtonsInstalled").toBool())
        return;
    menuBar->setProperty("qvCaptionButtonsInstalled", true);
    (new QVCaptionButtonHost(mainWindow, menuBar))->install();
}

#endif // FULLSCREENCLOSEBUTTON_H
