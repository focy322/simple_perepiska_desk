//
// Created by mark on 10/6/26.
//

#ifndef VENT_FILE_EXTENSIONS_H
#define VENT_FILE_EXTENSIONS_H

#include <array>
#include <string_view>
#include <algorithm>

#include <qtypes.h>
#include <QString>
#include <QFileInfo>
#include <QMimeDatabase>



inline constexpr uint IMAGE_EXTENSIONS_COUNT = 5;
inline constexpr uint VIDEO_EXTENSIONS_COUNT = 4;
inline constexpr uint AUDIO_EXTENSIONS_COUNT = 6;
inline constexpr uint FILE_CATEGORY_COUNT = 4;

inline constexpr std::array<std::string_view, IMAGE_EXTENSIONS_COUNT> imageExtensions{".jpeg", ".png", ".webp", ".gif", ".jpg"};
inline constexpr std::array<std::string_view, VIDEO_EXTENSIONS_COUNT> videoExtensions{".mp4", ".webm", ".quicktime", ".x-matroska"};
inline constexpr std::array<std::string_view, AUDIO_EXTENSIONS_COUNT> audioExtensions{".mpeg", ".ogg", ".wav", ".flac", ".aac", ".m4a"};
inline constexpr std::array<std::string_view, FILE_CATEGORY_COUNT>    fileCategories {"image", "video", "audio", "document"};

/**
 * @brief Определяет категорию по MIME-типу или содержимому локального файла, затем по расширению.
 * @param filePath
 * @return
 */
static QString defineFileCategory(const QString &filePath, const QString &contentType = {})
{
    QMimeDatabase mimeDatabase;
    QString mimeType = contentType.section(';', 0, 0).trimmed().toLower();
    if (mimeType.isEmpty() && QFileInfo(filePath).isFile())
        mimeType = mimeDatabase.mimeTypeForFile(filePath, QMimeDatabase::MatchContent).name();

    const auto mediaCategory = [](const QString &mime) -> QString {
        if (mime.startsWith("image/", Qt::CaseInsensitive))
            return QStringLiteral("image");
        if (mime.startsWith("video/", Qt::CaseInsensitive))
            return QStringLiteral("video");
        if (mime.startsWith("audio/", Qt::CaseInsensitive))
            return QStringLiteral("audio");
        return {};
    };
    const QString detectedCategory = mediaCategory(mimeType);
    if (!detectedCategory.isEmpty())
        return detectedCategory;
    if (!mimeType.isEmpty() && mimeType != "application/octet-stream" &&
        mimeType != "application/x-empty" && mimeType != "application/x-zerosize" && mimeType != "inode/x-empty")
        return QStringLiteral("document");

    const QString extensionCategory = mediaCategory(
        mimeDatabase.mimeTypeForFile(filePath, QMimeDatabase::MatchExtension).name());
    if (!extensionCategory.isEmpty())
        return extensionCategory;

    QString category;
    if (std::ranges::any_of(imageExtensions,[&filePath](std::string_view extension)
    {
           return filePath.endsWith(QLatin1StringView(extension.data(), static_cast<qsizetype>(extension.size())),Qt::CaseInsensitive);
    }))
        category.assign(fileCategories[0]);
    else if (std::ranges::any_of(videoExtensions,[&filePath](std::string_view extension)
    {
           return filePath.endsWith(QLatin1StringView(extension.data(), static_cast<qsizetype>(extension.size())),Qt::CaseInsensitive);
    }))
        category.assign(fileCategories[1]);
    else if (std::ranges::any_of(audioExtensions,[&filePath](std::string_view extension)
    {
           return filePath.endsWith(QLatin1StringView(extension.data(), static_cast<qsizetype>(extension.size())),Qt::CaseInsensitive);
    }))
        category.assign(fileCategories[2]);
    else
        category.assign(fileCategories[3]);


    return category;
}


#endif //VENT_FILE_EXTENSIONS_H
