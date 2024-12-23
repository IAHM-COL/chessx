/****************************************************************************
*   Copyright (C) 2012 by Jens Nissen jens-chessx@gmx.net                   *
****************************************************************************/

#include <QCoreApplication>
#include <QHash>

#include "ecoinfo.h"
#include "database.h"
#include "tags.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

static bool sortPlayersLt(const PlayerInfoListItem& a1, const PlayerInfoListItem& a2)
{
    if(a1.second == a2.second)
    {
        return (a1.first < a2.first);
    }
    return a1.second > a2.second;
}

EcoInfo::EcoInfo()
{
    m_database = nullptr;
    reset();
}

EcoInfo::~EcoInfo()
{
}

EcoInfo::EcoInfo(Database* db, const QString & eco)
{
    setDatabase(db);
    setCode(eco);
    reset();
    update();
}

QString EcoInfo::name() const
{
    return m_code;
}

void EcoInfo::setDatabase(Database* db)
{
    m_database = db;
}

void EcoInfo::setCode(const QString& eco)
{
    m_code = eco;
    update();
}

int EcoInfo::toResult(const QString& res) const
{
    if(res.startsWith("1/2"))
    {
        return Draw;
    }
    else if(res.startsWith('1'))
    {
        return WhiteWin;
    }
    else if(res.startsWith('0'))
    {
        return BlackWin;
    }
    else
    {
        return ResultUnknown;
    }
}

float EcoInfo::toPoints(const QString& res) const
{
    if(res.startsWith("1/2"))
    {
        return 0.5;
    }
    else if(res.startsWith('1'))
    {
        return 1.0;
    }
    else if(res.startsWith('0'))
    {
        return 0.0;
    }
    else
    {
        return -1.0;
    }
}

void EcoInfo::update()
{
    QHash<QString, float> playersWhite;
    QHash<QString, float> playersBlack;

    const IndexX* index = m_database->index();

    // Determine matching tag values
    ValueIndex eco = index->getValueIndex(m_code);

    // Clean previous statistics
    reset();

    for(int i = 0; i < (int) m_database->count(); ++i)
    {
        if(index->valueIndexFromTag(TagNameECO, i) != eco)
        {
            continue;
        }
        QString result = index->tagValue(TagNameResult, i);
        int res = toResult(result);
        QString whitePlayer = index->tagValue(TagNameWhite, i);
        QString blackPlayer = index->tagValue(TagNameBlack, i);
        // The following works as QHash initializes a default-constructed value to 0
        float fres = toPoints(result);
        if(fres >= 0)
        {
            playersWhite[whitePlayer] += fres;
            playersBlack[blackPlayer] += (1.0 - fres);
            m_gamesWhite[whitePlayer]++;
            m_gamesBlack[blackPlayer]++;
        }
        else
        {
            // This looks silly, but the []operator has a side effect!
            if(playersWhite[whitePlayer] == 0)
            {
                playersWhite[whitePlayer] = 0;
            }
            if(playersBlack[blackPlayer] == 0)
            {
                playersBlack[blackPlayer] = 0;
            }
        }

        m_result[res]++;
        m_count++;
    }

    for (auto it = playersWhite.cbegin(); it != playersWhite.cend(); ++it)
    {
        m_playersWhite.append(PlayerInfoListItem(it.key(), it.value()));
    }
    std::sort(m_playersWhite.begin(), m_playersWhite.end(), sortPlayersLt);

    for (auto it = playersBlack.cbegin(); it != playersBlack.cend(); ++it)
    {
        m_playersBlack.append(PlayerInfoListItem(it.key(), it.value()));
    }
    std::sort(m_playersBlack.begin(), m_playersBlack.end(), sortPlayersLt);
}


QString EcoInfo::formattedScore(const int result[4], int count) const
{
    if(!count)
    {
        return QCoreApplication::translate("EcoInfo", "<i>no games</i>");
    }
    QString score = "<b>";
    QChar scoresign[4] = {'*', '+', '=', '-'};
    QStringList results;
    results << "\\*" << "1-0" << "1/2-1/2" << "0-1";
    const int order[] = { WhiteWin, Draw, BlackWin, ResultUnknown };
    QString format = QString("<a href='result:%1'> &nbsp;%2%3</a>");

    for(int j=0;j<4;j++)
    {
        int i = order[j];
        score += format.arg(results[i]).arg(scoresign[i]).arg(result[i]);
    }
    int n = count - result[ResultUnknown];
    if(n)
    {
        score += QString(" &nbsp;(%1%)").arg((100.0 * result[WhiteWin] + 50.0 * result[Draw]) / (n), 1, 'f', 1);
    }
    score += "</b>";
    return score;
}


QString EcoInfo::formattedScore() const
{
    return tr("Total: %1").arg(formattedScore(m_result, m_count));
}

void EcoInfo::reset()
{
    for(int c = White; c <= Black; ++c)
    {
        for(int r = 0; r < 4; ++r)
        {
            m_result[r] = 0;
        }
    }
    m_playersWhite.clear();
    m_playersBlack.clear();
    m_gamesWhite.clear();
    m_gamesBlack.clear();
    m_count = 0;
    m_rating[0] = 99999;
    m_rating[1] = 0;
}

QString EcoInfo::formattedGameCount() const
{
    return QCoreApplication::translate("EcoInfo", "Games in database %1: %2")
           .arg(m_database->name()).arg(m_count);
}

QString EcoInfo::formattedRating() const
{
    if(!m_rating[1])
    {
        return QString();
    }
    else if(m_rating[0] == m_rating[1])
    {
        return QCoreApplication::translate("EcoInfo", "Rating: <b>%1</b>").arg(m_rating[0]);
    }
    else
        return QCoreApplication::translate("EcoInfo", "Rating: <b>%1-%2</b>")
               .arg(m_rating[0]).arg(m_rating[1]);
}

QString EcoInfo::listOfPlayers() const
{
    QString playersList;

    playersList.append(QString("<a name='ListWhite'></a><table><tr><th><a href=\"#ListBlack\">&#8681;</a>%1</th><th>%2</th></tr>").arg(tr("White Player"), tr("Score")));

    for(PlayerInfoList::const_iterator it = m_playersWhite.constBegin(); it != m_playersWhite.constEnd(); ++it)
    {
        playersList += QString("<tr><td><a href=\"player-white:%1\">%2</a></td><td>%3/%4</td></tr>")
                       .arg((*it).first, (*it).first)
                       .arg((*it).second)
                       .arg(m_gamesWhite[(*it).first]);
    }

    playersList = playersList.append("</table>");

    playersList.append(QString("<a name='ListBlack'></a><table><tr><th><a href=\"#ListWhite\">&#8679;</a>%1</th><th>%2</th></tr>").arg(tr("Black Player"), tr("Score")));

    for(PlayerInfoList::const_iterator it = m_playersBlack.begin(); it != m_playersBlack.end(); ++it)
    {
        playersList += QString("<tr><td><a href=\"player-black:%1\">%2</a></td><td>%3/%4</td></tr>")
                       .arg((*it).first, (*it).first)
                       .arg((*it).second)
                       .arg(m_gamesBlack[(*it).first]);
    }

    playersList = playersList.append("</table>");

    return playersList;
}
