/*
    Copyright 2007-2008 Robert Knight <robertknight@gmail.com>
    Copyright 1997,1998 by Lars Doelle <lars.doelle@on-line.de>
    Copyright 1996 by Matthias Ettrich <ettrich@kde.org>

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
#include "Emulation.h"

// System
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <memory>

// Qt
#include <QApplication>
#include <QClipboard>
#include <QHash>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QTextStream>
#include <QThread>
#include <QTime>

// Konsole
#include "KeyboardTranslator.h"
#include "Screen.h"
#include "TerminalCharacterDecoder.h"
#include "ScreenWindow.h"

using namespace Konsole;

Emulation::Emulation() :
  _currentScreen(nullptr),
  _encoding(QStringConverter::Utf8),
  _decoder(nullptr),
  _keyTranslator(nullptr),
  _usesMouse(false),
  _bracketedPasteMode(false)
{
  // create screens with a default size
  _screen[0] = new Screen(40,80);
  _screen[1] = new Screen(40,80);
  _currentScreen = _screen[0];

  // Create UTF-8 decoder by default
  _decoder = std::make_unique<QStringDecoder>(QStringConverter::Utf8);

  QObject::connect(&_bulkTimer1, &QTimer::timeout, this, &Emulation::showBulk);
  QObject::connect(&_bulkTimer2, &QTimer::timeout, this, &Emulation::showBulk);

  // listen for mouse status changes
  connect(this, &Emulation::programUsesMouseChanged, this, &Emulation::usesMouseChanged);
  connect(this, &Emulation::programBracketedPasteModeChanged, this, &Emulation::bracketedPasteModeChanged);

  connect(this, &Emulation::cursorChanged, [this](KeyboardCursorShape cursorShape, bool blinkingCursorEnabled) {
    emit titleChanged(50, QString("CursorShape=%1;BlinkingCursorEnabled=%2")
                               .arg(static_cast<int>(cursorShape)).arg(blinkingCursorEnabled));
  });
}

bool Emulation::programUsesMouse() const
{
    return _usesMouse;
}

void Emulation::usesMouseChanged(bool usesMouse)
{
    _usesMouse = usesMouse;
}

bool Emulation::programBracketedPasteMode() const
{
    return _bracketedPasteMode;
}

void Emulation::bracketedPasteModeChanged(bool bracketedPasteMode)
{
    _bracketedPasteMode = bracketedPasteMode;
}

ScreenWindow* Emulation::createWindow()
{
    ScreenWindow* window = new ScreenWindow();
    window->setScreen(_currentScreen);
    _windows << window;

    connect(window, &ScreenWindow::selectionChanged,
            this, &Emulation::bufferedUpdate);

    connect(this, &Emulation::outputChanged,
            window, &ScreenWindow::notifyOutputChanged);
    return window;
}

Emulation::~Emulation()
{
  for (ScreenWindow* window : _windows) {
    delete window;
  }

  delete _screen[0];
  delete _screen[1];
}

void Emulation::setScreen(int n)
{
  Screen *old = _currentScreen;
  _currentScreen = _screen[n & 1];
  if (_currentScreen != old)
  {
     // tell all windows onto this emulation to switch to the newly active screen
     for(ScreenWindow* window : const_cast<const QList<ScreenWindow*>&>(_windows))
         window->setScreen(_currentScreen);
  }
}

void Emulation::clearHistory()
{
    _screen[0]->setScroll(_screen[0]->getScroll(), false);
}

void Emulation::setHistory(const HistoryType& t)
{
  _screen[0]->setScroll(t);
  showBulk();
}

const HistoryType& Emulation::history() const
{
  return _screen[0]->getScroll();
}

void Emulation::setEncoding(QStringConverter::Encoding encoding)
{
  _encoding = encoding;
  
  // Create new decoder with the specified encoding
  _decoder = std::make_unique<QStringDecoder>(encoding);

  emit useUtf8Request(utf8());
}

void Emulation::setEncoding(EmulationCodec codec)
{
    if (codec == Utf8Codec)
        setEncoding(QStringConverter::Utf8);
    else // LocaleCodec
        setEncoding(QStringConverter::Latin1); // Fallback to Latin1 for locale
}

void Emulation::setKeyBindings(const QString& name)
{
  _keyTranslator = KeyboardTranslatorManager::instance()->findTranslator(name);
  if (!_keyTranslator)
  {
      _keyTranslator = KeyboardTranslatorManager::instance()->defaultTranslator();
  }
}

QString Emulation::keyBindings() const
{
  return _keyTranslator ? _keyTranslator->name() : QString();
}

void Emulation::receiveChar(wchar_t c)
{
  c &= 0xff;
  switch (c)
  {
    case '\b'      : _currentScreen->backspace();                 break;
    case '\t'      : _currentScreen->tab();                       break;
    case '\n'      : _currentScreen->newLine();                   break;
    case '\r'      : _currentScreen->toStartOfLine();             break;
    case 0x07      : emit stateSet(NOTIFYBELL);
                     break;
    default        : _currentScreen->displayCharacter(c);         break;
  };
}

void Emulation::sendKeyEvent(QKeyEvent* ev)
{
  emit stateSet(NOTIFYNORMAL);

  if (!ev->text().isEmpty())
  {
    emit sendData(ev->text().toUtf8().constData(), ev->text().length());
  }
}

void Emulation::sendString(const char*, int)
{
    // default implementation does nothing
}

void Emulation::sendMouseEvent(int /*buttons*/, int /*column*/, int /*row*/, int /*eventType*/)
{
    // default implementation does nothing
}

