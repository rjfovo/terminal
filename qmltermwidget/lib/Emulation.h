/*
    This file is part of Konsole, an X terminal.

    Copyright 2007-2008 by Robert Knight <robertknight@gmail.com>
    Copyright 1997,1998 by Lars Doelle <lars.doelle@on-line.de>

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

#ifndef EMULATION_H
#define EMULATION_H

// System
#include <stdio.h>

// Qt
#include <QKeyEvent>
#include <QTextStream>
#include <QTimer>
#include <QStringConverter>

// Konsole
#define KONSOLEPRIVATE_EXPORT

namespace Konsole
{

class KeyboardTranslator;
class HistoryType;
class Screen;
class ScreenWindow;
class TerminalCharacterDecoder;

/**
 * This enum describes the available states which
 * the terminal emulation may be set to.
 *
 * These are the values used by Emulation::stateChanged()
 */
enum
{
    /** The emulation is currently receiving user input. */
    NOTIFYNORMAL=0,
    /**
     * The terminal program has triggered a bell event
     * to get the user's attention.
     */
    NOTIFYBELL=1,
    /**
     * The emulation is currently receiving data from its
     * terminal input.
     */
    NOTIFYACTIVITY=2,

    // unused here?
    NOTIFYSILENCE=3
};

/**
 * Base class for terminal emulation back-ends.
 */
class KONSOLEPRIVATE_EXPORT Emulation : public QObject
{
Q_OBJECT

public:

  /**
   * This enum describes the available shapes for the keyboard cursor.
   * See setKeyboardCursorShape()
   */
  enum class KeyboardCursorShape {
      /** A rectangular block which covers the entire area of the cursor character. */
      BlockCursor = 0,
      /**
       * A single flat line which occupies the space at the bottom of the cursor
       * character's area.
       */
      UnderlineCursor = 1,
      /**
       * An cursor shaped like the capital letter 'I', similar to the IBeam
       * cursor used in Qt/KDE text editors.
       */
      IBeamCursor = 2
  };


   /** Constructs a new terminal emulation */
   Emulation();
  ~Emulation();

  /**
   * Creates a new window onto the output from this emulation.
   */
  ScreenWindow* createWindow();

  /** Returns the size of the screen image which the emulation produces */
  QSize imageSize() const;

  /**
   * Returns the total number of lines, including those stored in the history.
   */
  int lineCount() const;

  /**
   * Sets the history store used by this emulation.
   */
  void setHistory(const HistoryType&);
  /** Returns the history store used by this emulation. */
  const HistoryType& history() const;
  /** Clears the history scroll. */
  void clearHistory();

  /**
   * Copies the output history from @p startLine to @p endLine
   * into @p stream, using @p decoder to convert the terminal
   * characters into text.
   */
  virtual void writeToStream(TerminalCharacterDecoder* decoder,int startLine,int endLine);

  /**
   * Copies the complete output history into @p stream.
   */
  virtual void writeToStream(TerminalCharacterDecoder* decoder);

  /** Returns the codec used to decode incoming characters. */
  QStringConverter::Encoding encoding() const { return _encoding; }
  /** Sets the codec used to decode incoming characters. */
  void setEncoding(QStringConverter::Encoding encoding);

  /**
   * Convenience method.
   * Returns true if the current encoding used to decode incoming
   * characters is UTF-8
   */
  bool utf8() const
  { return _encoding == QStringConverter::Utf8; }

  /** TODO Document me */
  virtual char eraseChar() const;

  /**
   * Sets the key bindings used to key events
   */
  void setKeyBindings(const QString& name);
  /**
   * Returns the name of the emulation's current key bindings.
   */
  QString keyBindings() const;

  /**
   * Copies the current image into the history and clears the screen.
   */
  virtual void clearEntireScreen() =0;

  /** Resets the state of the terminal. */
  virtual void reset() =0;

  /**
   * Returns true if the active terminal program wants
   * mouse input events.
   */
  bool programUsesMouse() const;

  bool programBracketedPasteMode() const;

public slots:

  /** Change the size of the emulation's image */
  virtual void setImageSize(int lines, int columns);

