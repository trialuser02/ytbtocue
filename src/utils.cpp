#include "utils.h"

QString Utils::formatDuration(int duration)
{
    QString out;
    if(duration >= 3600)
        out = QString("%1:%2").arg(duration / 3600).arg(duration % 3600 / 60, 2, 10, QChar('0'));
    else
        out = QString("%1").arg(duration % 3600 / 60);
    out += QString(":%1").arg(duration % 60, 2, 10, QChar('0'));
    return out;
}
