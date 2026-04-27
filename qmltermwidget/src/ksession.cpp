/*
    This file is part of Konsole QML plugin,
    which is a terminal emulator from KDE.

    Copyright 2013      by Dmitry Zagnoyko <hiroshidi@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

// Own
#include "ksession.h"

// Qt
#include <QDir>
#include <QKeyEvent>

// Konsole
#include "KeyboardTranslator.h"
#include "HistorySearch.h"

KSession::KSession(QObject *parent) :
    QObject(parent), m_session(createSession(""))
{
    connect(m_session, SIGNAL(started()), this, SIGNAL(started()));
    connect(m_session, SIGNAL(finished()), this, SLOT(sessionFinished()));
    connect(m_session, SIGNAL(titleChanged()), this, SIGNAL(titleChanged()));
}

KSession::~KSession()
{
    if (m_session) {
        m_session->close();
        m_session->disconnect();
        delete m_session;
    }
}

void KSession::setTitle(QString name)
{
    m_session->setTitle(Session::NameRole, name);
}


Session *KSession::createSession(QString name)
{
    Session *session = new Session();

    session->setTitle(Session::NameRole, name);

    /* Thats a freaking bad idea!!!!
     * /bin/bash is not there on every system
     * better set it to the current $SHELL
     * Maybe you can also make a list available and then let the widget-owner decide what to use.
     * By setting it to $SHELL right away we actually make the first filecheck obsolete.
     * But as iam not sure if you want to do anything else ill just let both checks in and set this to $SHELL anyway.
     */

    //cool-old-term: There is another check in the code. Not sure if useful.

    QString envshell = getenv("SHELL");
    QString shellProg = !envshell.isEmpty() ? envshell : "/bin/bash";
    session->setProgram(shellProg);

    setenv("TERM", "xterm-256color", 1);

    //session->setProgram();

    QStringList args;
    // 使用 -i 参数启动 bash，确保是交互模式
    // 但避免使用 --login，因为它会读取 .bashrc 可能导致阻塞
    if (shellProg.contains("bash")) {
        args << "-i";
    }
    session->setArguments(args);
    session->setAutoClose(true);
    
    // 设置必要的环境变量以确保 shell 正常工作
    QStringList env = session->environment();
    // 设置 HOME 环境变量
    env << "HOME=" + QString::fromLocal8Bit(getenv("HOME"));
    // 设置 USER 环境变量
    env << "USER=" + QString::fromLocal8Bit(getenv("USER"));
    // 设置 PATH 环境变量
    env << "PATH=" + QString::fromLocal8Bit(getenv("PATH"));
    // 设置 TERM 环境变量已经在上面设置了
    // 设置 ENV 和 BASH_ENV 为空，避免额外的初始化文件
    env << "ENV=";
    env << "BASH_ENV=";
    session->setEnvironment(env);

    // Qt6 中 QTextCodec 已被移除，使用默认编码
    // session->setCodec(QTextCodec::codecForName("UTF-8"));

    session->setFlowControlEnabled(true);
    session->setHistoryType(HistoryTypeBuffer(1000));

    session->setDarkBackground(true);

    session->setKeyBindings("");

    return session;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////


int  KSession::getRandomSeed()
{
    return m_session->sessionId() * 31;
}

void  KSession::addView(TerminalDisplay *display)
{
    m_session->addView(display);
}

void KSession::removeView(TerminalDisplay *display)
{
    m_session->removeView(display);
}

void KSession::sessionFinished()
{
    emit finished();
}

void KSession::selectionChanged(bool textSelected)
{
    Q_UNUSED(textSelected)
}

void KSession::startShellProgram()
{
    if ( m_session->isRunning() ) {
        return;
    }

    m_session->run();
}

bool KSession::sendSignal(int signal)
{
    if ( !m_session->isRunning() ) {
        return false;
    }

    return m_session->sendSignal(signal);
}

int KSession::getShellPID()
{
    return m_session->processId();
}

void KSession::changeDir(const QString &dir)
{
    /*
       this is a very hackish way of trying to determine if the shell is in
       the foreground before attempting to change the directory.  It may not
       be portable to anything other than Linux.
    */
    QString strCmd;
    strCmd.setNum(getShellPID());
    strCmd.prepend("ps -j ");
    strCmd.append(" | tail -1 | awk '{ print $5 }' | grep -q \\+");
    int retval = system(strCmd.toStdString().c_str());

    if (!retval) {
        QString cmd = "cd " + dir + "\n";
        sendText(cmd);
    }
}

void KSession::setEnvironment(const QStringList &environment)
{
    m_session->setEnvironment(environment);
}


void KSession::setShellProgram(const QString &progname)
{
    m_session->setProgram(progname);
}

void KSession::setInitialWorkingDirectory(const QString &dir)
{
    if ( _initialWorkingDirectory != dir ) {
        _initialWorkingDirectory = dir;
        m_session->setInitialWorkingDirectory(dir);
        emit initialWorkingDirectoryChanged();
}   }

QString KSession::getInitialWorkingDirectory()
{
    return _initialWorkingDirectory;
}

void KSession::setArgs(const QStringList &args)
{
    m_session->setArguments(args);
}

