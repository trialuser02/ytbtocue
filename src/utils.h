/*
 * Copyright (c) 2019-2024, Ilya Kotov <iokotov@astralinux.ru>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UTILS_H
#define UTILS_H

#include <QString>

#define YTBTOCUE_VERSION_MAJOR 0
#define YTBTOCUE_VERSION_MINOR 7

#define YTBTOCUE_TOSTRING(s) #s
#define YTBTOCUE_STRINGIFY(s)         YTBTOCUE_TOSTRING(s)

#define YTBTOCUE_VERSION_INT (YTBTOCUE_VERSION_MAJOR<<8 | YTBTOCUE_VERSION_MINOR)
#define YTBTOCUE_VERSION_STR YTBTOCUE_STRINGIFY(YTBTOCUE_VERSION_MAJOR.YTBTOCUE_VERSION_MINOR)

class Utils
{
public:
    static QString formatDuration(int duration);

private:
    Utils() {}
};

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))

namespace Qt {

#if (QT_VERSION < QT_VERSION_CHECK(5, 14, 0))
constexpr auto SkipEmptyParts = QString::SkipEmptyParts;
constexpr auto KeepEmptyParts = QString::KeepEmptyParts;
#endif

inline namespace Literals {
inline namespace StringLiterals {

inline QString operator""_s(const char16_t *str, size_t size) noexcept
{
    return QString::fromUtf16(str, size);
}

constexpr inline QLatin1String operator""_L1(const char *str, size_t size) noexcept
{
    return QLatin1String(str, int(size));
}

inline QByteArray operator""_ba(const char *str, size_t size) noexcept
{
    return QByteArray(str, qsizetype(size));
}

} // StringLiterals
} // Literals
} // Qt

#elif QT_VERSION < QT_VERSION_CHECK(6, 4, 0)

#include <QLatin1String>

namespace Qt {
inline namespace Literals {
inline namespace StringLiterals {

inline QString operator""_s(const char16_t *str, size_t size) noexcept
{
    return QString(QStringPrivate(nullptr, const_cast<char16_t *>(str), qsizetype(size)));
}

constexpr inline QLatin1String operator""_L1(const char *str, size_t size) noexcept
{
    return QLatin1String(str, int(size));
}

inline QByteArray operator""_ba(const char *str, size_t size) noexcept
{
    return QByteArray(QByteArrayData(nullptr, const_cast<char *>(str), qsizetype(size)));
}

} // StringLiterals
} // Literals
} // Qt

using QLatin1StringView = QLatin1String;

#endif

using namespace Qt::Literals::StringLiterals;


#endif // UTILS_H
