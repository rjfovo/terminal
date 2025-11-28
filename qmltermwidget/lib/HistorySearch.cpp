/*
    Copyright 2013 Christian Surlykke

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
#include <QApplication>
#include <QTextStream>
#include <QDebug>

#include "TerminalCharacterDecoder.h"
#include "Emulation.h"
#include "HistorySearch.h"

HistorySearch::HistorySearch(EmulationPtr emulation, QRegularExpression regExp,
        bool forwards, int startColumn, int startLine,
        QObject* parent) :
QObject(parent),
m_emulation(emulation),
m_regExp(regExp),
m_forwards(forwards),
m_startColumn(startColumn),
m_startLine(startLine) {
}

HistorySearch::~HistorySearch() {
}

void HistorySearch::search() {
    bool found = false;

    // 修改：使用 pattern().isEmpty() 代替 isEmpty()
    if (! m_regExp.pattern().isEmpty())
    {
        if (m_forwards) {
            found = search(m_startColumn, m_startLine, -1, m_emulation->lineCount()) || search(0, 0, m_startColumn, m_startLine);
        } else {
            found = search(0, 0, m_startColumn, m_startLine) || search(m_startColumn, m_startLine, -1, m_emulation->lineCount());
        }

        if (found) {
            emit matchFound(m_foundStartColumn, m_foundStartLine, m_foundEndColumn, m_foundEndLine);
        }
        else {
            emit noMatchFound();
        }
    }

    deleteLater();
}

bool HistorySearch::search(int startColumn, int startLine, int endColumn, int endLine) {
    qDebug() << "search from" << startColumn << "," << startLine
            <<  "to" << endColumn << "," << endLine;

    int linesRead = 0;
    int linesToRead = endLine - startLine + 1;

    qDebug() << "linesToRead:" << linesToRead;

    int blockSize;
    while ((blockSize = qMin(10000, linesToRead - linesRead)) > 0) {

        QString string;
        QTextStream searchStream(&string);
        PlainTextDecoder decoder;
        decoder.begin(&searchStream);
        decoder.setRecordLinePositions(true);

        int blockStartLine = m_forwards ? startLine + linesRead : endLine - linesRead - blockSize + 1;
        int chunkEndLine = blockStartLine + blockSize - 1;
        m_emulation->writeToStream(&decoder, blockStartLine, chunkEndLine);

        int endPosition;
        int numberOfLinesInString = decoder.linePositions().size() - 1;
        if (numberOfLinesInString > 0 && endColumn > -1 )
        {
            endPosition = decoder.linePositions().at(numberOfLinesInString - 1) + endColumn;
        }
        else
        {
            endPosition = string.size();
        }

        // 使用 QRegularExpression 的匹配逻辑
        QRegularExpressionMatch match;
        int matchStart = -1;
        
        if (m_forwards)
        {
            match = m_regExp.match(string, startColumn);
            if (match.hasMatch()) {
                matchStart = match.capturedStart();
                if (matchStart >= endPosition) {
                    matchStart = -1;
                }
            }
        }
        else
        {
            QRegularExpressionMatchIterator iterator = m_regExp.globalMatch(string);
            QRegularExpressionMatch lastMatch;
            
            while (iterator.hasNext()) {
                QRegularExpressionMatch currentMatch = iterator.next();
                int currentStart = currentMatch.capturedStart();
                
                if (currentStart < endPosition && 
                    (startColumn == -1 || currentStart >= startColumn)) {
                    lastMatch = currentMatch;
                } else {
                    break;
                }
            }
            
            if (lastMatch.hasMatch()) {
                match = lastMatch;
                matchStart = match.capturedStart();
            }
        }

        if (matchStart > -1)
        {
            int matchEnd = matchStart + match.capturedLength() - 1;
            qDebug() << "Found in string from" << matchStart << "to" << matchEnd;

            int startLineNumberInString = findLineNumberInString(decoder.linePositions(), matchStart);
            m_foundStartColumn = matchStart - decoder.linePositions().at(startLineNumberInString);
            m_foundStartLine = startLineNumberInString + startLine + linesRead;

            int endLineNumberInString = findLineNumberInString(decoder.linePositions(), matchEnd);
            m_foundEndColumn = matchEnd - decoder.linePositions().at(endLineNumberInString);
            m_foundEndLine = endLineNumberInString + startLine + linesRead;

            qDebug() << "m_foundStartColumn" << m_foundStartColumn
                    << "m_foundStartLine" << m_foundEndLine
                    << "m_foundEndColumn" << m_foundEndColumn
                    << "m_foundEndLine" << m_foundEndLine;

            return true;
        }

        linesRead += blockSize;
    }

    qDebug() << "Not found";
    return false;
}

int HistorySearch::findLineNumberInString(QList<int> linePositions, int position) {
    int lineNum = 0;
    while (lineNum + 1 < linePositions.size() && linePositions[lineNum + 1] <= position)
        lineNum++;

    return lineNum;
}