void Emulation::receiveData(const char* text, int length)
{
    emit stateSet(NOTIFYACTIVITY);
    bufferedUpdate();

    if (!_decoder) {
        return;
    }

    // Convert the input data to QString using the decoder
    QString unicodeText = _decoder->decode(QByteArray::fromRawData(text, length));
    
    // Convert to wstring for processing
    std::wstring unicodeString = unicodeText.toStdWString();

    // Send characters to terminal emulator
    for (wchar_t c : unicodeString) {
        receiveChar(c);
    }

    // Look for z-modem indicator
    for (int i = 0; i < length; i++)
    {
        if (text[i] == '\030')
        {
            if ((length - i - 1 > 3) && (strncmp(text + i + 1, "B00", 3) == 0))
                emit zmodemDetected();
        }
    }
}

void Emulation::writeToStream(TerminalCharacterDecoder* decoder, int startLine, int endLine)
{
  _currentScreen->writeLinesToStream(decoder, startLine, endLine);
}

void Emulation::writeToStream(TerminalCharacterDecoder* decoder)
{
  _currentScreen->writeLinesToStream(decoder, 0, _currentScreen->getHistLines());
}

int Emulation::lineCount() const
{
    return _currentScreen->getLines() + _currentScreen->getHistLines();
}

#define BULK_TIMEOUT1 10
#define BULK_TIMEOUT2 40

void Emulation::showBulk()
{
    _bulkTimer1.stop();
    _bulkTimer2.stop();
    emit outputChanged();
    _currentScreen->resetScrolledLines();
    _currentScreen->resetDroppedLines();
}

void Emulation::bufferedUpdate()
{
   _bulkTimer1.setSingleShot(true);
   _bulkTimer1.start(BULK_TIMEOUT1);
   if (!_bulkTimer2.isActive())
   {
      _bulkTimer2.setSingleShot(true);
      _bulkTimer2.start(BULK_TIMEOUT2);
   }
}

char Emulation::eraseChar() const
{
  return '\b';
}

void Emulation::setImageSize(int lines, int columns)
{
  if ((lines < 1) || (columns < 1))
    return;

  QSize screenSize[2] = { QSize(_screen[0]->getColumns(),
                                _screen[0]->getLines()),
                          QSize(_screen[1]->getColumns(),
                                _screen[1]->getLines()) };
  QSize newSize(columns, lines);

  if (newSize == screenSize[0] && newSize == screenSize[1])
    return;

  _screen[0]->resizeImage(lines, columns);
  _screen[1]->resizeImage(lines, columns);

  emit imageSizeChanged(lines, columns);
  bufferedUpdate();
}

QSize Emulation::imageSize() const
{
  return QSize(_currentScreen->getColumns(), _currentScreen->getLines());
}

ushort ExtendedCharTable::extendedCharHash(ushort* unicodePoints, ushort length) const
{
    ushort hash = 0;
    for (ushort i = 0; i < length; i++)
    {
        hash = 31 * hash + unicodePoints[i];
    }
    return hash;
}

bool ExtendedCharTable::extendedCharMatch(ushort hash, ushort* unicodePoints, ushort length) const
{
    ushort* entry = extendedCharTable[hash];
    if (entry == nullptr || entry[0] != length)
       return false;
    for (int i = 0; i < length; i++)
    {
        if (entry[i+1] != unicodePoints[i])
           return false;
    }
    return true;
}

ushort ExtendedCharTable::createExtendedChar(ushort* unicodePoints, ushort length)
{
    ushort hash = extendedCharHash(unicodePoints, length);
    while (extendedCharTable.contains(hash))
    {
        if (extendedCharMatch(hash, unicodePoints, length))
        {
            return hash;
        }
        else
        {
            hash++;
        }
    }

    ushort* buffer = new ushort[length+1];
    buffer[0] = length;
    for (int i = 0; i < length; i++)
       buffer[i+1] = unicodePoints[i];

    extendedCharTable.insert(hash, buffer);
    return hash;
}

ushort* ExtendedCharTable::lookupExtendedChar(ushort hash, ushort& length) const
{
    ushort* buffer = extendedCharTable[hash];
    if (buffer)
    {
        length = buffer[0];
        return buffer+1;
    }
    else
    {
        length = 0;
        return nullptr;
    }
}

ExtendedCharTable::ExtendedCharTable()
{
}

ExtendedCharTable::~ExtendedCharTable()
{
    for (ushort* buffer : extendedCharTable) {
        delete[] buffer;
    }
}

// global instance
ExtendedCharTable ExtendedCharTable::instance;