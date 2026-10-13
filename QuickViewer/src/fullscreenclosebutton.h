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
 */
class QVCaptionButton : public QToolButton
{
public:
    QVCaptionButton(QVCaptionRole role, QWidget* parent = nullptr)
        : QToolButton(parent)
        , m_role(role)
    {
        setToolButtonStyle(Qt::ToolButtonIconOnly);
        setFocusPolicy(Qt::NoFocus);
        setCursor(Qt::ArrowCursor);
    }

    void setRole(QVCaptionRole role)
    {
        if(m_role == role)
            return;
        m_role = role;
        update();
    }

    QVCaptionRole role() const { return m_role; }

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

    void paintEvent(QPaintEvent*) override
    {
        const bool lit = underMouse() || isDown();

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
};

/**
 * @brief The QVMenuBarSpacer class
 *
 * The corner widget of the menu bar which only reserves the space used by the
 * caption buttons.
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
 * Replaces the fullscreen button of QuickViewer (and the extra close button
 * which was drawn by hand before) by the three caption buttons of Windows 11:
 * minimize, maximize/restore and close.
 *
 * The buttons are placed by hand as children of the menu bar, so that they
 * occupy the very top right corner of the window without any margin of the
 * style, exactly like the caption buttons of Windows: pushing the mouse against
 * the corner always hits the close button.
 *
 * Because they belong to the menu bar they show up in the toolbar which is
 * displayed on top of the image when the mouse cursor reaches the top edge of the
 * window - in full screen display as well as in a maximized window.
 */
class QVCaptionButtonHost : public QObject
{
public:
    QVCaptionButtonHost(QMainWindow* mainWindow, QMenuBar* menuBar)
        : QObject(menuBar)
        , m_mainWindow(mainWindow)
        , m_menuBar(menuBar)
        , m_minimizeButton(nullptr)
        , m_maximizeButton(nullptr)
        , m_closeButton(nullptr)
        , m_spacer(nullptr)
        , m_barHeight(30)
        , m_captionWidth(49)
        , m_cornerPress(false)
        , m_restoreFullScreen(false)
    {
    }

    void install()
    {
        // The buttons of QuickViewer which were placed there are not used any more.
        if(QWidget* oldCornerWidget = m_menuBar->cornerWidget())
            oldCornerWidget->hide();

        m_barHeight = qMax(30, m_menuBar->fontMetrics().height() + 14);
        m_captionWidth = qBound(40, qRound((m_barHeight + 4) * 1.44), 72);

        m_spacer = new QVMenuBarSpacer(m_menuBar);
        m_spacer->setSpacerSize(m_captionWidth * 3, m_barHeight);
        m_menuBar->setCornerWidget(m_spacer);

        m_minimizeButton = new QVCaptionButton(QVCaptionMinimize, m_menuBar);
        m_maximizeButton = new QVCaptionButton(QVCaptionMaximize, m_menuBar);
        m_closeButton = new QVCaptionButton(QVCaptionClose, m_menuBar);

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

        m_menuBar->installEventFilter(this);
        qApp->installEventFilter(this);

        updateRoles();
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

        switch(event->type()) {
        case QEvent::WindowStateChange:
            if(watched == m_mainWindow) {
                updateRoles();
                reposition();
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
            // Safety net: handle a click in the corner of the window even when the
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
        updateRoles();
    }

    void updateRoles()
    {
        if(!m_maximizeButton)
            return;
        const bool fullscreen = m_mainWindow->isFullScreen();
        m_maximizeButton->setRole(fullscreen ? QVCaptionRestore : QVCaptionMaximize);
        m_maximizeButton->setToolTip(fullscreen
            ? QString::fromUtf8("\xe5\x90\x91\xe4\xb8\x8b\xe8\xbf\x98\xe5\x8e\x9f(&R)")
            : QString::fromUtf8("\xe6\x9c\x80\xe5\xa4\xa7\xe5\x8c\x96(&X)"));
    }

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
        QWidget* buttons[] = { m_minimizeButton, m_maximizeButton, m_closeButton };
        for(QWidget* button : buttons) {
            if(button)
                button->raise();
        }
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
        QVCaptionButton* buttons[] = { m_closeButton, m_maximizeButton, m_minimizeButton };
        for(QVCaptionButton* button : buttons) {
            if(!button)
                continue;
            button->setGeometry(right - m_captionWidth, 0, m_captionWidth, height);
            right -= m_captionWidth;
        }
    }

    QMainWindow* m_mainWindow;
    QMenuBar* m_menuBar;
    QVCaptionButton* m_minimizeButton;
    QVCaptionButton* m_maximizeButton;
    QVCaptionButton* m_closeButton;
    QVMenuBarSpacer* m_spacer;
    int m_barHeight;
    int m_captionWidth;
    bool m_cornerPress;
    bool m_restoreFullScreen;
};

/**
 * @brief installCaptionButtons
 *
 * Replaces the fullscreen button of the menu bar by the three caption buttons of
 * Windows 11 (minimize, maximize/restore, close) in the top right corner.
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