// Qt6 中 QTextCodec 已被移除
// void KSession::setTextCodec(QTextCodec *codec)
// {
//     m_session->setCodec(codec);
// }

void KSession::setHistorySize(int lines)
{
    if ( historySize() != lines ) {
        if (lines < 0)
            m_session->setHistoryType(HistoryTypeFile());
        else
            m_session->setHistoryType(HistoryTypeBuffer(lines));
        emit historySizeChanged();
    }
}

int KSession::historySize() const
{
    if ( m_session->historyType().isUnlimited() ) {
        return -1;
    } else {
        return m_session->historyType().maximumLineCount();
    }
}

QString KSession::getHistory() const
{
    QString history;
    QTextStream historyStream(&history);
    PlainTextDecoder historyDecoder;

    historyDecoder.begin(&historyStream);
    m_session->emulation()->writeToStream(&historyDecoder);
    historyDecoder.end();

    return history;
}

void KSession::sendText(QString text)
{
    // 将不可打印字符转换为可读形式
    QString debugText;
    for (int i = 0; i < text.length(); i++) {
        QChar c = text.at(i);
        if (c.unicode() < 32) {
            debugText += QString("\\x%1").arg(c.unicode(), 2, 16, QChar('0'));
        } else {
            debugText += c;
        }
    }
    qDebug() << "KSession::sendText: text='" << debugText << "' length=" << text.length() << " raw='" << text << "'";
    m_session->sendText(text);
}

void KSession::sendKey(int rep, int key, int mod) const
{
    Q_UNUSED(rep);
    
    qDebug() << "KSession::sendKey: key=" << key << " mod=" << mod;
    
    // Forward key events from QML to the backend emulation
    Qt::KeyboardModifiers kbm = static_cast<Qt::KeyboardModifiers>(mod);
    
    // 在Qt6中，QKeyEvent构造函数需要文本参数
    // 对于功能键（如方向键、回车键等），文本为空
    // 注意：字符键的文本应该通过sendText发送，而不是通过sendKey
    QString text; // 空文本，因为字符键应该使用sendText
    
    // 创建QKeyEvent
    // Qt6中QKeyEvent的构造函数：QKeyEvent(Type type, int key, Qt::KeyboardModifiers modifiers, const QString &text = QString())
    QKeyEvent qkey(QEvent::KeyPress, key, kbm, text);

    if (m_session && m_session->emulation()) {
        m_session->emulation()->sendKeyEvent(&qkey);
    } else {
        qDebug() << "KSession::sendKey: m_session or emulation is null!";
    }
}

void KSession::clearScreen()
{
    m_session->emulation()->clearEntireScreen();
}

void KSession::search(const QString &regexp, int startLine, int startColumn, bool forwards)
{
    // Qt6 中 QRegExp 已被 QRegularExpression 替代
    // 这里需要检查 HistorySearch 是否支持 QRegularExpression
    // 暂时注释掉，因为 HistorySearch 可能也需要更新
    /*
    HistorySearch *history = new HistorySearch( QPointer<Emulation>(m_session->emulation()), QRegExp(regexp), forwards, startColumn, startLine, this);
    connect( history, SIGNAL(matchFound(int,int,int,int)), this, SIGNAL(matchFound(int,int,int,int)));
    connect( history, SIGNAL(noMatchFound()), this, SIGNAL(noMatchFound()));
    history->search();
    */
}

void KSession::setFlowControlEnabled(bool enabled)
{
    m_session->setFlowControlEnabled(enabled);
}

bool KSession::flowControlEnabled()
{
    return m_session->flowControlEnabled();
}

void KSession::setKeyBindings(const QString &kb)
{
    m_session->setKeyBindings(kb);
    emit changedKeyBindings(kb);
}

QString KSession::getKeyBindings()
{
   return m_session->keyBindings();
}

QStringList KSession::availableKeyBindings()
{
    return KeyboardTranslatorManager::instance()->allTranslators();
}

QString KSession::keyBindings()
{
    return m_session->keyBindings();
}

QString KSession::getTitle()
{
    // 首先尝试使用 shell 设置的标题（通过 OSC 转义序列）
    QString userTitle = m_session->userTitle();
    if (!userTitle.isEmpty()) {
        return userTitle;
    }

    // 然后尝试使用 currentDir
    QString currentDir = m_session->currentDir();
    if (!currentDir.isEmpty()) {
        if (currentDir == QDir::homePath()) {
            return currentDir;
        }
        if (currentDir == "/")
            return currentDir;
        return QDir(currentDir).dirName();
    }

    // 如果 currentDir 为空，使用 nameTitle 作为后备
    QString nameTitle = m_session->title(Konsole::Session::NameRole);
    if (!nameTitle.isEmpty()) {
        return nameTitle;
    }

    // 最后的后备
    return QStringLiteral("Terminal");
}

bool KSession::hasActiveProcess() const
{
    return m_session->processId() != m_session->foregroundProcessId();
}

QString KSession::foregroundProcessName()
{
    return m_session->foregroundProcessName();
}

QString KSession::currentDir() 
{
    return m_session->currentDir();
}
