//
// Created by mark on 10/6/26.
//

#ifndef VENT_FILE_EXTENSIONS_H
#define VENT_FILE_EXTENSIONS_H

#include <array>
#include <string_view>
#include <algorithm>

#include <qtypes.h>


inline constexpr uint IMAGE_EXTENSIONS_COUNT = 4;
inline constexpr uint VIDEO_EXTENSIONS_COUNT = 4;
inline constexpr uint AUDIO_EXTENSIONS_COUNT = 6;

inline constexpr std::array<std::string_view, IMAGE_EXTENSIONS_COUNT> imageExtensions{".jpeg", ".png", ".webp", ".gif"};
inline constexpr std::array<std::string_view, VIDEO_EXTENSIONS_COUNT> videoExtensions{".mp4", ".webm", ".quicktime", ".x-matroska"};
inline constexpr std::array<std::string_view, AUDIO_EXTENSIONS_COUNT> audioExtensions{".mpeg", ".ogg", ".wav", ".flac", ".aac", ".m4a"};

/**
 * @brief Определяет категорию файла по расширению (кружки и голосывые тут не обрабатываются)
 * @param filePath
 * @return
 */
static QString defineFileCategory(const QString &filePath)
{
    QString category;
    if (std::ranges::any_of(imageExtensions,[&filePath](std::string_view extension)
    {
           return filePath.endsWith(QLatin1StringView(extension.data(), static_cast<qsizetype>(extension.size())),Qt::CaseInsensitive);
    }))
        category.assign("image");
    else if (std::ranges::any_of(videoExtensions,[&filePath](std::string_view extension)
    {
           return filePath.endsWith(QLatin1StringView(extension.data(), static_cast<qsizetype>(extension.size())),Qt::CaseInsensitive);
    }))
        category.assign("video");
    else if (std::ranges::any_of(audioExtensions,[&filePath](std::string_view extension)
    {
           return filePath.endsWith(QLatin1StringView(extension.data(), static_cast<qsizetype>(extension.size())),Qt::CaseInsensitive);
    }))
        category.assign("audio");
    else
        category.assign("document");


    return category;
}


#endif //VENT_FILE_EXTENSIONS_H