  /**
   * Interprets a sequence of characters and sends the result to the terminal.
   */
  virtual void sendText(const QString& text) = 0;

  /**
   * Interprets a key press event and emits the sendData() signal with
   * the resulting character stream.
   */
  virtual void sendKeyEvent(QKeyEvent*);

  /**
   * Converts information about a mouse event into an xterm-compatible escape
   * sequence and emits the character sequence via sendData()
   */
  virtual void sendMouseEvent(int buttons, int column, int line, int eventType);

  /**
   * Sends a string of characters to the foreground terminal process.
   */
  virtual void sendString(const char* string, int length = -1) = 0;

  /**
   * Processes an incoming stream of characters.
   */
  void receiveData(const char* buffer, int len);

signals:

  /**
   * Emitted when a buffer of data is ready to send to the
   * standard input of the terminal.
   */
  void sendData(const char* data, int len);

  /**
   * Requests that sending of input to the emulation
   * from the terminal process be suspended or resumed.
   */
  void lockPtyRequest(bool suspend);

  /**
   * Requests that the pty used by the terminal process
   * be set to UTF 8 mode.
   */
  void useUtf8Request(bool);

  /**
   * Emitted when the activity state of the emulation is set.
   */
  void stateSet(int state);

  /** TODO Document me */
  void zmodemDetected();

  /**
   * Requests that the color of the text used
   * to represent the tabs associated with this
   * emulation be changed.
   */
  void changeTabTextColorRequest(int color);

  /**
   * Emitted when the program running in the shell indicates whether or
   * not it is interested in mouse events.
   */
  void programUsesMouseChanged(bool usesMouse);

  void programBracketedPasteModeChanged(bool bracketedPasteMode);

  /**
   * Emitted when the contents of the screen image change.
   */
  void outputChanged();

  /**
   * Emitted when the program running in the terminal wishes to update the
   * session's title.
   */
  void titleChanged(int title, const QString& newTitle);

  /**
   * Emitted when the program running in the terminal changes the
   * screen size.
   */
  void imageSizeChanged(int lineCount, int columnCount);

  /**
   * Emitted when the setImageSize() is called on this emulation for
   * the first time.
   */
  void imageSizeInitialized();

  /**
   * Emitted after receiving the escape sequence which asks to change
   * the terminal emulator's size
   */
  void imageResizeRequest(const QSize& size);

  /**
   * Emitted when the terminal program requests to change various properties
   * of the terminal display.
   */
  void profileChangeCommandReceived(const QString& text);

  /**
   * Emitted when a flow control key combination ( Ctrl+S or Ctrl+Q ) is pressed.
   */
  void flowControlKeyPressed(bool suspendKeyPressed);

  /**
   * Emitted when the cursor shape or its blinking state is changed via
   * DECSCUSR sequences.
   */
  void cursorChanged(KeyboardCursorShape cursorShape, bool blinkingCursorEnabled);

protected:
  virtual void setMode(int mode) = 0;
  virtual void resetMode(int mode) = 0;

  /**
   * Processes an incoming character.
   */
  virtual void receiveChar(wchar_t ch);

  /**
   * Sets the active screen.
   */
  void setScreen(int index);

  enum EmulationCodec
  {
      LocaleCodec = 0,
      Utf8Codec   = 1
  };
  void setEncoding(EmulationCodec codec);

  QList<ScreenWindow*> _windows;

  Screen* _currentScreen;
  Screen* _screen[2];

  QStringConverter::Encoding _encoding;
  std::unique_ptr<QStringDecoder> _decoder;
  const KeyboardTranslator* _keyTranslator;

protected slots:
  /**
   * Schedules an update of attached views.
   */
  void bufferedUpdate();

private slots:

  void showBulk();

  void usesMouseChanged(bool usesMouse);

  void bracketedPasteModeChanged(bool bracketedPasteMode);

private:
  bool _usesMouse;
  bool _bracketedPasteMode;
  QTimer _bulkTimer1;
  QTimer _bulkTimer2;

};

}

#endif // ifndef EMULATION_H