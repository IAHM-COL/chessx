/***************************************************************************
 *   (C) 2006-2007 Sean Estabrooks                                         *
 *   (C) 2007-2009 Michal Rudolf <mrudolf@kdewebdev.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "enginedata.h"
#include "qt6compat.h"
#include "uciengine.h"
#include <QRegularExpression>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

UCIEngine::UCIEngine(const QString& name,
                     const QString& command,
                     bool bTestMode,
                     const QString& directory,
                     bool log,
                     bool sendHistory) : EngineX(name, command, bTestMode, directory, log, sendHistory)
{
    m_quitAfterAnalysis = false;
    m_chess960 = false;
}

void UCIEngine::setStartPos(const BoardX& startPos)
{
    m_startPos = startPos;
}

bool UCIEngine::startAnalysis(const BoardX& board, int nv, const EngineParameter &mt, bool bNewGame, QString line)
{
    EngineX::setMoveTime(mt);
    m_mpv = nv;
    if(!isActive())
    {
        return false;
    }

    if(m_board == board)
    {
        return true;
    }
    m_board = board;
    if (!getSendHistory())
    {
        // Avoid sending history to engines
        line = "";
    }
    m_line = line;
    m_line.remove('=');

    m_chess960 = board.chess960();
    if (line.isEmpty())
    {
        m_position = board.toFen(m_chess960);
    }
    else
    {
        m_position = m_startPos.toFen(m_chess960);
    }

    send("stop");
    if (bNewGame)
    {
        m_waitingOn = "ucinewgame";
        send("ucinewgame");
        send("isready");
    }
    else
    {
        setPosition();
    }
    setAnalyzing(true);

    return true;
}

void UCIEngine::stopAnalysis()
{
    if(isAnalyzing())
    {
        send("stop");
    }
}

void UCIEngine::setMpv(int mpv)
{
    if (m_mpv != mpv)
    {
        EngineX::setMpv(mpv);
        if(isAnalyzing())
        {
            send("stop");
            if (hasOption("MultiPV"))
            {
                send(QString("setoption name MultiPV value %1").arg(m_mpv));
            }
            go();
        }
    }
}

void UCIEngine::setMoveTime(const EngineParameter &mt)
{
    if ((m_moveTime.ms_totalTime != mt.ms_totalTime) || (m_moveTime.analysisMode != mt.analysisMode))
    {
        EngineX::setMoveTime(mt);
        if(isAnalyzing())
        {
            send("stop");
            go();
        }
    }
}

void UCIEngine::protocolStart()
{
    //tell the engine we are using the uci protocol
    send("uci");
}

void UCIEngine::protocolEnd()
{
    send("quit");
    setActive(false);
    m_board.clear();
}

void UCIEngine::go()
{
    if (m_moveTime.tm == EngineParameter::TIME_GONG)
    {
        if (hasOption("UCI_AnalyseMode"))
        {
            send(QString("setoption name UCI_AnalyseMode value %1").arg((m_moveTime.ms_totalTime==0 || m_moveTime.analysisMode) ? "true":"false"));
        }
        if (!m_moveTime.ms_totalTime)
        {
            send("go infinite");
        }
        else
        {
            if ( m_moveTime.searchDepth < 0 )
            {
                send(QString("go movetime %1").arg(m_moveTime.ms_totalTime));
            }
            else
            {
                send(QString("go depth %1").arg(m_moveTime.searchDepth));
            }
        }
    }
    else if (m_moveTime.tm == EngineParameter::TIME_SUDDEN_DEATH)
    {
        send(QString("go wtime %1 btime %2 winc %3 binc %4")
             .arg(m_moveTime.ms_white)
             .arg(m_moveTime.ms_black)
             .arg(m_moveTime.ms_increment)
             .arg(m_moveTime.ms_increment));
    }
}

void UCIEngine::setPosition()
{
    m_waitingOn = "";
    if (hasOption("MultiPV"))
    {
        send(QString("setoption name MultiPV value %1").arg(m_mpv));
    }
    EngineOptionData chess960;
    if(getOption("UCI_Chess960", chess960))
    {
        bool default960 = (chess960.m_defVal.compare("true", Qt::CaseInsensitive)==0);
        if (default960 != m_chess960)
        {
            send(QString("setoption name UCI_Chess960 value %1").arg(m_chess960 ? "true":"false"));
        }
    }

    if (m_line.isEmpty())
    {
        send("position fen " + m_position);
    }
    else
    {
        send("position fen " + m_position + " moves " + m_line);
    }
    go();
}

void UCIEngine::processMessage(const QString& message)
{
    if(message == "uciok")
    {
        //once the engine is running wait for it to initialise
        m_waitingOn = "uciok";
        send("isready");
    }

    if(message.startsWith("id"))
    {
        QStringList list = message.split(" " ,SkipEmptyParts);
        if (list.length()>2)
        {
            if (list[1] == "name")
            {
                m_name = list[2];
            }
        }
    }
    if(message == "readyok")
    {
        if(m_waitingOn == "uciok")
        {
            //engine is now initialised and ready to go
            m_waitingOn = "";
            setActive(true);

            if(!m_bTestMode)
            {
                QString setHashOption;

                OptionValueMap::const_iterator i = m_mapOptionValues.constBegin();
                while(i != m_mapOptionValues.constEnd())
                {
                    QString key = i.key();
                    QVariant value = i.value();
                    if(EngineOptionData* dataSpec = EngineOptionData::FindInList(key, m_options))
                    {
                        if ((dataSpec->m_name != "UCI_Chess960") && (dataSpec->m_name != "MultiPV") && (dataSpec->m_name != "UCI_AnalyseMode"))
                        {
                            switch(dataSpec->m_type)
                            {
                            case OPT_TYPE_BUTTON:
                                if(value.toBool())
                                {
                                    send(QString("setoption name %1").arg(key));
                                }
                                break;
                            case OPT_TYPE_CHECK:
                            case OPT_TYPE_SPIN:
                            case OPT_TYPE_STRING:
                            case OPT_TYPE_COMBO:
                                if(dataSpec->m_defVal != value.toString() && !value.toString().isEmpty())
                                {
                                    // Defer sending 'Hash' until all other options have been sent. Certain engines initialize the
                                    // hash immediately upon receiving the option instead of deferring initialization to 'isready'.
                                    // This can result in slow engine startup when a large hash is specified. Example sequence
                                    // with Stockfish 17 illustrating the issue:
                                    // - setoption name Hash value 131072 -- performs a single-threaded 128 GiB TT initialization (very slow)
                                    // - setoption name Threads value 64 -- performs another TT initialization, this time with 64 threads (fast)
                                    // By sending 'Hash' last, we'll avoid the slow single-threaded TT initialization.
                                    if (dataSpec->m_name != "Hash")
                                    {
                                        send(QString("setoption name %1 value %2").arg(key, value.toString()));
                                    }
                                    else
                                    {
                                        setHashOption = QString("setoption name %1 value %2").arg(key, value.toString());
                                    }
                                }
                                break;
                            }
                        }
                    }
                    ++i;
                }

                if (setHashOption.size() > 0)
                {
                    // deferred: hash
                    send(setHashOption);
                }
            }
        }

        if(m_waitingOn == "ucinewgame")
        {
            //engine is now ready to analyse a new position
            setPosition();
        }
    }

    QString command = message.section(' ', 0, 0);

    if(command == "info" && isAnalyzing())
    {
        parseAnalysis(message);
    }
    else if(command == "bestmove" && isAnalyzing())
    {
        parseBestMove(message);
    }
    else if(command == "option")
    {
        parseOptions(message);
    }
}

void UCIEngine::parseBestMove(const QString& message)
{
    QString bestMove = message.section(' ', 1, 1, QString::SectionSkipEmpty);

    if (!bestMove.isEmpty())
    {
        Analysis analysis;
        Move move = m_board.parseMove(bestMove);
        Move::List moves;
        if (move.isLegal())
        {
            moves.append(move);
        }
        analysis.setVariation(moves);
        analysis.setBestMove(true);
        sendAnalysis(analysis);
    }
}

void UCIEngine::parseAnalysis(const QString& message)
{
    // Sample: info score cp 20  depth 3 nodes 423 time 15 pv f1c4 g8f6 b1c3
    Analysis analysis;
    bool multiPVFound, timeFound, nodesFound, depthFound, scoreFound;
    multiPVFound = timeFound = nodesFound = depthFound = scoreFound = false;

    QString info = message.section(' ', 1, -1, QString::SectionSkipEmpty);
    int section = 0;
    QString name;
    bool ok;

    //loop around the name value tuples
    while(!info.section(' ', section, section + 1, QString::SectionSkipEmpty).isEmpty())
    {
        name = info.section(' ', section, section, QString::SectionSkipEmpty);

        if(name == "multipv")
        {
            analysis.setNumpv(info.section(' ', section + 1, section + 1, QString::SectionSkipEmpty).toInt(&ok));
            section += 2;
            if(ok)
            {
                multiPVFound = true;
                continue;
            }
        }

        if(name == "time")
        {
            analysis.setTime(info.section(' ', section + 1, section + 1, QString::SectionSkipEmpty).toInt(&ok));
            section += 2;
            if(ok)
            {
                timeFound = true;
                continue;
            }
        }

        if(name == "nodes")
        {
            analysis.setNodes(info.section(' ', section + 1, section + 1, QString::SectionSkipEmpty).toLongLong(&ok));
            section += 2;
            if(ok)
            {
                nodesFound = true;
                continue;
            }
        }

        if(name == "depth")
        {
            analysis.setDepth(info.section(' ', section + 1, section + 1, QString::SectionSkipEmpty).toInt(&ok));
            section += 2;
            if(ok)
            {
                depthFound = true;
                continue;
            }
        }

        if(name == "score")
        {
            QString type = info.section(' ', section + 1, section + 1, QString::SectionSkipEmpty);
            if(type == "cp" || type == "mate")
            {
                int score = info.section(' ', section + 2, section + 2).toInt(&ok);
                if(type == "mate")
                {
                    analysis.setMovesToMate(score);
                    if(m_board.toMove() == Black)
                    {
                        analysis.setScore(-30000);
                    }
                    else
                    {
                        analysis.setScore(30000);
                    }
                }
                else if(m_board.toMove() == Black)
                {
                    analysis.setScore(-score);
                }
                else
                {
                    analysis.setScore(score);
                }
                section += 3;
                if(ok)
                {
                    scoreFound = true;
                    continue;
                }
            }
            else
            {
                section += 3;
            }
        }

        if (name == "upperbound" || name =="lowerbound")
        {
            if  (scoreFound && analysis.movesToMate() == 0)
            {
                // Work around bug in Stockfish
                ok = false;
            }
        }

        if(name == "pv")
        {
            BoardX board = m_board;
            Move::List moves;
            QString moveText;
            section++;
            while((moveText = info.section(' ', section, section, QString::SectionSkipEmpty)) != "")
            {
                Move move = board.parseMove(moveText);
                if(!move.isLegal())
                {
                    break;
                }
                board.doMove(move);
                moves.append(move);
                section++;
            }
            analysis.setVariation(moves);
        }

        //not understood, skip
        section += 2;
    }

    if ((timeFound && nodesFound && scoreFound && analysis.isValid()) ||
        (analysis.isAlreadyMate() && depthFound && analysis.depth() == 0))
    {
        if(!multiPVFound)
        {
            analysis.setNumpv(1);
        }
        sendAnalysis(analysis);
    }
}

void UCIEngine::parseOptions(const QString& message)
{
    enum ScanPhase { EXPECT_OPTION,
                     EXPECT_NAME,
                     EXPECT_TYPE_TOKEN,
                     EXPECT_TYPE,
                     EXPECT_DEFAULT_VALUE,
                     EXPECT_MIN_MAX_DEFAULT,
                     EXPECT_MIN_VALUE,
                     EXPECT_MAX_VALUE,
                     EXPECT_VAR_TOKEN,
                     EXPECT_VAR
                   } phase;

    phase = EXPECT_OPTION;
    QStringList list = message.split(QRegularExpression("\\s+"), SkipEmptyParts);

    QStringList nameVals;
    QString defVal;
    QString minVal;
    QString maxVal;
    QStringList varVals;
    OptionType optionType = OPT_TYPE_STRING;
    QString error;
    bool done = false;
    foreach(QString token, list)
    {
        switch(phase)
        {
        case EXPECT_OPTION:
            if(token == "option")
            {
                phase = EXPECT_NAME;
            }
            else
            {
                error = token;
            }
            break;
        case EXPECT_NAME:
            if(token == "name")
            {
                phase = EXPECT_TYPE;
            }
            else
            {
                error = token;
            }
            break;
        case EXPECT_TYPE:
            if(token == "type")
            {
                phase = EXPECT_TYPE_TOKEN;
            }
            else
            {
                nameVals << token;
            }
            break;
        case EXPECT_TYPE_TOKEN:
            if(token == "check")
            {
                optionType = OPT_TYPE_CHECK;
            }
            else if(token == "spin")
            {
                optionType = OPT_TYPE_SPIN;
            }
            else if(token == "combo")
            {
                optionType = OPT_TYPE_COMBO;
            }
            else if(token == "button")
            {
                optionType = OPT_TYPE_BUTTON;
                done = true;
            }
            else if(token == "string")
            {
                optionType = OPT_TYPE_STRING;
            }
            else
            {
                error = token;
            }

            phase = EXPECT_MIN_MAX_DEFAULT;
            break;
        case EXPECT_DEFAULT_VALUE:
            defVal = token;
            switch(optionType)
            {
            case OPT_TYPE_SPIN:
                phase = EXPECT_MIN_MAX_DEFAULT;
                break;
            case OPT_TYPE_COMBO:
                phase = EXPECT_VAR_TOKEN;
                break;
            case OPT_TYPE_STRING:
                if (defVal=="<empty>" || defVal=="\"\"")
                {
                    defVal = "";
                }
                done = true;
                break;
            default:
                done = true;
                break;
            }
            break;
        case EXPECT_MIN_MAX_DEFAULT:
            if(token == "default")
            {
                phase = EXPECT_DEFAULT_VALUE;
            }
            else if(token == "min")
            {
                phase = EXPECT_MIN_VALUE;
            }
            else if(token == "max")
            {
                phase = EXPECT_MAX_VALUE;
            }
            else
            {
                done = true;
            }
            break;
        case EXPECT_MIN_VALUE:
            minVal = token;
            phase = EXPECT_MIN_MAX_DEFAULT;
            break;
        case EXPECT_MAX_VALUE:
            maxVal = token;
            phase = EXPECT_MIN_MAX_DEFAULT;
            break;
        case EXPECT_VAR_TOKEN:
            if(token == "var")
            {
                phase = EXPECT_VAR;
            }
            else
            {
                done = true;
            }
            break;
        case EXPECT_VAR:
            varVals << token;
            phase = EXPECT_VAR_TOKEN;
            break;
        default:
            error = token;
            return;
        }

        if(done || !error.isEmpty())
        {
            break;
        }
    }
    if(!error.isEmpty())
    {
        QString s;
        QTextStream out(&s);

        out << "Cannot parse Option string: '"
                 << message
                 << "' looking at token '"
                 << error
                 << "'!";

        logError(s);
        return;
    }

    if (!done && (phase == EXPECT_DEFAULT_VALUE) && (optionType == OPT_TYPE_STRING))
    {
        // Workaround bug in Stockfish 8
        done = true; // Indicate an empty default value
    }

    if (done || (phase > EXPECT_DEFAULT_VALUE))
    {
        QString name = nameVals.join(" ");
        EngineOptionData option;
        option.m_name = name;
        option.m_minVal = minVal;
        option.m_maxVal = maxVal;
        option.m_defVal = defVal;
        option.m_varVals = varVals;
        option.m_type = optionType;

        if (!hasOption(name))
        {
            m_options.append(option);
        }
    }
    else
    {
        QString s;
        QTextStream out(&s);

        out << "Incomplete syntax parsing Option string: '"
            << message
            << "'!";

        logError(s);
        return;
    }
}
